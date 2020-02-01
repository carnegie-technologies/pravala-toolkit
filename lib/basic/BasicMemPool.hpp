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

#include "internal/MemPool.hpp"

namespace Pravala
{
class MemHandle;

/// @brief A basic implementation of a MemPool.
/// It allocates large "slabs" of continuous memory.
/// Each slab is a (configured) number of memory blocks (of configured size each).
/// Once it runs out of blocks, it tries to allocate another slab, up to configured limit.
class BasicMemPool: public MemPool
{
    public:
        /// @brief The number of blocks per slab.
        /// A single slab is allocated as a single, continuous segment of memory.
        /// When the pool runs out of packets additional slabs will be generated, up to MaxSlabs.
        const size_t BlocksPerSlab;

        /// @brief Max number of slabs.
        const size_t MaxSlabs;

        /// @brief The tag to be assigned to each generated memory block.
        const uint8_t MemTag;

        /// @brief Constructor.
        /// @param [in] payloadSize The size (in bytes) of payload data in each block that is a part of this pool
        ///                       (NOT including the block header).
        /// @param [in] blocksPerSlab The number of blocks per slab.
        /// @param [in] maxSlabs Max number of slabs (each slab is a collection of blocks).
        /// @param [in] payloadOffset The offset (in bytes) after the beginning of each block at which the payload
        ///                           memory starts. It MUST be at least the size of PoolMemBlock, AND a multiple of 4!
        /// @param [in] memTag The tag to be assigned to each generated memory block.
        BasicMemPool (
            size_t payloadSize, size_t blocksPerSlab,
            size_t maxSlabs = 4, size_t payloadOffset = DefaultPayloadOffset, uint8_t memTag = 0 );

        /// @brief Gets a MemHandle from the pool.
        /// @param [in] useFallback If true and the pool is empty,
        ///                         this method will allocate regular memory of the same size.
        ///                         Otherwise the MemHandle returned might be empty.
        /// @return A MemHandle from the pool; If the pool is empty, the handle returned will be either empty,
        ///         or it will use regular memory (depending on useFallback value).
        MemHandle getHandle ( bool useFallback = true );

        using MemPool::shutdown;

    protected:
        /// @brief Destructor.
        /// @note If a custom implementation of removeSlab() is required, removeSlabs() should be called
        ///       from a destructor at a higher level. Otherwise it will be called from this destructor
        ///       and use base version of removeSlab().
        virtual ~BasicMemPool();

        /// @brief Removes all slabs.
        /// It will call removeSlab() on each of existing slabs.
        void removeSlabs();

        /// @brief Generates a new slab.
        /// The slab generated MUST have at least ( PayloadOffset + PayloadSize ) * BlocksPerSlab bytes of data!
        /// Default version allocates the memory using posix_memalign on POSIX systems
        /// (using Platform::PageSize alignment), or malloc() on other systems.
        /// @return Pointer to newly generated slab, or 0 if it could not be generated.
        virtual char * generateSlab();

        /// @brief Removes given slab.
        /// Default version simply calls free() on the passed pointer.
        /// @param [in] slab Pointer to the slab to remove.
        virtual void removeSlab ( char * slab );

        virtual void addMoreBlocks();

    private:
        /// @brief An array of pointers to individual slabs.
        /// Each slab is represented as a pointer to the first byte of it.
        /// If the pointer is 0, that slab has not been allocated.
        /// Slabs are never deallocated (until we deallocate the entire store).
        char ** _slabs;

        size_t _allocatedSlabCount; ///< The number of allocated slabs (synchronized).
};
}
