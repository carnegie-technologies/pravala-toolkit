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

extern "C"
{
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
}

#include "File.hpp"
#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

#include "basic/MsvcSupport.hpp"

extern "C"
{
#include <direct.h> // for _mkdir
#include <io.h>
}

#ifdef mkdir
#undef mkdir
#endif

#define mkdir( a, b )    _mkdir ( a )

#else

extern "C"
{
#include <unistd.h>
}
#endif

using namespace Pravala;

ERRCODE File::write (
        const Buffer & buffer, const String & filePath,
        bool appendToFile, bool createDirs )
{
    return File::write ( buffer.get(), buffer.size(), filePath.c_str(), appendToFile, createDirs );
}

ERRCODE File::write (
        const MemHandle & handle, const String & filePath,
        bool appendToFile, bool createDirs )
{
    return File::write ( handle.get(), handle.size(), filePath.c_str(), appendToFile, createDirs );
}

ERRCODE File::write (
        const String & str, const String & filePath,
        bool appendToFile, bool createDirs )
{
    return File::write ( str.c_str(), str.length(), filePath.c_str(), appendToFile, createDirs );
}

ERRCODE File::write (
        const char * data, size_t dataSize, const char * filePath,
        bool appendToFile, bool createDirs )
{
    FILE * output;

    // 'b' is not used on Posix systems, but may be used elsewhere

    output = fopen ( filePath, appendToFile ? "ab" : "wb" );

    if ( !output && createDirs )
    {
        StringList pParts = String ( filePath ).split ( "/" );
        String tmp;

        for ( size_t i = 0; i + 1 < pParts.size(); ++i )
        {
            tmp.append ( "/" );
            tmp.append ( pParts[ i ] );

            if ( mkdir ( tmp.c_str(), 0700 ) != 0 && errno != EEXIST )
                return Error::MkdirFailed;
        }

        output = fopen ( filePath, appendToFile ? "ab" : "wb" );
    }

    if ( !output )
        return Error::OpenFailed;

    size_t written = 0;

    while ( written < dataSize )
    {
        size_t ret = fwrite ( data + written, 1, dataSize - written, output );

        if ( ret > 0 ) written += ret;

        if ( ferror ( output ) || ret == 0 )
        {
            fclose ( output );
            return Error::WriteFailed;
        }
    }

    fclose ( output );
    return Error::Success;
}

ERRCODE File::read ( const char * filePath, MemHandle & buffer )
{
    const int fd = open ( filePath,
#ifdef O_BINARY
                          O_BINARY |
#endif
                          O_RDONLY );

    if ( fd < 0 )
    {
        buffer.clear();
        return Error::OpenFailed;
    }

    const bool ret = buffer.readFile ( fd );

    close ( fd );

    return ret ? ( Error::Success ) : ( Error::ReadFailed );
}

#ifndef HAVE_ZLIB
ERRCODE File::readGzip ( const char *, MemHandle & )
{
    return Error::Unsupported;
}

#else
ERRCODE File::readGzip ( const char * filePath, MemHandle & data )
{
    data.clear();

    gzFile input = gzopen ( filePath, "rb" );

    if ( !input )
        return Error::OpenFailed;

    const int readSize = 512;
    int readRet = 0;

    Buffer buffer;

    do
    {
        assert ( input != 0 );

        char * buf = buffer.getAppendable ( readSize );

        if ( !buf )
        {
            gzclose ( input );
            return Error::MemoryError;
        }

        readRet = gzread ( input, buf, readSize );

        if ( readRet < 0 )
        {
            gzclose ( input );

            return Error::ReadFailed;
        }

        if ( readRet > 0 )
            buffer.markAppended ( readRet );
    }
    while ( readRet == readSize );

    gzclose ( input );

    data = buffer;

    return Error::Success;
}
#endif

bool File::exists ( const char * filePath )
{
    FILE * tmp = fopen ( filePath, "r" );

    if ( !tmp )
        return false;

    fclose ( tmp );

    return true;
}

bool File::isWritable ( const char * filePath, bool removeAfter )
{
    FILE * tmp = fopen ( filePath, "a" );

    // if we failed to open the file, we don't need to remove it
    if ( !tmp )
        return false;

    fclose ( tmp );

    if ( removeAfter )
        ::remove ( filePath );

    return true;
}

bool File::remove ( const char * filePath )
{
    return ( ::remove ( filePath ) == 0 );
}
