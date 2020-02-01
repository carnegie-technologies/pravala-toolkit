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

#include <cerrno>
#include <cstdio>

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

#include "basic/MsvcSupport.hpp"

extern "C"
{
#include <io.h>
}

#else

extern "C"
{
#include <unistd.h>
}
#endif

#ifdef SYSTEM_LINUX

extern "C"
{
#include <sys/mman.h>
}
#endif

#include "internal/MemPool.hpp"
#include "Platform.hpp"
#include "MemHandle.hpp"
#include "Buffer.hpp"

// Define MEMORY_DEBUGGING to enable some low-level logging:
// #define MEMORY_DEBUGGING

#ifdef MEMORY_DEBUGGING
#include <cstdio>
#endif

using namespace Pravala;

const MemHandle MemHandle::EmptyHandle;

/// @brief A helper function that creates a MemData structure based on provided values.
/// @param [in] data Pointer to external data block.
/// @param [in] dataSize The size of the data block.
/// @param [in] deallocator Function to be called when the memory is released.
///                         If it's 0, it means that memory does not need to be deallocated.
/// @param [in] deallocatorData User data that will be associated with the memory block.
///                             It can be used to store additional state needed by the deallocator.
///                             It will be stored inside the DeallocatorMemBlock and can
///                             be accessed by deallocator function (which is being passed a pointer
///                             to the DeallocatorMemBlock being released).
/// @return The MemData describing the memory. It may be empty.
///         If it isn't, it will include a single reference to the underlying block.
static MemData createDeallocatorData (
        char * data,
        size_t dataSize,
        DeallocatorMemBlock::DeallocatorFunctionType deallocator,
        void * deallocatorData )
{
    DeallocatorMemBlock * const block = ( DeallocatorMemBlock * ) malloc ( sizeof ( DeallocatorMemBlock ) );

    if ( !block )
    {
        return MemData();
    }

    block->init ( MemBlock::TypeDeallocator );

    block->data = data;
    block->size = dataSize;
    block->deallocator = deallocator;
    block->deallocatorData = deallocatorData;

    return MemData ( block, data, dataSize );
}

ExtMemHandle::ExtMemHandle (
        char * data,
        size_t dataSize,
        DeallocatorMemBlock::DeallocatorFunctionType deallocator,
        void * deallocatorData ):
    MemHandle ( createDeallocatorData ( data, dataSize, deallocator, deallocatorData ) )
{
}

MemHandle::MemHandle ( const MemData & memData ): _data ( memData )
{
    // This is an internal constructor that takes over MemData - without creating an additional reference!
}

MemHandle::MemHandle ( const Buffer & buffer ): _data ( buffer.getMemData() )
{
    // getMemData() gives us the original (even though it's a copy),
    // without an additional reference, so we have to create one:
    _data.ref();
}

MemHandle::MemHandle ( const MemHandle & oth ): _data ( oth.getMemData() )
{
    // getMemData() gives us the original, without an additional reference, so we have to create one:
    _data.ref();
}

MemHandle::MemHandle ( const char * filePath, bool * ok )
{
    const bool ret = readFile ( filePath );

    if ( ok != 0 )
    {
        *ok = ret;
    }
}

MemHandle::MemHandle ( const String & filePath, bool * ok )
{
    const bool ret = readFile ( filePath );

    if ( ok != 0 )
    {
        *ok = ret;
    }
}

MemHandle::MemHandle ( size_t dataSize )
{
    // dataSize is 0 or too big
    if ( dataSize < 1 || dataSize > ( ( ( size_t ) -1 ) - sizeof ( MemBlock ) ) )
    {
        return;
    }

    _data.block = ( MemBlock * ) malloc ( sizeof ( MemBlock ) + dataSize );

    if ( !_data.block )
    {
        return;
    }

    _data.block->init ( MemBlock::TypeBuffer );
    _data.size = dataSize;
    _data.mem = reinterpret_cast<char *> ( _data.block + 1 );
}

MemHandle & MemHandle::operator= ( const MemHandle & other )
{
    _data.replaceWith ( other.getMemData() );
    return *this;
}

