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

// #define DEBUG_MEM_POOL    1

#include <cstdlib>

#if DEBUG_MEM_POOL
#include <cstdio>
#endif

#include "Platform.hpp"
#include "BasicMemPool.hpp"
#include "MemHandle.hpp"

using namespace Pravala;

BasicMemPool::BasicMemPool (
        size_t payloadSize, size_t blocksPerSlab, size_t maxSlabs, size_t payloadOffset, uint8_t memTag ):
    MemPool ( payloadSize, payloadOffset ),

#if DEBUG_MEM_POOL
    BlocksPerSlab ( 10 ),
#else
    BlocksPerSlab ( blocksPerSlab ),
#endif

    MaxSlabs ( maxSlabs ),
    MemTag ( memTag ),
    _slabs ( ( char ** ) calloc ( MaxSlabs, sizeof ( char * ) ) ),
    _allocatedSlabCount ( 0 )
{
#if DEBUG_MEM_POOL
    ( void ) blocksPerSlab;

    printf ( "%08lx: BasicMemPool created; Payload size: %u; Blocks per slab: %u; Max slabs: %u; Payload offset: %u\n",
             ( ( unsigned long ) this ),
             ( unsigned ) PayloadSize, ( unsigned ) BlocksPerSlab, ( unsigned ) MaxSlabs, ( unsigned ) PayloadOffset );
#endif
}

BasicMemPool::~BasicMemPool()
{
    removeSlabs();

    free ( _slabs );

#if DEBUG_MEM_POOL
    printf ( "%08lx: BasicMemPool destroyed\n", ( ( unsigned long ) this ) );
#endif
}

MemHandle BasicMemPool::getHandle ( bool useFallback )
{
    assert ( PayloadSize > 0 );

    PoolMemBlock * const block = getBlock();

    if ( !block )
    {
        // Pool is out of memory (or shutting down).

        if ( !useFallback )
        {
            // No fallback, return an empty handle.
            return MemHandle();
        }

        // Let's generate a handle that uses regular memory:
        return MemHandle ( PayloadSize );
    }

    assert ( block->u.memPool == this );
    assert ( block->getTag() == MemTag );

    // This does not create another reference, it just takes over the one we already have:
    return MemHandle ( MemData ( block, reinterpret_cast<char *> ( block ) + PayloadOffset, PayloadSize ) );
}

void BasicMemPool::removeSlabs()
{
    assert ( _freeBlocksCount == _allocatedBlocksCount );

    for ( size_t i = 0; i < _allocatedSlabCount; ++i )
    {
        char * const slab = _slabs[ i ];
        _slabs[ i ] = 0;

        removeSlab ( slab );
    }

    _freeBlocksCount = _allocatedBlocksCount = _allocatedSlabCount = 0;
}

char * BasicMemPool::generateSlab()
{
    char * slab = 0;

#ifdef _POSIX_C_SOURCE
    // We want memory to be page-aligned, so we use posix_memalign instead of malloc.
    // This is required for vhost to setup kernel memory mapping (this is done elsewhere).

    const int ret = posix_memalign (
        ( void ** ) &slab, Platform::PageSize, ( PayloadOffset + PayloadSize ) * BlocksPerSlab );

    ( void ) ret;
#else
    slab = ( char * ) malloc ( ( PayloadOffset + PayloadSize ) * BlocksPerSlab );
#endif

    return slab;
}

void BasicMemPool::removeSlab ( char * slab )
{
    free ( slab );
}

void BasicMemPool::addMoreBlocks()
{
    // This is called with MemPool's mutex locked.

    if ( _allocatedSlabCount >= MaxSlabs )
    {
#if DEBUG_MEM_POOL
        printf ( "%08lx: Not allocating more blocks: All possible slabs are already used; Allocated slabs: %u\n",
                 ( ( unsigned long ) this ), ( unsigned ) _allocatedSlabCount );
#endif
        return;
    }

    assert ( !_slabs[ _allocatedSlabCount ] );

    if ( !( _slabs[ _allocatedSlabCount ] = generateSlab() ) )
    {
#if DEBUG_MEM_POOL
        printf ( "%08lx: Could not allocate memory for a new slab; Allocated slabs: %u\n",
                 ( ( unsigned long ) this ), ( unsigned ) _allocatedSlabCount );
#endif

        return;
    }

    char * ptr = _slabs[ _allocatedSlabCount ];

    for ( size_t idx = 0; idx < BlocksPerSlab; ++idx, ptr += ( PayloadOffset + PayloadSize ) )
    {
        PoolMemBlock * const block = reinterpret_cast<PoolMemBlock *> ( ptr );

        block->init ( MemBlock::TypePool, MemTag );
        block->u.next = _poolHead;

        _poolHead = block;
    }

    ++_allocatedSlabCount;

    _allocatedBlocksCount += BlocksPerSlab;
    _freeBlocksCount += BlocksPerSlab;

#if DEBUG_MEM_POOL
    printf ( "%08lx: Allocated a new slab[%u] with %u more blocks; Allocated blocks: %u\n",
             ( ( unsigned long ) this ),
             ( unsigned ) _allocatedSlabCount - 1, ( unsigned ) BlocksPerSlab, ( unsigned ) _allocatedBlocksCount );
#endif
}
