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

#include "socket/Socket.hpp"
#include "socket/TcpServer.hpp"

#include "internal/Socks5ReplyMessage.hpp"

namespace Pravala
{
class Socks5ServerSocket;

/// @brief SOCKS5 server that handles SOCKS5 sockets during initial handshake.
/// At the moment it only supports TCP SOCKS5.
class Socks5Server: protected TcpServer::Owner, protected SocketOwner
{
    public:
        /// @brief The owner of the Socks5Server
        class Owner
        {
            protected:
                /// @brief Generates a connecting TCP socket.
                /// Used for generating sockets to connect to remote hosts on behalf of proxy clients.
                /// Default implementation generates a basic TCP socket.
                /// Can be overloaded to generate special socket types.
                /// @param [in] owner The initial owner to set.
                /// @return A new TCP socket object, or 0 on error.
                virtual TcpSocket * socks5GenerateOutboundTcpSocket ( SocketOwner * owner );

                /// @brief Called when TCP link is successfully created for a client.
                /// Sockets passed in this callback are in no way connected to each other,
                /// data forwarding should be done by the owner.
                /// @param [in] clientSock Socket connected to proxy's client.
                ///                        Owner should reference this socket, or it will be destroyed.
                /// @param [in] remoteSock Socket connected to remote host
                ///                        (generated earlier by socks5GenerateConnectingTcpSocket() ).
                ///                        Owner should reference this socket, or it will be destroyed.
                virtual void socks5NewOutboundTcpLink ( TcpSocket * clientSock, TcpSocket * remoteSock ) = 0;

                /// @brief Destructor
                virtual ~Owner();

                friend class Socks5Server;
        };

        /// @brief Constructor
        /// @param [in] owner The owner of this TcpServer object (not in an owned-object sense,
        ///                    it will just receive the callbacks)
        Socks5Server ( Owner & owner );

        /// @brief Creates a new listening socket (IP version)
        /// @param [in] localAddr Local address to listen on
        /// @param [in] extraData Additional data delivered to the owner in callbacks (iface index, for example)
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        ERRCODE addListener ( const SockAddr & localAddr, uint8_t extraData = 0, int backlog = 4 );

        /// @brief Closes all listeners
        void closeListeners();

        /// @brief Destructor
        ~Socks5Server();

    protected:
        /// @brief Called by a Socks5ServerSocket when proxy client requests an outbound TCP connection.
        /// @param [in] sock SOCKS5 socket that received the request.
        /// @param [in] destAddr The address to connect to.
        /// @return SOCKS5 reply code.
        ///         Connection never succeeds right away, so 'success' means 'no error, in progress'.
        Socks5ReplyMessage::Reply socks5TcpConnectRequested ( Socks5ServerSocket * sock, const SockAddr & destAddr );

        virtual void incomingTcpConnection ( TcpServer * tcpServer, uint8_t extraData, TcpSocket * socket );

        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketClosed ( Socket * sock, ERRCODE reason );
        virtual void socketConnected ( Socket * sock );
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason );
        virtual void socketReadyToSend ( Socket * sock );

        /// @brief Removes all sockets associated with the given socket.
        /// Given socket, and all other sockets associated with it are unreferenced and removed from _socks.
        /// If that sock is not part of _socks, or it is not stored in the correct SocketState, only 'sock' is removed.
        /// @param [in] sock The socket to remove.
        void removeSocket ( TcpSocket * sock );

    private:
        /// @brief Contains state associated with with a socket.
        struct SocketState
        {
            Socks5ServerSocket * srvSock; ///< Server socket handling proxy's client.
            TcpSocket * outboundSock;     ///< Outbound TCP socket.

            /// @brief Default constructor.
            inline SocketState(): srvSock ( 0 ), outboundSock ( 0 )
            {
            }
        };

        static TextLog _log; ///< Log stream

        Owner & _owner; ///< Owner of the listener.
        TcpServer _tcpServer; ///< TCP server that we use to wait for incoming TCP connections.

        /// @brief Stores per-socket state.
        /// Each state is stored in up to two copies, once indexed by Socks5ServerSocket,
        /// and once by outbound TcpSocket.
        HashMap<TcpSocket *, SocketState> _socks;

        friend class Socks5ServerSocket;
};
}
