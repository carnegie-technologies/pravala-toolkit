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

#ifdef SYSTEM_WINDOWS
#include "os/shared/packetWriter/CorePacketWriter.hpp"
#else
#include "os/shared/packetWriter/PosixPacketWriter.hpp"
#endif

namespace Pravala
{
/// @brief A class for writing data packets (UDP packets, IP packets).
/// The implementations are platform-specific.
/// The common assumption is, that packets may be lost.
/// Even if one of the 'write' methods succeeds, the data may, eventually, be dropped anyway.
class PacketWriter:
#ifdef SYSTEM_WINDOWS
    public CorePacketWriter
#else
    public PosixPacketWriter
#endif
{
    public:
        /// @brief Constructor.
        /// @param [in] wType The type of the writer.
        /// @param [in] flags Additional flags.
        /// @param [in] queueSize The size of the queue.
        ///                       This is the max number of individual packets that can be buffered.
        /// @param [in] speedLimit The speed limit (in Mbps).
        ///                        It is only supported on Posix platforms and only when threading is enabled.
        ///                        Its precision depends on used bucket size, low limit used with small bucket
        ///                        size may not be enforceable.
        PacketWriter ( WriterType wType, uint16_t flags = 0, uint16_t queueSize = 16, uint16_t speedLimit = 0 );

        /// @brief Destructor.
        ~PacketWriter();

        /// @brief Configures the file descriptor to use.
        /// It will replace currently used file descriptor.
        void setupFd ( int fileDesc );

        /// @brief Stops using currently set file descriptor.
        void clearFd();

        /// @brief Writes the data.
        /// @param [in,out] data The data to write/send. On success this vector will be cleared.
        /// @return Standard error code.
        ERRCODE write ( MemHandle & data );

        /// @brief Writes the data.
        /// @param [in,out] data The data to write/send. On success this vector will be cleared.
        /// @return Standard error code.
        ERRCODE write ( MemVector & data );

        /// @brief Writes the data.
        /// @param [in] data The data to write/send.
        /// @param [in] dataSize The size of the data.
        /// @return Standard error code.
        ERRCODE write ( const char * data, size_t dataSize );

        /// @brief Writes the data.
        /// @note This function will always fail if the writer is in basic mode.
        /// @param [in] addr The address to send the data to. It must be valid.
        /// @param [in,out] data The data to write/send. On success this vector will be cleared.
        /// @return Standard error code.
        ERRCODE write ( const SockAddr & addr, MemHandle & data );

        /// @brief Writes the data.
        /// @note This function will always fail if the writer is in basic mode.
        /// @param [in] addr The address to send the data to. It must be valid.
        /// @param [in,out] data The data to write/send. On success this vector will be cleared.
        /// @return Standard error code.
        ERRCODE write ( const SockAddr & addr, MemVector & data );

        /// @brief Writes the data.
        /// @note This function will always fail if the writer in basic mode.
        /// @param [in] addr The address to send the data to. It must be valid.
        /// @param [in] data The data to write/send.
        /// @param [in] dataSize The size of the data.
        /// @return Standard error code.
        ERRCODE write ( const SockAddr & addr, const char * data, size_t dataSize );
};
}
