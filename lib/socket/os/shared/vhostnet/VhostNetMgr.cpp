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

// #define DEBUG_VHOSTNET 1

extern "C"
{
#include <sys/ioctl.h>
#include <linux/vhost.h>
}

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "basic/Platform.hpp"
#include "VhostNetMgr.hpp"

using namespace Pravala;

VhostNetMgr & VhostNetMgr::get()
{
    static VhostNetMgr * global = 0;

    if ( !global )
    {
        global = new VhostNetMgr();
    }

    return *global;
}

VhostNetMgr::VhostNetMgr():
    RegisteredMemoryTag ( MemBlock::assignTag() ), _mutex ( "VhostNetMgr" )
{
    const size_t len = sizeof ( struct vhost_memory ) + MaxMemoryRegions * sizeof ( struct vhost_memory_region );
    int ret = posix_memalign ( ( void ** ) &_mem, Platform::PageSize, len );

    ( void ) ret;
    assert ( ret == 0 );
    assert ( _mem != 0 );

    memset ( _mem, 0, len );
}

VhostNetMgr::~VhostNetMgr()
{
    MutexLock mlock ( _mutex );

    _devices.clear();
}

bool VhostNetMgr::registerVhostNet ( int vhostFd, VhostNet * vn )
{
    assert ( vn != 0 );
    assert ( vn->isValid() );
    assert ( vhostFd >= 0 );

    MutexLock mlock ( _mutex );

    assert ( !_devices.contains ( vhostFd ) );

    if ( !updateMemInfo ( vhostFd ) )
    {
        return false;
    }

    _devices.insert ( vhostFd, vn );

    return true;
}

void VhostNetMgr::unregisterVhostNet ( int vhostFd )
{
    MutexLock mlock ( _mutex );

    _devices.remove ( vhostFd );
}

bool VhostNetMgr::updateMemInfo ( int vhostFd )
{
    // Do not use LOG here since it may be called on the PacketDataStore memory allocation path

    assert ( _mem != 0 );

#ifdef DEBUG_VHOSTNET
    fprintf ( stderr, "%s: vhostFd: %d, mem regions: %d\n", __PRETTY_FUNCTION__, vhostFd, _mem->nregions );
#endif

    int ret = ioctl ( vhostFd, VHOST_SET_MEM_TABLE, _mem );

    if ( ret < 0 )
    {
        fprintf ( stderr, "Failed to update memory region on FD: %d. Error: %s\n", vhostFd, strerror ( errno ) );

        return false;
    }

    return true;
}

void VhostNetMgr::updateAllMemInfo()
{
    // Do not use LOG here since it may be called on the PacketDataStore memory allocation path

#ifdef DEBUG_VHOSTNET
    fprintf ( stderr, "%s\n", __PRETTY_FUNCTION__ );
#endif

    for ( HashMap<int, VhostNet *>::Iterator it ( _devices ); it.isValid(); )
    {
        assert ( it.key() >= 0 );

        if ( updateMemInfo ( it.key() ) )
        {
            it.next();
        }
        else
        {
            // We failed to update a region, so we should probably shut it down.
            // This is safe to call in a loop since it delays notifications till end of loop.
            it.value()->closeAndScheduleNotify();
        }
    }
}

ERRCODE VhostNetMgr::addMemoryRegion ( char * startAddrC, size_t len )
{
    // Do not use LOG here since it's called on the PacketDataStore memory allocation path

    // We need the starting address as a uint64_t (on all platforms)
    const uint64_t startAddr = ( uint64_t ) startAddrC;

    if ( startAddr % Platform::PageSize != 0 || len < Platform::PageSize )
    {
        return Error::InvalidParameter;
    }

    MutexLock mlock ( _mutex );

    const uint32_t idx = _mem->nregions;

    if ( idx >= MaxMemoryRegions )
    {
        return Error::MemoryError;
    }

    _mem->regions[ idx ].guest_phys_addr = startAddr;
    _mem->regions[ idx ].userspace_addr = startAddr;
    _mem->regions[ idx ].memory_size = len;

    ++_mem->nregions;

#ifdef DEBUG_VHOSTNET
    fprintf ( stderr, "%s: startAddr: %lu, len: %lu, idx: %d\n",
              __PRETTY_FUNCTION__, ( uint64_t ) startAddr, len, idx );
#endif

    updateAllMemInfo();

    return Error::Success;
}

ERRCODE VhostNetMgr::removeMemoryRegion ( char * startAddr )
{
    // Do not use LOG here since it's called on the PacketDataStore memory allocation path

    MutexLock mlock ( _mutex );

    const int32_t regions = _mem->nregions;

    assert ( regions <= MaxMemoryRegions );

    // < 0 = not found
    int32_t idxToRemove = -1;

    for ( int32_t i = 0; i < regions; ++i )
    {
        if ( _mem->regions[ i ].userspace_addr == ( uint64_t ) startAddr )
        {
            idxToRemove = i;
            break;
        }
    }

#ifdef DEBUG_VHOSTNET
    fprintf ( stderr, "%s: startAddr: %lu, idxToRemove: %d\n", __PRETTY_FUNCTION__,
              ( uint64_t ) startAddr, idxToRemove );
#endif

    if ( idxToRemove < 0 )
    {
        return Error::NotFound;
    }

    assert ( regions > 0 );

    const int32_t idxToMove = regions - 1;

    assert ( idxToMove >= 0 );
    assert ( idxToMove < regions );
    assert ( idxToMove < MaxMemoryRegions );

    // If this isn't the last region in the list, move the last region to this slot.
    // We don't strictly need to memset the now-unused region, since it will be ignored both by
    // our code and the system, once we decrement _mem->nregions.
    if ( idxToMove != idxToRemove )
    {
        _mem->regions[ idxToRemove ] = _mem->regions[ idxToMove ];
    }

    --_mem->nregions;

    updateAllMemInfo();

    return Error::Success;
}

bool VhostNetMgr::isInMemRange ( const void * addrC )
{
    MutexLock mlock ( _mutex );

    const uint64_t addr = ( uint64_t ) addrC;
    const int32_t regions = _mem->nregions;

    for ( int32_t i = 0; i < regions; ++i )
    {
        if ( addr >= _mem->regions[ i ].userspace_addr
             && addr < ( _mem->regions[ i ].userspace_addr + _mem->regions[ i ].memory_size ) )
        {
            return true;
        }
    }

    return false;
}

bool VhostNetMgr::isInMemRange ( const MemVector & data )
{
    const size_t numChunks = data.getNumChunks();

    if ( numChunks < 1 )
        return false;

    const struct iovec * chunks = data.getChunks();

    for ( size_t i = 0; i < numChunks; ++i )
    {
        if ( !isInMemRange ( chunks[ i ].iov_base ) )
        {
            return false;
        }
    }

    return true;
}
