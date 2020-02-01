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

#include "Math.hpp"
#include "MemVector.hpp"

#ifdef NDEBUG
#define CHECK_VECTOR
#else
#define CHECK_VECTOR \
    do { \
        size_t tmpCheckSize = 0; \
        for ( IndexType tmpCheckIdx = 0; tmpCheckIdx < _dataVec.size(); ++tmpCheckIdx ) { \
            assert ( _dataVec.getFirst ( tmpCheckIdx ) != 0 ); \
            assert ( _dataVec.getSecond ( tmpCheckIdx ).iov_base != 0 ); \
            assert ( _dataVec.getSecond ( tmpCheckIdx ).iov_len > 0 ); \
            tmpCheckSize += _dataVec.getSecond ( tmpCheckIdx ).iov_len; \
        } \
        assert ( tmpCheckSize == _dataSize ); \
    } while ( false )
#endif

using namespace Pravala;

typedef MemBlock * MemBlockPtr;

const MemVector MemVector::EmptyVector;

MemVector::MemVector ( IndexType numEntries ): _dataVec ( min<IndexType> ( numEntries, MaxChunks ) ), _dataSize ( 0 )
{
}

MemVector::MemVector ( const MemVector & other ): _dataVec ( other._dataVec ), _dataSize ( other._dataSize )
{
    refAllBlocks();

    CHECK_VECTOR;
}

MemVector::MemVector ( const MemHandle & mh ): _dataVec ( mh.isEmpty() ? 0 : 1 ), _dataSize ( 0 )
{
    MemData mData ( mh.getMemData() );

    if ( !mData.ref() )
    {
        return;
    }

    assert ( mData.mem != 0 );
    assert ( mData.block != 0 );
    assert ( mData.size > 0 );

    appendMemData ( mData );
    _dataSize = mData.size;

    assert ( _dataVec.size() == 1 );

    CHECK_VECTOR;
}

MemVector & MemVector::operator= ( const MemVector & other )
{
    if ( this == &other )
    {
        return *this;
    }

    // This is a different object, which means it has its own references.
    // We can safely remove current state!
    clear();

    if ( other.getNumChunks() < 1 || other.getDataSize() < 1 )
    {
        return *this;
    }

    _dataVec = other._dataVec;
    _dataSize = other._dataSize;

    refAllBlocks();

    CHECK_VECTOR;
    return *this;
}

MemVector & MemVector::operator= ( const MemHandle & mh )
{
    clear();

    if ( mh.isEmpty() )
    {
        return *this;
    }

    MemData mData ( mh.getMemData() );

    if ( !mData.ref() )
    {
        return *this;
    }

    assert ( mData.mem != 0 );
    assert ( mData.block != 0 );
    assert ( mData.size > 0 );

    appendMemData ( mData );
    _dataSize = mData.size;

    assert ( _dataVec.size() == 1 );

    CHECK_VECTOR;
    return *this;
}

MemVector::~MemVector()
{
    clear();
}

void MemVector::clear()
{
    const MemBlockPtr * const mem = _dataVec.getFirstMemory();

    for ( IndexType i = 0; i < _dataVec.size(); ++i )
    {
        assert ( mem[ i ] != 0 );

        mem[ i ]->unref();
    }

    _dataVec.clear();
    _dataSize = 0;
}

