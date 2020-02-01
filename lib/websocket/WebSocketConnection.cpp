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
#include <cstdlib>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#if ( __GNUC__ >= 6 ) || ( defined __clang__ )
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

extern "C"
{
#include <openssl/sha.h>
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include "base64/Base64.hpp"
#include "sys/SocketApi.hpp"

#include "internal/WebSocketFrameHeader.hpp"
#include "internal/WebSocketFrame.hpp"
#include "WebSocketConnection.hpp"
#include "WebSocketListener.hpp"

// From RFC 6455
#define WS_GUID            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// If the connection is idle and isn't in the Established state, it will time out after this many milliseconds.
#define WS_WAIT_TIMEOUT    5000

// If WebSocket negotiation has completed, and we've requested the socket to be closed, or we received a close
// request from the other side, the maximum amount of time we'll wait (e.g. for the other side to ACK the close)
// before we close the socket. In milliseconds.
#define WS_CLOSING_TIMEOUT    5000

// If we want to close the socket, the maximum amount of time we'll wait before closing the socket (to allow
// for the packets to be transmitted). In milliseconds.
#define CLOSING_TIMEOUT       1000

// Number of bytes to read at a time
#define READ_SIZE             1024

// We only support WebSocket version 13 (RFC 6455) right now.
#define WS_VERSION            "13"

// Below are various headers or parts of a header to send in response to a client connection.
//
// WebSocket is not supported in HTTP prior to 1.1, and the header framing is likely going to be different for
// HTTP versions after 1.1.
//
// We simply close the connection if the client doesn't talk HTTP 1.1.

// Complete header to send when the URL was not found
#define SRV_HTTP404 \
    "HTTP/1.1 404 Not Found\r\n" \
    "Connection: close\r\n\r\n"

// Complete header to send when the request was bad
#define SRV_HTTP400_BAD_REQ \
    "HTTP/1.1 400 Bad Request\r\n" \
    "Connection: close\r\n\r\n"

// Complete header to send to request a specific WebSocket version.
#define SRV_HTTP400_VERSION \
    "HTTP/1.1 400 Bad Request\r\n" \
    "Sec-WebSocket-Version: " WS_VERSION "\r\n\r\n"

// Part of the header to send when switching to WebSocket mode
#define SRV_HTTP101_UPGRADE \
    "HTTP/1.1 101 Switching Protocols\r\n" \
    "Upgrade: websocket\r\n" \
    "Connection: Upgrade\r\n"

// Key of the header to include as part of the header to send when switching to WebSocket mode.
// This is the server's response to the client's Sec-WebSocket-Key: header.
#define SRV_HDR_WS_ACCEPT      "Sec-WebSocket-Accept: "

// Key of the header to include as part of the header to send when switching to WebSocket mode to select the protocol
#define SRV_HDR_WS_PROTOCOL    "Sec-WebSocket-Protocol: "

// CRLF to mark the end of a header
#define CRLF                   "\r\n"

using namespace Pravala;

TextLog WebSocketConnection::_log ( "web_socket_connection" );

WebSocketConnection::WebSocketConnection():
    _timer ( *this ),
    _state ( Disconnected ),
    _continueState ( ContinueNone ),
    _listener ( 0 ),
    _fd ( -1 ),
    _isServer ( false )
{
}

WebSocketConnection::~WebSocketConnection()
{
    assert ( _state == Disconnected );
    assert ( _continueState == ContinueNone );
    assert ( !_listener );
    assert ( _fd < 0 );
    assert ( !_timer.isActive() );
}

WebSocketConnection * WebSocketConnection::generate ( WebSocketListener * listener, int fd )
{
    if ( fd < 0 || !listener )
        return 0;

    WebSocketConnection * ws = getFromPool ( listener );

    assert ( ws != 0 );

    if ( !ws )
        return 0;

    assert ( ws->_state == Disconnected );
    assert ( ws->_continueState == ContinueNone );
    assert ( ws->_fd == -1 );
    assert ( !ws->_listener );

    ws->_listener = listener;
    ws->_fd = fd;
    ws->_timer.start ( WS_WAIT_TIMEOUT );
    ws->_state = ServerWait;
    ws->_isServer = true;

    EventManager::setFdHandler ( fd, ws, EventManager::EventRead );

    return ws;
}

void WebSocketConnection::close()
{
    _writeQueue.clear();
    _readBuf.clear();
    _parser.reset();

    _state = Disconnected;
    _continueState = ContinueNone;
    _listener = 0;

    if ( _fd >= 0 )
    {
        EventManager::closeFd ( _fd );
        _fd = -1;
    }

    _timer.stop();

    // In case the owner still needs to know what the URL or protocol were, we leave those
    // until we return to pool.
}

void WebSocketConnection::returnsToPool()
{
    close();

    _url.clear();
    _protocol.clear();
}

void WebSocketConnection::handleRetError ( ssize_t ret )
{
    assert ( ret <= 0 );

    if ( ret < 0
#ifdef SYSTEM_WINDOWS
         && WSAGetLastError() == WSAEWOULDBLOCK
#else
         && ( errno == EAGAIN || errno == EWOULDBLOCK )
#endif
    )
    {
        // Error occurred, but not a fatal one, so we don't need to close it.
        return;
    }

    LOG ( L_DEBUG, "Fatal error, closing socket. Connection may already be closed. Instance: " << ( uint64_t ) this );

    // Fatal error, close and notify owner
    close();

    WebSocketConnectionOwner * owner = getOwner();

    if ( owner != 0 )
    {
        owner->wsClosed ( this );
    }
}

void WebSocketConnection::receiveFdEvent ( int fd, short int events )
{
    assert ( fd == _fd );
    assert ( _fd >= 0 );
    ( void ) fd;

    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        char * const buf = _readBuf.getAppendable ( READ_SIZE );

        assert ( buf != 0 );

        if ( !buf )
        {
            errno = ENOMEM;
            handleRetError ( -1 );
            return;
        }

        const ssize_t ret = recv ( fd, buf, READ_SIZE, 0 );

        if ( ret <= 0 )
        {
            handleRetError ( ret );
            return;
        }

        _readBuf.markAppended ( ret );

        switch ( _state )
        {
            case Disconnected:
                // We shouldn't get any events when we're in the disconnected state.
                //
                // i.e. the last event of a socket should put it into the disconnected, and we shouldn't
                // get anything afterwards. The server socket starts in the ServerWait state.
                /// @todo What state does the client socket start in?
                assert ( false );
                abort();
                break;

            case ServerWait:
                handleHttp();
                break;

            case Established:
            case WsClosing:
                // This may call callbacks, i.e. we may no longer exist after this, so return after calling it.
                processWebSocketFrame();
                return;
                break;

            case Closing:
                // If we get something from the other side while we're closing, ignore it and just close the socket.
                close();
                return;
        }
    }

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        while ( !_writeQueue.isEmpty() )
        {
            MemHandle & mh = _writeQueue.first();

            ssize_t ret = ::send ( fd, mh.get(), mh.size(), 0 );

            if ( ret <= 0 )
            {
                handleRetError ( ret );
                return;
            }

            if ( ( size_t ) ret == mh.size() )
            {
                // We wrote this entire MemHandle, remove it from the queue and continue

                _writeQueue.removeFirst();
                continue;
            }

            assert ( ret > 0 );

            // We wrote some of it, just consume it from the MemHandle
            mh.consume ( ret );
        }

        if ( _writeQueue.isEmpty() )
        {
            EventManager::disableWriteEvents ( _fd );
        }
    }
}

