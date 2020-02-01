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

#include "basic/HashMap.hpp"
#include "basic/Mutex.hpp"
#include "error/Error.hpp"

#include "VhostNet.hpp"

struct vhost_memory;
struct vring;

namespace Pravala
{
/// @brief Class managing usage of the vhost-net device
class VhostNetMgr
{
    public:
        /// @brief Tag to assign to memory blocks that are associated with memory registered with VhostNetMgr.
        /// Only memory blocks with matching tag value will be handled by vhost-net system.
        const uint8_t RegisteredMemoryTag;

        /// @brief Gets the global VhostNetMgr instance
        /// @return The global VhostNetMgr instance
        static VhostNetMgr & get();

        /// @brief Add a memory region to vhost-net
        /// @param [in] startAddr Starting address of this region (must be page aligned)
        /// @param [in] len Size (in bytes) of this memory region. This must be at least the size of 1 page.
        /// @return Standard error code
        ///   Error::InvalidParameter   - if startAddr isn't aligned or len isn't at least a page sized
        ///   Error::MemoryError        - if there are no more available memory regions
        ERRCODE addMemoryRegion ( char * startAddr, size_t len );

        /// @brief Remove a memory region from vhost-net
        /// @param [in] startAddr Starting address of region to free. This must be the same address that was
        /// used when adding this region.
        /// @return Standard error code
        ///   Error::NotFound           - if this startAddr wasn't found
        ERRCODE removeMemoryRegion ( char * startAddr );

        /// @brief Returns if the specified address is in a memory range registered with VhostNet
        /// @note This is POTENTIALLY HEAVY (due to requiring the mutex lock),
        /// calling this function should be avoided on the critical path.
        /// @param [in] addr Address to check
        /// @return True if the specified address is in a memory range registered with VhostNet, false otherwise
        bool isInMemRange ( const void * addr );

        /// @brief Returns if each chunk in the vector is in a memory range registered with VhostNet
        /// @note This is POTENTIALLY HEAVY (due to requiring the mutex lock),
        /// calling this function should be avoided on the critical path.
        /// @param [in] data Memory vector to check.
        /// @return True if all parts of the vector are in a memory range registered with VhostNet;
        ///         False otherwise (or if the vector is empty).
        bool isInMemRange ( const MemVector & data );

    protected:
        /// @brief Called by a VhostNet object as it generates to register itself with vhost-net.
        /// This updates _devices and updates VhostNet's memory regions with the regions we know about.
        /// @note This function locks the mutex itself, so a mutex lock is not necessary to call this function
        /// @note This is intended to be called only by VhostNet
        /// @param [in] vhostFd vhost-net FD of VhostNet object to register
        ///
        /// If this vhostFd is already registered, the old registration entry is overwritten and will receive
        /// no further memory region updates (i.e. the overwritten VhostNet entry may stop receiving/sending packets).
        /// Overwriting an old entry will not cause a crash.
        //
        /// However, this should never happen since the previous owner of this FD should have called unregister
        /// before closing the vhostFd.
        ///
        /// @param [in] vn vhost-net object using this FD
        /// This does not ref the object - it just adds it to _devices.
        /// @return True if memory regions update was successful, false otherwise.
        /// If this returned false, the VhostNet should not be used, and unregister doesn't need to be called.
        bool registerVhostNet ( int vhostFd, VhostNet * vn );

        /// @brief Called by VhostNet to unregister itself from vhost-net when it is being deleted.
        /// @note This function locks the mutex itself, so a mutex lock is not necessary to call this function
        /// @note This is intended to be called only by VhostNet
        /// @param [in] vhostFd vhost-net FD of VhostNet object to unregister
        /// This does not close the FD or unref the object - it just removes it from _devices.
        void unregisterVhostNet ( int vhostFd );

        friend class VhostNet;

    private:
        /// @brief Maximum number of memory regions.
        /// This is from the kernel 3.11 source VHOST_MEMORY_MAX_NREGIONS define.
        static const uint8_t MaxMemoryRegions = 64;

        Mutex _mutex; ///< Mutex used when any data structures in this object are read/changed

        /// @brief Map of registered devices
        /// References are NOT held to objects in this map.
        ///
        /// VhostNet will call registerVhostNet() to add itself to this map, and unregisterVhostNet()
        /// to remove itself from this map when it is returning to pool.
        /// <vhostFd, VhostNet object>
        HashMap<int, VhostNet *> _devices;

        /// @brief vhost_memory and its memory regions
        struct vhost_memory * _mem;

        /// @brief Constructor
        VhostNetMgr();

        /// @brief Destructor
        ~VhostNetMgr();

        /// @brief Updates a single vhost-net FD with the current memory regions
        /// @note The caller must hold a lock on _mutex before calling this function
        /// @param [in] vhostFd vhost-net FD to send memory regions to
        /// @return True if operation succeeded, false if operation failed (and FD should be closed)
        bool updateMemInfo ( int vhostFd );

        /// @brief Update all vhost-net FDs with the current memory regions
        /// @note The caller must hold a lock on _mutex before calling this function
        void updateAllMemInfo();
};
}
