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

#include "sys/SocketApi.hpp"

#include "WebSocketConnection.hpp"
#include "WebSocketHandler.hpp"
#include "WebSocketListener.hpp"

using namespace Pravala;

TextLog WebSocketListener::_log ( "web_socket_listener" );

WebSocketListener::WebSocketListener ( WebSocketListenerOwner & owner ):
    _owner ( owner ),
    _tcpServer ( *this )
{
}

WebSocketListener::~WebSocketListener()
{
    for ( HashSet<WebSocketConnection *>::Iterator it ( _connInProgress ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );
        it.value()->unrefOwner ( this );
    }

    _connInProgress.clear();
}

ERRCODE WebSocketListener::addListener ( const IpAddress & localAddr, uint16_t localPort )
{
    return _tcpServer.addListener ( localAddr, localPort );
}

void WebSocketListener::incomingTcpConnection (
        TcpServer *, uint8_t, int sockFd,
        const IpAddress & localAddr, uint16_t localPort,
        const IpAddress & remoteAddr, uint16_t remotePort )
{
    ( void ) localAddr;
    ( void ) localPort;
    ( void ) remoteAddr;
    ( void ) remotePort;

    WebSocketConnection * conn = WebSocketConnection::generate ( this, sockFd );

    LOG ( L_INFO, "WebSocketListener accepted incoming connection. "
          << "Fd: " << sockFd
          << "; Local: " << localAddr.toString() << ":" << localPort
          << "; Remote: " << remoteAddr.toString() << ":" << remotePort
          << "; Connection: " << ( uint64_t ) conn );

    if ( !conn )
    {
        LOG ( L_ERROR, "Failed to generate WebSocketConnection, closing."
              << "Fd: " << sockFd
              << "; Local: " << localAddr.toString() << ":" << localPort
              << "; Remote: " << remoteAddr.toString() << ":" << remotePort );

        SocketApi::close ( sockFd );
        return;
    }

    _connInProgress.insert ( conn );
}

void WebSocketListener::incomingUnixConnection ( TcpServer *, uint8_t, int sockFd, const String & sockName )
{
    ( void ) sockName;

    WebSocketConnection * conn = WebSocketConnection::generate ( this, sockFd );

    LOG ( L_INFO, "WebSocketListener accepted incoming connection. "
          << "Fd: " << sockFd
          << "; Unix sock: " << sockName
          << "; Connection: " << ( uint64_t ) conn );

    if ( !conn )
    {
        LOG ( L_ERROR, "Failed to generate WebSocketConnection, closing."
              << "Fd: " << sockFd
              << "; Unix sock: " << sockName );

        SocketApi::close ( sockFd );
        return;
    }

    _connInProgress.insert ( conn );
}

void WebSocketListener::wsRead ( WebSocketConnection *, MemHandle &, bool )
{
    // We shouldn't get any of these, since we pass it off to a handler and remove us from it
    // before WebSocketConnection should be calling back with any WebSocket frames.

    assert ( false );
}

void WebSocketListener::wsClosed ( WebSocketConnection * sock )
{
    assert ( sock != 0 );

    // If we got a connection closed, we should be the one owning this socket right now,
    // and it should be one that was in the process of connecting.
    assert ( _connInProgress.contains ( sock ) );
    assert ( sock->getOwner() == this );

    LOG ( L_INFO, "Connection closed: " << ( uint64_t ) sock );

    sock->unrefOwner ( this );
    sock = 0;
    _connInProgress.remove ( sock );
}

void WebSocketListener::wsEstablished ( WebSocketConnection * sock, WebSocketHandler * handler )
{
    assert ( sock != 0 );
    assert ( handler != 0 );

    _connInProgress.remove ( sock );

    handler->addConnection ( this, sock );

    sock->unrefOwner ( this );
    sock = 0;
}
