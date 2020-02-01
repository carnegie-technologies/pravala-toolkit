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

#include "basic/SockAddr.hpp"
#include "basic/MemHandle.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"

struct mmsghdr;
struct iovec;

namespace Pravala
{
class LogId;

/// @brief Used for reading multiple packets at a time.
/// It uses recvmmsg or recvfrom, depending on the platform and build configuration.
/// @note Unlike PacketWriter it can only be used with sockets!
class PacketReader
{
    public:
        /// @brief Maximum number of packets to read at a time.
        const uint16_t MaxPackets;

        /// @brief Constructor.
        /// @param [in] maxPackets Maximum number of packets to read at a time.
        PacketReader ( uint16_t maxPackets );

        /// @brief Destructor.
        ~PacketReader();

        /// @brief Reads packets from the socket.
        /// @param [in] fd The socket FD to read packets from.
        /// @param [in] logId The object performing the read (for logging).
        /// @param [out] packetsRead The number of packets received.
        /// @return Standard error code.
        ERRCODE readPackets ( int fd, const LogId & logId, uint16_t & packetsRead );

        /// @brief Gets one of the packets read using readPackets().
        /// It will also remove this packet from internal buffer, so the next call with the same index
        /// will fail (unless readPackets() is called again).
        /// @note It is possible that some of the packets reported as read by readPackets() were invalid
        ///       (they could be, for example, truncated). In that case this function will return 'false'
        ///       for those packets. But even if it does, the packets after the invalid ones may still be valid.
        ///       This means that this function should be called for every index in the range reported by readPackets().
        /// @param [in] idx The index of packet to read.
        /// @param [out] data Received data. On success the content will be replaced with the packet's data.
        /// @param [out] addr The address this packet was received from.
        ///                   On success it will be replaced with the packet's source address.
        ///                   If the packet was received from IPv4 address mapped to IPv6,
        ///                   the address stored here will be converted to IPv4.
        /// @return True if a valid packet was retrieved; False otherwise.
        bool getPacket ( uint16_t idx, MemHandle & data, SockAddr & addr );

    protected:
        static TextLogLimited _log; ///< Log stream.

        struct mmsghdr * const _recvMsgs; ///< Receive message descriptors
        struct iovec * const _recvIovecs; ///< IO vectors for each received message
        SockAddr * const _recvAddrs;      ///< The remote address of each packet received
        MemHandle * const _recvData;      ///< Handles to data received

        /// @brief The last number of packets received.
        /// It is used for re-initializing the state before the next read.
        uint16_t _lastReadCount;
};
}
