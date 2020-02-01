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

#include "basic/HashMap.hpp"
#include "log/TextLog.hpp"
#include "socket/Socket.hpp"
#include "socket/TcpServer.hpp"
#include "HttpParser.hpp"

namespace Pravala
{
/// @brief A very simple HTTP server.
class HttpServer: protected TcpServer::Owner, protected SocketOwner
{
    public:
        /// @brief Should be inherited by classes that want to process incoming requests.
        class Owner
        {
            protected:
                /// @brief Called when an HTTP GET request is received.
                /// @param [in] server The server that received the request.
                /// @param [in] remoteAddr The address from which the request came.
                /// @param [in] request HTTP Parser with request's details.
                /// @param [out] respHeaders The set of headers to include in the response (keyed by header names).
                ///                          Headers will be sanitized (':' and spaces will be removed from names,
                ///                          and \r and \n will be removed from both names and values).
                ///                          Also, Content-Type and Content-Length headers will be ignored.
                /// @param [out] respContentType The content type to use for the payload.
                /// @param [out] respPayload The data to send as a payload.
                /// @return The HTTP code to include in the response.
                virtual int httpHandleGetRequest (
                    HttpServer * server, const SockAddr & remoteAddr, HttpParser & request,
                    HashMap<String, String> & respHeaders, String & respContentType, MemHandle & respPayload ) = 0;

                friend class HttpServer;
        };

        /// @brief Some HTTP status codes.
        enum StatusCode
        {
            StatusOK               = 200, ///< Success.
            StatusNotFound         = 404, ///< Not found.
            StatusMethodNotAllowed = 405  ///< Method not allowed.
        };

        /// @brief Constructor.
        /// @param [in] owner The owner of the server that will receive the callbacks.
        HttpServer ( Owner & owner );

        /// @brief Adds an address to listen on.
        /// @param [in] localAddr Local address to listen on.
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow.
        ERRCODE addListener ( const SockAddr & localAddr, int backlog = 4 );

        /// @brief Closes all listeners.
        void closeListeners();

        /// @brief Destructor.
        virtual ~HttpServer();

        /// @brief Returns the description for the status code.
        /// @param [in] code The status code to return the description for.
        /// @return The description of the status code, or empty string if it's not known (see enum StatusCode).
        static const char * getStatusCodeDesc ( int code );

    protected:
        static TextLog _log; ///< Log stream.

        virtual void incomingTcpConnection ( TcpServer * tcpServer, uint8_t extraData, TcpSocket * socket );

        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketClosed ( Socket * sock, ERRCODE reason );
        virtual void socketConnected ( Socket * sock );
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason );
        virtual void socketReadyToSend ( Socket * sock );

    private:
        /// @brief Stores the socket state with incoming and outgoing data.
        struct SocketState
        {
            MemHandle readBuf;  ///< Buffer with data read.
            MemHandle writeHeaderBuf; ///< Buffer with response header to be written.
            MemHandle writePayloadBuf; ///< Buffer with payload to be written.
        };

        Owner & _owner; ///< Owner of this server (receives callbacks).

        TcpServer _tcpServer; ///< TCP server that we use to wait for incoming TCP connections.

        /// @brief Stores per-socket state.
        HashMap<Socket *, SocketState> _socks;

        /// @brief Writes the header and payload information to the socket.
        /// The socket is closed when the complete header + payload is sent.
        /// @param [in] sock The socket to send the data over.
        void sendData ( Socket * sock );
};
}