void MemVector::refAllBlocks()
{
    MemBlockPtr * const blocks = _dataVec.getFirstWritableMemory();
    struct iovec * const chunks = _dataVec.getSecondWritableMemory();

    for ( IndexType i = 0; i < _dataVec.size(); ++i )
    {
        assert ( blocks[ i ] != 0 );
        assert ( chunks[ i ].iov_base != 0 );
        assert ( chunks[ i ].iov_len > 0 );

        if ( !blocks[ i ]->ref() )
        {
            // Creating a reference failed, we need a copy.
            // Let's have MemData do that for us!
            // We could always rely on MemData to do that for us, but we would have to create a brand new
            // MemData object for each entry.
            // Since in most cases referencing the block itself should work, that would be wasteful.

            MemData mData ( blocks[ i ], static_cast<char *> ( chunks[ i ].iov_base ), chunks[ i ].iov_len );

            // Technically MemData::ref() first tries to create a reference (again), which we already know won't work
            // (unless something on another thread just released a reference, but we shouldn't count on it).
            // However, MemBlock::ref() failing should be a very rare situation to begin with.
            // So instead of exposing just the copy functionality from MemData
            // (with its "advanced" parameter that controls whether the original should be unreferenced),
            // we keep the code simple and just call MemData::ref().

            if ( !mData.ref() )
            {
                // It failed to create a reference AND a copy!
                // Let's clear the entire state (better than having corrupted data).
                // However, we already created some references (up to, not including, index i).
                // Let's remove them (we cannot use clear() because it would unref ALL the blocks).

                for ( IndexType j = 0; j < i; ++j )
                {
                    _dataVec.getFirst ( j )->unref();
                }

                _dataVec.clear();
                _dataSize = 0;
                return;
            }

            assert ( mData.size == chunks[ i ].iov_len );

            // It succeeded! But it is probably using different block/memory now!
            blocks[ i ] = mData.block;
            chunks[ i ].iov_base = mData.mem;

            assert ( mData.size == chunks[ i ].iov_len );
        }
    }
}

void MemVector::stealFrom ( MemVector & other )
{
    if ( this == &other )
        return;

    clear();

    _dataVec = other._dataVec;
    _dataSize = other._dataSize;

    // We cannot use other.clear(), because it would unref() the blocks; We have to clear its state directly.
    other._dataVec.clear();
    other._dataSize = 0;

    CHECK_VECTOR;
}

bool MemVector::append ( const MemHandle & mh, size_t offset )
{
    if ( getNumChunks() >= MaxChunks )
    {
        return false;
    }

    MemData mData ( mh.getMemData() );

    if ( mData.size < offset )
    {
        return false;
    }
    else if ( offset < mData.size )
    {
        mData.size -= offset;
        mData.mem += offset;

        if ( !mData.ref() )
        {
            return false;
        }

        assert ( mData.mem != 0 );
        assert ( mData.block != 0 );
        assert ( mData.size > 0 );

        appendMemData ( mData );
        _dataSize += mData.size;

        CHECK_VECTOR;
    }

    return true;
}

bool MemVector::append ( const MemVector & vec, size_t offset )
{
    if ( this == &vec || offset > vec.getDataSize() )
    {
        return false;
    }

    const IndexType addCount = vec.getNumChunks();

    if ( addCount < 1 || offset == vec.getDataSize() )
    {
        return true;
    }

    // So we can remove entries from partially appended vector if something fails.
    const IndexType orgCount = _dataVec.size();
    const size_t orgDataSize = _dataSize;

    {
        const size_t newCount = addCount + orgCount;

        if ( newCount > MaxChunks || newCount < addCount || newCount < orgCount )
        {
            return false;
        }
    }

    _dataVec.ensureSizeAllocated ( orgCount + addCount );

    const MemBlockPtr * const blocks = vec._dataVec.getFirstMemory();
    const struct iovec * const chunks = vec._dataVec.getSecondMemory();

    for ( IndexType i = 0; i < addCount; ++i )
    {
        if ( offset >= chunks[ i ].iov_len )
        {
            // Skipping the entire chunk.
            offset -= chunks[ i ].iov_len;
            continue;
        }

        // See refAllBlocks() for comments for the following code:

        MemData mData ( blocks[ i ], static_cast<char *> ( chunks[ i ].iov_base ), chunks[ i ].iov_len );

        if ( offset > 0 )
        {
            assert ( offset < mData.size );

            mData.mem += offset;
            mData.size -= offset;

            offset = 0;
        }

        if ( !mData.ref() )
        {
            for ( size_t j = 0; j < i; ++j )
            {
                _dataVec.getFirst ( orgCount + j )->unref();
            }

            _dataVec.truncate ( orgCount );
            _dataSize = orgDataSize;

            CHECK_VECTOR;
            return false;
        }

        appendMemData ( mData );
        _dataSize += mData.size;
    }

    CHECK_VECTOR;
    return true;
}

