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

#include "UdpListener.hpp"
#include "UdpListenerSocket.hpp"
#include "UdpFdListener.hpp"

using namespace Pravala;

TextLog UdpListener::_log ( "udp_listener" );

UdpListener::UdpListener(): _listeningSock ( 0 )
{
}

UdpListener::~UdpListener()
{
    assert ( !_listeningSock );
    assert ( _connectedSocks.isEmpty() );
}

int UdpListener::increaseRcvBufSize ( int )
{
    return -1;
}

int UdpListener::increaseSndBufSize ( int )
{
    return -1;
}

ERRCODE UdpListener::reinitialize ( const SockAddr & localAddr )
{
    if ( _listeningSock != 0 )
    {
        LOG ( L_ERROR, "Could not reinitialize UdpListener at " << _localAddr
              << " - it has a listening UDP socket" );

        return Error::AddrInUse;
    }

    if ( !_connectedSocks.isEmpty() )
    {
        LOG ( L_ERROR, "Could not reinitialize UdpListener at " << _localAddr
              << " - it has " << _connectedSocks.size() << " connected UDP socket(s)" );

        return Error::AddrInUse;
    }

    return reinitializeImpl ( localAddr );
}

UdpSocket * UdpListener::generateListeningSock ( SocketOwner * owner, ERRCODE * errCode )
{
    if ( _listeningSock != 0 )
    {
        if ( errCode != 0 )
        {
            *errCode = Error::AlreadyExists;
        }

        LOG_ERR ( L_ERROR, Error::AlreadyExists, "Listening socket generation failed; Local: "
                  << _localAddr.toString() );

        return 0;
    }

    UdpListenerSocket * sock = new UdpListenerSocket ( *this, owner, EmptySockAddress );

    if ( sock != 0 )
    {
        _listeningSock = sock;

        if ( errCode != 0 )
        {
            *errCode = Error::Success;
        }

        return sock;
    }

    if ( errCode != 0 )
    {
        *errCode = Error::MemoryError;
    }

    return 0;
}

UdpSocket * UdpListener::generateConnectedSock (
        SocketOwner * owner, const SockAddr & rAddr, ERRCODE * errCode )
{
    SockAddr remoteAddr ( rAddr );

    if ( !remoteAddr.hasIpAddr() || !remoteAddr.hasPort() )
    {
        if ( errCode != 0 )
        {
            *errCode = Error::InvalidParameter;
        }

        LOG_ERR ( L_ERROR, Error::InvalidParameter, "Connected socket generation failed; Remote address ("
                  << remoteAddr << ") is invalid" );

        return 0;
    }

    if ( remoteAddr.isIPv6MappedIPv4() )
    {
        remoteAddr.convertToV4();

        LOG ( L_DEBUG, "Converting IPv6-mapped-IPv4 address: " << rAddr << " -> " << remoteAddr );
    }

    if ( _connectedSocks.contains ( remoteAddr ) )
    {
        if ( errCode != 0 )
        {
            *errCode = Error::AlreadyExists;
        }

        LOG_ERR ( L_ERROR, Error::AlreadyExists, "Connected socket generation failed; Local: "
                  << _localAddr << "; Remote: " << remoteAddr );

        return 0;
    }

    UdpListenerSocket * sock = new UdpListenerSocket ( *this, owner, remoteAddr );

    if ( sock != 0 )
    {
        _connectedSocks[ remoteAddr ] = sock;

        if ( errCode != 0 )
        {
            *errCode = Error::Success;
        }

        LOG ( L_DEBUG, "Generated connected socket; Local: " << _localAddr << "; Remote: " << remoteAddr );

        return sock;
    }

    if ( errCode != 0 )
    {
        *errCode = Error::MemoryError;
    }

    return 0;
}

void UdpListener::dataReceived ( SockAddr & remote, MemHandle & data )
{
    assert ( remote.isIPv4() || remote.isIPv6() );
    assert ( !data.isEmpty() );

    if ( data.isEmpty() )
        return;

    if ( remote.isIPv6MappedIPv4() )
    {
        remote.convertToV4();

        LOG ( L_DEBUG4, "Got packet from (remote): " << remote
              << " [converted from IPv6-mapped-IPv4]; To (local): " << _localAddr
              << "; Length: " << data.size() );
    }
    else
    {
        LOG ( L_DEBUG4, "Got packet from (remote): " << remote
              << "; To (local): " << _localAddr
              << "; Length: " << data.size() );
    }

    UdpListenerSocket * sock = 0;

    if ( !_connectedSocks.find ( remote, sock ) )
    {
        if ( _listeningSock != 0 )
        {
            LOG ( L_DEBUG4, "Using listening socket for packet from (remote): " << remote
                  << "; To (local): " << _localAddr
                  << "; Length: " << data.size() );

            _listeningSock->sockDataReceivedFrom ( remote, data );
            return;
        }

        LOG ( L_DEBUG4, "No listening socket; Dropping packet from (remote): " << remote
              << "; To (local): " << _localAddr
              << "; Length: " << data.size() );
    }

    if ( !sock )
    {
        LOG ( L_DEBUG, "No connected socket and no listener, dropping packet from (remote): " << remote
              << "; To (local): " << _localAddr
              << "; Length: " << data.size() );
        return;
    }

    assert ( sock != 0 );

    LOG ( L_DEBUG4, "Using connected socket for packet from (remote): " << remote
          << "; To (local): " << _localAddr << "; Length: " << data.size() );

    sock->doSockDataReceived ( data );
}

void UdpListener::unregisterUdpSocket ( UdpListenerSocket * sock )
{
    assert ( sock != 0 );
    assert ( this == &sock->_listener );

    if ( !sock )
        return;

    // Either the listener is no longer valid - in which case we may have unregistered the sock before
    // it can unregister itself, or the sock has to exist either as a listening sock or in _connectedSocks.
    assert ( !isValid() || _listeningSock == sock || _connectedSocks.contains ( sock->getRemoteSockAddr() ) );

    if ( _listeningSock == sock )
    {
        LOG ( L_DEBUG, "Unregistering listening UDP socket; Local: " << _localAddr );

        _listeningSock = 0;
        return;
    }

    LOG ( L_DEBUG, "Unregistering UDP socket; Local: " << _localAddr << "; Remote: " << sock->getRemoteSockAddr() );

    _connectedSocks.remove ( sock->getRemoteSockAddr() );
}

void UdpListener::notifyClosed()
{
    if ( isValid() )
    {
        // This function got called, but we're not closing, so don't do anything

        assert ( false );
        return;
    }

    // UdpListenerSocket::notifyClosed() does not call any callbacks,
    // only schedules 'closed' event to be run at the end of the loop.
    // So there is no need to hold a self reference here anymore (older code was calling
    // multiple callbacks immediately, and it required a self reference to be able to do that safely).

    if ( _listeningSock != 0 )
    {
        _listeningSock->notifyClosed();
    }

    for ( HashMap<SockAddr, UdpListenerSocket *>::Iterator it ( _connectedSocks ); it.isValid(); it.next() )
    {
        assert ( it.isValid() );
        assert ( it.value() != 0 );

        it.value()->notifyClosed();
    }
}