void WebSocketConnection::timerExpired ( Timer * timer )
{
    assert ( timer == &_timer );
    ( void ) timer;

    LOG ( L_DEBUG, "Timer expired, closing socket. Instance: " << ( uint64_t ) this );

    close();
}

void WebSocketConnection::handleHttp()
{
    const HttpParser::HttpParserState parseState = _parser.parse ( _readBuf );

    if ( parseState != HttpParser::ParseHeadersDone )
    {
        if ( parseState == HttpParser::ParseFailed )
        {
            LOG ( L_DEBUG, "HTTP headers had fatal parsing error. Closing. Instance: " << ( uint64_t ) this
                  << "; Error: " << _parser.getErrorName() << ": " << _parser.getErrorDesc() );

            close();
        }

        // Incomplete headers, nothing more to do yet
        return;
    }

    if ( _parser.isHttp10() )
    {
        LOG ( L_DEBUG, "HTTP 1.0 not supported. Closing. Instance: " << ( uint64_t ) this );

        close();
        return;
    }

    if ( !_parser.isUpgrade() )
    {
        LOG ( L_DEBUG, "Non-upgrade HTTP requests are not supported. Closing. Instance: "
              << ( uint64_t ) this );

        if ( _isServer )
        {
            sendRawMsg ( SRV_HTTP400_BAD_REQ, strlen ( SRV_HTTP400_BAD_REQ ), true );
            return;
        }

        close();
        return;
    }

    if ( _parser.getHeaders().value ( "Upgrade" ).toLower() != "websocket" )
    {
        LOG ( L_DEBUG, "Bad upgrade. Closing. Instance: " << ( uint64_t ) this );

        if ( _isServer )
        {
            sendRawMsg ( SRV_HTTP400_BAD_REQ, strlen ( SRV_HTTP400_BAD_REQ ), true );
            return;
        }

        close();
        return;
    }

    if ( _isServer )
    {
        handleHttpServer();
    }
    else
    {
        handleHttpClient();
    }
}

