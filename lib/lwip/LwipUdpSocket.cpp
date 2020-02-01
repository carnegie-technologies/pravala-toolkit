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

#include <cassert>

extern "C"
{
// lwip hijacks ntohs, ntohl, etc.
// To avoid "different exception specifier" compiler error with the new gcc-6.1.1
// netinet/in.h should be included before some lwip headers.
#include <netinet/in.h>

#include "lwip/udp.h"
}

#include "internal/pbuf_custom_MemHandle.hpp"
#include "LwipUdpSocket.hpp"

#define LOG_UDP( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "UDP Socket [" << this << "]; " << X )

using namespace Pravala;

LwipUdpSocket::LwipUdpSocket ( Receiver & receiver ):
    LwipSocket ( _lwipSock.ip ),
    _receiver ( receiver )
{
    _lwipSock.udp = udp_new();

    if ( !_lwipSock.udp )
    {
        LOG ( L_ERROR, "Could not allocate memory for new lwIP UDP socket" );
        return;
    }

    udp_recv ( _lwipSock.udp, &recvFunc, this );

    LOG_UDP ( L_DEBUG2, "Created" );

    checkLwipTimer();
}

LwipUdpSocket::~LwipUdpSocket()
{
    close();
}

LwipUdpSocket * LwipUdpSocket::getUDP()
{
    return this;
}

const char * LwipUdpSocket::getTypeName() const
{
    return "UDP";
}

void LwipUdpSocket::close()
{
    if ( !_lwipSock.udp )
        return;

    // Disable all callback functions so we don't get a callback when closing the socket
    _lwipSock.udp->recv = 0;

    udp_remove ( _lwipSock.udp );
    _lwipSock.udp = 0;

    checkLwipTimer();
}