bool MemVector::prepend ( const MemHandle & mh )
{
    if ( getNumChunks() >= MaxChunks )
    {
        return false;
    }

    if ( mh.getMemData().size > 0 )
    {
        MemData mData ( mh.getMemData() );

        if ( !mData.ref() )
        {
            return false;
        }

        assert ( mData.mem != 0 );
        assert ( mData.block != 0 );
        assert ( mData.size > 0 );

        prependMemData ( mData );
        _dataSize += mData.size;

        CHECK_VECTOR;
    }

    return true;
}

bool MemVector::consume ( size_t numBytes )
{
    if ( numBytes >= _dataSize )
    {
        clear();
        return false;
    }

    IndexType idx = 0;

    const MemBlockPtr * const blocks = _dataVec.getFirstMemory();
    struct iovec * const chunks = _dataVec.getSecondWritableMemory();

    // Since numBytes < _dataSize, we will use up numBytes before going past the last chunk,
    // so there is no need to check the index.
    while ( numBytes > 0 )
    {
        assert ( idx < _dataVec.size() );

        struct iovec & chunk = chunks[ idx ];

        assert ( chunk.iov_len > 0 );

        if ( numBytes < chunk.iov_len )
        {
            // We need part of this chunk.
            chunk.iov_len -= numBytes;
            chunk.iov_base = static_cast<char *> ( chunk.iov_base ) + numBytes;

            assert ( numBytes < _dataSize );

            _dataSize -= numBytes;
            numBytes = 0;
            break;
        }

        assert ( numBytes >= chunk.iov_len );
        assert ( _dataSize > chunk.iov_len );

        numBytes -= chunk.iov_len;
        _dataSize -= chunk.iov_len;

        blocks[ idx ]->unref();
        ++idx;
    }

    // Now 'idx' is the index of the first chunk we want to keep (and also the number of chunks to remove).

    assert ( idx < _dataVec.size() );
    assert ( _dataSize > 0 );
    assert ( numBytes == 0 );

    if ( idx > 0 )
    {
        _dataVec.leftTrim ( idx );
    }

    assert ( _dataVec.size() > 0 );

    CHECK_VECTOR;
    return true;
}

void MemVector::truncate ( size_t numBytes )
{
    if ( numBytes >= _dataSize )
    {
        return;
    }
    else if ( numBytes < 1 )
    {
        clear();
        return;
    }

    assert ( _dataSize > numBytes );

    size_t toRemove = _dataSize - numBytes;

    const MemBlockPtr * const blocks = _dataVec.getFirstMemory();
    struct iovec * const chunks = _dataVec.getSecondWritableMemory();

    assert ( _dataVec.size() > 0 );

    // First let's remove the entire chunks that can be removed from the end.

    IndexType idx = _dataVec.size() - 1;

    while ( toRemove >= chunks[ idx ].iov_len )
    {
        toRemove -= chunks[ idx ].iov_len;
        _dataSize -= chunks[ idx ].iov_len;

        blocks[ idx ]->unref();

        assert ( idx > 0 );

        --idx;
    }

    // Here idx is the index of the last chunk we want to keep.
    // We may still need to remove some bytes from the end of it.

    assert ( toRemove < chunks[ idx ].iov_len );
    assert ( toRemove < _dataSize );

    chunks[ idx ].iov_len -= toRemove;
    _dataSize -= toRemove;

    _dataVec.truncate ( idx + 1 );

    assert ( _dataVec.size() > 0 );
    assert ( _dataSize = numBytes );

    CHECK_VECTOR;
}

MemHandle MemVector::getChunk ( IndexType idx ) const
{
    if ( idx >= _dataVec.size() )
    {
        return MemHandle();
    }

    DataArray::ConstTuple data ( _dataVec[ idx ] );
    MemData mData ( data.first, static_cast<char *> ( data.second.iov_base ), data.second.iov_len );

    mData.ref();

    return MemHandle ( mData );
}