void WebSocketConnection::handleHttpServer()
{
    String wsKey = _parser.getHeaders().value ( "Sec-WebSocket-Key" );

    if ( wsKey.isEmpty() )
    {
        LOG ( L_DEBUG, "Server: Missing key header, closing. Instance: " << ( uint64_t ) this );

        sendRawMsg ( SRV_HTTP400_BAD_REQ, strlen ( SRV_HTTP400_BAD_REQ ), true );
        return;
    }

    const String cliRequestedVersions = _parser.getHeaders().value ( "Sec-WebSocket-Version" );
    const StringList cliVersions = cliRequestedVersions.split ( ", " );

    if ( !cliVersions.findValue ( WS_VERSION ) )
    {
        LOG ( L_DEBUG, "Server: Unsupported version. Client requested: '" << cliRequestedVersions
              << "'; Server supports: " << WS_VERSION << "; Instance: " << ( uint64_t ) this );

        // We don't want to close it, since if the client supports our version, they can continue to
        // negotiate by supporting our version.
        sendRawMsg ( SRV_HTTP400_VERSION, strlen ( SRV_HTTP400_VERSION ), false );
        return;
    }

    const StringList cliProtocols = _parser.getHeaders().value ( "Sec-WebSocket-Protocol" ).split ( ", " );

    WebSocketHandler * handler = 0;
    String protocol;

    // We shouldn't be getting the server callback if we don't have a listener
    assert ( _listener != 0 );

    ERRCODE eCode = _listener->getHandler ( _parser.getUrl(), cliProtocols, handler, protocol );

    LOG_ERR ( L_DEBUG, eCode, "Server: Handler result. Url: '" << _parser.getUrl()
              << "'; Protocols: '" << String::join ( cliProtocols, "','" )
              << "'; Instance: " << ( uint64_t ) this );

    if ( eCode == Error::Unsupported )
    {
        sendRawMsg ( SRV_HTTP400_BAD_REQ, strlen ( SRV_HTTP400_BAD_REQ ), true );
        return;
    }

    if ( NOT_OK ( eCode ) || !handler )
    {
        sendRawMsg ( SRV_HTTP404, strlen ( SRV_HTTP404 ), true );
        return;
    }

    assert ( handler != 0 );

    /// @todo support extensions

    // The client's request is okay. We now need to compose the HTTP header response to send back.

    // To compose the "Accept" response header as per RFC 6455:
    // 1. Append the WebSocket GUID to the client's base64 encoded key request (without decoding the client's key)
    // 2. SHA1 the result of (1)
    // 3. Base64 encode the result of (2)

    wsKey.append ( WS_GUID );

    char wsAccept[ SHA_DIGEST_LENGTH ];

    SHA1 ( ( const unsigned char * ) wsKey.c_str(), wsKey.length(), ( unsigned char * ) wsAccept );

    MemHandle wsAcceptMh = Base64::encode ( wsAccept, SHA_DIGEST_LENGTH );

    LOG ( L_DEBUG2, "WS Key: " << wsKey << "; WS Accept: " << wsAcceptMh.toString()
          << "; Instance: " << ( uint64_t ) this );

    // Compose the HTTP header.

    // Pre-reserve some space, 1024 bytes should be more than enough for the headers that we need to append
    Buffer reply ( strlen ( SRV_HTTP101_UPGRADE ) + 1024 );

    reply.append ( SRV_HTTP101_UPGRADE );

    reply.append ( SRV_HDR_WS_ACCEPT );
    reply.append ( wsAcceptMh );
    reply.append ( CRLF );

    // Only send the protocol field if it isn't empty per RFC 6455
    if ( !protocol.isEmpty() )
    {
        reply.append ( SRV_HDR_WS_PROTOCOL );
        reply.append ( protocol );
        reply.append ( CRLF );
    }

    // Final CRLF for end of header
    reply.appendData ( CRLF, strlen ( CRLF ) );

    if ( _writeQueue.isEmpty() )
    {
        EventManager::enableWriteEvents ( _fd );
    }

    _writeQueue.append ( reply.getHandle() );

    _state = Established;

    _timer.stop();

    LOG_ERR ( L_DEBUG, eCode, "Server: Established, reply queued. Instance: " << ( uint64_t ) this );

    LOG_ERR ( L_DEBUG4, eCode, "Server: Established, reply: " << reply.toString()
              << "; Instance: " << ( uint64_t ) this );

    assert ( _listener != 0 );

    _listener->wsEstablished ( this, handler );
    _listener = 0;
}

