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

#include <cstdio>
#include <cerrno>

#include "internal/MemBlock.hpp"
#include "internal/MemData.hpp"
#include "MemHandle.hpp"
#include "Buffer.hpp"

// Define MEMORY_DEBUGGING to enable some low-level logging:
// #define MEMORY_DEBUGGING

#ifdef MEMORY_DEBUGGING
#include <cstdio>

#define LOG_TOO_MANY_REFS \
    do { \
        printf ( "BUFFER[%llx]: Too many references created (%u); Creating a copy; AllocSize: %lu; Size: %lu; " \
                 "MemBlock: %llx; Type: %d; Tag: %d\n", \
                 ( long long unsigned ) this, ( unsigned ) _data->getRefCount(), \
                 ( long unsigned ) _allocSize, ( long unsigned ) _size, \
                 ( long long unsigned ) _data, ( int ) _data->getType(), ( int ) _data->getTag() ); \
    } \
    while ( false )
#else
#define LOG_TOO_MANY_REFS
#endif

/// @brief Minimum buffer size to allocate
/// This value has been chosen based on analysis of typical buffer sizes used by our application
#define MIN_BUF_SIZE    80

using namespace Pravala;

/// @brief A helper function that calculates an increased size of data to allocate.
/// @param [in] minPayloadRequired The minimum number of payload bytes that should we accommodated.
/// @param [in] curAlloc Current size of allocated data.
/// @return An increased size of data to allocate.
inline size_t getAllocSize ( size_t minPayloadRequired, size_t curAlloc )
{
    /// @todo This should be improved!
    size_t newAllocSize = ( curAlloc * 3 ) / 2;

    if ( newAllocSize < sizeof ( MemBlock ) + minPayloadRequired )
    {
        newAllocSize = sizeof ( MemBlock ) + minPayloadRequired;
    }

    return newAllocSize;
}

/// @brief A helper function that allocates and configures a new block.
/// @param [in] allocSize The size of data to allocate. It MUST be > sizeof(MemBlock).
/// @return Newly allocated block.
inline MemBlock * allocateBlock ( size_t allocSize )
{
    assert ( allocSize > sizeof ( MemBlock ) );

    MemBlock * ret = ( MemBlock * ) malloc ( allocSize );

    if ( !ret )
    {
        return 0;
    }

    ret->init ( MemBlock::TypeBuffer );

    return ret;
}

Buffer::Buffer ( size_t preAllocateMemory ):
    _data ( 0 ), _allocSize ( 0 ), _size ( 0 )
{
    if ( preAllocateMemory > 0 )
    {
        getAppendable ( preAllocateMemory );
    }
}

Buffer::Buffer ( const Buffer & other ):
    _data ( other._data ), _allocSize ( other._allocSize ), _size ( other._size )
{
    if ( !_data )
    {
        assert ( _allocSize == 0 );
        assert ( _size == 0 );

        return;
    }

    assert ( _allocSize > sizeof ( MemBlock ) );

    if ( _data->ref() )
    {
        return;
    }

    // There are too many references. We need to create a brand new copy of the data.
    LOG_TOO_MANY_REFS;

    _data = 0;
    _size = 0;
    _allocSize = 0;

    appendData ( other.get(), other.size() );
}

Buffer & Buffer::operator= ( const Buffer & other )
{
    // Copying the same buffer to itself, or copying another buffer that uses the same underlying memory
    if ( &other == this || other._data == _data )
    {
        // If they share the data, they really should have the same size values...
        assert ( other._size == _size );
        assert ( other._allocSize == _allocSize );

        return *this;
    }

    assert ( _data != other._data );

    if ( _data != 0 )
    {
        _data->unref();
    }

    _data = other._data;
    _allocSize = other._allocSize;
    _size = other._size;

    if ( !_data )
    {
        assert ( _allocSize == 0 );
        assert ( _size == 0 );

        return *this;
    }

    assert ( _allocSize > sizeof ( MemBlock ) );

    if ( _data->ref() )
    {
        return *this;
    }

    // There are too many references. We need to create a brand new copy of the data.
    LOG_TOO_MANY_REFS;

    _data = 0;
    _size = _allocSize = 0;

    appendData ( other.get(), other.size() );

    return *this;
}

void Buffer::clear()
{
    if ( _data != 0 )
    {
        _data->unref();

        _data = 0;
        _size = _allocSize = 0;
    }

    assert ( _size == 0 );
    assert ( _allocSize == 0 );
}

