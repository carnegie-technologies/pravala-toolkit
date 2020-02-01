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

#include "socket/TcpSocket.hpp"
#include "http/HttpParser.hpp"

#include "HttpServer.hpp"

using namespace Pravala;

static const String HdrContentType ( "Content-Type" );
static const String HdrContentLength ( "Content-Length" );

TextLog HttpServer::_log ( "http_server" );

HttpServer::HttpServer ( Owner & owner ):
    _owner ( owner ),
    _tcpServer ( *this )
{
}

HttpServer::~HttpServer()
{
    for ( HashMap<Socket *, SocketState>::Iterator it ( _socks ); it.isValid(); it.next() )
    {
        if ( it.key() != 0 )
        {
            it.key()->unrefOwner ( this );
        }
    }

    _socks.clear();
}

void HttpServer::closeListeners()
{
    _tcpServer.closeListeners();
}

ERRCODE HttpServer::addListener ( const SockAddr & localAddr, int backlog )
{
    const ERRCODE eCode = _tcpServer.addListener ( localAddr, 0, backlog );

    LOG_ERR ( IS_OK ( eCode ) ? L_INFO : L_ERROR, eCode, "Adding listener: " << localAddr );

    return eCode;
}

void HttpServer::socketDataReceived ( Socket * sock, MemHandle & data )
{
    if ( !sock || !_socks.contains ( sock ) )
        return;

    SocketState & sockState = _socks[ sock ];

    if ( !sockState.writeHeaderBuf.isEmpty() || !sockState.writePayloadBuf.isEmpty() )
    {
        LOG ( L_DEBUG3, sock->getLogId() << ": Received additional data after the initial request; Ignoring" );
        return;
    }

    if ( sockState.readBuf.isEmpty() )
    {
        sockState.readBuf = data;
    }
    else
    {
        MemHandle newData ( sockState.readBuf.size() + data.size() );

        memcpy ( newData.getWritable(), sockState.readBuf.get(), sockState.readBuf.size() );
        memcpy ( newData.getWritable ( sockState.readBuf.size() ), data.get(), data.size() );

        sockState.readBuf = newData;
    }

    LOG ( L_DEBUG4, sock->getLogId() << ": ReadBuf '" << sockState.readBuf.toString() << "'" );

    HttpParser httpParser;
    const HttpParser::HttpParserState pState = httpParser.parse ( sockState.readBuf );

    switch ( pState )
    {
        case HttpParser::ParseIncomplete:
            LOG ( L_DEBUG3, sock->getLogId() << ": Incomplete headers, waiting for more data" );
            return;
            break;

        case HttpParser::ParseFailed:
            LOG ( L_DEBUG2, sock->getLogId() << ": Parsing failed; Closing the socket" );

            _socks.remove ( sock );
            sock->unrefOwner ( this );
            return;
            break;

        case HttpParser::ParseHeadersDone:
            // We handle it below.
            break;
    }

    assert ( pState == HttpParser::ParseHeadersDone );

    if ( httpParser.getMethod() != HttpParser::MethodGet )
    {
        LOG ( L_DEBUG, sock->getLogId()
              << ": Unsupported method: " << httpParser.getMethod() << " (" << httpParser.getMethodName() << ")" );

        const String respStr = String ( "HTTP/%1 %2 %3\r\n\r\n" )
                               .arg ( httpParser.isHttp10() ? "1.0" : "1.1" )
                               .arg ( StatusMethodNotAllowed )
                               .arg ( getStatusCodeDesc ( StatusMethodNotAllowed ) );

        sockState.writeHeaderBuf = MemHandle ( respStr.length() );
        memcpy ( sockState.writeHeaderBuf.getWritable(), respStr.c_str(), respStr.length() );

        sendData ( sock );
        return;
    }

    SockAddr remAddr;

    if ( sock->getIpSocket() != 0 )
    {
        remAddr = sock->getIpSocket()->getRemoteSockAddr();
    }

    HashMap<String, String> respHeaders;
    String respContentType;

    const int respCode = _owner.httpHandleGetRequest (
        this, remAddr, httpParser, respHeaders, respContentType, sockState.writePayloadBuf );

    String respStr = String ( "HTTP/%1 %2 %3\r\n%4: %5\r\n" )
                     .arg ( httpParser.isHttp10() ? "1.0" : "1.1" )
                     .arg ( respCode )
                     .arg ( getStatusCodeDesc ( respCode ) )
                     .arg ( HdrContentLength )
                     .arg ( sockState.writePayloadBuf.size() );

    if ( !sockState.writePayloadBuf.isEmpty() )
    {
        // Doesn't make sense to specify the content type, if there is no content...
        respStr.append ( String ( "%1: %2\r\n" ).arg ( HdrContentType, respContentType ) );
    }

    for ( HashMap<String, String>::Iterator it ( respHeaders ); it.isValid(); it.next() )
    {
        String hName = it.key().removeChars ( " \t\r\n:" );

        if ( hName.isEmpty()
             || hName.compare ( HdrContentType, false ) == 0
             || hName.compare ( HdrContentLength, false ) == 0 )
        {
            continue;
        }

        String hValue = it.key().removeChars ( "\r\n" );

        if ( !hValue.isEmpty() )
        {
            respStr.append ( String ( "%1: %2\r\n" ).arg ( hName, hValue ) );
        }
    }

    respStr.append ( "\r\n" );

    sockState.writeHeaderBuf = MemHandle ( respStr.length() );
    memcpy ( sockState.writeHeaderBuf.getWritable(), respStr.c_str(), respStr.length() );

    sendData ( sock );
    return;
}