MemHandle & MemHandle::operator= ( const Buffer & buffer )
{
    _data.replaceWith ( buffer.getMemData() );
    return *this;
}

MemHandle::~MemHandle()
{
    clear();
}

void MemHandle::clear()
{
    _data.unref();
}

void MemHandle::setZero()
{
    char * mem;

    if ( _data.size > 0 && ( mem = getWritable() ) != 0 )
    {
        memset ( mem, 0, _data.size );
    }
}

bool MemHandle::getData ( size_t & offset, void * mem, size_t memSize ) const
{
    const size_t newOffset = offset + memSize;

    if ( newOffset < offset || newOffset < memSize || newOffset > _data.size )
    {
        // This also tests overflows...
        return false;
    }

    if ( memSize < 1 )
    {
        return true;
    }

    if ( !mem )
    {
        return false;
    }

    assert ( _data.mem != 0 );
    assert ( offset + memSize <= _data.size );

    memcpy ( mem, _data.mem + offset, memSize );
    offset += memSize;

    return true;
}

bool MemHandle::setData ( size_t & offset, const void * mem, size_t memSize )
{
    const size_t newOffset = offset + memSize;

    if ( newOffset < offset || newOffset < memSize || newOffset > _data.size )
    {
        // This also tests overflows...
        return false;
    }

    if ( memSize < 1 )
    {
        return true;
    }

    if ( !mem || !_data.ensureWritable() )
    {
        return false;
    }

    assert ( _data.block != 0 );
    assert ( _data.mem != 0 );
    assert ( _data.block->getRefCount() == 1 );
    assert ( !_data.block->usesReadOnlyType() );
    assert ( offset + memSize <= _data.size );

    memcpy ( _data.mem + offset, mem, memSize );
    offset += memSize;

    return true;
}

char * MemHandle::getWritable ( size_t offset )
{
    if ( offset >= _data.size || !_data.ensureWritable() )
    {
        return 0;
    }

    assert ( _data.block != 0 );
    assert ( _data.mem != 0 );
    assert ( _data.block->getRefCount() == 1 );
    assert ( !_data.block->usesReadOnlyType() );
    assert ( offset < _data.size );

    return _data.mem + offset;
}

MemHandle MemHandle::getHandle ( size_t offset ) const
{
    if ( offset >= _data.size )
    {
        return MemHandle();
    }

    // We cannot use MemHandle ( MemHandle& ) constructor, since it may create a brand new copy of the data.
    // This happens when there are too many references to the underlying MemBlock.
    // Here we only want part of the data, and we don't want to copy everything.

    MemData data = _data;

    assert ( data.block != 0 );
    assert ( data.mem != 0 );
    assert ( data.size > offset );

    data.mem += offset;
    data.size -= offset;

    data.ref();

    // This does NOT create another reference:
    return MemHandle ( data );
}

MemHandle MemHandle::getHandle ( size_t offset, size_t newSize ) const
{
    if ( offset >= _data.size || newSize < 1 )
    {
        return MemHandle();
    }

    // We cannot use MemHandle ( MemHandle& ) constructor, since it may create a brand new copy of the data.
    // This happens when there are too many references to the underlying MemBlock.
    // Here we only want part of the data, and we don't want to copy everything.

    MemData data = _data;

    assert ( data.block != 0 );
    assert ( data.mem != 0 );
    assert ( data.size > offset );

    data.mem += offset;
    data.size -= offset;

    if ( data.size > newSize )
    {
        data.size = newSize;
    }

    data.ref();

    // This does NOT create another reference:
    return MemHandle ( data );
}

bool MemHandle::truncate ( size_t newSize )
{
    if ( newSize >= _data.size )
    {
        return ( _data.size > 0 );
    }

    if ( newSize < 1 )
    {
        clear();
        return false;
    }

    assert ( _data.block != 0 );
    assert ( _data.mem != 0 );

    _data.size = newSize;
    return true;
}

