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

#include "Socks5TcpProxyServer.hpp"

using namespace Pravala;

TextLog Socks5TcpProxyServer::_log ( "socks5_proxy" );

Socks5TcpProxyServer::SockData::SockData():
    otherSock ( 0 ), bytesSent ( 0 ), bytesReceived ( 0 ), sockType ( SockInvalid )
{
}

Socks5TcpProxyServer::SockData::SockData ( SockType sType, TcpSocket * sock ):
    otherSock ( sock ), bytesSent ( 0 ), bytesReceived ( 0 ), sockType ( sType )
{
}

Socks5TcpProxyServer::Socks5TcpProxyServer ( Log::LogLevel logLevel ): _server ( *this ), _logLevel ( logLevel )
{
}

Socks5TcpProxyServer::~Socks5TcpProxyServer()
{
    for ( HashMap<Socket *, SockData>::Iterator it ( _socks ); it.isValid(); it.next() )
    {
        if ( it.key() != 0 )
        {
            it.key()->unrefOwner ( this );
        }
    }
}

ERRCODE Socks5TcpProxyServer::addListener ( const SockAddr & localAddr )
{
    return _server.addListener ( localAddr );
}

void Socks5TcpProxyServer::removeSocks ( Socket * a, Socket * b )
{
    if ( a != 0 && _socks.remove ( a ) > 0 )
    {
        a->unrefOwner ( this );
    }

    if ( b != 0 && _socks.remove ( b ) > 0 )
    {
        b->unrefOwner ( this );
    }
}

void Socks5TcpProxyServer::socks5NewOutboundTcpLink ( TcpSocket * clientSock, TcpSocket * remoteSock )
{
    if ( !clientSock || !remoteSock )
    {
        LOG ( L_ERROR, "Invalid socket pointers" );
        return;
    }

    LOG ( _logLevel, "New TCP link; Client: " << clientSock->getLogId()
          << "; Remote: " << remoteSock->getLogId() );

    clientSock->refOwner ( this );
    remoteSock->refOwner ( this );

    _socks.insert ( clientSock, SockData ( SockClient, remoteSock ) );
    _socks.insert ( remoteSock, SockData ( SockRemote, clientSock ) );
}

void Socks5TcpProxyServer::socketConnected ( Socket * sock )
{
    if ( !sock )
        return;

    LOG ( L_WARN, "Ignoring callback from: " << sock->getLogId() );
}

void Socks5TcpProxyServer::socketConnectFailed ( Socket * sock, ERRCODE reason )
{
    ( void ) reason;

    if ( !sock )
        return;

    LOG ( L_WARN, "Ignoring callback from: " << sock->getLogId() << "; Reason: " << reason );
}

void Socks5TcpProxyServer::socketClosed ( Socket * sock, ERRCODE reason )
{
    ( void ) reason;

    if ( !sock )
        return;

    Socket * const otherSock = _socks.value ( sock ).otherSock;

    if ( !otherSock )
    {
        LOG ( L_WARN, "Socket " << sock->getLogId() << " closed; Missing other socket; Closing it" );
    }
    else
    {
        LOG ( _logLevel, "Socket " << sock->getLogId() << " closed [R: " << _socks.value ( sock ).bytesReceived
              << ", W: " << _socks.value ( sock ).bytesSent << "]; Closing the other socket as well: "
              << otherSock->getLogId() << " [R: " << _socks.value ( otherSock ).bytesReceived
              << ", W: " << _socks.value ( otherSock ).bytesSent << "]" );
    }

    removeSocks ( sock, otherSock );
}

void Socks5TcpProxyServer::socketDataReceived ( Socket * sock, MemHandle & data )
{
    if ( !sock || data.isEmpty() )
        return;

    SockData & srcData = _socks[ sock ];

    Socket * const otherSock = srcData.otherSock;

    if ( !otherSock )
    {
        LOG ( L_WARN, "Socket " << sock->getLogId() << " bytesReceived data; Missing other socket; Closing it" );

        removeSocks ( sock, otherSock );
        return;
    }

    const size_t orgSize = data.size();

    srcData.bytesReceived += orgSize;

    const ERRCODE eCode = otherSock->send ( data );

    const size_t newSize = data.size();
    size_t dataPassed = 0;

    if ( newSize < orgSize )
    {
        dataPassed = orgSize - newSize;

        _socks[ otherSock ].bytesSent += dataPassed;
    }

    if ( eCode == Error::SoftFail )
    {
        LOG ( L_DEBUG3, "SoftFail error while passing data bytes from " << sock->getLogId()
              << " to " << otherSock->getLogId() );
    }
    else if ( IS_OK ( eCode ) )
    {
        LOG ( L_DEBUG4, "Passed " << dataPassed << " data bytes from " << sock->getLogId()
              << " to " << otherSock->getLogId() );
    }
    else
    {
        LOG_ERR ( L_ERROR, eCode, "Error passing data from " << sock->getLogId()
                  << " to " << otherSock->getLogId() << "; Removing both sockets" );

        removeSocks ( sock, otherSock );
    }
}

void Socks5TcpProxyServer::socketReadyToSend ( Socket * sock )
{
    if ( !sock )
        return;

    SockData & destData = _socks[ sock ];

    Socket * const otherSock = destData.otherSock;

    if ( !otherSock )
    {
        LOG ( L_WARN, "Socket " << sock->getLogId() << " is ready to send; Missing other socket; Closing it" );

        removeSocks ( sock, otherSock );
        return;
    }

    MemHandle data = otherSock->getReadBuffer();
    const size_t orgSize = data.size();

    const ERRCODE eCode = sock->send ( data );

    const size_t newSize = data.size();

    // Let's clear our copy of the data!
    data.clear();

    LOG_ERR ( IS_OK ( eCode ) ? L_DEBUG2 : L_ERROR, eCode,
              "Passing data from " << otherSock->getLogId() << " to " << sock->getLogId() );

    if ( newSize < orgSize )
    {
        destData.bytesSent += ( orgSize - newSize );
        otherSock->consumeReadBuffer ( orgSize - newSize );
    }

    if ( NOT_OK ( eCode ) && eCode != Error::SoftFail )
    {
        removeSocks ( sock, otherSock );
    }
}