void HttpServer::sendData ( Socket * sock )
{
    SocketState & state = _socks[ sock ];

    if ( !state.writeHeaderBuf.isEmpty() )
    {
        sock->send ( state.writeHeaderBuf );

        if ( !state.writeHeaderBuf.isEmpty() )
        {
            return;
        }
    }

    if ( !state.writePayloadBuf.isEmpty() )
    {
        sock->send ( state.writePayloadBuf );

        if ( !state.writePayloadBuf.isEmpty() )
        {
            return;
        }
    }

    // close socket
    LOG ( L_DEBUG3, sock->getLogId() << ": No more data to send; Closing socket" );
    _socks.remove ( sock );
    sock->unrefOwner ( this );
}

void HttpServer::socketReadyToSend ( Socket * sock )
{
    if ( !sock || !_socks.contains ( sock ) )
        return;

    sendData ( sock );
}

void HttpServer::socketClosed ( Socket * sock, ERRCODE reason )
{
    ( void ) reason;

    if ( _socks.remove ( sock ) > 0 )
    {
        LOG_ERR ( L_DEBUG2, reason, sock->getLogId() << ": Socket removed" );
        sock->unrefOwner ( this );
    }
}

void HttpServer::socketConnectFailed ( Socket * sock, ERRCODE reason )
{
    // We treat 'closed' and 'connect failed' the same way...
    socketClosed ( sock, reason );
}

void HttpServer::socketConnected ( Socket * sock )
{
    if ( sock != 0 )
    {
        LOG ( L_WARN, sock->getLogId() << ": Ignoring unexpected callback" );
    }
}

void HttpServer::incomingTcpConnection ( TcpServer * tcpServer, uint8_t, TcpSocket * socket )
{
    ( void ) tcpServer;
    assert ( tcpServer == &_tcpServer );

    if ( socket != 0 )
    {
        LOG ( L_DEBUG2, socket->getLogId() << ": Added new socket" );

        socket->refOwner ( this );

        _socks.insert ( ( Socket * ) socket, SocketState() );
    }
}

const char * HttpServer::getStatusCodeDesc ( int code )
{
    switch ( ( StatusCode ) code )
    {
        case StatusOK:
            return "OK";

        case StatusNotFound:
            return "Not Found";

        case StatusMethodNotAllowed:
            return "Method Not Allowed";
    }

    return "";
}
