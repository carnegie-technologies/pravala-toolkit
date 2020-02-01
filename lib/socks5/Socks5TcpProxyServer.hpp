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

#include "Socks5Server.hpp"

using namespace Pravala;

/// @brief A SOCKS5 TCP Proxy Server.
class Socks5TcpProxyServer: protected SocketOwner, protected Socks5Server::Owner
{
    public:
        /// @brief Constructor.
        /// @param [in] logLevel The log level for some basic logs. L_DEBUG by default.
        Socks5TcpProxyServer ( Log::LogLevel logLevel = L_DEBUG );

        /// @brief Destructor
        ~Socks5TcpProxyServer();

        /// @brief Creates a new listening socket.
        /// @param [in] localAddr Local address to listen on.
        ERRCODE addListener ( const SockAddr & localAddr );

    protected:
        /// @brief A helper enum that describes the type of the socket.
        enum SockType
        {
            SockInvalid, ///< Invalid type.
            SockClient,  ///< The socket facing SOCKS5 client.
            SockRemote   ///< The socket connected to remote server (on behalf of a client).
        };

        /// @brief Contains a state for each socket the server handles.
        /// Used for both client and remote facing sockets.
        struct SockData
        {
            /// @brief The other socket associated with the link.
            /// If this entry describes a client-facing socket, then this is the remote-facing socket.
            /// Otherwise it is a client-facing socket.
            /// @note This object does NOT hold a reference to this socket!
            TcpSocket * otherSock;

            size_t bytesSent;     ///< Total number of bytes sent over this socket.
            size_t bytesReceived; ///< Total number of bytes received over this socket.

            /// @brief The type of the socket this state describes.
            /// If it is SockClient, then they key in _socks is the client socket,
            /// and 'otherSock' is the remote socket. Otherwise 'otherSock' is the client socket.
            SockType sockType;

            /// @brief Default constructor.
            SockData();

            /// @brief Constructor.
            /// @param [in] sType The type of this socket this state describes.
            ///                   It is OPPOSITE to the type of 'sock'.
            /// @param [in] sock The socket to set as 'otherSock'. Reference count is NOT incremented!
            SockData ( SockType sType, TcpSocket * sock );
        };

        static TextLog _log; ///< Log stream.

        Socks5Server _server; ///< Our SOCKS5 server.

        /// @brief All socket pairs.
        /// It includes both client and remote facing sockets (as keys) and their states.
        /// There are two entries for each pair of sockets: A:SockData_with_B_pointer and B:SockData_with_A_pointer.
        HashMap<Socket *, SockData> _socks;

        const Log::LogLevel _logLevel; ///< The log level to be used by basic logs.

        /// @brief Helper function that removes both sockets from _socks.
        /// @note This function will call unrefOwner() only on sockets that were found (and removed) from _socks.
        /// @param [in] a First socket to remove. Can be 0.
        /// @param [in] b Second socket to remove. Can be 0.
        virtual void removeSocks ( Socket * a, Socket * b );

        virtual void socks5NewOutboundTcpLink ( TcpSocket * clientSock, TcpSocket * remoteSock );
        virtual void socketConnected ( Socket * sock );
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason );
        virtual void socketClosed ( Socket * sock, ERRCODE reason );
        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketReadyToSend ( Socket * sock );
};