const char * Buffer::get ( size_t offset ) const
{
    if ( _size <= offset )
    {
        return 0;
    }

    assert ( _data != 0 );

    return reinterpret_cast<const char *> ( _data + 1 ) + offset;
}

char * RwBuffer::getWritable ( size_t offset )
{
    if ( _size <= offset || !_data )
    {
        return 0;
    }

    assert ( _data != 0 );
    assert ( _data->getRefCount() > 0 );

    if ( _data->getRefCount() == 1 )
    {
        return reinterpret_cast<char *> ( _data + 1 ) + offset;
    }

    // Someone else uses this block, we need to make a copy!

    assert ( _allocSize > sizeof ( MemBlock ) );

    MemBlock * const newData = allocateBlock ( _allocSize );

    if ( !newData )
    {
        return 0;
    }

    if ( _size > 0 )
    {
        memcpy ( reinterpret_cast<char *> ( newData + 1 ), reinterpret_cast<const char *> ( _data + 1 ), _size );
    }

    _data->unref();
    _data = newData;

    assert ( _data != 0 );
    assert ( _data->getRefCount() == 1 );

    return reinterpret_cast<char *> ( _data + 1 ) + offset;
}

void RwBuffer::consumeData ( size_t consumeSize )
{
    if ( _size < 1 || consumeSize < 1 )
    {
        return;
    }

    if ( _size <= consumeSize )
    {
        clear();
        return;
    }

    assert ( consumeSize < _size );
    assert ( _data != 0 );

    if ( _data->getRefCount() == 1 )
    {
        // There is nobody else using this block. Let's consume the data!
        // New size:
        _size -= consumeSize;

        assert ( _size > 0 );

        memmove ( reinterpret_cast<char *> ( _data + 1 ),
                  reinterpret_cast<const char *> ( _data + 1 ) + consumeSize,
                  _size );
        return;
    }

    // Someone else uses this block, we need to make a copy!
    // Also, we can allocate less memory than before!

    const size_t newAllocSize = sizeof ( MemBlock ) + ( _size - consumeSize );
    MemBlock * const newData = allocateBlock ( newAllocSize );

    if ( !newData )
    {
        return;
    }

    _allocSize = newAllocSize;
    _size -= consumeSize;

    assert ( _size > 0 );

    memcpy ( reinterpret_cast<char *> ( newData + 1 ),
             reinterpret_cast<const char *> ( _data + 1 ) + consumeSize,
             _size );

    _data->unref();
    _data = newData;
}

void RwBuffer::truncateData ( size_t truncatedSize )
{
    if ( truncatedSize < _size )
    {
        // This only affects this particular buffer!
        // The actual data is not affected, so we don't need to ensure that nobody else uses the memory block.
        // If any of the users tries to actually change the data (with getAppendable, getWritable, etc.),
        // a new copy will be created then.

        _size = truncatedSize;

        if ( _size == 0 )
        {
            assert ( _data != 0 );

            _data->unref();
            _data = 0;
            _allocSize = 0;
        }
    }
}

char * Buffer::getAppendable ( size_t count )
{
    // count is 0 or too big
    if ( count < 1 || count > ( ( ( size_t ) -1 ) - _size - sizeof ( MemBlock ) ) )
    {
        return 0;
    }

    if ( !_data )
    {
        _allocSize = count + sizeof ( MemBlock );

        assert ( count < _allocSize );
        assert ( sizeof ( MemBlock ) < _allocSize );
        assert ( sizeof ( MemBlock ) + count <= _allocSize );

        _data = allocateBlock ( _allocSize );

        if ( !_data )
        {
            _allocSize = 0;
            return 0;
        }

        assert ( _size == 0 );

        return reinterpret_cast<char *> ( _data + 1 );
    }

    assert ( _data != 0 );
    assert ( _data->getRefCount() > 0 );

    if ( _data->getRefCount() > 1 )
    {
        // Someone else uses this block, we need to make a copy!

        const size_t newAllocSize = getAllocSize ( _size + count, _allocSize );

        assert ( count < newAllocSize );
        assert ( _size < newAllocSize );
        assert ( sizeof ( MemBlock ) < newAllocSize );
        assert ( sizeof ( MemBlock ) + _size + count <= newAllocSize );

        MemBlock * const newData = allocateBlock ( newAllocSize );

        if ( !newData )
        {
            return 0;
        }

        _allocSize = newAllocSize;

        if ( _size > 0 )
        {
            memcpy ( reinterpret_cast<char *> ( newData + 1 ), reinterpret_cast<const char *> ( _data + 1 ), _size );
        }

        _data->unref();
        _data = newData;
    }
    else if ( _allocSize < sizeof ( MemBlock ) + _size + count )
    {
        // We are the only user but we need more memory.

        assert ( _data != 0 );
        assert ( _data->getRefCount() == 1 );

        const size_t newAllocSize = getAllocSize ( _size + count, _allocSize );

        assert ( count < newAllocSize );
        assert ( _size < newAllocSize );
        assert ( sizeof ( MemBlock ) < newAllocSize );
        assert ( sizeof ( MemBlock ) + _size + count <= newAllocSize );

        MemBlock * const newData = ( MemBlock * ) realloc ( _data, newAllocSize );

        if ( !newData )
        {
            return 0;
        }

        // Previous _data is already deallocated by realloc()

        _data = newData;

        _data->init ( MemBlock::TypeBuffer );

        _allocSize = newAllocSize;
    }

    assert ( _data != 0 );
    assert ( _data->getRefCount() == 1 );
    assert ( count < _allocSize );
    assert ( _size < _allocSize );
    assert ( sizeof ( MemBlock ) < _allocSize );
    assert ( sizeof ( MemBlock ) + _size + count <= _allocSize );

    return reinterpret_cast<char *> ( _data + 1 ) + _size;
}

