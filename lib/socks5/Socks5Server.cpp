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

#include "socket/TcpFdSocket.hpp"

#include "internal/Socks5ServerSocket.hpp"

#include "Socks5Server.hpp"

using namespace Pravala;

TextLog Socks5Server::_log ( "socks5_server" );

TcpSocket * Socks5Server::Owner::socks5GenerateOutboundTcpSocket ( SocketOwner * owner )
{
    return new TcpFdSocket ( owner );
}

Socks5Server::Owner::~Owner()
{
}

Socks5Server::Socks5Server ( Owner & owner ):
    _owner ( owner ),
    _tcpServer ( *this )
{
}

Socks5Server::~Socks5Server()
{
    for ( HashMap<TcpSocket *, SocketState>::Iterator it ( _socks ); it.isValid(); it.next() )
    {
        if ( it.key() != 0 )
        {
            it.key()->unrefOwner ( this );
        }
    }

    _socks.clear();
}

void Socks5Server::closeListeners()
{
    _tcpServer.closeListeners();
}

ERRCODE Socks5Server::addListener ( const SockAddr & localAddr, uint8_t extraData, int backlog )
{
    return _tcpServer.addListener ( localAddr, extraData, backlog );
}

void Socks5Server::socketDataReceived ( Socket * sock, MemHandle & data )
{
    ( void ) data;

    if ( sock != 0 )
    {
        LOG ( L_WARN, sock->getLogId() << ": Ignoring unexpected callback; Data: " << data.getHexDump() );
    }
}

void Socks5Server::socketReadyToSend ( Socket * sock )
{
    if ( sock != 0 )
    {
        LOG ( L_WARN, sock->getLogId() << ": Ignoring unexpected callback" );
    }
}

void Socks5Server::socketClosed ( Socket * sock, ERRCODE reason )
{
    ( void ) reason;

    TcpSocket * const tcpSock
        = ( sock != 0 && sock->getIpSocket() != 0 ) ? ( sock->getIpSocket()->getTcpSocket() ) : 0;

    if ( !tcpSock )
        return;

    SocketState sockState = _socks.value ( tcpSock );

    if ( tcpSock == sockState.srvSock )
    {
        LOG_ERR ( L_DEBUG, reason, tcpSock->getLogId() << ": Server socket closed; Closing all associated sockets" );

        removeSocket ( tcpSock );
        return;
    }

    if ( !sockState.srvSock || tcpSock != sockState.outboundSock )
    {
        // Something is weird (and shouldn't happen), let's remove everything!

        LOG ( L_ERROR, tcpSock->getLogId() << ": Socket configuration is invalid; Closing all associated sockets" );

        removeSocket ( tcpSock );
        return;
    }

    LOG_ERR ( L_DEBUG, reason, tcpSock->getLogId()
              << ": Outbound TCP socket closed; Failing the server socket: " << sockState.srvSock->getLogId() );

    _socks[ sockState.srvSock ].outboundSock = 0;

    tcpSock->unrefOwner ( this );
    _socks.remove ( tcpSock );

    sockState.srvSock->handledTcpConnect ( Socks5ReplyMessage::ReplyConnectionRefused );

    // We don't remove the server socket yet, it needs time to clean-up.

    return;
}

void Socks5Server::socketConnectFailed ( Socket * sock, ERRCODE reason )
{
    // We treat 'closed' and 'connect failed' the same way...
    socketClosed ( sock, reason );
}

void Socks5Server::socketConnected ( Socket * sock )
{
    TcpSocket * tcpSock
        = ( sock != 0 && sock->getIpSocket() != 0 ) ? ( sock->getIpSocket()->getTcpSocket() ) : 0;

    if ( !tcpSock )
        return;

    SocketState sockState = _socks.value ( tcpSock );

    if ( !sockState.srvSock
         || !sockState.outboundSock
         || ( tcpSock != sockState.srvSock && tcpSock != sockState.outboundSock ) )
    {
        // Something is weird (and shouldn't happen), let's remove everything!

        LOG ( L_ERROR, tcpSock->getLogId() << ": Socket configuration is invalid; Closing all associated sockets" );

        removeSocket ( tcpSock );
        return;
    }

    if ( tcpSock == sockState.srvSock )
    {
        LOG ( L_DEBUG3, sockState.srvSock->getLogId()
              << ": Server socket is now connected; Outbound socket: " << sockState.outboundSock->getLogId() );

        // Regardless (this will remove both socket states):
        _socks.remove ( sockState.srvSock );
        _socks.remove ( sockState.outboundSock );

        TcpFdSocket * const basicClientSock = sockState.srvSock->generateTcpFdSock ( 0 );

        if ( !basicClientSock )
        {
            LOG ( L_ERROR, sockState.srvSock->getLogId() << ": Could not generate basic TCP socket" );
        }
        else
        {
            LOG ( L_DEBUG2, "Successfully established a TCP link between SOCKS5 client ("
                  << basicClientSock->getLogId() << ") and a remote host ("
                  << sockState.outboundSock->getLogId() << "); Passing it to the owner" );

            _owner.socks5NewOutboundTcpLink ( basicClientSock, sockState.outboundSock );

            // Remove our reference to basic socket:
            basicClientSock->unrefOwner ( this );
        }

        // And now cleanup other objects, error or not!
        sockState.srvSock->unrefOwner ( this );
        sockState.outboundSock->unrefOwner ( this );
        return;
    }

    LOG ( L_DEBUG2, tcpSock->getLogId()
          << ": Outbound TCP socket connected; Notifying server socket: " << sockState.srvSock->getLogId() );

    ERRCODE eCode = sockState.srvSock->handledTcpConnect (
        Socks5ReplyMessage::ReplySuccess, tcpSock->getLocalSockAddr() );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, tcpSock->getLogId()
                  << ": Outbound TCP socket connected, but server socket (" << sockState.srvSock->getLogId()
                  << ") failed to handle it; Closing outbound TCP socket and cancelling server socket" );

        _socks.remove ( tcpSock );
        _socks[ sockState.srvSock ].outboundSock = 0;

        tcpSock->unrefOwner ( this );
        tcpSock = 0;

        // We don't remove the server socket yet, it needs time to clean-up.
        return;
    }
}

