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

#include "UdpSocket.hpp"
#include "PacketReader.hpp"
#include "PacketWriter.hpp"

namespace Pravala
{
/// @brief Represents a basic UDP socket that uses its own file descriptor.
/// Passing UdpFdSocket objects between threads is NOT SUPPORTED.
class UdpFdSocket: public UdpSocket, protected EventManager::FdEventHandler
{
    public:
        /// @brief Used for enabling/disabling asynchronous writes (if possible)
        static ConfigNumber<bool> optUseAsyncWrites;

        /// @brief Used for enabling/disabling multi-writes (if possible, using sendmmsg).
        static ConfigNumber<bool> optUseMultiWrites;

        /// @brief The length of the write queue (if needed).
        static ConfigLimitedNumber<uint16_t> optQueueSize;

        /// @brief The number of messages to read at a time (if possible, using recvmmsg).
        static ConfigLimitedNumber<uint16_t> optMultiReadSize;

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        UdpFdSocket ( SocketOwner * owner );

        /// @brief Connects the socket.
        /// See description in IpSocket.
        /// This class also allows sockets to be disconnected (by connecting to AF_UNSPEC address).
        /// In that case, no callbacks are generated.
        /// @param [in] addr The address to connect the socket to.
        /// @return Standard error code.
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual void close();
        virtual int stealSockFd();
        virtual bool getOption ( int level, int optName, MemHandle & value ) const;

        virtual int increaseRcvBufSize ( int size );
        virtual int increaseSndBufSize ( int size );

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType );

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual ERRCODE sendTo ( const SockAddr & addr, const char * data, size_t dataSize );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemHandle & data );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemVector & data );

        virtual const SockAddr & getLocalSockAddr() const;
        virtual const SockAddr & getRemoteSockAddr() const;

        virtual UdpSocket * generateConnectedSock (
            SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode );

        virtual String getLogId ( bool extended = false ) const;

    protected:
        PacketWriter _writer; ///< PacketWriter used by this socket.
        PacketReader _reader; ///< PacketReader used by this socket.

        SockAddr _localAddr; ///< The local address and port of this socket.

        /// @brief This is the address and port of the remote host.
        /// It's either the host that this socket is connected to, or the last host that the data was received from.
        SockAddr _remoteAddr;

        int _sockFd; ///< Underlying socket file descriptor.

        /// @brief Destructor.
        virtual ~UdpFdSocket();

        /// @brief A helper function that obtains the destination address for 'send' operations.
        /// It will check if provided address is valid, and set addrToUse.
        /// If the socket is connected, destAddr should either be invalid, or the same as _remoteAddr.
        /// If the socket is not connected, destAddr should be valid.
        /// @param [in] destAddr Requested destination address.
        /// @param [out] destAddrToUse Destination address that should be used.
        ///                            If it is invalid after a successful call,
        ///                            it means that no address should be used at all (send() instead of sendto()).
        /// @return True if the destAddr was accepted and data can be sent (using destAddrToUse);
        ///         False if there was something wrong with destAddr and we can't send the data.
        bool getDestAddrToUse ( const SockAddr & destAddr, SockAddr & destAddrToUse );

        /// @brief Handles an error code returned by one of PacketWriter's methods.
        /// @param [in] eCode The result of running PacketWriter::write().
        /// @param [in] addr The address passed to write(). For logging.
        /// @return Error code received.
        ERRCODE handleWriteResult ( ERRCODE eCode, const SockAddr & addr );

        virtual bool sockInitFd ( SocketApi::SocketType sockType, int & sockFd );
        virtual void receiveFdEvent ( int fd, short int events );
};
}