int32_t LwipUdpSocket::bind ( const SockAddr & orgAddr )
{
    if ( !_lwipSock.udp )
    {
        LOG_UDP ( L_ERROR, "Cannot bind to " << orgAddr << ", UDP socket is closed" );
        return EBADF;
    }

    if ( isBound() )
    {
        LOG_UDP ( L_ERROR, "Cannot bind to " << orgAddr << ", UDP socket is already bound to " << _localAddr );
        return EINVAL;
    }

    SockAddr address = orgAddr;

    if ( address.sa.sa_family == AF_UNSPEC && address.sa_in.sin_addr.s_addr == 0 && isIpV4Only() )
    {
        // This is a very special case.
        // Binding IPv4 sockets to AF_UNSPEC address, that has all IPv4 address bytes (not including the port number)
        // set to 0, should behave like binding to 0.0.0.0 address (keeping the same port number).

        address.sa.sa_family = AF_INET;

        LOG_UDP ( L_DEBUG, "Converting an AF_UNSPEC zero address to v4 address: " << address );
    }

    /// @todo TODO Document this behaviour.

    // Linux/Android behaviour of UDP bind:
    // - v4 sockets accept only v4 addresses (v6-mapped v4 addresses are also rejected)
    // - v6 sockets accept v6 and v6-mapped v4 addresses
    // - v6-only sockets accept only real v6 addresses
    //
    // MacOS behaves the same, except it uses different errno codes in some cases.

    if ( isIpV4Only() && !address.isIPv4() )
    {
        LOG_UDP ( L_ERROR, "Cannot bind an IPv4 socket to non-IPv4 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EINVAL
        return EINVAL;
#else
        // Linux/Android uses EAFNOSUPPORT
        return EAFNOSUPPORT;
#endif
    }

    if ( isIpV6() && !address.isIPv6() )
    {
        LOG_UDP ( L_ERROR, "Cannot bind an IPv6 socket to non-IPv6 address: " << address );

        // Linux/Android and MacOS use EINVAL (yes, it's different than in v4 case)
        return EINVAL;
    }

    // Since IPv6-Only is also an IPv6 socket, if we are IPv6-only we know that by this point
    // it's an IPv6 address. But let's make sure it's not a v6-mapped v4 address:
    if ( isIpV6Only() && address.isIPv6MappedIPv4() )
    {
        LOG_UDP ( L_ERROR, "Cannot bind an IPv6-Only socket to IPv6-Mapped IPv4 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EADDRNOTAVAIL
        return EADDRNOTAVAIL;
#else
        // Linux/Android uses EINVAL
        return EINVAL;
#endif
    }

    LOG_UDP ( L_DEBUG2, "Trying to bind to: " << address );

    ip_addr_t addr;

    if ( !convertFromSockAddr ( address, addr ) )
    {
        LOG_UDP ( L_ERROR, "Failed to convert: " << address );
        return EINVAL;
    }

    if ( !prepareSocketForAddr ( addr ) )
    {
        LOG_UDP ( L_ERROR, "Cannot use address " << address << " for bind" );
        return EINVAL;
    }

    const err_t err = udp_bind ( _lwipSock.udp, &addr, address.getPort() );

    checkLwipTimer();

    if ( err != 0 )
    {
        LOG_UDP ( L_ERROR, "Failed to bind to " << address
                  << " due to lwIP error: [" << err << "] " << lwip_strerr ( err ) );

        return mapLwipErrorToErrno ( err );
    }

    if ( !convertToSockAddr ( _lwipSock.udp->local_ip, _lwipSock.udp->local_port, _localAddr )
         || !_localAddr.getAddr().isValid()
         || _localAddr.getPort() < 1 )
    {
        LOG_UDP ( L_ERROR, "lwIP failed to set a valid local IP address / port after udp_bind succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if bind succeeds
        return EBADF;
    }

    setFlag ( SocketFlagIsBound, true );

    LOG_UDP ( L_DEBUG, "Successfully bound to: " << _localAddr );

    return 0;
}

int32_t LwipUdpSocket::connect ( const SockAddr & address )
{
    if ( !_lwipSock.udp )
    {
        LOG_UDP ( L_ERROR, "Cannot connect to " << address << ", UDP socket is closed" );
        return EBADF;
    }

    if ( address.sa.sa_family == AF_UNSPEC )
    {
        // The address is 'AF_UNSPEC' - this is a disconnect request.

        LOG_UDP ( L_DEBUG, "Disconnecting UDP socket connected to " << getRemoteAddr() );

        udp_disconnect ( _lwipSock.udp );

        checkLwipTimer();

        setFlag ( SocketFlagIsConnected, false );
        return 0;
    }

    if ( !address.hasIpAddr() || !address.hasPort() )
    {
        LOG_UDP ( L_ERROR, "Cannot connect to " << address << ", bad address or port number" );
        return EINVAL;
    }

    // We don't check if we are already connected.
    // Unlike TCP sockets, UDP sockets can be re-connected to new addresses.

    /// @todo TODO Document this behaviour.

    // Linux/Android behaviour of UDP connect:
    // - v4 sockets accept only v4 addresses (v6-mapped v4 addresses are also rejected)
    // - v6 sockets accept v6, v6-mapped v4, AND v4 addresses (this is different than bind)
    // - v6-only sockets accept only real v6 addresses
    //
    // MacOS behaves the same, except:
    // - it uses different errno codes in some cases
    // - v6 sockets don't accept v4 addresses

    if ( isIpV4Only() && !address.isIPv4() )
    {
        LOG_UDP ( L_ERROR, "Cannot connect an IPv4 socket to IPv6 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EINVAL
        return EINVAL;
#else
        // Linux/Android uses EAFNOSUPPORT
        return EAFNOSUPPORT;
#endif
    }

#ifdef SYSTEM_APPLE
    if ( isIpV6() && !address.isIPv6() )
    {
        LOG_UDP ( L_ERROR, "Cannot connect an IPv6 socket to IPv4 address: " << address );

        // MacOS uses EINVAL, Linux/Android ACCEPTS it!
        return EINVAL;
    }
#endif

    if ( isIpV6Only() )
    {
        if ( !address.isIPv6() )
        {
            LOG_UDP ( L_ERROR, "Cannot connect an IPv6-Only socket to IPv4 address: " << address );

            // Linux/Android uses EAFNOSUPPORT, MacOS uses EINVAL, but it should already be handled above (isIpV6 case)
            return EAFNOSUPPORT;
        }
        else if ( address.isIPv6MappedIPv4() )
        {
            LOG_UDP ( L_ERROR, "Cannot connect an IPv6-Only socket to IPv6-Mapped IPv4 address: "
                      << address );

            // Linux/Android uses ENETUNREACH.
            // MacOS ALLOWS for that, but sockets connected this way can't send traffic (send fails).
            // Since we don't really support it internally, we fail here.
            return ENETUNREACH;
        }
    }

    LOG_UDP ( L_DEBUG2, "Trying to connect to: " << address );

    ip_addr_t addr;

    if ( !convertFromSockAddr ( address, addr ) )
    {
        LOG_UDP ( L_ERROR, "Failed to convert: " << address );
        return EINVAL;
    }

    if ( !prepareSocketForAddr ( addr ) )
    {
        LOG_UDP ( L_ERROR, "Cannot use address " << address << " for connect" );
        return EINVAL;
    }

    const err_t err = udp_connect ( _lwipSock.udp, &addr, address.getPort() );

    checkLwipTimer();

    if ( err != 0 )
    {
        LOG_UDP ( L_ERROR, "Failed to connect to " << address
                  << " due to lwIP error: [" << err << "] " << lwip_strerr ( err ) );

        return mapLwipErrorToErrno ( err );
    }

    if ( !convertToSockAddr ( _lwipSock.udp->local_ip, _lwipSock.udp->local_port, _localAddr )
         || !_localAddr.getAddr().isValid()
         || _localAddr.getPort() < 1 )
    {
        LOG_UDP ( L_ERROR, "lwIP failed to set a valid local IP address / port after udp_connect succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if connect succeeds
        return EBADF;
    }

    if ( !convertToSockAddr ( _lwipSock.udp->remote_ip, _lwipSock.udp->remote_port, _remoteAddr )
         || !_remoteAddr.getAddr().isValid()
         || _remoteAddr.getPort() < 1 )
    {
        LOG_UDP ( L_ERROR, "lwIP failed to set a valid remote IP address / port after udp_connect succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if connect succeeds
        return EBADF;
    }

    setFlag ( SocketFlagIsBound, true );
    setFlag ( SocketFlagIsConnected, true );

    LOG_UDP ( L_DEBUG, "Connected to: " << _remoteAddr << "; localAddr: " << _localAddr );

    // Here we use isEquivalent because the given address could be v6 mapped v4,
    // which we internally convert to a regular v4 address before passing it to lwIP.
    assert ( address.isEquivalent ( _remoteAddr ) );

    return 0;
}

int32_t LwipUdpSocket::send ( const MemHandle & data )
{
    if ( !_lwipSock.udp )
        return ENOTCONN;

    LOG_UDP ( L_DEBUG4, "[" << _localAddr << "->" << _remoteAddr << "]: " << data.size() << " bytes" );

    // We use our custom pbuf object to avoid copying the IpPacket's memory.
    // Passing the pointer directly into lwIP's stack is safe because lwIP will eventually
    // call pbuf_custom_MemHandle::customFreeFunc to free it.
    struct pbuf * const buffer = reinterpret_cast<struct pbuf *> ( new pbuf_custom_MemHandle ( data ) );

    const err_t err = udp_send ( _lwipSock.udp, buffer );

    pbuf_free ( buffer );

    checkLwipTimer();

    return mapLwipErrorToErrno ( err );
}

int32_t LwipUdpSocket::sendTo ( const MemHandle & data, const SockAddr & sockAddr )
{
    if ( !_lwipSock.udp )
        return ENOTCONN;

    if ( sockAddr.sa.sa_family == AF_UNSPEC )
    {
        // sendTo() used with an empty address = send()
        return send ( data );
    }

    ip_addr_t addr;

    if ( !convertFromSockAddr ( sockAddr, addr ) )
    {
        LOG_UDP ( L_ERROR, "Failed to convert: " << sockAddr );
        return EINVAL;
    }

    if ( !prepareSocketForAddr ( addr ) )
    {
        LOG_UDP ( L_ERROR, "Cannot use address " << sockAddr << " for sendto" );
        return EINVAL;
    }

    LOG_UDP ( L_DEBUG4, "[" << _localAddr << "->" << sockAddr << "]: " << data.size() << " bytes" );

    // We use our custom pbuf object to avoid copying the IpPacket's memory.
    // Passing the pointer directly into lwIP's stack is safe because lwIP will eventually
    // call pbuf_custom_MemHandle::customFreeFunc to free it.
    struct pbuf * const buffer = reinterpret_cast<struct pbuf *> ( new pbuf_custom_MemHandle ( data ) );

    const err_t err = udp_sendto ( _lwipSock.udp, buffer, &addr, sockAddr.getPort() );

    pbuf_free ( buffer );

    checkLwipTimer();

    return mapLwipErrorToErrno ( err );
}

void LwipUdpSocket::readEvent ( pbuf * buffer, const ip_addr_t * addr, uint16_t port )
{
    assert ( buffer != 0 );
    assert ( addr != 0 );

    LwipBufferIterator iter ( buffer );

    // The LwipBufferIterator now holds a new reference, and since the pbuf provided by
    // lwIP's callback here gives us a single reference, we must unref the original pbuf.
    // pbuf_free is really pbuf_unref, just poorly named.
    pbuf_free ( buffer );

    SockAddr sa;

    if ( !convertToSockAddr ( *addr, port, sa ) )
    {
        LOG_UDP ( L_WARN, "Failed to convert src addr " << ipaddr_ntoa ( addr )
                  << " to SockAddr; Dropping packet of size " << iter.getSize() << " bytes" );
        return;
    }

    LOG_UDP ( L_DEBUG4, "[" << sa << "->" << _localAddr << "]: " << iter.getSize() << " bytes" );

    _receiver.lwipUdpSocketReceivedData ( this, iter, sa );
}

void LwipUdpSocket::recvFunc ( void * arg, udp_pcb * udp, pbuf * buffer, const ip_addr_t * addr, uint16_t port )
{
    assert ( arg != 0 );
    assert ( buffer != 0 );
    assert ( addr != 0 );
    assert ( static_cast<LwipUdpSocket *> ( arg )->_lwipSock.udp == udp );
    ( void ) udp;

    static_cast<LwipUdpSocket *> ( arg )->readEvent ( buffer, addr, port );
}

int32_t LwipUdpSocket::getOption ( int32_t level, int32_t optName, MemHandle & optVal )
{
    int val = 0;

    if ( level == SOL_SOCKET && optName == SO_TYPE )
    {
        val = SOCK_DGRAM;
    }
    else
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

    return 0;
}