bool MemVector::storeContinuous ( MemHandle & memory ) const
{
    if ( _dataSize < 1 )
    {
        memory.clear();
        return true;
    }

    if ( _dataVec.size() == 1 )
    {
        // Single memory chunk.
        memory = getChunk ( 0 );
        return true;
    }

    char * mem = 0;

    if ( memory.size() >= _dataSize )
    {
        mem = memory.getWritable();
    }

    if ( !mem )
    {
        memory = MemHandle ( _dataSize );
        mem = memory.getWritable();

        if ( !mem || memory.size() < _dataSize )
        {
            memory.clear();
            return false;
        }
    }

    assert ( mem != 0 );
    assert ( memory.size() >= _dataSize );

    size_t offset = 0;
    const struct iovec * const chunks = _dataVec.getSecondMemory();

    for ( IndexType i = 0; i < _dataVec.size(); ++i )
    {
        const struct iovec & chunk = chunks[ i ];

        assert ( offset + chunk.iov_len <= _dataSize );
        assert ( offset + chunk.iov_len <= memory.size() );

        memcpy ( mem + offset, chunk.iov_base, chunk.iov_len );

        offset += chunk.iov_len;
    }

    assert ( offset == _dataSize );

    // We may have been given a handle that was bigger than _dataSize.
    memory.truncate ( _dataSize );

    assert ( memory.get() != 0 );
    assert ( memory.size() == _dataSize );

    return true;
}

