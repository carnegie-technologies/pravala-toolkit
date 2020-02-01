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

#include "socket/UdpFdSocket.hpp"

namespace Pravala
{
class Socks5TcpSocketUdpWrapper;

/// @brief Represents a UDP socket that can connect to a SOCKS5 proxy server and send/receive data over UDP
class Socks5UdpSocket: public UdpFdSocket, protected SocketOwner
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] proxyAddr The address of SOCKS5 proxy server to use.
        Socks5UdpSocket ( SocketOwner * owner, const SockAddr & proxyAddr );

        virtual void close();
        virtual int stealSockFd();

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual ERRCODE sendTo ( const SockAddr & addr, const char * data, size_t dataSize );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemHandle & data );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemVector & data );

        virtual UdpSocket * generateConnectedSock ( SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode = 0 );

        /// @brief Returns the address of SOCKS5 proxy.
        /// @return The address of SOCKS5 proxy.
        inline const SockAddr & getProxySockAddr() const
        {
            return _proxyAddr;
        }

        /// @brief Returns the UDP address on which the proxy waits for data from us.
        /// @return The UDP address on which the proxy waits for data from us.
        const SockAddr & getProxyUdpSockAddr() const;

    protected:
        const SockAddr _proxyAddr; ///< The SOCKS5 proxy server to use

        /// @brief Coordinating SOCKS5 TCP socket.
        /// It handles the initial handshake, and has to stay open until the UDP socket is closed.
        Socks5TcpSocketUdpWrapper * _tcpSocket;

        /// @brief Destructor.
        virtual ~Socks5UdpSocket();

        /// @brief Internal function for sending data.
        /// @param [in] addr The address to send the data to.
        /// @param [in] data The data to send.
        /// @param [in] dataSize The size of the data.
        /// @return Standard error code.
        ERRCODE doSend ( const SockAddr & addr, const char * data, size_t dataSize );

        /// @brief Internal function for sending data.
        /// @param [in] addr The address to send the data to.
        /// @param [in] data The data to send.
        /// @return Standard error code.
        ERRCODE doSend ( const SockAddr & addr, const MemVector & data );

        virtual String getLogId ( bool extended = false ) const;

        virtual void doSockClosed ( ERRCODE reason );
        virtual void doSockConnectFailed ( ERRCODE reason );

        virtual void socketClosed ( Socket * sock, ERRCODE reason );
        virtual void socketConnected ( Socket * sock );
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason );
        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketReadyToSend ( Socket * sock );

        virtual void receiveFdEvent ( int fd, short int events );

        friend class Socks5TcpSocketUdpWrapper;
};
}
