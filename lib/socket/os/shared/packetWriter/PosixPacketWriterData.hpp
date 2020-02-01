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

#include "CorePacketWriter.hpp"

struct mmsghdr;

namespace Pravala
{
/// @brief A class that contains core data for PosixPacketWriter.
/// It does not synchronize access to data structures, which should be controlled by the caller.
class PosixPacketWriterData
{
    protected:
        const bool IsSocketWriter; ///< Set to true if configured type was 'SocketWriter'.
        const uint16_t QueueSize; ///< The size of the queue.

        MemVector * const _data;      ///< Array with MemVectors holding all the data.
        SockAddr * const _dest;       ///< Array with all the destinations. Only used in 'Socket' modes.

        /// @brief Array with message headers.
        /// Only used in 'Socket' modes, and only on platforms that support sendmmsg().
        struct mmsghdr * const _msgs;

        /// @brief Constructor.
        /// @param [in] wType The type of the writer.
        /// @param [in] flags Additional flags.
        /// @param [in] queueSize The size of the queue.
        ///                       This is the max number of individual packets that can be buffered.
        ///                       Ignored in 'BasicWriter' mode.
        PosixPacketWriterData ( CorePacketWriter::WriterType wType, uint16_t flags, uint16_t queueSize );

        /// @brief Destructor.
        ~PosixPacketWriterData();

        /// @brief An internal function writing packets from the write queue.
        /// @param [in] fd The file descriptor to write to.
        /// @param [in] index The index in packet array to start writing from.
        /// @param [in] maxPackets The max number of packets to write.
        /// @param [in] maxBytes The max number of bytes to write. It is not strictly enforced,
        ///                      even a single allowed byte will cause an entire packet to be written.
        /// @param [out] packetsWritten The number of packets written will be stored there.
        /// @param [out] bytesWritten The number of bytes written will be stored there.
        ///                           May be larger than maxBytes (see above).
        /// @return Standard error code. It returns:
        ///         'Success' if all available packets have been written;
        ///         'SoftFail' if the socket doesn't, temporarily, accept more data;
        ///         'Closed' if the socket has been closed, or there was a fatal error making the socket unusable.
        ERRCODE dataWritePackets (
            int fd, uint16_t index, uint16_t maxPackets, uint32_t maxBytes,
            uint16_t & packetsWritten, uint32_t & bytesWritten );

        /// @brief An internal function that simply writes a single packet.
        /// @param [in] fd The file descriptor to write to.
        /// @param [in] addr The address to send the data to. Doesn't need to be valid. Ignored in 'basic' mode.
        /// @param [in] data The data to send/write.
        /// @return Standard error code.
        ERRCODE dataWritePacket ( int fd, const SockAddr & addr, MemVector & data );
};
}
