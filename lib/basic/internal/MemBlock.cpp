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

extern "C"
{
#if defined( SYSTEM_LINUX )
#include <sys/mman.h>
#include <malloc.h>
#elif defined( SYSTEM_APPLE )
#include <malloc/malloc.h>
#endif
}

#include <cstdlib>

#include "MemBlock.hpp"
#include "MemPool.hpp"

using namespace Pravala;

/// @brief Global mutex.
static Mutex globMutex ( "GlobalMemBlock" );

/// @brief The last registered tag value.
static uint8_t globLastTag ( 0 );

uint8_t MemBlock::assignTag()
{
    MutexLock m ( globMutex );

    if ( globLastTag < 0xFF )
    {
        return ++globLastTag;
    }

    return 0;
}

void MemBlock::releaseBlock()
{
    assert ( _refCount == 0 );

    if ( _refCount > 0 )
        return;

    switch ( _type )
    {
        case TypeUnknown:
        case TypeMax:
            break;

        case TypeBuffer:
            free ( this );
            return;
            break;

        case TypeMMapRO:
#ifdef SYSTEM_LINUX
            {
                ExternalMemBlock * const block = ( ExternalMemBlock * ) this;

                assert ( block != 0 );
                assert ( block->data != 0 );
                assert ( block->data != MAP_FAILED );
                assert ( block->size > 0 );

                munmap ( block->data, block->size );

                free ( block );
            }
#endif
            return;
            break;

        case TypeDeallocator:
            {
                DeallocatorMemBlock * const block = ( DeallocatorMemBlock * ) this;

                assert ( block != 0 );

                if ( block->deallocator != 0 )
                {
                    // If it's 0 it could mean there is no need to deallocate it.
                    block->deallocator ( block );
                }

                free ( block );
            }
            return;
            break;

        case TypePool:
            {
                PoolMemBlock * const block = reinterpret_cast<PoolMemBlock *> ( this );

                assert ( block != 0 );
                assert ( block->_refCount == 0 );

                // Because we are returning it to the pool, we set the reference counter back to 1.
                // Also, this is to be consistent with the blocks that were originally placed in the pool,
                // they will all have the reference counter set to 1.
                block->_refCount = 1;

                assert ( block->u.memPool != 0 );

                block->u.memPool->releaseBlock ( block );
            }
            return;
            break;
    }

    assert ( false );
}

size_t MemBlock::getMemorySize() const
{
    assert ( _refCount > 0 );

    switch ( _type )
    {
        case TypeUnknown:
        case TypeMax:
        case TypeDeallocator:
            break;

        case TypeBuffer:
#if defined( SYSTEM_APPLE ) || defined( HAVE_MALLOC_USABLE_SIZE )
            {
#if defined( SYSTEM_APPLE )
                const size_t s = malloc_size ( const_cast<MemBlock *> ( this ) );
#else
                const size_t s = malloc_usable_size ( const_cast<MemBlock *> ( this ) );
#endif

                return ( s > sizeof ( *this ) ) ? ( s - sizeof ( *this ) ) : 0;
            }
#endif
            break;

        case TypeMMapRO:
#ifdef SYSTEM_LINUX
            {
                const ExternalMemBlock * const block = ( const ExternalMemBlock * ) this;

                assert ( block != 0 );
                assert ( block->data != 0 );
                assert ( block->data != MAP_FAILED );
                assert ( block->size > 0 );

                return block->size;
            }
#endif
            break;

        case TypePool:
            {
                const PoolMemBlock * const block = reinterpret_cast<const PoolMemBlock *> ( this );

                assert ( block != 0 );
                assert ( block->u.memPool != 0 );

                return block->u.memPool->PayloadSize;
            }
            break;
    }

    return 0;
}
