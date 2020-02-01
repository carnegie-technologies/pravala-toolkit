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

#include "basic/BasicMemPool.hpp"

namespace Pravala
{
/// @brief A memory pool that registers generated memory with VhostNetMgr.
/// Only packets that use registered memory can be sent/received using VhostNet.
/// This pool also tags all generated packet using VhostNetMgr::RegisteredMemoryTag.
class VhostNetMemPool: public BasicMemPool
{
    public:
        /// @brief Constructor.
        /// @param [in] payloadSize The size (in bytes) of payload data in each block that is a part of this pool
        ///                       (NOT including the block header).
        /// @param [in] blocksPerSlab The number of blocks per slab.
        /// @param [in] maxSlabs Max number of slabs (each slab is a collection of blocks).
        /// @param [in] payloadOffset The offset (in bytes) after the beginning of each block at which the payload
        ///                           memory starts. It MUST be at least the size of PoolMemBlock, AND a multiple of 4!
        VhostNetMemPool (
            size_t payloadSize, size_t blocksPerSlab,
            size_t maxSlabs = 4, size_t payloadOffset = DefaultPayloadOffset );

    protected:
        /// @brief Destructor.
        virtual ~VhostNetMemPool();

        /// @brief Generates a new slab.
        /// The slab generated MUST have at least ( PayloadOffset + PayloadSize ) * BlocksPerSlab bytes of data!
        /// Default version simply allocates the memory using malloc().
        /// @return Pointer to newly generated slab, or 0 if it could not be generated.
        virtual char * generateSlab();

        /// @brief Removes given slab.
        /// Default version simply calls free() on the passed pointer.
        /// @param [in] slab Pointer to the slab to remove.
        virtual void removeSlab ( char * slab );
};
}
