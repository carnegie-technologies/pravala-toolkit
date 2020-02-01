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

#include "socket/PacketDataStore.hpp"

#include "PBufMemHandle.hpp"

using namespace Pravala;

PBufMemHandle::PBufMemHandle ( struct pbuf * buffer ):
    ExtMemHandle ( ( char * ) buffer->payload, buffer->len, releasePBuf, buffer )
{
    assert ( buffer != 0 );
    assert ( !buffer->next );
    assert ( buffer->len > 0 );
    assert ( buffer->len == buffer->tot_len );

    pbuf_ref ( buffer );
}

void PBufMemHandle::releasePBuf ( DeallocatorMemBlock * block )
{
    if ( block != 0 && block->deallocatorData != 0 )
    {
        pbuf_free ( ( struct pbuf * ) block->deallocatorData );
    }
}

MemHandle PBufMemHandle::getPacket ( struct pbuf * buffer )
{
    if ( !buffer || buffer->tot_len < 1 )
    {
        return MemHandle();
    }

    if ( !buffer->next )
    {
        // Single-part pbuf. We can just reference the memory!

        assert ( buffer->len > 0 );
        assert ( buffer->len == buffer->tot_len );

        return PBufMemHandle ( buffer );
    }

    // Multi-part buffer. We must copy the data...

    MemHandle mh = PacketDataStore::getPacket ( buffer->tot_len );

    if ( !mh.getWritable() || buffer->tot_len > mh.size() )
    {
        mh.clear();
        return mh;
    }

    mh.truncate ( buffer->tot_len );

    // lwIP's pbuf is a linked a list of buffers, so we need to copy the entire chain into our MemHandle
    for ( size_t offset = 0; buffer != 0; buffer = buffer->next )
    {
        assert ( offset + buffer->len <= mh.size() );

        memcpy ( mh.getWritable ( offset ), buffer->payload, buffer->len );
        offset += buffer->len;
    }

    return mh;
}
