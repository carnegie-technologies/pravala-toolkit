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

#include "basic/NoCopy.hpp"
#include "basic/MemVector.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"

struct vring;

namespace Pravala
{
/// @brief Wrapper around a (Linux kernel) virtio ring
/// The Vring on its own is NOT vhost-net specific, however the internalSetup function is!
class Vring: public NoCopy
{
    public:
        /// @brief Contains the values used for the (kernel) indexes of the vrings used for RX and TX
        /// when this vring is used with vhost-net.
        ///
        /// See kernel source: drivers/vhost/net.c
        enum NetVringIdx
        {
            RxVringIdx = 0, ///< Index of the vring used for RX (kernel symbol VHOST_NET_VQ_RX)
            TxVringIdx = 1  ///< Index of the vring used for TX (kernel symbol VHOST_NET_VQ_TX)
        };

        /// @brief Gets the number of free descriptors in the descriptor table
        /// @return The number of free descriptors in the descriptor table
        inline uint16_t getFreeDescs() const
        {
            return _freeDescs;
        }

        /// @brief Clears all memory associated with this vring
        /// This does not de-associate this Vring from the vhostFd or backendFd
        void clear();

        /// @brief Checks whether given memory object can be handled by the Vring.
        /// It will check if given memory object uses memory pools, and whether its tag is the same as the one
        /// registered in the constructor.
        /// @param [in] mem The memory object to check.
        /// @tparam T The type of the object to check, either MemHandle or MemVector.
        /// @return True if the memory object has the right type and tag; False otherwise.
        template<typename T> inline bool canUseMemory ( const T & mem ) const
        {
            return canUseMemory ( mem, MemTag );
        }

        /// @brief Checks whether given MemHandle can be handled by the Vring.
        /// It will check if given handle uses memory pools, and whether its tag is correct.
        /// @param [in] mem The memory handle to check.
        /// @param [in] memTag Required memory tag.
        /// @return True if the memory handle has the right type and tag; False otherwise.
        inline static bool canUseMemory ( const MemHandle & mem, uint8_t memTag )
        {
            return ( mem.getMemoryType() == MemBlock::TypePool && mem.getMemoryTag() == memTag );
        }

        /// @brief Checks whether given MemVector can be handled by the Vring.
        /// It will check if given vector uses memory pools, and whether its tag is correct.
        /// @param [in] mem The memory vector to check.
        /// @param [in] memTag Required memory tag.
        /// @return True if the memory handle has the right type and tag; False otherwise (or if empty).
        inline static bool canUseMemory ( const MemVector & mem, uint8_t memTag )
        {
            const MemBlock * block = 0;
            size_t idx = 0;

            // We will get '0' once we get past the last block.
            while ( ( block = mem.getBlock ( idx++ ) ) != 0 )
            {
                if ( block->getType() != MemBlock::TypePool || block->getTag() != memTag )
                {
                    return false;
                }
            }

            // If idx = 1 it means that the first block was 0, meaning an empty vector.
            return ( idx > 1 );
        }

    protected:
        static TextLog _log; ///< Log stream

        /// @brief Maximum number of descriptors in this vring
        /// According to the virtio specs, this must be a power of 2, and should be at least 8
        const uint16_t MaxDescs;

        /// @brief The memory tag that we are willing to handle.
        /// This tag should be associated with all memory blocks that are registered with vring system.
        const uint8_t MemTag;

        /// @brief Describes a RX ring
        /// This contains pointers into various areas of _rxRingData.
        /// This field is NOT the one described by the memory map of a vring in Vring.cpp.
        struct vring * _ring;

        /// @brief MemHandle associated with each descriptor in the ring.
        /// This is an array of MaxDescs length.
        MemHandle * _descMH;

        uint16_t _nextDescIdx; ///< Index of the descriptor to fill next
        uint16_t _freeDescs; ///< Number of descriptors that are owned by us (and not the system).

        /// @brief Constructor
        /// @param [in] maxDescs Maximum number of descriptors in this ring
        /// @param [in] memTag The tag associated with memory blocks that can be handled.
        ///                    Vrings can only handle memory that has been properly registered.
        ///                    This tag value should be used to mark memory blocks that are within that memory.
        /// According to the virtio specs, this must be a power of 2, and must be at least 8
        Vring ( uint16_t maxDescs, uint8_t memTag );

        /// @brief Destructor
        ~Vring();

        /// @brief Gets the virtio header length
        /// @note This function should not be used, as it will return 0, which is likely not the virtio header length,
        /// until internalSetup has been successful
        /// @return The virtio header length, or 0 if internalSetup has not been successfully called yet.
        inline uint16_t getVheaderLen() const
        {
            return _vheaderLen;
        }

        /// @brief Allocate and set up this Vring's data structures, and set it up with vhost-net.
        ///
        /// This should only be called once (unless it fails), in which case you can call clear() then try
        /// calling it again.
        ///
        /// This will not do anything if this Vring is already initialized. If any other error occurs,
        /// this object will be in an invalid state and clear() must be called to clean up.
        ///
        /// This should only be called after vhostFd is ready to set up for vrings, otherwise it will fail.
        ///
        /// @param [in] vringIdx vhost-net index for this vring
        /// @param [in] vhostFd vhost-net FD to set this vring up with.
        /// This FD must be valid. This FD is never stored or closed.
        /// @param [in] backendFd (Tunnel or similar) FD that this vring will interact with, i.e. RX or TX
        /// This FD must be valid. This FD is never stored or closed.
        /// @return Standard error code
        ERRCODE internalSetup ( NetVringIdx vringIdx, int vhostFd, int backendFd );

    private:
        uint16_t _vheaderLen; ///< Length of the virtio header
};
}