void Buffer::markAppended ( size_t count )
{
    // count is 0 or too big
    if ( count < 1 || count > ( ( ( size_t ) -1 ) - _size - sizeof ( MemBlock ) ) )
    {
        return;
    }

    // This only affects this particular buffer!
    // The actual data is not affected, so we don't need to ensure that nobody else uses the memory block.

    if ( !_data )
    {
        assert ( false );

        return;
    }

    if ( sizeof ( MemBlock ) + _size + count > _allocSize )
    {
        assert ( false );

        return;
    }

    _size += count;

    assert ( _size >= count );
    assert ( sizeof ( MemBlock ) + _size > count );
}

void Buffer::appendData ( const char * data, size_t count )
{
    // count is 0 or too big
    if ( count < 1 || count > ( ( ( size_t ) -1 ) - _size - sizeof ( MemBlock ) ) )
    {
        return;
    }

    if ( !data )
    {
        assert ( false );

        return;
    }

    // Now, this can get tricky! If we do something like this:
    //
    // Buffer buf;
    //
    // (...)
    //
    // buf.appendData(buf.get(2), 5);
    //
    // we are appending data that is in this buffer into the same buffer!
    // Now, if we simply call getAppendable, the original memory may get reallocated.
    // This means, that after calling getAppendable, the 'data' pointer may be no longer valid.
    //
    // Let's check whether 'data' points to the middle of our underlying memory:
    //

    if ( _data != 0 && data >= ( ( const char * ) _data ) && data < ( ( const char * ) _data ) + _allocSize )
    {
        // Let's remember the original offset!
        const ptrdiff_t offset = data - ( ( const char * ) _data );

#ifndef NDEBUG
        assert ( offset >= 0 );

        // Technically this is not up to the buffer, but if either of these tests fails there is something
        // seriously wrong with the code that uses the buffer (and not the buffer itself):

        assert ( offset + count <= _allocSize );
        assert ( offset + count <= sizeof ( MemBlock ) + _size );

        // count is at least 1:
        const char checkByteA = data[ 0 ];

        // even if count is 1, both indexes should be valid:
        const char checkByteB = data[ count / 2 ];
        const char checkByteC = data[ count - 1 ];
#endif

        char * const mem = getAppendable ( count );

        if ( !mem )
        {
            return;
        }

#ifndef NDEBUG
        assert ( mem != 0 );
        assert ( ( size_t ) offset < _allocSize );
        assert ( offset + count <= _allocSize );
        assert ( offset + count <= sizeof ( MemBlock ) + _size );

        assert ( checkByteA == ( ( const char * ) _data )[ offset ] );
        assert ( checkByteB == ( ( const char * ) _data )[ offset + count / 2 ] );
        assert ( checkByteC == ( ( const char * ) _data )[ offset + count - 1 ] );
#endif

        // Instead of using the original 'data' let's use the data that is stored at the
        // same offset in our current buffer (it should be the same data):
        memcpy ( mem, ( ( const char * ) _data ) + offset, count );

        markAppended ( count );

#ifndef NDEBUG
        assert ( checkByteA == ( ( const char * ) _data )[ offset ] );
        assert ( checkByteB == ( ( const char * ) _data )[ offset + count / 2 ] );
        assert ( checkByteC == ( ( const char * ) _data )[ offset + count - 1 ] );
#endif

        return;
    }

    // In all other cases we can safely use getAppendable and markAppended:

    char * const mem = getAppendable ( count );

    if ( !mem )
    {
        return;
    }

    memcpy ( mem, data, count );

    markAppended ( count );
}

