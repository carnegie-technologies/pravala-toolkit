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

#include "socket/PacketReader.hpp"
#include "socket/PacketWriter.hpp"
#include "socket/UdpListener.hpp"

namespace Pravala
{
/// @brief UDP listener implementation that uses FDs
class UdpFdListener: public UdpListener, protected EventManager::FdEventHandler
{
    public:
        /// @brief Maximum sending speed in Mbps.
        /// It also enables threading mode. 0 means unlimited.
        static ConfigNumber<uint16_t> optMaxSendSpeed;

        /// @brief Maximum number of packets to receive per recvmmsg syscall
        static ConfigLimitedNumber<uint16_t> optMaxRecvPackets;

        /// @brief Maximum number of packets to buffer before sending
        static ConfigLimitedNumber<uint16_t> optMaxSendPackets;

        /// @brief UDP socket receive buffer size (in bytes) to try to use.
        static ConfigLimitedNumber<int> optRecvBufSize;

        /// @brief UDP socket send buffer size (in bytes) to try to use.
        static ConfigLimitedNumber<int> optSendBufSize;

        /// @brief Generates a bound UdpFdListener.
        /// @param [in] localAddr The local address and port to bind to.
        ///                        A "zero" address means "any" address, zero port means dynamically allocated port.
        /// @param [out] errCode If used, the status of the operation will be stored there.
        /// @return A pointer to the newly created UDP listener, or 0 if it could not be created.
        ///         It is caller's responsibility to delete this socket.
        static UdpFdListener * generate ( const SockAddr & localAddr, ERRCODE * errCode = 0 );

        virtual String getLogId ( bool extended = false ) const;

        virtual ERRCODE send ( const SockAddr & sa, const char * data, size_t dataSize );
        virtual ERRCODE send ( const SockAddr & sa, MemHandle & data );
        virtual ERRCODE send ( const SockAddr & sa, MemVector & data );

        virtual int increaseRcvBufSize ( int size );
        virtual int increaseSndBufSize ( int size );
        virtual bool isValid() const;

    protected:
        PacketWriter _writer; ///< PacketWriter used by this listener.
        PacketReader _reader; ///< PacketReader used by this listener.

        int _fd; ///< Underlying file descriptor

        /// @brief Constructor.
        UdpFdListener();

        /// @brief Destructor.
        ~UdpFdListener();

        /// @brief A helper function that closes the socket.
        void closeSock();

        /// @brief Handles an error code returned by one of PacketWriter's methods.
        /// @param [in] eCode The result of running PacketWriter::write().
        /// @param [in] addr The address passed to write(). For logging.
        /// @return Error code received.
        ERRCODE handleWriteResult ( ERRCODE eCode, const SockAddr & addr );

        virtual ERRCODE reinitializeImpl ( const SockAddr & localAddr );

        virtual void receiveFdEvent ( int fd, short int events );

        friend class UdpListener;
};
}
