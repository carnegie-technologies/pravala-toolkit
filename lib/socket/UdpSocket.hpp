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
/// @brief An abstract UDP socket that extends IpSocket with UDP-specific API calls.
class UdpSocket: public IpSocket
{
    public:
        /// @brief Sends the data over the socket.
        /// The data will be sent as a single datagram.
        /// @param [in] addr Remote address/port to send the data to.
        /// @param [in,out] data Data to send. On success this object is cleared.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE sendTo ( const SockAddr & addr, MemHandle & data ) = 0;

        /// @brief Sends the data over the socket.
        /// The data will be sent as a single datagram.
        /// @param [in] addr Remote address/port to send the data to.
        /// @param [in,out] data Data to send. On success this object is cleared.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE sendTo ( const SockAddr & addr, MemVector & data ) = 0;

        /// @brief Sends the data over the socket.
        /// The data will be sent as a single datagram.
        /// @param [in] addr Remote address/port to send the data to.
        /// @param [in] data Pointer to the data to send.
        /// @param [in] dataSize The size of the data to send.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE sendTo ( const SockAddr & addr, const char * data, size_t dataSize ) = 0;

        /// @brief Generates a new UdpSocket object, that will use the same local address
        ///        as this socket, but will be connected to a specific remote host.
        /// @param [in] owner A pointer to the owner of the new socket. Can be null.
        /// @param [in] remoteAddr The remote address to connect to.
        /// @param [out] errCode If used, the status of the operation will be stored there.
        /// @return A pointer to a newly created UDP socket, or 0 if it could not be created.
        ///         It is caller's responsibility to delete this socket.
        ///         If this function succeeds, the new socket will already be in 'connected' state,
        ///         and no future 'connected' or 'connect failed' callbacks will be generated.
        virtual UdpSocket * generateConnectedSock (
            SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode = 0 ) = 0;

        virtual UdpSocket * getUdpSocket();

    protected:
        /// @brief Set to mark UDP socket as connected at the UDP socket's level.
        /// This will be set together with the regular 'connected' flag in simple UDP sockets.
        /// If the socket is complex and requires some additional steps, this will be set first.
        static const uint16_t SockUdpFlagConnected = ( 1 << ( SockIpNextFlagShift + 0 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockUdpNextEventShift = SockIpNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockUdpNextFlagShift = SockIpNextFlagShift + 1;

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        UdpSocket ( SocketOwner * owner );

        virtual SocketApi::SocketType ipSockGetType ( const SockAddr & forAddr );
};
}
