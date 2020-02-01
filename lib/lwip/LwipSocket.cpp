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

extern "C"
{
// lwip hijacks ntohs, ntohl, etc.
// To avoid "different exception specifier" compiler error with the new gcc-6.1.1
// netinet/in.h should be included before some lwip headers.
#include <netinet/in.h>

#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
}

#include "basic/IpAddress.hpp"

#include "internal/LwipEventPoller.hpp"
#include "LwipSocket.hpp"

#define LOG_SOCK( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "Socket [" << this << "]; " << X )

using namespace Pravala;

TextLog LwipSocket::_log ( "lwip_socket" );

LwipSocket::LwipSocket ( IpPcbPtr & pcb ): _pcb ( pcb ), _flags ( 0 )
{
    // This will start lwIP event polling if it is the first reference
    LwipEventPoller::get().ref();
}

LwipSocket::~LwipSocket()
{
    // This will stop lwIP event polling if it is the last reference
    LwipEventPoller::get().unref();
}

void LwipSocket::checkLwipTimer()
{
    LwipEventPoller::get().checkLwipTimer();
}

LwipTcpSocket * LwipSocket::getTCP()
{
    return 0;
}

LwipUdpSocket * LwipSocket::getUDP()
{
    return 0;
}

const char * LwipSocket::getTypeName() const
{
    return "Unknown";
}

int32_t LwipSocket::getTag() const
{
    if ( !_pcb )
        return 0;

    return _pcb->tag;
}

void LwipSocket::setTag ( int32_t tag )
{
    if ( !_pcb )
        return;

    _pcb->tag = tag;
}

int32_t LwipSocket::mapLwipErrorToErrno ( err_t error )
{
    // This mapping is taken from: lwip/src/api/sockets.c
    switch ( error )
    {
        case ERR_OK:         // No error, everything OK.
            return 0;

        case ERR_MEM:        // Out of memory error.
            return ENOMEM;

        case ERR_BUF:        // Buffer error.
            return ENOBUFS;

        case ERR_TIMEOUT:    // Timeout. lwIP treats this as 'would block' when returning an errno code
        case ERR_WOULDBLOCK: // Operation would block.
            return EWOULDBLOCK;

        case ERR_RTE:        // Routing problem.
            return EHOSTUNREACH;

        case ERR_INPROGRESS: // Operation in progress
            return EINPROGRESS;

        case ERR_VAL:        // Illegal value.
            return EINVAL;

        case ERR_USE:        // Address in use.
            return EADDRINUSE;

        case ERR_ALREADY:    // Already connecting.
            return EALREADY;

        case ERR_ISCONN:     // Conn already established.
            return EISCONN;

        case ERR_CLSD:       // Connection closed.
        case ERR_CONN:       // Not connected.
            return ENOTCONN;

        case ERR_ABRT:       // Connection aborted.
            return ECONNABORTED;

        case ERR_RST:        // Connection reset.
            return ECONNRESET;

        case ERR_ARG:        // Illegal argument.
            return EIO;

        case ERR_IF:         // Low-level netif error
            return -1;
    }

    // Fallback to I/O error if we don't have a mapping
    return EIO;
}

bool LwipSocket::convertFromSockAddr ( const SockAddr & addr, ip_addr_t & lwipAddr )
{
    IpAddress ipAddr ( addr );

    // It is safe to call this even if the address is not mapped.
    ipAddr.convertToV4();

    if ( ipAddr.isIPv4() )
    {
        memcpy ( &lwipAddr.u_addr.ip4, &ipAddr.getV4(), sizeof ( lwipAddr.u_addr.ip4 ) );
        lwipAddr.type = IPADDR_TYPE_V4;
        return true;
    }
    else if ( ipAddr.isIPv6() )
    {
        memcpy ( &lwipAddr.u_addr.ip6, &ipAddr.getV6(), sizeof ( lwipAddr.u_addr.ip6 ) );
        lwipAddr.type = IPADDR_TYPE_V6;
        return true;
    }

    return false;
}