char * MemVector::getContinuousWritable ( size_t size, MemHandle * useHandle )
{
    if ( _dataSize < 1 || size > _dataSize )
    {
        return 0;
    }
    else if ( size < 1 )
    {
        size = _dataSize;
    }

    assert ( size > 0 );
    assert ( _dataSize > 0 );
    assert ( size <= _dataSize );
    assert ( _dataVec.size() > 0 );

    {
        DataArray::Tuple data ( _dataVec[ 0 ] );

        if ( data.second.iov_len >= size )
        {
            // The first (or only) chunk of data is sufficient to satisfy this request.

            assert ( data.first != 0 );

            if ( data.first->getRefCount() < 2 && !data.first->usesReadOnlyType() )
            {
                // Nobody else uses this memory and it's not a R/O type - we can just return the data pointer!

                assert ( data.first->getRefCount() == 1 );

                return static_cast<char *> ( data.second.iov_base );
            }

            // That memory is shared or read-only; We need to make a copy.
            // If passed MemHandle cannot be used, we just do the default memory copy.
            // If 'useHandle' can be used, we will use that for storing the data copied.
            // In that case the path is the same as for multiple chunks.
            if ( !useHandle || useHandle->size() < size )
            {
                assert ( size > 0 );
                assert ( size <= data.second.iov_len );

                // MemData that stores the first 'size' bytes of the first chunk:
                MemData mData ( data.first, static_cast<char *> ( data.second.iov_base ), size );

                if ( size == data.second.iov_len )
                {
                    // This is the entire chunk. All we need is to tell MemData to make a copy
                    // (which will unreference the original) and replace the original.

                    if ( !mData.ensureWritable() || mData.size != data.second.iov_len )
                    {
                        // Something went wrong...
                        return 0;
                    }

                    data.first = mData.block;
                    data.second.iov_base = mData.mem;

                    assert ( data.second.iov_len == mData.size );
                    assert ( data.first->getRefCount() == 1 );

                    CHECK_VECTOR;
                    return static_cast<char *> ( data.second.iov_base );
                }

                assert ( size < data.second.iov_len );

                // Because we are not abandoning the old block, we need to use the internal method to just create
                // data copy, without unrefing the original:

                if ( !mData.intMakeCopy ( false ) || mData.size != size )
                {
                    // Something went wrong...
                    return 0;
                }

                // Now let's remove the bytes we copied from the old block:
                data.second.iov_base = static_cast<char *> ( data.second.iov_base ) + size;
                data.second.iov_len -= size;

                // And insert the new block at the beginning
                // (also, we CANNOT use 'data' after this, since references may be no longer valid):
                prependMemData ( mData );

                CHECK_VECTOR;
                return mData.mem;
            }
        }
    }

    // We need to use multiple chunks or we had a single chunk that was shared, but there was specific
    // memory provided for it (and it was large enough).
    // Either way we have to copy data.

    // The data in which we will store the first 'size' bytes of the vector.
    MemData mData;

    if ( useHandle != 0 && useHandle->size() >= size )
    {
        MemData & useData = useHandle->_data;

        if ( useData.ensureWritable() && useData.size >= size )
        {
            // We can use that data! Let's "steal" it!
            mData = useData;
            useData.clear();
        }
    }

    if ( !mData.block )
    {
        // We need to generate some data.
        MemHandle mh ( size );
        MemData & useData = mh._data;

        if ( useData.ensureWritable() && useData.size >= size )
        {
            // We can use that data! Let's "steal" it!
            mData = useData;
            useData.clear();
        }
        else
        {
            // We tried, we failed...
            return 0;
        }
    }

    assert ( mData.block != 0 );
    assert ( mData.mem != 0 );
    assert ( mData.size >= size );

    if ( mData.size > size )
    {
        mData.size = size;
    }

    IndexType idx = 0;

    // Below we will be decrementing 'size', treating it as 'bytes remaining to be copied'.
    // We use a new block to ensure that 'blocks' and 'chunks' is not used after modifying the vector later.
    {
        const MemBlockPtr * const blocks = _dataVec.getFirstMemory();
        struct iovec * const chunks = _dataVec.getSecondWritableMemory();

        size_t offset = 0;

        while ( size > 0 )
        {
            assert ( idx < _dataVec.size() );

            struct iovec & chunk = chunks[ idx ];

            assert ( chunk.iov_len > 0 );

            if ( size < chunk.iov_len )
            {
                // We need part of this chunk.
                assert ( offset + size <= mData.size );

                memcpy ( mData.mem + offset, chunk.iov_base, size );

                offset += size;
                chunk.iov_len -= size;
                chunk.iov_base = static_cast<char *> ( chunk.iov_base ) + size;

                size = 0;
                break;
            }

            // We need the entire chunk.
            assert ( offset + chunk.iov_len <= mData.size );

            memcpy ( mData.mem + offset, chunk.iov_base, chunk.iov_len );

            offset += chunk.iov_len;

            assert ( size >= chunk.iov_len );

            size -= chunk.iov_len;

            blocks[ idx ]->unref();
            ++idx;
        }

        assert ( size == 0 );
        assert ( offset == mData.size );
    }

    // Now 'idx' is the index of the first chunk we want to keep (and also the number of chunks to remove).
    // It is possible that we used all chunks, in which case idx == _dataVec.size().
    assert ( idx <= _dataVec.size() );

    if ( idx == 0 )
    {
        // We cannot get rid of any chunks - the first data chunk in the vector is still needed.
        // This happens when that first chunk is bigger than 'size', and only part of it was copied.
        // Let's insert the new data at the beginning (in front of that partially used chunk).
        // Note: That chunk's pointer and size should have already been modified.
        prependMemData ( mData );

        // And that's it!

        CHECK_VECTOR;
        return mData.mem;
    }

    // idx > 0, which means we have at least one chunk at the beginning that we don't need.
    // Normally we would remove 'idx' chunks from the beginning of the vector, and then prepend it with mData.
    // But this means that we would be shifting things in the vector twice - once left, to move all the chunks
    // we want to keep to the beginning of the vector, and once right, to prepend the vector with mData.
    // Instead, we shift chunks by 'idx-1' positions to the left. Once we do that, the last chunk that we don't need
    // will be at the beginning of the vector, and the first chunk that we do want to keep will be in the second slot.
    // Then we will just overwrite the first chunk with mData and avoid having to move everything for the second time.

    if ( idx > 1 )
    {
        // We trim 'idx-1' chunks from the left.
        // After this, we won't need the first one, but we want to overwrite the first slot with the new MemData.
        // Note: All the chunks we are removing have been unrefed already!

        _dataVec.leftTrim ( idx - 1 );
    }

    // Now we moved the chunks to the left (if there were more than 1 chunk to remove).
    // The first chunk is not needed anymore, so we can just overwrite it with mData.
    // Note: We don't use 'blocks' or 'chunks' from before, in case the underlying array was modified.
    DataArray::Tuple data ( _dataVec[ 0 ] );

    data.first = mData.block;
    data.second.iov_base = mData.mem;
    data.second.iov_len = mData.size;

    CHECK_VECTOR;
    return mData.mem;
}
