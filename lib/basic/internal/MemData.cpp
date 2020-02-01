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

#include "MemData.hpp"
#include "MemPool.hpp"

using namespace Pravala;

bool MemData::intMakeCopy ( bool unrefOrg )
{
    if ( !block || size < 1 )
    {
        assert ( !block );
        assert ( !mem );
        assert ( size == 0 );

        return false;
    }

    MemBlock * newBlock = 0;
    char * newMem = 0;

    if ( block->getType() == MemBlock::TypePool )
    {
        PoolMemBlock * const pBlock = reinterpret_cast<PoolMemBlock *> ( block );

        assert ( pBlock != 0 );
        assert ( pBlock->getType() == MemBlock::TypePool );
        assert ( pBlock->u.memPool != 0 );

        // This tries to find a new block for us to use!
        if ( ( newBlock = pBlock->u.memPool->getBlock() ) != 0 )
        {
            assert ( size <= pBlock->u.memPool->PayloadSize );

            newMem = reinterpret_cast<char *> ( newBlock ) + pBlock->u.memPool->PayloadOffset;
        }
    }

    if ( !newBlock )
    {
        // This is not pooled block, or the pool is empty. Let's use malloc.
        if ( !( newBlock = ( MemBlock * ) malloc ( sizeof ( MemBlock ) + size ) ) )
        {
            return false;
        }

        newBlock->init ( MemBlock::TypeBuffer );
        newMem = reinterpret_cast<char *> ( newBlock + 1 );
    }
    else
    {
        // We got a block from the pool!
        assert ( newBlock->getType() == MemBlock::TypePool );
        assert ( newMem != 0 );
    }

    assert ( newBlock != 0 );
    assert ( newMem - reinterpret_cast<char *> ( newBlock ) > 0 );
    assert ( newMem - reinterpret_cast<char *> ( newBlock ) >= ( int ) sizeof ( MemBlock ) );

    memcpy ( newMem, mem, size );

    if ( unrefOrg )
    {
        block->unref();
    }

    block = newBlock;
    mem = newMem;

    // The size stays the same.
    assert ( size > 0 );

    return true;
}
