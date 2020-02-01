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

#include "TcpSocket.hpp"

namespace Pravala
{
/// @brief Represents an FD-based TCP socket that can connect to a server and send/receive data over TCP.
/// This version will read a single data chunk from the underlying socket and put it in internal buffer.
/// Then it will deliver it to its owner. If the owner does not consume all of that data, the data not consumed
/// will remain in TcpSocket's internal buffer and TcpSocket will stop reading - until the owner
/// consumes all pending data from TcpSocket's internal buffer using consumeReadBuffer().
class TcpFdSocket: public TcpSocket, protected EventManager::FdEventHandler
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TcpFdSocket ( SocketOwner * owner );

        /// @brief Generates a new TcpFdSocket object that takes over the underlying socket FD.
        /// It can be used to convert more complex socket types to a basic FD-based TCP socket,
        /// after their role (like a special initial handshake) has been fulfilled.
        /// The new socket will have a single reference.
        /// This socket will not be unreferenced by this call, but it will not have a valid FD anymore.
        /// IF there is any data in the read buffer, it will also be transferred to the newly created socket.
        /// @param [in] owner The owner of the new socket. Can be 0.
        /// @return Regular TcpFdSocket socket that uses the same file descriptor,
        ///         or 0 if this socket's state does not allow for it to be converted.
        virtual TcpFdSocket * generateTcpFdSock ( SocketOwner * owner );

        /// @brief Sets the new max read size.
        /// This is the maximum number of bytes that will be read from the file descriptor in a single read call.
        /// @param [in] maxReadSize The new max read size to set. This functions does nothing if it's < 1.
        void setMaxReadSize ( uint16_t maxReadSize );

        virtual uint16_t getDetectedMtu() const;

        virtual void consumeReadBuffer ( size_t size );

        virtual void close();
        virtual int stealSockFd();
        virtual bool getOption ( int level, int optName, MemHandle & value ) const;

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType );
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual String getLogId ( bool extended = false ) const;

        /// @brief Tries to detect network MTU based on internal TCP data.
        /// It only works on platforms that expose that data.
        /// @param [in] fd The socket FD to use.
        /// @return Detected MTU value, or 0 if it could not be calculated.
        static uint16_t detectMtu ( int fd );

    protected:
        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTcpFdNextEventShift = SockTcpNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTcpFdNextFlagShift = SockTcpNextFlagShift;

        int _sockFd; ///< Underlying socket file descriptor.

        uint16_t _maxReadSize; ///< The max size of a single read operation.

        /// @brief Constructor.
        /// Initializes the socket using given parameters.
        /// If the socket FD is valid, it will set 'valid', 'connected' and 'tcp connected' flags.
        /// If this socket is inherited by another socket type that requires additional steps before being
        /// considered 'connected', that flag should be UNSET by the inheriting class after calling this constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] sockFd The already connected socket FD to use.
        /// @param [in] localAddr The local address of the connection.
        /// @param [in] remoteAddr The remote address of the connection.
        TcpFdSocket ( SocketOwner * owner, int sockFd, const SockAddr & localAddr, const SockAddr & remoteAddr );

        /// @brief Constructor.
        /// Initializes the socket using given parameters.
        /// If the socket passed has a valid FD that can be stolen,
        /// it will set 'valid', 'connected' and 'tcp connected' flags.
        /// If this socket is inherited by another socket type that requires additional steps before being
        /// considered 'connected', that flag should be UNSET by the inheriting class after calling this constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] sock Existing TCP connection to use.
        ///                  It should be a TCP socket object that allows for its FD to be stolen.
        ///                  The new socket will take over that FD (if possible),
        ///                  and this socket's read buffer (if not empty and the FD is transferred).
        TcpFdSocket ( SocketOwner * owner, TcpSocket * sock );

        /// @brief Destructor
        virtual ~TcpFdSocket();

        virtual bool sockInitFd ( SocketApi::SocketType sockType, int & sockFd );

        virtual void doSockConnectFailed ( ERRCODE reason );
        virtual void receiveFdEvent ( int fd, short int events );

        friend class TcpServer;
};
}
