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

extern "C"
{
#include "lwip/pbuf.h"
}

#include "basic/MemHandle.hpp"

namespace Pravala
{
/// @brief Wrapper around ExtMemHandle that can reference memory stored in a pbuf object.
/// Instead of copying memory, this handle will reference the memory stored in the pbuf object,
/// and unreference that object once the memory is no longer needed.
/// @note It is only possible with single-part pbuf objects.
///       Also, if the data inside the pbuf object is modified through the pbuf object,
///       MemHandle's memory will also change.
class PBufMemHandle: public ExtMemHandle
{
    public:
        /// @brief Generates a MemHandle object that contains the memory in pbuf object passed.
        /// If pbuf object passed is a multi-part buffer, MemHandle generated will be from PacketDataStore
        /// and will contain copy of the memory. If pbuf object passed is a single-part buffer,
        /// then MemHandle generated will reference the original memory
        /// (in which case a reference in pbuf object will be incremented).
        /// @param [in] buffer Buffer to represent as a MemHandle object.
        /// @return MemHandle containing the memory passed in pbuf.
        static MemHandle getPacket ( struct pbuf * buffer );

    protected:
        /// @brief Constructor.
        /// References memory stored in the buffer passed.
        /// @param [in] buffer The memory to reference. It MUST be a valid, non-empty single-part buffer!
        PBufMemHandle ( struct pbuf * buffer );

        /// @brief Custom deallocator to release pbuf objects backing up PBufMemHandle objects.
        /// It releases pbuf whose pointer is stored in block header's deallocatorData field.
        /// It does NOT free the actual block header.
        /// @param [in] block The block to be released.
        static void releasePBuf ( DeallocatorMemBlock * block );
};
}