void Socks5Server::removeSocket ( TcpSocket * sock )
{
    if ( !sock )
        return;

    SocketState sockState = _socks.value ( sock );

    if ( sock != sockState.srvSock && sock != sockState.outboundSock )
    {
        sock->unrefOwner ( this );
        return;
    }

    if ( sockState.srvSock != 0 )
    {
        LOG ( L_DEBUG, "Removing server socket: " << sockState.srvSock->getLogId() );

        sockState.srvSock->unrefOwner ( this );
        _socks.remove ( sockState.srvSock );
    }

    if ( sockState.outboundSock != 0 )
    {
        LOG ( L_DEBUG, "Removing outbound TCP socket: " << sockState.outboundSock->getLogId() );

        sockState.outboundSock->unrefOwner ( this );
        _socks.remove ( sockState.outboundSock );
    }
}

void Socks5Server::incomingTcpConnection ( TcpServer * tcpServer, uint8_t, TcpSocket * socket )
{
    ( void ) tcpServer;
    assert ( tcpServer == &_tcpServer );

    if ( !socket )
        return;

    Socks5ServerSocket * srvSock = new Socks5ServerSocket ( this, socket );

    if ( !srvSock->isValid() )
    {
        LOG ( L_ERROR, "Could not generate valid Socks5ServerSocket; Incoming connection: " << socket->getLogId() );

        srvSock->unrefOwner ( this );
        srvSock = 0;
    }

    if ( !srvSock )
        return;

    LOG ( L_DEBUG, "Generated new Socks5ServerSocket: " << srvSock->getLogId() );

    SocketState sockState;

    sockState.srvSock = srvSock;

    _socks.insert ( srvSock, sockState );
}

Socks5ReplyMessage::Reply Socks5Server::socks5TcpConnectRequested (
        Socks5ServerSocket * srvSock, const SockAddr & destAddr )
{
    if ( !destAddr.isIPv4() && !destAddr.isIPv6() )
    {
        return Socks5ReplyMessage::ReplyAddressTypeNotSupported;
    }

    // We CANNOT use a reference to a SocketState here, because we will be inserting it to the same map later.
    // If it was a reference and the map was reorganized during insert(), we would be inserting a value that
    // may not be valid anymore.
    SocketState sockState = _socks.value ( srvSock );

    assert ( sockState.srvSock == srvSock );

    if ( sockState.outboundSock != 0 )
    {
        LOG ( L_ERROR, srvSock->getLogId() << ": There is already a TCP socket associated with that server socket: "
              << sockState.outboundSock->getLogId() );

        return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
    }

    TcpSocket * conSocket = _owner.socks5GenerateOutboundTcpSocket ( this );

    if ( !conSocket )
    {
        LOG ( L_ERROR, srvSock->getLogId() << ": Could not generate an outbound TCP socket" );

        return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
    }

    ERRCODE eCode = conSocket->connect ( destAddr );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, srvSock->getLogId() << ": Error connecting outbound TCP socket to: " << destAddr );

        conSocket->unrefOwner ( this );
        conSocket = 0;

        return Socks5ReplyMessage::ReplyHostUnreachable;
    }

    LOG ( L_DEBUG2, srvSock->getLogId() << ": Initiated outbound TCP connection attempt; Outbound socket: "
          << conSocket->getLogId() );

    sockState.outboundSock = conSocket;

    assert ( sockState.srvSock == srvSock );
    assert ( sockState.outboundSock == conSocket );

    // Here sockState MUST be an actual value, not a reference:
    _socks[ srvSock ] = sockState;
    _socks[ conSocket ] = sockState;

    // Reply will not be sent anyway (until we actually connect or fail to connect), but let's set it to something:
    return Socks5ReplyMessage::ReplySuccess;
}
