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

#include "basic/HashSet.hpp"
#include "basic/String.hpp"
#include "error/Error.hpp"
#include "object/SimpleObject.hpp"
#include "log/TextLog.hpp"
#include "socket/TcpServer.hpp"

#include "WebSocketHandler.hpp"
#include "WebSocketConnection.hpp"

namespace Pravala
{
/// @brief Owner of a WebSocketListener
class WebSocketListenerOwner
{
    protected:
        /// @brief Gets the handler that can handle a WebSocket connection from a client requesting the specific URL
        /// and protocols, if any.
        /// @param [in] listener Listener calling this function
        /// @param [in] url URL requested by the client
        /// @param [in] protocols List of protocols supported by the client in preference order.
        /// First item in the list is the most preferred protocol by the client.
        /// This can be case sensitive or case insensitive depending on the specifications for each protocol.
        /// @param [out] handler Handler to use (if Error::Success)
        /// @param [out] protocol Protocol to use between the client and this handler (if Error::Success)
        /// This may be empty - which means "use default protocol", which is only valid if protocols is empty.
        /// If non-empty, this must match exactly one of the items in protocols.
        /// @return Standard error code
        ///     Error::Success      - Handler found, and handler is using one of the client's supported protocols
        ///     Error::NotFound     - No handler at the requested URL
        ///                           (HTTP error 404 will be returned to the client)
        ///     Error::Unsupported  - Handler does not supported any of the client requested protocols
        ///                           (HTTP error 400 will be returned to the client)
        virtual ERRCODE getHandler (
            WebSocketListener * listener, const String & url, const StringList & protocols,
            WebSocketHandlerPtr & handler, String & protocol ) = 0;

        friend class WebSocketListener;
};

/// @brief Listens for WebSocket connections
class WebSocketListener:
    public TcpServer::Owner,
    public WebSocketConnectionOwner
{
    public:
        /// @brief Constructor
        /// @param [in] owner Owner of this WebSocketListener
        WebSocketListener ( WebSocketListenerOwner & owner );

        /// @brief Destructor
        ~WebSocketListener();

        /// @brief Creates a new listening socket
        /// @param [in] localAddr Local address to listen on
        /// @param [in] localPort Local port to listen on
        /// @return Standard error code
        ERRCODE addListener ( const IpAddress & localAddr, uint16_t localPort );

    protected:
        virtual void incomingTcpConnection (
            TcpServer * listener,
            uint8_t extraData,
            int sockFd,
            const IpAddress & localAddr,
            uint16_t localPort,
            const IpAddress & remoteAddr,
            uint16_t remotePort );

        virtual void incomingUnixConnection (
            TcpServer * listener,
            uint8_t extraData,
            int sockFd,
            const String & sockName );

        virtual void wsRead ( WebSocketConnection * sock, MemHandle & data, bool isText );
        virtual void wsClosed ( WebSocketConnection * sock );

        /// @brief Called only once, when a WebSocketConnection has established a WebSocket connection,
        /// i.e. completed WebSocket negotiation.
        /// @param [in] sock WebSocketConnection that has established a WebSocket connection
        /// @param [in] handler WebSocketHandler that should continue handling this
        virtual void wsEstablished ( WebSocketConnection * sock, WebSocketHandler * handler );

        /// @brief Gets the handler that can handle a WebSocket connection from a client requesting the specific URL
        /// and protocols, if any.
        /// @param [in] url URL requested by the client
        /// @param [in] protocols List of protocols supported by the client in preference order.
        /// First item in the list is the most preferred protocol by the client.
        /// This can be case sensitive or case insensitive depending on the specifications for each protocol.
        /// @param [out] handler Handler to use (if Error::Success)
        /// @param [out] protocol Protocol to use between the client and this handler (if Error::Success)
        /// This may be empty - which means "use default protocol", which is only valid if protocols is empty.
        /// If non-empty, this must match exactly one of the items in protocols.
        /// @return Standard error code
        ///     Error::Success      - Handler found, and handler is using one of the client's supported protocols
        ///     Error::NotFound     - No handler at the requested URL
        ///                           (HTTP error 404 will be returned to the client)
        ///     Error::Unsupported  - Handler does not supported any of the client requested protocols
        ///                           (HTTP error 400 will be returned to the client)
        inline ERRCODE getHandler (
            const String & url, const StringList & protocols,
            WebSocketHandlerPtr & handler, String & protocol )
        {
            return _owner.getHandler ( this, url, protocols, handler, protocol );
        }

        friend class WebSocketConnection;

    private:
        static TextLog _log; ///< Logging stream

        WebSocketListenerOwner & _owner; ///< Owner of this listener

        TcpServer _tcpServer; ///< Listening TCP server

        HashSet<WebSocketConnection *> _connInProgress; ///< Connections in progress
};
}
