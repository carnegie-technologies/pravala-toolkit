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

#include "lwip/tcp.h"
}

#include "basic/Math.hpp"

#include "LwipTcpSocket.hpp"

#define LOG_TCP( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "TCP Socket [" << this << "]; " << X )

using namespace Pravala;

LwipTcpSocket::LwipTcpSocket ( Receiver & receiver ):
    LwipSocket ( _lwipSock.ip ),
    _receiver ( receiver ),
    _lastError ( 0 )
{
    _lwipSock.tcp = tcp_new();

    if ( !_lwipSock.tcp )
    {
        LOG ( L_ERROR, "Could not allocate memory for new lwIP TCP socket" );
        return;
    }

    tcp_arg ( _lwipSock.tcp, this );
    tcp_err ( _lwipSock.tcp, &errorFunc );
    tcp_recv ( _lwipSock.tcp, &recvFunc );

    // This specifies the callback function that should be called when the sent data has been acknowledged;
    // lwIP calls this the 'sent' function, but it is really the acknowledged function.
    tcp_sent ( _lwipSock.tcp, &ackedFunc );

    LOG_TCP ( L_DEBUG2, "Created" );

    checkLwipTimer();
}

LwipTcpSocket::~LwipTcpSocket()
{
    closeOrAbort();
}

LwipTcpSocket * LwipTcpSocket::getTCP()
{
    return this;
}

const char * LwipTcpSocket::getTypeName() const
{
    return "TCP";
}

err_t LwipTcpSocket::closeOrAbort()
{
    if ( !_lwipSock.tcp )
        return ERR_OK;

    // Disable all callback functions so we don't get a callback when closing the socket
    _lwipSock.tcp->sent = 0;
    _lwipSock.tcp->recv = 0;
    _lwipSock.tcp->connected = 0;
    _lwipSock.tcp->errf = 0;
    _lwipSock.tcp->poll = 0;

    _lwipSock.tcp->callback_arg = 0;

    // If tcp_close succeeds, then _tcp will be freed at a later time by lwIP.
    // However tcp_close can fail if it cannot send a FIN packet for some reason.
    // In that case, we call tcp_abort, which sends a RST packet, always succeeds, and frees _tcp immediately.
    // In both cases the _tcp pointer is unsafe to reference afterwards.

    err_t err = tcp_close ( _lwipSock.tcp );

    if ( err != ERR_OK )
    {
        LOG_TCP ( L_ERROR, "Failed to close socket due to lwIP error: [" << err << "] " << lwip_strerr ( err )
                  << "; Aborting socket" );

        tcp_abort ( _lwipSock.tcp );
        err = ERR_ABRT;
    }
    else if ( !_sentQueue.isEmpty() )
    {
        List<MemHandle> * const mhList = new List<MemHandle> ( _sentQueue );

        if ( _log.shouldLog ( L_DEBUG ) )
        {
            size_t dataSize = 0;

            for ( size_t i = 0; i < _sentQueue.size(); ++i )
            {
                dataSize += _sentQueue.at ( i ).size();
            }

            ( void ) dataSize;

            LOG_TCP ( L_DEBUG, "Socket being closed still has " << dataSize << " bytes of data to be sent; "
                      "Creating copy of the data queue [" << mhList << "] and configuring cleanup callbacks" );
        }

        tcp_arg ( _lwipSock.tcp, mhList );
        tcp_err ( _lwipSock.tcp, &closedErrorFunc );

        // This specifies the callback function that should be called when the sent data has been acknowledged;
        // lwIP calls this the 'sent' function, but it is really the acknowledged function.
        tcp_sent ( _lwipSock.tcp, &closedAckedFunc );
    }
    else
    {
        LOG_TCP ( L_DEBUG2, "Closed" );
    }

    _sentQueue.clear();
    _lwipSock.tcp = 0;

    checkLwipTimer();

    return err;
}