bool LwipSocket::convertToSockAddr ( const ip_addr_t & lwipAddr, uint16_t port, SockAddr & sa )
{
    return ( ( lwipAddr.type == IPADDR_TYPE_V4
               && sa.setAddr ( AF_INET, &lwipAddr.u_addr.ip4, sizeof ( lwipAddr.u_addr.ip4 ) )
               && sa.setPort ( port ) )
             ||
             ( lwipAddr.type == IPADDR_TYPE_V6
               && sa.setAddr ( AF_INET6, &lwipAddr.u_addr.ip6, sizeof ( lwipAddr.u_addr.ip6 ) )
               && sa.setPort ( port ) ) );
}

bool LwipSocket::prepareSocketForAddr ( const ip_addr_t & addr )
{
    if ( !_pcb )
        return false;

    if ( addr.type != IPADDR_TYPE_V4 && addr.type != IPADDR_TYPE_V6 )
    {
        LOG_SOCK ( L_ERROR, "Invalid lwIP address type: " << addr.type );
        return false;
    }

    // True if the target address type is IPv6
    const bool targetV6 = ( addr.type == IPADDR_TYPE_V6 );

    if ( !targetV6 && isIpV6Only() )
    {
        LOG_SOCK ( L_ERROR, "Address " << ipaddr_ntoa ( &addr )
                   << " is V4 and this is a V6-only socket, cannot use address for bind / connect" );
        return false;
    }

    if ( targetV6 && isIpV4Only() )
    {
        LOG_SOCK ( L_ERROR, "Address " << ipaddr_ntoa ( &addr )
                   << " is V6 and this is a V4-only socket, cannot use address for bind / connect" );
        return false;
    }

    // Normally we don't allow changing the lwIP TCP socket type after it is already bound,
    // the only exception is if the socket is already bound to the IPv4 / IPv6 any address.
    if ( isBound() )
    {
        assert ( _pcb->local_ip.type == IPADDR_TYPE_V4 || _pcb->local_ip.type == IPADDR_TYPE_V6 );

        if ( _pcb->local_ip.type == addr.type )
        {
            // The lwIP TCP socket's local address type already matches the target IP address type,
            // and since it's already bound, we don't need to check and change the type.
            return true;
        }

        if ( !ip_addr_isany ( &_pcb->local_ip ) )
        {
            LOG_SOCK ( L_ERROR, "Target address " << ipaddr_ntoa ( &addr ) << " and local address "
                       << ipaddr_ntoa ( &_pcb->local_ip ) << " are incompatible" );

            // We don't allow changing the local address type if it is not the IPv4 / IPv6 any address.
            return false;
        }

        if ( _pcb->local_ip.type != addr.type )
        {
            ip_addr_set_any ( targetV6, &_pcb->local_ip );

            LOG_SOCK ( L_DEBUG2, "Local address changed to " << ipaddr_ntoa ( &_pcb->local_ip ) );
        }

        // We can't return here because the socket was bound to the IPv4 / IPv6 any address,
        // and we actually need to continue and change the socket's type.
        // Doing this after binding seems to be safe as far as lwIP's code goes.
    }

    assert ( !isConnecting() );
    assert ( !isConnected() );

    // Convert the internal lwIP TCP socket type to the same type as the IP address if needed
    if ( _pcb->local_ip.type != addr.type || _pcb->remote_ip.type != addr.type )
    {
        // Note: lwIP uses the type assigned to the local and remote IP addresses to determine
        // whether the socket is V4 or V6. Even if addresses are empty.
        IP_SET_TYPE_VAL ( _pcb->local_ip, addr.type );
        IP_SET_TYPE_VAL ( _pcb->remote_ip, addr.type );
    }

    checkLwipTimer();
    return true;
}
