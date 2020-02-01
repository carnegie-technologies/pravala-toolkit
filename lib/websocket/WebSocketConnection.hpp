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

#pragma once

#include "basic/List.hpp"
#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"
#include "http/HttpParser.hpp"
#include "object/PooledOwnedObject.hpp"
#include "event/EventManager.hpp"
#include "event/Timer.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
class WebSocketConnection;
class WebSocketListener;
class WebSocketHandler;

/// @brief Owner of a WebSocketConnection
class WebSocketConnectionOwner
{
    protected:
        /// @brief Called when a WebSocketConnection has read some WebSocket framed data.
        /// This can only get called after wsEstablished has been called.
        /// @param [in] sock WebSocketConnection that has read some data
        /// @param [in] data Data that was read
        /// @param [in] isText True if the data read was "text", otherwise it was binary
        virtual void wsRead ( WebSocketConnection * sock, MemHandle & data, bool isText ) = 0;

        /// @brief Called when a WebSocketConnection has closed its connection.
        /// This can be called at any time.
        /// @param [in] sock WebSocketConnection that has closed its connection
        virtual void wsClosed ( WebSocketConnection * sock ) = 0;

        friend class WebSocketConnection;
};

/// @brief Class that performs the HTTP WebSocket negotiation, then passes WebSocket frames if negotiation succeeds
class WebSocketConnection:
    public PooledOwnedObject<WebSocketConnection, WebSocketConnectionOwner>,
    public EventManager::FdEventHandler,
    public Timer::Receiver
{
    public:
        /// @brief Queue some data to be written out of this WebSocketConnection
        /// This will not generate any callbacks.
        /// @param [in] data Data to write
        /// @param [in] len Length of data to write
        /// @param [in] isText True if data should be framed as text, false if it should framed as binary
        /// @return True if the write was queued, false otherwise (i.e. if this WebSocket isn't connected)
        bool send ( const char * data, size_t len, bool isText );

        /// @brief Queue some data to be written out of this WebSocketConnection
        /// This will not generate any callbacks.
        /// @param [in] data Data to write
        /// @param [in] isText True if data should be framed as text, false if it should framed as binary
        /// @return True if the write was queued, false otherwise (i.e. if this WebSocket isn't connected)
        bool send ( const MemHandle & data, bool isText );

        /// @brief Shuts down this WebSocketConnection, sending a Close frame if needed
        /// @param [in] force Immediately close the socket without sending the Close message
        void shutdown ( bool force );

        /// @brief Gets the URL of this connection
        /// @return The URL of this connection
        /// This can be empty if the URL hasn't been determined yet.
        inline const String & getUrl() const
        {
            return _url;
        }

        /// @brief Gets the negotiated protocol of this connection
        /// @return The negotiated protocol of this connection.
        /// This can be empty if the negotiated protocol was the default, or if negotiation hasn't completed yet.
        inline const String & getProtocol() const
        {
            return _protocol;
        }

    protected:
        virtual void receiveFdEvent ( int fd, short events );
        virtual void timerExpired ( Timer * timer );

        /// @brief Called by WebSocketListener to generate a new WebSocketConnection object
        /// @param [in] listener Listener that generated this object
        /// @param [in] fd FD of the TCP connection
        static WebSocketConnection * generate ( WebSocketListener * listener, int fd );

        friend class WebSocketListener;

    private:
        /// @brief The connection state of a WebSocketConnection
        enum ConnectionState
        {
            Disconnected,   ///< Disconnected
            ServerWait,     ///< Connected in server mode, waiting for WebSocket negotiation to complete
            // ClientWait (TODO) ///< Connected in client mode, waiting for WebSocket negotiation to complete
            Established,    ///< WebSocket negotiation completed

            /// @brief WebSocket connection closing
            ///
            /// This state is only used after WebSocket negotiation has completed. It is used to give the other side
            /// some time to respond to our WebSocket close frame before we actually close the socket.
            ///
            /// Sends from the external API will fail if the connection is in this state.
            /// Sends from internal code are expected to check the state before sending if it matters.
            ///
            /// There is no guarantee that any packets in the write queue will be sent
            /// if the state is Closing.
            ///
            /// When entering this state, a timer should be set to close the socket, in case the other side doesn't
            /// respond.
            ///
            WsClosing,

            /// @brief Connection closing
            ///
            /// This state is used to try to flush our write buffer to the other side before closing the socket.
            ///
            /// There is no guarantee that any packets in the write queue will be sent
            /// if the state is Closing.
            ///
            /// When entering this state, a timer should be set to close the socket, in case the other side doesn't
            /// respond.
            ///
            Closing
        };

        /// @brief State for receiving fragmented messages
        enum ContinuationState
        {
            ContinueNone,   ///< Not receiving a fragmented message
            ContinueText,   ///< Receiving a fragmented message, next fragment will be treated as text
            ContinueBinary  ///< Receiving a fragmented message, next fragment will be treated as binary
        };

        static TextLog _log; ///< Static log

        SimpleTimer _timer; ///< Timer to close a connection that isn't in the "Established" state

        List<MemHandle> _writeQueue; ///< Queue of frames to write
        RwBuffer _readBuf; ///< Read buffer

        HttpParser _parser; ///< HTTP parser

        String _url; ///< URL this connection is handling
        String _protocol; ///< Negotiated protocol for this connection

        ConnectionState _state; ///< Current connection state of this object
        ContinuationState _continueState; ///< Current continuation state of this object

        /// @brief If server, the listener that generated this server
        /// @note This pointer is only set and used while the _state is ServerWait.
        WebSocketListener * _listener;

        int _fd; ///< The FD of the TCP connection

        bool _isServer; ///< True if this connection is in server mode

        /// @brief Constructor
        WebSocketConnection();

        /// @brief Destructor
        ~WebSocketConnection();

        /// @brief Called when this object returns to pool
        void returnsToPool();

        /// @brief Called to generate a new WebSocketConnection object
        /// @return A new WebSocketConnection object
        static WebSocketConnection * generateNew()
        {
            return new WebSocketConnection();
        }

        /// @brief Called to handle incoming data when in negotiation (ServerWait/ClientWait) state
        void handleHttp();

        /// @brief Called to handle incoming data after the header has been parsed while we are in the ServerWait state
        void handleHttpServer();

        /// @brief Called to handle incoming data after the header has been parsed while we are in the ClientWait state
        void handleHttpClient();

        /// @brief Processes a WebSocket frame.
        ///
        /// This tries to parse a WebSocket frame and if possible delivers the data via a callback.
        ///
        /// This function may call callbacks, so the caller must return after calling this function
        void processWebSocketFrame();

        /// @brief Handle a return code from send/recv
        /// If there was a fatal error, this will close the socket and notify the owner.
        /// Otherwise, if it was a non-fatal error (e.g. EAGAIN or similar), this will do nothing.
        ///
        /// This function may call callbacks, so the caller must return after calling this function
        ///
        /// @param [in] ret Return code to handle
        void handleRetError ( ssize_t ret );

        /// @brief Send a message without WebSocket framing (e.g. HTTP)
        /// If there was insufficient memory to copy the msg, the socket will be closed.
        /// @param [in] msg String containing the message to send
        /// @param [in] len Length of message to send
        /// @param [in] closeSock If true, also set the state to Closing (prepare to close the socket)
        void sendRawMsg ( const char * msg, size_t len, bool closeSock );

        /// @brief Send a message without WebSocket framing (e.g. HTTP)
        /// If there was insufficient memory to copy the msg, the socket will be closed.
        /// @param [in] msg String containing the message to send
        /// @param [in] closeSock If true, also set the state to Closing (prepare to close the socket)
        inline void sendRawMsg ( const char * msg, bool closeSock )
        {
            sendRawMsg ( msg, strlen ( msg ), closeSock );
        }

        /// @brief Send a WebSocket close frame and set the state to Closing
        /// @note This should only be called if the _state is currently Established
        void sendWebSocketClose();

        /// @brief Immediately close the FD and reset the internal state
        void close();

        POOLED_OWNED_OBJECT_HDR ( WebSocketConnection, WebSocketConnectionOwner );
};
}
