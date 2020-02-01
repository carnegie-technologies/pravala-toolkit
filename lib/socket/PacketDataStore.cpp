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

#include <cstdlib>

#include "basic/Math.hpp"

#include "PacketMemPool.hpp"
#include "PacketDataStore.hpp"

using namespace Pravala;

// MSVC doesn't like static const integrals defined in implementation files.
// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it...
#ifndef _MSC_VER
const uint8_t PacketDataStore::PacketMaxSlabs;
const uint16_t PacketDataStore::PacketSize;
#endif

ConfigLimitedNumber<uint32_t> PacketDataStore::optMaxMemorySize
(
        0,
        "os.packet_store.max_memory",
        "The max amount of pre-allocated memory that can be used by packet data store (in megabytes). "
        "If 0, packet data store for 'regular' blocks will not be used.",
        0, 1024, 16
);

ConfigLimitedNumber<uint32_t> PacketDataStore::optMaxSmallMemorySize
(
        0,
        "os.packet_store.max_small_memory",
        "The max amount of pre-allocated memory that can be used by packet data store for headers and small "
        "packets (in kilobytes). If 0, the small memory blocks will not be used.",
        0, 1024 * 1024, 1024
);

ConfigNumber<uint32_t> PacketDataStore::optMinMemorySavingsToOptimizePackets
(
        0,
        "os.packet_store.min_memory_savings_to_optimize",
        "When above 0, the minimum size (in bytes) of memory savings that will cause packets to be optimized.",
        PacketSize / 2
);

ConfigNumber<bool> PacketDataStore::optForcePacketOptimization
(
        0,
        "os.packet_store.force_packet_optimization",
        "When enabled, packets will be optimized even when pooled memory is not available (using allocated memory).",
        false
);

Mutex PacketDataStore::_stMutex ( "PacketDataStore" );
PacketMemPool * PacketDataStore::_mainPool ( 0 );
PacketMemPool * PacketDataStore::_smallPool ( 0 );
size_t PacketDataStore::_misses ( 0 );

MemHandle PacketDataStore::getPacket ( uint16_t reqSize )
{
    if ( reqSize < 1 )
    {
        reqSize = PacketSize;
    }

    _stMutex.lock();

    if ( reqSize <= SmallPacketSize && _smallPool != 0 )
    {
        // 'false' to disable fallback, we still have the regular pool to try.
        MemHandle ret = _smallPool->getHandle ( false );

        if ( !ret.isEmpty() )
        {
            _stMutex.unlock();
            return ret;
        }
    }

    if ( reqSize <= PacketSize && _mainPool != 0 )
    {
        // 'false' to disable fallback. We want to know when pool-ed allocation failed, to count it as a 'miss'.
        MemHandle ret = _mainPool->getHandle ( false );

        if ( !ret.isEmpty() )
        {
            _stMutex.unlock();
            return ret;
        }
    }

    // We couldn't get memory from the pool (for whatever reason).
    // It's a "miss"!

    ++_misses;

    _stMutex.unlock();

    // Let's generate a handle that uses regular memory.
    return MemHandle ( reqSize );
}

bool PacketDataStore::optimizePacket ( MemHandle & packet )
{
    const size_t packetMemSize = packet.getMemorySize();

    if ( packet.isEmpty() || packetMemSize < 1 )
    {
        // No need to optimize if:
        // - packet is empty
        // - we cannot determine the size of memory allocated for the packet (mem size = 0)
        return false;
    }

    _stMutex.lock();

    const uint32_t minSavings = optMinMemorySavingsToOptimizePackets.value();
    const bool forceOptimization = optForcePacketOptimization.value();

    if ( minSavings < 1 || packet.size() + minSavings > packetMemSize )
    {
        // No need to optimize if:
        // - optimization is disabled (min savings = 0)
        // - we would not save enough memory

        _stMutex.unlock();
        return false;
    }

    MemHandle optPacket;

    if ( _smallPool != 0
         && packet.size() <= SmallPacketSize
         && SmallPacketSize + minSavings <= packetMemSize )
    {
        // We have a pool with small packets, this packet is small enough, and we would save enough memory.

        // 'false' to disable fallback - we may or may not want to allocate the memory (depending on the options).
        optPacket = _smallPool->getHandle ( false );
    }

    if ( optPacket.size() < packet.size()
         && _mainPool != 0
         && packet.size() <= PacketSize
         && PacketSize + minSavings <= packetMemSize )
    {
        // We have a pool with regular packets, this packet is small enough, and we would save enough memory.

        // 'false' to disable fallback - we may or may not want to allocate the memory (depending on the options).
        optPacket = _smallPool->getHandle ( false );
    }

    _stMutex.unlock();

    if ( optPacket.size() < packet.size() && forceOptimization )
    {
        // Pool-based attempts didn't work, but we are allowed to allocate regular memory!

        optPacket = MemHandle ( packet.size() );
    }

    if ( optPacket.getWritable() != 0 && optPacket.size() >= packet.size() )
    {
        memcpy ( optPacket.getWritable(), packet.get(), packet.size() );

        optPacket.truncate ( packet.size() );

        packet = optPacket;
        return true;
    }

    return false;
}