bool MemHandle::consume ( size_t numBytes )
{
    if ( numBytes < 1 )
    {
        return ( _data.size > 0 );
    }

    if ( numBytes >= _data.size )
    {
        clear();
        return false;
    }

    assert ( _data.block != 0 );
    assert ( _data.mem != 0 );
    assert ( _data.size > numBytes );

    _data.mem += numBytes;
    _data.size -= numBytes;

    return true;
}

bool MemHandle::readFile ( const char * filePath )
{
    errno = 0;

    const int fd = open ( filePath,
#ifdef O_BINARY
                          O_BINARY |
#endif
                          O_RDONLY );

    if ( fd < 0 )
    {
        clear();
        return false;
    }

    const bool ret = readFile ( fd );

    close ( fd );

    return ret;
}

bool MemHandle::readFile ( int fd )
{
    clear();

    if ( fd < 0 )
    {
        return false;
    }

    struct stat fStat;

    if ( fstat ( fd, &fStat ) == 0 && fStat.st_size > 0 && ( fStat.st_mode & S_IFREG ) == S_IFREG )
    {
        // Everything looks normal!

        /// @todo TODO: Figure out when mmap can be enabled (it should be available on other platforms, not just Linux).

#ifdef SYSTEM_LINUX
        void * const map = mmap ( 0, fStat.st_size, PROT_READ, MAP_SHARED, fd, 0 );

        // If it fails we fall back to regular read below!
        if ( map != MAP_FAILED )
        {
            ExternalMemBlock * const block = ( ExternalMemBlock * ) malloc ( sizeof ( ExternalMemBlock ) );

            if ( !block )
            {
                munmap ( map, fStat.st_size );
                return false;
            }

            block->init ( MemBlock::TypeMMapRO );

            block->data = ( char * ) map;
            block->size = fStat.st_size;

            // We cleared the handle at the beginning!
            assert ( !_data.block );

            _data.block = block;
            _data.mem = block->data;
            _data.size = fStat.st_size;

            // We already own the reference, so no need to do anything else.
            return true;
        }
#endif

        MemBlock * const block = ( MemBlock * ) malloc ( ( size_t ) ( sizeof ( MemBlock ) + fStat.st_size ) );

        if ( !block )
        {
            return false;
        }

        const ssize_t ret = read ( fd, ( ( char * ) block ) + sizeof ( MemBlock ), ( size_t ) fStat.st_size );

        if ( ret <= 0 )
        {
            free ( block );
            return false;
        }

        block->init ( MemBlock::TypeBuffer );

        // We cleared the handle at the beginning!
        assert ( !_data.block );

        _data.block = block;
        _data.mem = reinterpret_cast<char *> ( block + 1 );
        _data.size = ret;

        // We already own the reference, so no need to do anything else.
        return true;
    }

    // We couldn't stat the file, or the size was 0 (which happens if we are trying to read from /proc/),
    // or it wasn't really a file. Let's just try reading from the descriptor until the end of it.

    Buffer buffer;

    while ( true )
    {
        char * const mem = buffer.getAppendable ( Platform::PageSize );

        if ( !mem )
        {
            break;
        }

        const ssize_t ret = read ( fd, mem, Platform::PageSize );

        if ( ret == 0 )
        {
            _data.replaceWith ( buffer.getMemData() );
            return true;
        }

        if ( ret < 0 )
        {
            break;
        }

        buffer.markAppended ( ret );
    }

    return false;
}

bool MemHandle::writeDataToFile ( const char * data, size_t size, const char * filePath, bool appendToFile )
{
    FILE * output;

    // 'b' is not used on Posix systems, but may be used elsewhere
    output = fopen ( filePath, appendToFile ? "ab" : "wb" );

    if ( !output )
    {
        return false;
    }

    size_t written = 0;

    while ( written < size )
    {
        size_t ret = fwrite ( data + written, 1, size - written, output );

        if ( ret > 0 )
            written += ret;

        if ( ferror ( output ) || ret == 0 )
        {
            fclose ( output );
            return false;
        }
    }

    fclose ( output );
    return true;
}
