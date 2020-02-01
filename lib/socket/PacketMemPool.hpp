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

#ifdef ENABLE_VHOSTNET
#include "os/shared/vhostnet/VhostNetMemPool.hpp"
#else
#include "basic/BasicMemPool.hpp"
#endif

namespace Pravala
{
/// @brief A wrapper around one of memory pool implementations to be used for network packets.
class PacketMemPool:
#ifdef ENABLE_VHOSTNET
    public VhostNetMemPool
#else
    public BasicMemPool
#endif
{
    public:

        /// @brief Constructor.
        /// @param [in] payloadSize The size (in bytes) of payload data in each block that is a part of this pool
        ///                       (NOT including the block header).
        /// @param [in] blocksPerSlab The number of blocks per slab.
        /// @param [in] maxSlabs Max number of slabs (each slab is a collection of blocks).
        /// @param [in] payloadOffset The offset (in bytes) after the beginning of each block at which the payload
        ///                           memory starts. It MUST be at least the size of PoolMemBlock, AND a multiple of 4!
        inline PacketMemPool (
            size_t payloadSize, size_t blocksPerSlab,
            size_t maxSlabs = 4, size_t payloadOffset = DefaultPayloadOffset ):
#ifdef ENABLE_VHOSTNET
            VhostNetMemPool
#else
            BasicMemPool
#endif
                ( payloadSize, blocksPerSlab, maxSlabs, payloadOffset )
        {
        }
};
}