void PacketDataStore::init()
{
    MutexLock m ( _stMutex );

    if ( !_mainPool && optMaxMemorySize.value() > 0 )
    {
        // Total number of block (if all the slabs are used):
        const uint32_t numBlocks
            = optMaxMemorySize.value() * 1024 * 1024 / ( PacketSize + MemPool::DefaultPayloadOffset );

        // Max number of slabs.
        // We want to organize blocks in PacketMaxSlabs slabs, but we don't want them to be too small.
        // So the number of slabs will be lower than PacketMaxSlabs, if there is not enough memory.
        // If "mem_size_in_MB * 4" is lower than PacketMaxSlabs, we will create less slabs,
        // each of them holding approximately 256KB.
        const uint32_t maxSlabs = min<uint32_t> ( optMaxMemorySize.value() * 4, PacketMaxSlabs );

        _mainPool = new PacketMemPool ( PacketSize, max ( 1U, numBlocks / maxSlabs ), maxSlabs );
    }

    if ( !_smallPool && optMaxSmallMemorySize.value() > 0 )
    {
        // Total number of block (if all the slabs are used):
        const uint32_t numBlocks
            = optMaxSmallMemorySize.value() * 1024 / ( SmallPacketSize + MemPool::DefaultPayloadOffset );

        // Max number of slabs.
        // We want to organize blocks in PacketMaxSlabs slabs, but we don't want them to be too small.
        // So the number of slabs will be lower than PacketMaxSlabs, if there is not enough memory.
        // If "1 + mem_size_in_KB/64" is lower than PacketMaxSlabs, we will create less slabs,
        // each of them holding up to 64KB.
        // For instance:
        // - if size = 1, we will have 1 slab with 1KB
        // - if size = 63, we will have 1 slab with 63KB
        // - if size = 64, we will have 2 slabs with 32KB in each
        // - if size = 127, we will have 2 slabs with almost 64KB in each
        // - if size = 128, we will have 3 slabs with around 42KB in each
        // etc.
        const uint32_t maxSlabs = min<uint32_t> ( 1 + optMaxSmallMemorySize.value() / 64, PacketMaxSlabs );

        _smallPool = new PacketMemPool ( SmallPacketSize, max ( 1U, numBlocks / maxSlabs ), maxSlabs );
    }
}

void PacketDataStore::shutdown()
{
    MutexLock m ( _stMutex );

    if ( _mainPool != 0 )
    {
        _mainPool->shutdown();
        _mainPool = 0;
    }

    if ( _smallPool != 0 )
    {
        _smallPool->shutdown();
        _smallPool = 0;
    }
}

size_t PacketDataStore::getFreeBlocksCount()
{
    MutexLock m ( _stMutex );

    return ( _mainPool != 0 ) ? _mainPool->getFreeBlocksCount() : 0;
}

size_t PacketDataStore::getAllocatedBlocksCount()
{
    MutexLock m ( _stMutex );

    return ( _mainPool != 0 ) ? _mainPool->getAllocatedBlocksCount() : 0;
}

size_t PacketDataStore::getAllocatedMemorySize()
{
    MutexLock m ( _stMutex );

    return ( ( _mainPool != 0 )
             ? _mainPool->getAllocatedBlocksCount() * ( _mainPool->PayloadOffset + _mainPool->PayloadSize )
             : 0 )
           + ( ( _smallPool != 0 )
               ? _smallPool->getAllocatedBlocksCount() * ( _smallPool->PayloadOffset + _smallPool->PayloadSize )
               : 0 );
}

size_t PacketDataStore::getMisses()
{
    MutexLock m ( _stMutex );

    return _misses;
}
