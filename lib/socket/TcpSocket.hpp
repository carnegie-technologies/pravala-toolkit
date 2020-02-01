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

#include "IpSocket.hpp"

namespace Pravala
{
/// @brief An abstract TCP socket.
class TcpSocket: public IpSocket
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TcpSocket ( SocketOwner * owner );

        /// @brief Tries to detect network MTU based on internal TCP data.
        /// It only works on platforms that expose that data.
        /// @return Detected MTU value, or 0 if it could not be calculated.
        virtual uint16_t getDetectedMtu() const;

        virtual const MemHandle & getReadBuffer() const;

        virtual const SockAddr & getLocalSockAddr() const;
        virtual const SockAddr & getRemoteSockAddr() const;

        virtual TcpSocket * getTcpSocket();

    protected:
        /// @brief Set to mark TCP socket as connected at the TCP socket's level.
        /// This will be set together with the regular 'connected' flag in simple TCP sockets.
        /// If the socket is complex and requires some additional steps, this will be set first.
        static const uint16_t SockTcpFlagConnected = ( 1 << ( SockIpNextFlagShift + 0 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTcpNextEventShift = SockIpNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTcpNextFlagShift = SockIpNextFlagShift + 1;

        SockAddr _localAddr; ///< The local address and port of this socket.
        SockAddr _remoteAddr; ///< The remote address and port of the host this socket is connected or connecting to.

        MemHandle _readBuf; ///< Buffer with the data read from the remote side.

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] localAddr The local address of the connection.
        /// @param [in] remoteAddr The remote address of the connection.
        TcpSocket ( SocketOwner * owner, const SockAddr & localAddr, const SockAddr & remoteAddr );

        virtual SocketApi::SocketType ipSockGetType ( const SockAddr & forAddr );
};
}
