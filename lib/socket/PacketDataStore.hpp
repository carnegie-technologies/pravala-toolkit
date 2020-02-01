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

#include "basic/MemHandle.hpp"
#include "basic/Mutex.hpp"
#include "config/ConfigNumber.hpp"

namespace Pravala
{
class PacketMemPool;

/// @brief Source of MemHandles for network packets.
class PacketDataStore
{
    public:
        /// @brief Max number of slabs (each slab is a collection of blocks) per PacketDataStore.
        static const uint8_t PacketMaxSlabs = 16;

        /// @brief The size of payload in each block.
        /// This does NOT include the block header!
        /// We need to support standard packets (so up to 1500), plus potentially some extra headers.
        /// The standard block header is 16 bytes (on 64 bit arch), and 1520 + 16 gives us a round (in hex) value
        /// of 1536 (= 0x600).
        static const uint16_t PacketSize = 1520;

        /// @brief The size of payload in each "small" block.
        /// This does NOT include the block header!
        /// "Small" packets are used for constructing headers, or optimizing memory usage when the full packet
        /// is actually small (like an empty TCP ACK packet).
        /// Quick tests show that a large percentage of IP packets fits in 72 bytes (IPv6), or in 52 bytes (IPv4).
        /// @todo This may need to be re-examined and adjusted.
        static const uint16_t SmallPacketSize = 72;

        /// @brief The max amount of memory that can be used by the data store (in megabytes).
        static ConfigLimitedNumber<uint32_t> optMaxMemorySize;

        /// @brief The max amount of memory that can be used by the data store for small blocks (in megabytes).
        static ConfigLimitedNumber<uint32_t> optMaxSmallMemorySize;

        /// @brief The minimum size of memory (in bytes) that can be saved to perform packet optimization.
        static ConfigNumber<uint32_t> optMinMemorySavingsToOptimizePackets;

        /// @brief When enabled, packets will be optimized even if the small memory pool is not available.
        /// With this option enabled, if a packet should be optimized (given optMinMemorySavingsToOptimizePackets)
        /// while small memory pool is not available, regular memory will be allocated.
        static ConfigNumber<bool> optForcePacketOptimization;

        /// @brief Returns a new MemHandle for network packet data.
        /// If the data store has not been initialized, or the memory pool is empty,
        /// this function will still return a non-empty MemHandle, but it will use regular memory instead.
        /// @param [in] reqSize The size of the memory requested. It is used as a hint for memory pool selection.
        ///                    Returned handle may be smaller or larger than the size requested.
        /// @return A new MemHandle for network packet data.
        static MemHandle getPacket ( uint16_t reqSize = PacketSize );

        /// @brief Optimizes packet's memory.
        /// Depending on configured options, this packet's memory may be replaced with a smaller memory block.
        /// In that case the content of the packet will be copied to the new memory.
        /// @note If the memory is used to read data that is immediately written elsewhere and the memory released,
        ///       it does NOT make sense to optimize packets.
        ///       It only makes sense to call this if the data is to be stored for later.
        /// @param [in,out] packet The packet to optimize.
        /// @return True if the memory has been modified; False otherwise.
        static bool optimizePacket ( MemHandle & packet );

        /// @brief Initializes PacketDataStore.
        static void init();

        /// @brief Shuts down PacketDataStore.
        static void shutdown();

        /// @brief Returns the number of free blocks stored by PacketDataStore.
        /// @note This count only includes regular blocks, and NOT the small blocks!
        /// @return The number of free blocks stored by PacketDataStore.
        static size_t getFreeBlocksCount();

        /// @brief Returns the total number of blocks allocated by PacketDataStore.
        /// @note This count only includes regular blocks, and NOT the small blocks!
        /// @return The total number of blocks allocated by PacketDataStore.
        static size_t getAllocatedBlocksCount();

        /// @brief Returns the amount of memory used by PacketDataStore.
        /// @note This value includes both regular AND small blocks!
        /// @return The amount (in bytes) of memory used by PacketDataStore.
        static size_t getAllocatedMemorySize();

        /// @brief Returns the number of times regular memory allocation was used instead of pre-allocated packets.
        /// This counts data store "misses".
        /// They happen when there are no free blocks and no more blocks can be allocated.
        /// @return The number of times regular memory allocation was used instead of packet store.
        static size_t getMisses();

    protected:
        static Mutex _stMutex; ///< Mutex for synchronizing operations.
        static PacketMemPool * _mainPool; ///< Pointer to the main packet memory pool.
        static PacketMemPool * _smallPool; ///< Pointer to the memory pool with small packets.
        static size_t _misses; ///< Counts "misses" (when memory was requested but pool was empty/unavailable).
};
}
