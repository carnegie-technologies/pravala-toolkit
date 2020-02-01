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

#include "VhostNetMemPool.hpp"
#include "VhostNetMgr.hpp"

using namespace Pravala;

VhostNetMemPool::VhostNetMemPool (
        size_t payloadSize, size_t blocksPerSlab, size_t maxSlabs, size_t payloadOffset ):
    BasicMemPool ( payloadSize, blocksPerSlab, maxSlabs, payloadOffset, VhostNetMgr::get().RegisteredMemoryTag )
{
}

VhostNetMemPool::~VhostNetMemPool()
{
    // We MUST call it here. This way it will call our version of removeSlab().
    // If we didn't, BasicMemPool's destructor would call it,
    // but at that point it would use its own version of removeSlab().
    removeSlabs();
}

char * VhostNetMemPool::generateSlab()
{
    char * const slab = BasicMemPool::generateSlab();

    if ( !slab )
        return 0;

    VhostNetMgr::get().addMemoryRegion ( slab, ( PayloadOffset + PayloadSize ) * BlocksPerSlab );

    return slab;
}

void VhostNetMemPool::removeSlab ( char * slab )
{
    if ( !slab )
        return;

    VhostNetMgr::get().removeMemoryRegion ( slab );

    BasicMemPool::removeSlab ( slab );
}
