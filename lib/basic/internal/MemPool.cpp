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

#include "MemPool.hpp"

using namespace Pravala;

MemPool::MemPool ( size_t payloadSize, size_t payloadOffset ):
    PayloadSize ( payloadSize ),
    PayloadOffset ( payloadOffset ),
    _mutex ( "memory_pool" ),
    _poolHead ( 0 ),
    _freeBlocksCount ( 0 ),
    _allocatedBlocksCount ( 0 ),
    _isShuttingDown ( false )
{
    assert ( PayloadSize > 0 );
    assert ( PayloadOffset >= sizeof ( PoolMemBlock ) );
    assert ( PayloadOffset % 4 == 0 );
}

MemPool::~MemPool()
{
}

void MemPool::shutdown()
{
    _mutex.lock();

    if ( !_isShuttingDown )
    {
        _isShuttingDown = true;

        if ( _freeBlocksCount >= _allocatedBlocksCount )
        {
            _mutex.unlock();
            delete this;
            return;
        }
    }

    _mutex.unlock();
}

void MemPool::addMoreBlocks()
{
}
