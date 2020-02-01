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

#include <cassert>

extern "C"
{
#include <sys/uio.h>
#include "lwip/pbuf.h"
}

#include "LwipBufferIterator.hpp"

using namespace Pravala;

LwipBufferIterator::LwipBufferIterator(): _buffer ( 0 ), _offset ( 0 )
{
}

LwipBufferIterator::LwipBufferIterator ( struct pbuf * buffer ): _buffer ( buffer ), _offset ( 0 )
{
    if ( _buffer != 0 )
    {
        pbuf_ref ( _buffer );

        // consume(0) just consumes empty pbuf objects in the chain.
        consume ( 0 );

        // If the buffer consists of just empty pbuf objects, consume(0) just clears it...

        assert ( !_buffer || _buffer->len > 0 );
        assert ( !_buffer || _offset < _buffer->len );
    }
}

LwipBufferIterator::LwipBufferIterator ( const LwipBufferIterator & other ):
    _buffer ( other._buffer ), _offset ( other._offset )
{
    if ( _buffer != 0 )
    {
        pbuf_ref ( _buffer );

        // No need for consume(0) - the other LwipBufferIterator should have taken care of it already.

        assert ( _buffer->len > 0 );
        assert ( _offset < _buffer->len );
    }
}

LwipBufferIterator::~LwipBufferIterator()
{
    clear();
}

LwipBufferIterator & LwipBufferIterator::operator= ( const LwipBufferIterator & other )
{
    // No harm in doing that, even if we are assigning LwipBufferIterator to itself.
    _offset = other._offset;

    if ( this == &other || _buffer == other._buffer )
    {
        // 'other' is the same object, or they both use the same underlying pbuf buffer.
        return *this;
    }

    // We can't remove the reference from our current buffer until we hold one in the new buffer.
    // If they are in some way related, removing the current one could also destroy the new one.

    struct pbuf * const oldBuf = _buffer;

    _buffer = other._buffer;

    if ( _buffer != 0 )
    {
        pbuf_ref ( _buffer );

        // No need for consume(0) - the other LwipBufferIterator should have taken care of it already.

        assert ( _buffer->len > 0 );
        assert ( _offset < _buffer->len );
    }

    if ( oldBuf != 0 )
    {
        // pbuf_free is really pbuf_unref, just poorly named.
        pbuf_free ( oldBuf );
    }

    return *this;
}

void LwipBufferIterator::clear()
{
    _offset = 0;

    if ( _buffer != 0 )
    {
        // pbuf_free is really pbuf_unref, just poorly named.
        pbuf_free ( _buffer );
        _buffer = 0;
    }
}

const char * LwipBufferIterator::getCurrentData() const
{
    if ( !_buffer )
        return 0;

    assert ( _buffer->len > 0 );
    assert ( _offset < _buffer->len );

    return ( ( const char * ) _buffer->payload ) + _offset;
}

size_t LwipBufferIterator::getCurrentSize() const
{
    if ( !_buffer )
        return 0;

    assert ( _buffer->len > 0 );
    assert ( _offset < _buffer->len );

    return ( _buffer->len - _offset );
}

size_t LwipBufferIterator::getSize() const
{
    if ( !_buffer )
        return 0;

    assert ( _buffer->len > 0 );
    assert ( _offset < _buffer->tot_len );

    return ( _buffer->tot_len - _offset );
}

bool LwipBufferIterator::consume ( size_t numBytes )
{
    // We can't use getCurrentSize() or getSize() in this function.
    // One of the features of this one is to skip empty pbuf elements in the chain.
    // getCurrentSize(), getSize(), and getCurrentData() assume that the current pbuf object is not empty!

    if ( !_buffer || _offset + numBytes >= _buffer->tot_len )
    {
        clear();
        return false;
    }

    assert ( _buffer != 0 );
    assert ( _offset <= _buffer->len );

    // Could be 0!
    size_t curSize = _buffer->len - _offset;

    // We want to keep skipping pbuf elements as long as:
    // - there are more bytes to consume
    // - the current pbuf objects is empty (even if we don't need to consume any more bytes).

    while ( numBytes > 0 || curSize < 1 )
    {
        if ( numBytes < curSize )
        {
            // We don't need to skip the chunk, just modify the offset.
            // After that we are done!
            _offset += numBytes;

            assert ( _buffer->len > 0 );
            assert ( _offset < _buffer->len );

            return true;
        }

        // We need to skip the entire chunk.

        numBytes -= curSize;

        // First, reference the next chunk in the list:
        struct pbuf * const nextChunk = _buffer->next;

        pbuf_ref ( nextChunk );

        // Then release the current one:
        pbuf_free ( _buffer );

        // And start using the next one, starting at offset 0:
        _buffer = nextChunk;
        _offset = 0;

        // And the 'current size' of the new chunk is the size of the entire chunk:
        curSize = _buffer->len;
    }

    assert ( _buffer->len > 0 );
    assert ( _offset < _buffer->len );

    return true;
}

void LwipBufferIterator::appendToIovecArray ( SimpleArray<struct iovec> & array ) const
{
    struct iovec vec;

    for ( struct pbuf * buf = _buffer; buf != 0; buf = buf->next )
    {
        if ( buf->len < 1 || !buf->payload )
            continue;

        vec.iov_base = buf->payload;
        vec.iov_len = buf->len;

        array.append ( vec );
    }
}