void Buffer::append ( const char * str )
{
    if ( str != 0 )
        appendData ( str, strlen ( str ) );
}

void Buffer::append ( const String & str )
{
    appendData ( str.c_str(), str.length() );
}

void Buffer::append ( const MemHandle & memHandle )
{
    const MemData & mData = memHandle.getMemData();

    if ( mData.size < 1 )
        return;

    if ( !mData.block )
    {
        assert ( false );
        return;
    }

    // Maybe we can reference the memory without having to copy it?

    if ( !_data
         && mData.block->getType() == MemBlock::TypeBuffer
         && mData.mem == reinterpret_cast<char *> ( mData.block + 1 ) )
    {
        // This is possible if (all are valid):
        //  - this buffer is still empty
        //  - MemHandle uses buffer memory block
        //  - MemHandle's memory points to right after MemBlock
        //    (which means it starts at the beginning of the payload).

        if ( mData.block->ref() )
        {
            _data = mData.block;
            _size = mData.size;

            // Technically we don't know what the allocated data size is.
            // But at the very least it should be sizeof(MemBlock) + data size.
            // If we never need to append or modify the size - great.
            // If we do, we will simply reallocate memory then (even if we wouldn't really need to).
            _allocSize = sizeof ( MemBlock ) + _size;
            return;
        }

        // There are too many references. We need to perform a regular 'append'.
        LOG_TOO_MANY_REFS;
    }

    appendData ( mData.mem, mData.size );
}

void Buffer::append ( const Buffer & buf )
{
    // Maybe we can reference the memory without having to copy it?
    // This is only possible if this buffer is still empty.
    if ( !_data )
    {
        operator= ( buf );
        return;
    }

    appendData ( buf.get(), buf.size() );
}

MemData Buffer::getMemData() const
{
    if ( _size < 1 )
    {
        // We may still have a valid memory block, but if there is no memory used, there is no point returning it.
        return MemData();
    }

    assert ( _data != 0 );
    assert ( _size > 0 );

    return MemData ( _data, reinterpret_cast<char *> ( _data + 1 ), _size );
}

MemHandle Buffer::getHandle ( size_t offset ) const
{
    if ( offset >= _size )
    {
        return MemHandle();
    }

    assert ( _data != 0 );
    assert ( _size > 0 );

    // We don't use MemHandle(Buffer&) constructor, because it may copy ALL the data.
    // If offset is > 0, we will need to copy less.

    MemData mData ( _data, reinterpret_cast<char *> ( _data + 1 ), _size );

    assert ( mData.block != 0 );
    assert ( mData.mem != 0 );
    assert ( mData.size > offset );

    mData.mem += offset;
    mData.size -= offset;

    mData.ref();

    // This does NOT create another reference:
    return MemHandle ( mData );
}

MemHandle Buffer::getHandle ( size_t offset, size_t handleSize ) const
{
    if ( offset >= _size || handleSize < 1 )
    {
        return MemHandle();
    }

    assert ( _data != 0 );
    assert ( _size > 0 );

    // We don't use MemHandle(Buffer&) constructor, because it may copy ALL the data.
    // If offset is > 0 or size < _size, we will need to copy less.

    MemData mData ( _data, reinterpret_cast<char *> ( _data + 1 ), _size );

    assert ( mData.block != 0 );
    assert ( mData.mem != 0 );
    assert ( mData.size > offset );

    mData.mem += offset;
    mData.size -= offset;

    if ( mData.size > handleSize )
    {
        mData.size = handleSize;
    }

    mData.ref();

    // This does NOT create another reference:
    return MemHandle ( mData );
}

bool Buffer::writeToFile ( const char * filePath, bool appendToFile )
{
    return MemHandle::writeDataToFile ( get(), size(), filePath, appendToFile );
}

bool Buffer::writeToFile ( const String & filePath, bool appendToFile )
{
    return MemHandle::writeDataToFile ( get(), size(), filePath.c_str(), appendToFile );
}
