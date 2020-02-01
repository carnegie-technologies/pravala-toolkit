/*
 *  Copyright 2019 Carnegie Technologies
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <cerrno>

extern "C"
{
// lwip hijacks ntohs, ntohl, etc.
// To avoid "different exception specifier" compiler error with the new gcc-6.1.1
// netinet/in.h should be included before some lwip headers.
#include <netinet/in.h>

#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
}

#include "LwipSocket.hpp"

#define LOG_SOCK( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "Socket [" << this << "]; " << X )

/// @def CHECK_SETSOCKOPT_PARAM_SIZE Checks if the size of the parameter matches the field that we want to use.
/// If it doesn't, EINVAL is returned immediately.
#define CHECK_SETSOCKOPT_PARAM_SIZE( param ) \
    if ( sizeof ( param ) != optVal.size() ) { \
        return EINVAL; \
    }

/// @def CASE_SETSOCKOPT_WARN Convenience macro handles options that we want to warn about.
/// This checks if the value of the option matches "opt". If it does, it then verifies the parameter size,
/// then sets the option name and flag to log a warning.
#define CASE_SETSOCKOPT_WARN( opt, param ) \
    case opt: \
        CHECK_SETSOCKOPT_PARAM_SIZE ( param ); \
        optNameStr = #opt; \
        warnIgnored = true; \
        break;

/// @def CASE_SETSOCKOPT_IP_SET_OPTION Convenience macro handles setting a boolean option using lwIP's ip_set_option.
#define CASE_SETSOCKOPT_IP_SET_OPTION( opt, lwipOptName ) \
    case opt: \
        optNameStr = #opt; \
        if ( val.i != 0 ) { \
            ip_set_option ( _pcb, lwipOptName ); \
        } \
        else { \
            ip_reset_option ( _pcb, lwipOptName ); \
        } \
        break;

using namespace Pravala;

int32_t LwipSocket::getOption ( int32_t level, int32_t optName, MemHandle & optVal )
{
    if ( !_pcb )
        return EBADF;

    int val = 0;
    int32_t respErrorCode = 0;

    if ( level == SOL_SOCKET )
    {
        // Note: the SO_* option names are actually defined with different values in lwIP's headers
        // so we have to map them to the lwIP versions named SOF_*

        switch ( optName )
        {
#ifdef SO_REUSEPORT
            case SO_REUSEPORT:
                // Treat this the same as SO_REUSEADDR: intentional fall-through here
#endif
            case SO_REUSEADDR:
                val = ( ip_get_option ( _pcb, SOF_REUSEADDR ) == SOF_REUSEADDR ) ? 1 : 0;
                break;

            case SO_BROADCAST:
                val = ( ip_get_option ( _pcb, SOF_BROADCAST ) == SOF_BROADCAST ) ? 1 : 0;
                break;

            case SO_KEEPALIVE:
                val = ( ip_get_option ( _pcb, SOF_KEEPALIVE ) == SOF_KEEPALIVE ) ? 1 : 0;
                break;

#ifdef SO_DOMAIN
            case SO_DOMAIN:
                val = ( isIpV4Only() ? AF_INET : AF_INET6 );
                break;
#endif

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else if ( level == IPPROTO_IP )
    {
        switch ( optName )
        {
#ifdef IP_MTU_DISCOVER
            case IP_MTU_DISCOVER:
                // Default path MTU discovery (this will actually happen on the server)
                val = IP_PMTUDISC_WANT;
                break;
#endif

#ifdef IP_MULTICAST_ALL
            case IP_MULTICAST_ALL:
#endif
            case IP_MULTICAST_LOOP:
                // We don't support multicast, and we're most definitely not going to get system-wide subs
                // or support loop backs
                val = 0;
                break;

            case IP_MULTICAST_TTL:
                // Default TTL = 1
                val = 1;
                break;

            case IP_TTL:
                val = _pcb->ttl;
                break;

            case IP_TOS:
                val = _pcb->tos;
                break;

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else if ( level == IPPROTO_IPV6 )
    {
        if ( isIpV4Only() )
        {
            LOG_SOCK ( L_WARN, "Received socket option 'get' request for IPPROTO_IPV6 option "
                       << optName << " on IPv4-only socket" );

            return ENOPROTOOPT;
        }

        switch ( optName )
        {
#ifdef IPV6_MTU_DISCOVER
            case IPV6_MTU_DISCOVER:
                // Default path MTU discovery (this will actually happen on the server)
                val = IP_PMTUDISC_WANT;
                break;
#endif

            case IPV6_UNICAST_HOPS:
                val = _pcb->ttl;
                break;

            case IPV6_V6ONLY:
                val = isIpV6Only();
                break;

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else
    {
        respErrorCode = ENOPROTOOPT;
    }

    if ( respErrorCode == ENOPROTOOPT )
    {
        LOG_SOCK ( L_WARN, "Received socket option request for unsupported option; Level : "
                   << level << "; Name : " << optName );

        return respErrorCode;
    }

    MemHandle mh ( sizeof ( val ) );

    if ( mh.size() != sizeof ( val ) || !mh.getWritable() )
    {
        return ENOBUFS;
    }

    memcpy ( mh.getWritable(), &val, sizeof ( val ) );

    optVal = mh;

    return respErrorCode;
}

int32_t LwipSocket::setOption ( int32_t level, int32_t optName, const MemHandle & optVal )
{
    if ( !_pcb )
        return EBADF;

    union
    {
        int i;
        struct ipv6_mreq ip6Mreq;
#ifndef SYSTEM_QNX
        struct ip_mreqn ip4Mreq;
        struct ip_mreq_source ip4MreqSource;
#endif
    } val;

    if ( optVal.size() > sizeof ( val ) )
    {
        LOG_SOCK ( L_WARN, "Received socket option 'set' " << optName << " with invalid data size : "
                   << optVal.size() << "; Expected at most: " << sizeof ( val ) );

        return EINVAL;
    }

    memset ( &val, 0, sizeof ( val ) );

    memcpy ( &val, optVal.get(), optVal.size() );

    const char * optNameStr = "unknown";

    ( void ) optNameStr;

    // True if we're handling the option by pretending it worked, but the option is actually unsupported.
    bool warnIgnored = false;

    int32_t respErrorCode = 0;

    if ( level == SOL_SOCKET )
    {
        // Note: the SO_* option names are actually defined with different values in lwIP's headers
        // so we have to map them to the lwIP versions named SOF_*

        switch ( optName )
        {
#ifdef SO_PRIORITY
            CASE_SETSOCKOPT_WARN ( SO_PRIORITY, val.i );
#endif

            CASE_SETSOCKOPT_IP_SET_OPTION ( SO_BROADCAST, SOF_BROADCAST );
            CASE_SETSOCKOPT_IP_SET_OPTION ( SO_KEEPALIVE, SOF_KEEPALIVE );

#ifdef SO_REUSEPORT
            case SO_REUSEPORT:
                // Treat this the same as SO_REUSEADDR: intentional fall-through here
#endif
            CASE_SETSOCKOPT_IP_SET_OPTION ( SO_REUSEADDR, SOF_REUSEADDR );

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else if ( level == IPPROTO_IP )
    {
        switch ( optName )
        {
#ifndef SYSTEM_QNX
            CASE_SETSOCKOPT_WARN ( IP_ADD_MEMBERSHIP, val.ip4Mreq );
            CASE_SETSOCKOPT_WARN ( IP_ADD_SOURCE_MEMBERSHIP, val.ip4MreqSource );
            CASE_SETSOCKOPT_WARN ( IP_BLOCK_SOURCE, val.ip4MreqSource );
            CASE_SETSOCKOPT_WARN ( IP_DROP_MEMBERSHIP, val.ip4Mreq );
            CASE_SETSOCKOPT_WARN ( IP_DROP_SOURCE_MEMBERSHIP, val.ip4MreqSource );
#endif

#ifdef IP_MTU_DISCOVER
            CASE_SETSOCKOPT_WARN ( IP_MTU_DISCOVER, val.i );
#endif

#ifdef IP_MULTICAST_ALL
            CASE_SETSOCKOPT_WARN ( IP_MULTICAST_ALL, val.i );
#endif
            CASE_SETSOCKOPT_WARN ( IP_MULTICAST_TTL, val.i );

            case IP_TTL:
                optNameStr = "IP_TTL";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val.i );

                if ( val.i < 1 || val.i > 255 )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IP option"
                               " 'IP_TTL' with an invalid TTL. Value received: " << val.i );

                    return EINVAL;
                }

                _pcb->ttl = ( uint8_t ) val.i;
                break;

            case IP_TOS:
                optNameStr = "IP_TOS";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val.i );

                if ( val.i < 1 || val.i > 255 )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IP option"
                               " 'IP_TOS' with an invalid TOS. Value received: " << val.i );

                    return EINVAL;
                }

                _pcb->tos = ( uint8_t ) val.i;
                break;

#ifndef SYSTEM_QNX
            case IP_UNBLOCK_SOURCE:
                optNameStr = "IP_UNBLOCK_SOURCE";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val.ip4MreqSource );
                // Since we don't actually block anything, return the code for "not being blocked"
                respErrorCode = EADDRNOTAVAIL;
                break;
#endif

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else if ( level == IPPROTO_IPV6 )
    {
        if ( isIpV4Only() )
        {
            LOG_SOCK ( L_WARN, "Received socket option 'get' request for an IPPROTO_IPV6"
                       " option on IPv4-only socket" );

            return ENOPROTOOPT;
        }

        switch ( optName )
        {
#ifdef IPV6_ADD_MEMBERSHIP
            CASE_SETSOCKOPT_WARN ( IPV6_ADD_MEMBERSHIP, val.ip6Mreq );
#endif

#ifdef IPV6_DROP_MEMBERSHIP
            CASE_SETSOCKOPT_WARN ( IPV6_DROP_MEMBERSHIP, val.ip6Mreq );
#endif

#ifdef IPV6_MTU
            CASE_SETSOCKOPT_WARN ( IPV6_MTU, val.i );
#endif

#ifdef IPV6_MTU_DISCOVER
            CASE_SETSOCKOPT_WARN ( IPV6_MTU_DISCOVER, val.i );
#endif

            CASE_SETSOCKOPT_WARN ( IPV6_MULTICAST_HOPS, val.i );
            CASE_SETSOCKOPT_WARN ( IPV6_MULTICAST_LOOP, val.i );

#ifdef IPV6_ADDRFORM
            case IPV6_ADDRFORM:
                optNameStr = "IPV6_ADDRFORM";

                CHECK_SETSOCKOPT_PARAM_SIZE ( val.i );

                // This converts an AF_INET6 socket to an AF_INET.
                // The socket must be bound and connected to a V4 address already.
                if ( val.i != AF_INET )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IPV6 option "
                               "'IPV6_ADDRFORM', but could not convert type since the requested type isn't AF_INET. "
                               "Requested type: " << val.i );

                    return EINVAL;
                }
                else if ( !isBound() || !isConnected() || !_localAddr.isIPv4() || !_remoteAddr.isIPv4() )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IPV6 option "
                               "'IPV6_ADDRFORM', but could not convert type since the socket is not bound "
                               "and connected to IPv4 addresses." );

                    return EINVAL;
                }

                // At this point the lwIP socket must already be a V4 socket
                // if it is bound and connected to a V4 address.
                // Note: lwIP uses the type assigned to the local and remote IP addresses to determine
                // whether the socket is V4 or V6. Even if addresses are empty.
                assert ( _pcb->local_ip.type == IPADDR_TYPE_V4 );
                assert ( _pcb->remote_ip.type == IPADDR_TYPE_V4 );

                setFlag ( SocketFlagIpV4Only, true );

                LOG_SOCK ( L_DEBUG, "Converted socket from AF_INET6 to AF_INET. Local address/port: " << _localAddr );
                break;
#endif

            case IPV6_UNICAST_HOPS:
                optNameStr = "IPV6_UNICAST_HOPS";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val.i );

                if ( val.i < 1 || val.i > 255 )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IPV6 option "
                               "'IPV6_UNICAST_HOPS' with an invalid TTL. Value received: " << val.i );

                    return EINVAL;
                }

                _pcb->ttl = ( uint8_t ) val.i;
                break;

            case IPV6_V6ONLY:
                optNameStr = "IPV6_V6ONLY";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val.i );

                // Linux doesn't allow this for TCP sockets (this can happen when bound to a v4-mapped-v6 address)
                if ( isBound() && _localAddr.isIPv4() )
                {
                    LOG_SOCK ( L_WARN, "Received socket option 'set' request for the IPPROTO_IPV6 option"
                               " 'IPV6_V6ONLY', but we are already bound to an IPv4 local address." );

                    return EINVAL;
                }

                setFlag ( SocketFlagIpV6Only, val.i );
                break;

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else
    {
        respErrorCode = ENOPROTOOPT;
    }

    if ( respErrorCode == ENOPROTOOPT )
    {
        LOG_SOCK ( L_WARN, "Received socket option 'set' request for unsupported option; Level : "
                   << level << "; Name : [" << optName << "] " << optNameStr );
    }
    else if ( warnIgnored )
    {
        LOG_SOCK ( L_WARN, "Received socket option 'set' request for unsupported option that we are ignoring;"
                   " Level : " << level << "; Name : [" << optName << "] " << optNameStr );
    }
    else
    {
        LOG_SOCK ( L_DEBUG2, "Setting socket option; Level : " << level
                   << "; Name : [" << optName << "] " << optNameStr
                   << "; Value (if int): " << val.i << "; Result error code: " << respErrorCode );
    }

    checkLwipTimer();
    return respErrorCode;
}