void WebSocketConnection::handleHttpClient()
{
    /// @todo implement client mode support

    assert ( false );
}

bool WebSocketConnection::send ( const char * data, size_t len, bool isText )
{
    if ( _state != Established )
        return false;

    /// @todo support extensions

    WebSocketFrame frame ( isText ? WebSocketFrameHeader::OpText : WebSocketFrameHeader::OpBinary,
            true,                // we only support single fragment right now
            _isServer ? false : true,                // needsMask is only true on client
            data, len );

    if ( _writeQueue.isEmpty() )
    {
        EventManager::enableWriteEvents ( _fd );
    }

    frame.appendHandles ( _writeQueue );

    return true;
}

bool WebSocketConnection::send ( const MemHandle & data, bool isText )
{
    if ( _state != Established )
        return false;

    /// @todo support extensions

    WebSocketFrame frame ( isText ? WebSocketFrameHeader::OpText : WebSocketFrameHeader::OpBinary,
            true,                // we only support single fragment right now
            _isServer ? false : true,                // needsMask is only true on client
            data );

    if ( _writeQueue.isEmpty() )
    {
        EventManager::enableWriteEvents ( _fd );
    }

    frame.appendHandles ( _writeQueue );

    return true;
}

void WebSocketConnection::sendRawMsg ( const char * msg, size_t len, bool closeSock )
{
    if ( len < 1 )
        return;

    // Add it directly to the write queue so it doesn't have WebSocket framing
    MemHandle mh ( len );
    char * const mem = mh.getWritable();

    if ( !mem || mh.isEmpty() )
    {
        // No memory to write, we have no choice but to close it
        close();
        return;
    }

    memcpy ( mem, msg, len );

    if ( _writeQueue.isEmpty() )
    {
        EventManager::enableWriteEvents ( _fd );
    }

    _writeQueue.append ( mh );

    if ( closeSock )
    {
        _state = Closing;

        // Start a timer to actually close the socket if we don't get anything else.
        //
        // Use CLOSING_TIMEOUT since this function should only be used when we haven't completed
        // WebSocket negotiation yet.
        _timer.start ( CLOSING_TIMEOUT );
    }
}

void WebSocketConnection::sendWebSocketClose()
{
    /// @todo support optional WebSocket close status codes
    assert ( _state == Established );

    WebSocketFrame frame ( WebSocketFrameHeader::OpClose, true );

    if ( frame.isEmpty() )
    {
        // No memory to create a close message to send, just close the socket
        close();
        return;
    }

    if ( _writeQueue.isEmpty() )
    {
        EventManager::enableWriteEvents ( _fd );
    }

    frame.appendHandles ( _writeQueue );

    _state = WsClosing;

    // Start a timer to actually close the socket if we don't get anything else
    _timer.start ( WS_CLOSING_TIMEOUT );
}

