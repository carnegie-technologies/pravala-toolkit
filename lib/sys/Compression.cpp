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
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
}

#include <cassert>

#include "Compression.hpp"

/// @brief Value to add to windowBits when initializing deflate to tell zlib's deflate() to write a gzip
/// header/trailer instead of a zlib header/trailer.
#define ZLIB_WINDOWBITS_USE_GZIP_ENCODING    16

using namespace Pravala;

#ifndef HAVE_ZLIB
MemHandle Compression::gzip ( const MemHandle &, ERRCODE * eCode )
{
    if ( eCode != 0 )
    {
        *eCode = Error::Unsupported;
    }

    return MemHandle();
}

#else
MemHandle Compression::gzip ( const MemHandle & data, ERRCODE * eCode )
{
    if ( data.isEmpty() )
    {
        if ( eCode != 0 )
        {
            *eCode = Error::InvalidParameter;
        }

        return MemHandle();
    }

    z_stream strm;

    // zlib doesn't modify it, but its field isn't const
    strm.next_in = reinterpret_cast<unsigned char *> ( const_cast<char *> ( data.get() ) );
    strm.avail_in = data.size();

    assert ( strm.next_in != 0 );
    assert ( strm.avail_in > 0 );

    strm.next_out = 0;
    strm.avail_out = 0;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // This function will setup all other fields in strm
    int err = deflateInit2 ( &strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                             15 + ZLIB_WINDOWBITS_USE_GZIP_ENCODING, // "windowBits" parameter (see deflateInit2 docs).
                             // 15 is the default value in deflateInit
                             // We also add ZLIB_WINDOWBITS_USE_GZIP_ENCODING to use gzip encoding.
                             8, // 8 is the default value in deflateInit
                             Z_DEFAULT_STRATEGY );

    if ( err != Z_OK )
    {
        if ( eCode != 0 )
        {
            *eCode = ( err == Z_MEM_ERROR ) ? ( Error::MemoryError ) : ( Error::InternalError );
        }

        return MemHandle();
    }

    MemHandle out ( deflateBound ( &strm, data.size() ) );

    if ( out.isEmpty() )
    {
        deflateEnd ( &strm );

        if ( eCode != 0 )
        {
            *eCode = Error::MemoryError;
        }

        return MemHandle();
    }

    strm.next_out = reinterpret_cast<unsigned char *> ( out.getWritable() );
    strm.avail_out = out.size();

    assert ( strm.next_out != 0 );
    assert ( strm.avail_out > 0 );

    err = deflate ( &strm, Z_FINISH );

    // According to the zlib docs, this really shouldn't happen, since we're compressing from a buffer, and
    // the output buffer size is allocated according to deflateBound, and Z_FINISH was used.
    if ( err != Z_STREAM_END )
    {
        deflateEnd ( &strm );

        if ( eCode != 0 )
        {
            *eCode = Error::InternalError;
        }

        assert ( false );

        return MemHandle();
    }

    out.truncate ( strm.total_out );

    err = deflateEnd ( &strm );

    // According to the zlib docs, this also shouldn't happen, for the same reasons as why deflate shouldn't fail.
    if ( err != Z_OK )
    {
        if ( eCode != 0 )
        {
            *eCode = Error::InternalError;
        }

        assert ( false );

        return MemHandle();
    }

    if ( eCode != 0 )
    {
        *eCode = Error::Success;
    }

    return out;
}
#endif
