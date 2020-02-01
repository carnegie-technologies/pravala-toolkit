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

#include "lwip/tcp.h"

#ifdef SYSTEM_LINUX
#include <linux/tcp.h>
#endif
}

#include "LwipTcpSocket.hpp"

#define LOG_TCP( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "TCP Socket [" << this << "]; " << X )

// Normally TCP_NODELAY is defined in <netinet/tcp.h>, but lwIP's headers
// also define some conflicting things, so we can't include both.
// lwIP's socket API is disabled because it uses threads, so we can't just include "lwip/sockets.h" either.
// So we just define this constant here.
#ifndef TCP_NODELAY
#define TCP_NODELAY    0x01
#endif

/// @def CHECK_SETSOCKOPT_PARAM_SIZE Checks if the size of the parameter matches the field that we want to use.
/// If it doesn't, EINVAL is returned immediately.
#define CHECK_SETSOCKOPT_PARAM_SIZE( param ) \
    if ( sizeof ( param ) != optVal.size() ) { \
        return EINVAL; \
    }

using namespace Pravala;

int32_t LwipTcpSocket::getOption ( int32_t level, int32_t optName, MemHandle & optVal )
{
    int32_t respErrorCode = 0;
    int val = 0;

    if ( level == SOL_SOCKET )
    {
        switch ( optName )
        {
            case SO_ACCEPTCONN:
                val = ( _lwipSock.tcp->state == LISTEN );
                break;

            case SO_ERROR:
                val = _lastError;
                _lastError = 0; // SO_ERROR is cleared on get
                break;

            case SO_TYPE:
                val = SOCK_STREAM;
                break;

            default:
                respErrorCode = ENOPROTOOPT;
        }
    }
    else if ( level == IPPROTO_TCP )
    {
        switch ( optName )
        {
            case TCP_NODELAY:
                val = tcp_nagle_disabled ( _lwipSock.tcp );
                break;

#ifdef TCP_KEEPALIVE
            case TCP_KEEPALIVE:
                val = _lwipSock.tcp->keep_idle;
                break;
#endif

// This will make it available on all systems with a Linux kernel, including Android.
#if defined( SYSTEM_LINUX ) && defined( TCP_INFO )
            case TCP_INFO:
                return ( ( optVal = getTcpInfo() ).isEmpty() ) ? EINVAL : 0;
#endif

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
        return LwipSocket::getOption ( level, optName, optVal );
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

int32_t LwipTcpSocket::setOption ( int32_t level, int32_t optName, const MemHandle & optVal )
{
    int32_t respErrorCode = 0;
    int val = 0;

    if ( optVal.size() > sizeof ( val ) )
    {
        LOG_TCP ( L_WARN, "Received socket option 'set' " << optName << " with invalid data size : "
                  << optVal.size() << "; Expected at most: " << sizeof ( val ) );

        return EINVAL;
    }

    memcpy ( &val, optVal.get(), optVal.size() );

    const char * optNameStr = "unknown";

    ( void ) optNameStr;

    if ( level == IPPROTO_TCP )
    {
        switch ( optName )
        {
            case TCP_NODELAY:
                optNameStr = "TCP_NODELAY";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val );

                if ( val != 0 )
                {
                    // NODELAY is enabled - no nagle.
                    tcp_nagle_disable ( _lwipSock.tcp );
                }
                else
                {
                    // NODELAY is disabled - use nagle.
                    tcp_nagle_enable ( _lwipSock.tcp );
                }
                break;

#ifdef TCP_KEEPALIVE
            case TCP_KEEPALIVE:
                optNameStr = "TCP_KEEPALIVE";
                CHECK_SETSOCKOPT_PARAM_SIZE ( val );
                _lwipSock.tcp->keep_idle = ( uint32_t ) val;
                break;
#endif

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
        return LwipSocket::setOption ( level, optName, optVal );
    }

    LOG_TCP ( L_DEBUG2, "Setting socket option; Level : " << level
              << "; Name : [" << optName << "] " << optNameStr
              << "; Value (if int): " << val << "; Result error code: " << respErrorCode );

    checkLwipTimer();

    return respErrorCode;
}