void WebSocketConnection::processWebSocketFrame()
{
    MemHandle data ( _readBuf );

    _readBuf.clear();

    // SELF REFERENCE until we unref ourselves after the switch block.
    // DO NOT RETURN!
    simpleRef();

    while ( !data.isEmpty() )
    {
        WebSocketConnectionOwner * owner = getOwner();

        if ( !owner )
        {
            break;
        }

        WebSocketFrameHeader hdr;
        MemHandle payload;

        if ( !hdr.parseAndConsume ( data, payload ) )
        {
            // Not enough data for complete header
            LOG ( L_DEBUG4, "Incomplete frame. Instance: " << ( uint64_t ) this );
            break;
        }

        assert ( payload.size() == hdr.getPayloadSize() );

        const size_t payloadSize = payload.size();

        LOG ( L_DEBUG4, "Got frame; Header size: " << hdr.getHdrSize() << "; Payload size: " << payloadSize
              << "; Opcode: 0x" << String::number ( hdr.getOpCode(), String::Int_HEX )
              << "; Instance: " << ( uint64_t ) this );

        /// @todo support pre-mask extensions

        if ( hdr.hasMask() )
        {
            LOG ( L_DEBUG4, "Unmasking using mask: 0x" << String::number ( hdr.getMask(), String::Int_Hex ) );

            char * const payloadBuf = payload.getWritable();

            if ( !payloadBuf )
            {
                // Not enough memory to unmask

                LOG ( L_ERROR, "Not enough memory to unmask frame, closing socket" );

                sendWebSocketClose();
                break;
            }

            // In place unmask
            hdr.mask ( payloadBuf, payloadSize );
        }

        /// @todo support post-mask extensions

        switch ( hdr.getOpCode() )
        {
            case WebSocketFrameHeader::OpContinuation:
                // If the OpCode is "Continuation", then this frame is a continuation of the message in the previous
                // frame, and therefore the continuation state must NOT be "none", since we need to know whether this
                // frame is binary or text.
                if ( _continueState == ContinueNone )
                {
                    LOG ( L_DEBUG, "Got continuation, but expecting new frame; Sending close. "
                          << "Instance: " << ( uint64_t ) this );

                    sendWebSocketClose();
                    break;
                }

                if ( owner != 0 )
                {
                    owner->wsRead ( this, payload, _continueState == ContinueText );
                }

                if ( hdr.isFin() )
                {
                    _continueState = ContinueNone;
                }
                break;

            case WebSocketFrameHeader::OpText:
                // If FIN is set, this is the first and only frame of the message (i.e. not in the middle
                // of some other stream of frames).
                //
                // If FIN is not set, the next parts of this message will use the Continuation OpCode.
                //
                if ( _continueState != ContinueNone )
                {
                    LOG ( L_DEBUG, "Got fragment, but expecting continuation; Sending close. "
                          << "Instance: " << ( uint64_t ) this );

                    sendWebSocketClose();
                    break;
                }

                if ( !hdr.isFin() )
                {
                    // This isn't the last frame, set the continue state to Text so that the next time we get
                    // a continuation OpCode, we know we're receiving text.
                    _continueState = ContinueText;
                }

                if ( owner != 0 )
                {
                    owner->wsRead ( this, payload, true );
                }
                break;

            case WebSocketFrameHeader::OpBinary:
                // If FIN is set, this is the first and only frame of the message (i.e. not in the middle
                // of some other stream of frames).
                //
                // If FIN is not set, the next parts of this message will use the Continuation OpCode.
                //
                if ( _continueState != ContinueNone )
                {
                    LOG ( L_DEBUG, "Got fragment, but expecting continuation; Sending close. "
                          << "Instance: " << ( uint64_t ) this );

                    sendWebSocketClose();
                    break;
                }

                if ( !hdr.isFin() )
                {
                    // This isn't the last frame, set the continue state to Binary so that the next time we get
                    // a continuation OpCode, we know we're receiving binary.
                    _continueState = ContinueBinary;
                }

                if ( owner != 0 )
                {
                    owner->wsRead ( this, payload, false );
                }
                break;

            case WebSocketFrameHeader::OpPong:
                /// @todo handle pong messages
                break;

            case WebSocketFrameHeader::OpPing:
                {
                    LOG ( L_DEBUG3, "Got ping, sending pong; Instance: " << ( uint64_t ) this );

                    WebSocketFrame pongFrame ( WebSocketFrameHeader::OpPong, true, false, payload );

                    if ( _writeQueue.isEmpty() )
                    {
                        EventManager::enableWriteEvents ( _fd );
                    }

                    pongFrame.appendHandles ( _writeQueue );

                    break;
                }

            case WebSocketFrameHeader::OpClose:
                if ( _state != Established )
                {
                    LOG ( L_DEBUG, "Got close while not in Established state, immediately closing. Current state: "
                          << _state << "; Instance: " << ( uint64_t ) this );

                    close();
                    break;
                }

                LOG ( L_DEBUG, "Got close, responding with close. Instance: " << ( uint64_t ) this );

                /// @todo support optional close status codes
                sendWebSocketClose();
                break;

            default:
                LOG ( L_DEBUG, "Closing. Got bad opcode: " << String::number ( hdr.getOpCode() )
                      << "; Instance: " << ( uint64_t ) this );

                sendWebSocketClose();
        }
    }

    if ( !data.isEmpty() )
    {
        _readBuf.append ( data );
    }

    simpleUnref();
}

void WebSocketConnection::shutdown ( bool force )
{
    if ( force || _state != Established )
    {
        close();
        return;
    }

    sendWebSocketClose();
}