int32_t LwipTcpSocket::bind ( const SockAddr & orgAddr )
{
    if ( !_lwipSock.tcp )
    {
        LOG_TCP ( L_ERROR, "Cannot bind to " << orgAddr << ", TCP socket is closed" );
        return EBADF;
    }

    if ( isBound() )
    {
        LOG_TCP ( L_ERROR, "Cannot bind to " << orgAddr << ", TCP socket is already bound to " << _localAddr );
        return EINVAL;
    }

    SockAddr address = orgAddr;

    if ( address.sa.sa_family == AF_UNSPEC && address.sa_in.sin_addr.s_addr == 0 && isIpV4Only() )
    {
        // This is a very special case.
        // Binding IPv4 sockets to AF_UNSPEC address, that has all IPv4 address bytes (not including the port number)
        // set to 0, should behave like binding to 0.0.0.0 address (keeping the same port number).

        address.sa.sa_family = AF_INET;

        LOG_TCP ( L_DEBUG, "Converting an AF_UNSPEC zero address to v4 address: " << address );
    }

    /// @todo TODO Document this behaviour.

    // Linux/Android behaviour of TCP bind:
    // - v4 sockets accept only v4 addresses (v6-mapped v4 addresses are also rejected)
    // - v6 sockets accept v6 and v6-mapped v4 addresses
    // - v6-only sockets accept only real v6 addresses
    //
    // MacOS is the same, except it uses different errno codes in some cases.

    if ( isIpV4Only() && !address.isIPv4() )
    {
        LOG_TCP ( L_ERROR, "Cannot bind an IPv4 socket to non-IPv4 address: " << address );

        // Linux/Android and MacOS use EAFNOSUPPORT.
        return EAFNOSUPPORT;
    }

    if ( isIpV6() && !address.isIPv6() )
    {
        LOG_TCP ( L_ERROR, "Cannot bind an IPv6 socket to non-IPv6 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EAFNOSUPPORT
        return EAFNOSUPPORT;
#else
        // Linux/Android uses EINVAL
        return EINVAL;
#endif
    }

    // Since IPv6-Only is also an IPv6 socket, if we are IPv6-only we know that by this point
    // it's an IPv6 address. But let's make sure it's not a v6-mapped v4 address:
    if ( isIpV6Only() && address.isIPv6MappedIPv4() )
    {
        LOG_TCP ( L_ERROR, "Cannot bind an IPv6-Only socket to IPv6-Mapped IPv4 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EADDRNOTAVAIL
        return EADDRNOTAVAIL;
#else
        // Linux/Android uses EINVAL
        return EINVAL;
#endif
    }

    LOG_TCP ( L_DEBUG2, "Trying to bind to: " << address );

    ip_addr_t addr;

    if ( !convertFromSockAddr ( address, addr ) )
    {
        LOG_TCP ( L_ERROR, "Failed to convert: " << address );
        return EINVAL;
    }

    if ( !prepareSocketForAddr ( addr ) )
    {
        LOG_TCP ( L_ERROR, "Cannot use address " << address << " for bind" );
        return EINVAL;
    }

    const err_t err = tcp_bind ( _lwipSock.tcp, &addr, address.getPort() );

    checkLwipTimer();

    if ( err != 0 )
    {
        LOG_TCP ( L_ERROR, "Failed to bind to " << address
                  << " due to lwIP error: [" << err << "] " << lwip_strerr ( err ) );

        return mapLwipErrorToErrno ( err );
    }

    if ( !convertToSockAddr ( _lwipSock.tcp->local_ip, _lwipSock.tcp->local_port, _localAddr )
         || !_localAddr.getAddr().isValid()
         || _localAddr.getPort() < 1 )
    {
        LOG_TCP ( L_ERROR, "lwIP failed to set a valid local IP address / port after tcp_bind succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if bind succeeds
        return EBADF;
    }

    setFlag ( SocketFlagIsBound, true );

    LOG_TCP ( L_DEBUG, "Successfully bound to: " << _localAddr );

    return 0;
}

int32_t LwipTcpSocket::connect ( const SockAddr & address )
{
    if ( !_lwipSock.tcp )
    {
        LOG_TCP ( L_ERROR, "Cannot connect to " << address << ", TCP socket is closed" );
        return EBADF;
    }

    if ( !address.hasIpAddr() || !address.hasPort() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect to " << address << ", bad address or port number" );
        return EINVAL;
    }

    if ( isConnecting() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect to " << address << ", TCP socket is already connecting to " << _remoteAddr );
        return EALREADY;
    }

    if ( isConnected() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect to " << address << ", TCP socket is already connected to " << _remoteAddr );
        return EISCONN;
    }

    /// @todo TODO Document this behaviour.

    // Linux/Android behaviour of TCP connect:
    // - v4 sockets accept only v4 addresses (v6-mapped v4 addresses are also rejected)
    // - v6 sockets accept v6 and v6-mapped v4 addresses (the same as TCP bind, but different than UDP connect)
    // - v6-only sockets accept only real v6 addresses
    //
    // MacOS is the same, except it uses different errno codes in some cases.

    if ( isIpV4Only() && !address.isIPv4() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect an IPv4 socket to IPv6 address: " << address );

        // Linux/Android and MacOS use EAFNOSUPPORT.
        return EAFNOSUPPORT;
    }

    if ( isIpV6() && !address.isIPv6() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect an IPv6 socket to IPv4 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EAFNOSUPPORT
        return EAFNOSUPPORT;
#else
        // Linux/Android uses EINVAL
        return EINVAL;
#endif
    }

    // Since IPv6-Only is also an IPv6 socket, if we are IPv6-only we know that by this point
    // it's an IPv6 address. But let's make sure it's not a v6-mapped v4 address:
    if ( isIpV6Only() && address.isIPv6MappedIPv4() )
    {
        LOG_TCP ( L_ERROR, "Cannot connect an IPv6-Only socket to IPv6-Mapped IPv4 address: " << address );

#ifdef SYSTEM_APPLE
        // MacOS uses EINVAL
        return EINVAL;
#else
        // Linux/Android uses ENETUNREACH
        return ENETUNREACH;
#endif
    }

    LOG_TCP ( L_DEBUG2, "Trying to connect to: " << address );

    ip_addr_t addr;

    if ( !convertFromSockAddr ( address, addr ) )
    {
        LOG_TCP ( L_ERROR, "Failed to convert: " << address );
        return EINVAL;
    }

    if ( !prepareSocketForAddr ( addr ) )
    {
        LOG_TCP ( L_ERROR, "Cannot use address " << address << " for connect" );
        return EINVAL;
    }

    const err_t err = tcp_connect ( _lwipSock.tcp, &addr, address.getPort(), &connectedFunc );

    checkLwipTimer();

    if ( err != 0 )
    {
        LOG_TCP ( L_ERROR, "Failed to connect to " << address
                  << " due to lwIP error: [" << err << "] " << lwip_strerr ( err ) );

        return mapLwipErrorToErrno ( err );
    }

    if ( !convertToSockAddr ( _lwipSock.tcp->local_ip, _lwipSock.tcp->local_port, _localAddr )
         || !_localAddr.getAddr().isValid()
         || _localAddr.getPort() < 1 )
    {
        LOG_TCP ( L_ERROR, "lwIP failed to set a valid local IP address / port after tcp_connect succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if connect succeeds
        return EBADF;
    }

    if ( !convertToSockAddr ( _lwipSock.tcp->remote_ip, _lwipSock.tcp->remote_port, _remoteAddr )
         || !_remoteAddr.getAddr().isValid()
         || _remoteAddr.getPort() < 1 )
    {
        LOG_TCP ( L_ERROR, "lwIP failed to set a valid remote IP address / port after tcp_connect succeeded with: "
                  << address );

        // Return 'bad fd' since this is an internal error that really shouldn't happen if connect succeeds
        return EBADF;
    }

    // lwIP's connect never succeeds immediately; success means the initial SYN packet has been sent.
    // We will get a callback on connectedFunc when the connect succeeds, or errorFunc if it fails.
    setFlag ( SocketFlagIsBound, true );
    setFlag ( SocketFlagIsConnecting, true );

    LOG_TCP ( L_DEBUG, "Connecting to: " << _remoteAddr << "; localAddr: " << _localAddr );

#ifndef NO_LOGGING
    // Here we use isEquivalent because the given address could be v6 mapped v4,
    // which we internally convert to a regular v4 address before passing it to lwIP.
    if ( _log.shouldLog ( L_WARN ) && !address.isEquivalent ( _remoteAddr ) )
    {
        LOG_TCP ( L_WARN, "Remote addr: " << _remoteAddr
                  << " set by lwIP is different from given addr: " << address );
    }
#endif

    // lwIP returns success if the connect starts successfully,
    // so we map that return code to 'in progress'.
    // This way our connect function never succeeds immediately,
    // and always returns an error code.
    return EINPROGRESS;
}

int32_t LwipTcpSocket::send ( const MemHandle & data )
{
    if ( !_lwipSock.tcp )
        return ENOTCONN;

    LOG_TCP ( L_DEBUG4, "[" << _localAddr << "->" << _remoteAddr << "]: " << data.size() << " bytes" );

    // We must maintain a const ref to the data payload; lwIP expects that the data
    // will not change until it has been sent and acknowledged by the remote side.
    //
    // To achieve this, we create a copy of the MemHandle that holds a reference to the data
    // payload and append it to _sentQueue. MemHandle guarantees that as long as we don't try
    // to modify our copy, the memory referenced by it won't be modified and won't be moved.
    //
    // To guarantee our copy of the data remains unmodified, we must ABSOLUTELY NOT call
    // any MemHandle methods that could modify the memory.
    //
    // However it is still safe to call some non-const methods, such as consume(), that
    // will modify the MemHandle object, but won't change or move the memory referenced by
    // the MemHandle.

    _sentQueue.append ( data );

    // Here we use the memory actually stored in _sentQueue, not from the MemHandle we were passed.
    // It really should be the same, but this way we give lwIP the same memory we are actually
    // referencing in the _sentQueue, in case MemHandle does something internally after we copy
    // it into _sentQueue.
    const MemHandle & constRef = _sentQueue.last();

    // We use 0 as the flag for tcp_write so lwIP won't copy the data being passed.
    // The whole point of this is to avoid unnecessary copying of data.
    err_t err = tcp_write ( _lwipSock.tcp, constRef.get(), constRef.size(), 0 );

    if ( err == ERR_OK )
    {
        err = tcp_output ( _lwipSock.tcp );
    }

    checkLwipTimer();
    return mapLwipErrorToErrno ( err );
}

err_t LwipTcpSocket::consumedEvent ( uint16_t len )
{
    assert ( len > 0 );

    if ( len == 0 )
        return ERR_OK;

    LOG_TCP ( L_DEBUG2, "" << len << " bytes acknowledged by " << _remoteAddr );

    while ( len > 0 && !_sentQueue.isEmpty() )
    {
        MemHandle & data = _sentQueue.first();

        if ( len < data.size() )
        {
            // Just consume part of the entry, since the length of the data to consume
            // is less than the length of the first data entry.
            // This DOES NOT change or move the memory stored in the MemHandle.
            data.consume ( len );

            // Then we're done because we have consumed all the bytes from the event
            len = 0;
            break;
        }

        // We have consumed the entire entry, remove it
        len -= data.size();
        _sentQueue.removeFirst();
    }

    if ( len > 0 )
    {
        LOG_TCP ( L_ERROR, "" << len << " bytes remaining to acknowledge (by " << _remoteAddr
                  << "), but the sent queue is empty; Disconnecting potentially corrupt TCP stream." );

        return disconnectEvent ( ERR_BUF );
    }

    // If the TCP socket has consumed some data (the data has been ACKed by the remote side),
    // then the max send size will also have increased.
    _receiver.lwipTcpSocketMaxSendSizeIncreased ( this, getMaxSendSize() );

    return ERR_OK;
}

size_t LwipTcpSocket::getMaxSendSize() const
{
    if ( !_lwipSock.tcp )
        return 0;

    return tcp_sndbuf ( _lwipSock.tcp );
}

int32_t LwipTcpSocket::sendTo ( const MemHandle & data, const SockAddr & )
{
    return send ( data );
}

err_t LwipTcpSocket::connectedEvent()
{
    setFlag ( SocketFlagIsConnecting, false );
    setFlag ( SocketFlagIsConnected, true );

    LOG_TCP ( L_DEBUG, "Successfully connected to: " << _remoteAddr << "; localAddr: " << _localAddr );

    _receiver.lwipTcpSocketConnected ( this );

    return ERR_OK;
}

err_t LwipTcpSocket::disconnectEvent ( err_t error )
{
    const err_t ret = closeOrAbort();

    _lastError = mapLwipErrorToErrno ( error );

    setFlag ( SocketFlagIsBound, false );
    setFlag ( SocketFlagIsConnecting, false );
    setFlag ( SocketFlagIsConnected, false );

    LOG_TCP ( L_DEBUG, "Disconnected due to lwIP error: [" << error << "] " << lwip_strerr ( error ) );

    _receiver.lwipTcpSocketDisconnected ( this, _lastError );

    // Note: closeOrAbort may call tcp_abort, in which case it will return ERR_ABRT,
    // which we MUST return if tcp_abort was called during an lwIP callback since
    // the socket is freed during the callback.
    return ret;
}

err_t LwipTcpSocket::readEvent ( pbuf * buffer, err_t error )
{
    if ( !buffer )
    {
        LOG_TCP ( L_DEBUG, "Receive failed because the connection was closed normally" );

        assert ( error == ERR_OK );

        return disconnectEvent ( error );
    }
    else if ( error != ERR_OK )
    {
        LOG_TCP ( L_DEBUG, "Receive failed due to lwIP error: [" << error << "] " << lwip_strerr ( error ) );

        pbuf_free ( buffer );

        return disconnectEvent ( error );
    }

    assert ( buffer != 0 );

    LwipBufferIterator iter ( buffer );

    // Tell lwIP we have received the data in buffer
    tcp_recved ( _lwipSock.tcp, buffer->tot_len );

    // The LwipBufferIterator now holds a new reference, and since the pbuf provided by
    // lwIP's callback here gives us a single reference, we must unref the original pbuf.
    // pbuf_free is really pbuf_unref, just poorly named.
    pbuf_free ( buffer );

    LOG_TCP ( L_DEBUG4, "[" << _remoteAddr << "->" << _localAddr << "]: " << iter.getSize() << " bytes" );

    _receiver.lwipTcpSocketReceivedData ( this, iter );

    return ERR_OK;
}

err_t LwipTcpSocket::connectedFunc ( void * arg, tcp_pcb * tcp, err_t error )
{
    assert ( arg != 0 );
    assert ( static_cast<LwipTcpSocket *> ( arg )->_lwipSock.tcp == tcp );
    ( void ) tcp;

    // The lwIP connect callback is always supposed to succeed
    assert ( error == ERR_OK );
    ( void ) error;

    return static_cast<LwipTcpSocket *> ( arg )->connectedEvent();
}

void LwipTcpSocket::errorFunc ( void * arg, err_t error )
{
    assert ( arg != 0 );

    // The TCP object has already been freed when this function is called.
    static_cast<LwipTcpSocket *> ( arg )->_lwipSock.tcp = 0;

    static_cast<LwipTcpSocket *> ( arg )->disconnectEvent ( error );
}

err_t LwipTcpSocket::recvFunc ( void * arg, tcp_pcb * tcp, pbuf * buffer, err_t error )
{
    assert ( arg != 0 );
    assert ( static_cast<LwipTcpSocket *> ( arg )->_lwipSock.tcp == tcp );
    ( void ) tcp;

    return static_cast<LwipTcpSocket *> ( arg )->readEvent ( buffer, error );
}

err_t LwipTcpSocket::ackedFunc ( void * arg, tcp_pcb * tcp, uint16_t len )
{
    assert ( arg != 0 );
    assert ( static_cast<LwipTcpSocket *> ( arg )->_lwipSock.tcp == tcp );
    ( void ) tcp;

    return static_cast<LwipTcpSocket *> ( arg )->consumedEvent ( len );
}

err_t LwipTcpSocket::closedAckedFunc ( void * arg, tcp_pcb * tcp, uint16_t len )
{
    assert ( arg != 0 );
    assert ( len > 0 );

    List<MemHandle> * const mhList = static_cast<List<MemHandle> *> ( arg );

    if ( !mhList || !tcp || len < 1 )
        return ERR_OK;

    SockAddr remoteAddr;

    if ( _log.shouldLog ( L_ERROR ) )
    {
        // We check L_ERROR to set remoteAddr in case it's needed here OR below (where L_ERROR is used)

        convertToSockAddr ( tcp->remote_ip, tcp->remote_port, remoteAddr );

        LOG ( L_DEBUG2, "" << len << " bytes in data queue [" << mhList << "] acknowledged by " << remoteAddr );
    }

    while ( len > 0 && !mhList->isEmpty() )
    {
        MemHandle & data = mhList->first();

        if ( len < data.size() )
        {
            // Just consume part of the entry, since the length of the data to consume
            // is less than the length of the first data entry.
            // This DOES NOT change or move the memory stored in the MemHandle.
            data.consume ( len );

            // Then we're done because we have consumed all the bytes from the event
            len = 0;
            break;
        }

        // We have consumed the entire entry, remove it
        len -= data.size();
        mhList->removeFirst();
    }

    if ( len > 0 )
    {
        LOG ( L_ERROR, "" << len << " bytes in data queue [" << mhList << "] remaining to acknowledge (by "
              << remoteAddr << "), but the sent queue is empty; Disconnecting potentially corrupt TCP stream." );

        tcp->callback_arg = 0;
        tcp->sent = 0;
        tcp->errf = 0;

        tcp_abort ( tcp );

        delete mhList;

        return ERR_ABRT;
    }

    if ( mhList->isEmpty() )
    {
        LOG ( L_DEBUG, "Outgoing data queue [" << mhList << "] is now empty; Removing" );

        tcp->callback_arg = 0;
        tcp->sent = 0;
        tcp->errf = 0;

        delete mhList;
    }

    return ERR_OK;
}

void LwipTcpSocket::closedErrorFunc ( void * arg, err_t error )
{
    ( void ) error;

    List<MemHandle> * const mhList = static_cast<List<MemHandle> *> ( arg );

    if ( mhList != 0 )
    {
        // tcp_pcb has been destroyed already.
        // all we need to do is to remove the memory list.

        LOG ( L_DEBUG, "Disconnected a socket due to lwIP error: [" << error << "] " << lwip_strerr ( error )
              << "; Removing data queue [" << mhList << "]" );

        delete mhList;
    }
}
