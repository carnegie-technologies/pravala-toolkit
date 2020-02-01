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

/*
 * This implementation is based on code in public domain found at:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * The original code included the following notice:
 *
 *
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * (This is a heavily cut-down "BSD license".)
 *
 * This differs from Colin Plumb's older public domain implementation in that
 * no exactly 32-bit integer data type is required (any 32-bit or wider
 * unsigned integer data type will do), there's no compile-time endianness
 * configuration, and the function prototypes match OpenSSL's.  No code from
 * Colin Plumb's implementation has been reused; this comment merely compares
 * the properties of the two independent implementations.
 *
 * The primary goals of this implementation are portability and ease of use.
 * It is meant to be fast, but not as fast as possible.  Some known
 * optimizations are not included to reduce source code size and avoid
 * compile-time configuration.
 */

#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"

#include "Md5.hpp"

using namespace Pravala;

/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F( x, y, z )    ( ( z ) ^ ( ( x ) & ( ( y ) ^ ( z ) ) ) )
#define G( x, y, z )    ( ( y ) ^ ( ( z ) & ( ( x ) ^ ( y ) ) ) )
#define H( x, y, z )    ( ( x ) ^ ( y ) ^ ( z ) )
#define I( x, y, z )    ( ( y ) ^ ( ( x ) | ~( z ) ) )

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP( f, a, b, c, d, x, t, s ) \
    ( a ) += f ( ( b ), ( c ), ( d ) ) + ( x ) + ( t ); \
    ( a ) = ( ( ( a ) << ( s ) ) | ( ( ( a ) & 0xffffffff ) >> ( 32 - ( s ) ) ) ); \
    ( a ) += ( b );

/*
 * SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined( __i386__ ) || defined( __x86_64__ ) || defined( __vax__ )
#define SET( n ) \
    ( *( const uint32_t * ) &ptr[ ( n ) * 4 ] )
#define GET( n ) \
    SET ( n )
#else
#define SET( n ) \
    ( _ctx.block[ ( n ) ]   \
          = ( uint32_t ) ptr[ ( n ) * 4 ]   \
            | ( ( uint32_t ) ptr[ ( n ) * 4 + 1 ] << 8 )   \
            | ( ( uint32_t ) ptr[ ( n ) * 4 + 2 ] << 16 )   \
            | ( ( uint32_t ) ptr[ ( n ) * 4 + 3 ] << 24 ) )
#define GET( n ) \
    ( _ctx.block[ ( n ) ] )
#endif

Md5::Context::Context()
{
    reset();
}

void Md5::Context::reset()
{
    memset ( this, 0, sizeof ( Context ) );

    a = 0x67452301;
    b = 0xefcdab89;
    c = 0x98badcfe;
    d = 0x10325476;

    lo = 0;
    hi = 0;
}

Md5::Md5(): _isFinalized ( false )
{
}

void Md5::reset()
{
    _isFinalized = false;
    _ctx.reset();
}

void Md5::getResult ( unsigned char result[ 16 ] )
{
    if ( !_isFinalized )
        finalize();

    result[ 0 ] = _ctx.a;
    result[ 1 ] = _ctx.a >> 8;
    result[ 2 ] = _ctx.a >> 16;
    result[ 3 ] = _ctx.a >> 24;
    result[ 4 ] = _ctx.b;
    result[ 5 ] = _ctx.b >> 8;
    result[ 6 ] = _ctx.b >> 16;
    result[ 7 ] = _ctx.b >> 24;
    result[ 8 ] = _ctx.c;
    result[ 9 ] = _ctx.c >> 8;
    result[ 10 ] = _ctx.c >> 16;
    result[ 11 ] = _ctx.c >> 24;
    result[ 12 ] = _ctx.d;
    result[ 13 ] = _ctx.d >> 8;
    result[ 14 ] = _ctx.d >> 16;
    result[ 15 ] = _ctx.d >> 24;
}

String Md5::getResult ( bool lowerCase )
{
    unsigned char result[ 16 ];

    getResult ( result );

    return String::hexDump ( ( const char * ) result, 16, false, "", lowerCase );
}

String Md5::generate ( const char * data, size_t size, bool lowerCase )
{
    Md5 md5;

    if ( !md5.update ( data, size ) )
        return String::EmptyString;

    return md5.getResult ( lowerCase );
}

bool Md5::update ( const char * str )
{
    if ( _isFinalized )
        return false;

    return update ( str, strlen ( str ) );
}

bool Md5::update ( const String & str )
{
    if ( _isFinalized )
        return false;

    return update ( str.c_str(), str.length() );
}

bool Md5::update ( const Buffer & buf )
{
    if ( _isFinalized )
        return false;

    return update ( buf.get(), buf.size() );
}

bool Md5::update ( const MemHandle & mem )
{
    if ( _isFinalized )
        return false;

    return update ( mem.get(), mem.size() );
}

bool Md5::update ( const char * data, size_t size )
{
    if ( _isFinalized )
        return false;

    unsigned long used, free;

    uint32_t saved_lo = _ctx.lo;

    if ( ( _ctx.lo = ( saved_lo + size ) & 0x1fffffff ) < saved_lo )
        _ctx.hi++;

    _ctx.hi += size >> 29;

    used = saved_lo & 0x3f;

    if ( used )
    {
        free = 64 - used;

        if ( size < free )
        {
            memcpy ( &_ctx.buffer[ used ], data, size );
            return true;
        }

        memcpy ( &_ctx.buffer[ used ], data, free );

        data = ( const char * ) ( ( const unsigned char * ) data + free );
        size -= free;
        body ( _ctx.buffer, 64 );
    }

    if ( size >= 64 )
    {
        data = ( const char * ) body ( ( const unsigned char * ) data, size & ~( unsigned long ) 0x3f );
        size &= 0x3f;
    }

    memcpy ( _ctx.buffer, data, size );

    return true;
}

void Md5::finalize()
{
    if ( _isFinalized )
        return;

    // 0x3F is 63

    uint32_t memUsed = _ctx.lo & 0x3F;

    _ctx.buffer[ memUsed++ ] = 0x80;

    assert ( memUsed <= 64 );

    uint32_t memFree = 64 - memUsed;

    if ( memFree < 8 )
    {
        memset ( &_ctx.buffer[ memUsed ], 0, memFree );
        body ( _ctx.buffer, 64 );
        memUsed = 0;
        memFree = 64;
    }

    memset ( &_ctx.buffer[ memUsed ], 0, memFree - 8 );

    _ctx.lo <<= 3;
    _ctx.buffer[ 56 ] = _ctx.lo;
    _ctx.buffer[ 57 ] = _ctx.lo >> 8;
    _ctx.buffer[ 58 ] = _ctx.lo >> 16;
    _ctx.buffer[ 59 ] = _ctx.lo >> 24;
    _ctx.buffer[ 60 ] = _ctx.hi;
    _ctx.buffer[ 61 ] = _ctx.hi >> 8;
    _ctx.buffer[ 62 ] = _ctx.hi >> 16;
    _ctx.buffer[ 63 ] = _ctx.hi >> 24;

    body ( _ctx.buffer, 64 );

    _isFinalized = true;
}

const unsigned char * Md5::body ( const unsigned char * data, size_t size )
{
    assert ( ( size & ~( unsigned long ) 0x3f ) == size );

    uint32_t a, b, c, d;
    uint32_t saved_a, saved_b, saved_c, saved_d;

    const unsigned char * ptr = data;

    a = _ctx.a;
    b = _ctx.b;
    c = _ctx.c;
    d = _ctx.d;

    do
    {
        saved_a = a;
        saved_b = b;
        saved_c = c;
        saved_d = d;

        /* Round 1 */
        STEP ( F, a, b, c, d, SET ( 0 ), 0xd76aa478, 7 )
        STEP ( F, d, a, b, c, SET ( 1 ), 0xe8c7b756, 12 )
        STEP ( F, c, d, a, b, SET ( 2 ), 0x242070db, 17 )
        STEP ( F, b, c, d, a, SET ( 3 ), 0xc1bdceee, 22 )
        STEP ( F, a, b, c, d, SET ( 4 ), 0xf57c0faf, 7 )
        STEP ( F, d, a, b, c, SET ( 5 ), 0x4787c62a, 12 )
        STEP ( F, c, d, a, b, SET ( 6 ), 0xa8304613, 17 )
        STEP ( F, b, c, d, a, SET ( 7 ), 0xfd469501, 22 )
        STEP ( F, a, b, c, d, SET ( 8 ), 0x698098d8, 7 )
        STEP ( F, d, a, b, c, SET ( 9 ), 0x8b44f7af, 12 )
        STEP ( F, c, d, a, b, SET ( 10 ), 0xffff5bb1, 17 )
        STEP ( F, b, c, d, a, SET ( 11 ), 0x895cd7be, 22 )
        STEP ( F, a, b, c, d, SET ( 12 ), 0x6b901122, 7 )
        STEP ( F, d, a, b, c, SET ( 13 ), 0xfd987193, 12 )
        STEP ( F, c, d, a, b, SET ( 14 ), 0xa679438e, 17 )
        STEP ( F, b, c, d, a, SET ( 15 ), 0x49b40821, 22 )

        /* Round 2 */
        STEP ( G, a, b, c, d, GET ( 1 ), 0xf61e2562, 5 )
        STEP ( G, d, a, b, c, GET ( 6 ), 0xc040b340, 9 )
        STEP ( G, c, d, a, b, GET ( 11 ), 0x265e5a51, 14 )
        STEP ( G, b, c, d, a, GET ( 0 ), 0xe9b6c7aa, 20 )
        STEP ( G, a, b, c, d, GET ( 5 ), 0xd62f105d, 5 )
        STEP ( G, d, a, b, c, GET ( 10 ), 0x02441453, 9 )
        STEP ( G, c, d, a, b, GET ( 15 ), 0xd8a1e681, 14 )
        STEP ( G, b, c, d, a, GET ( 4 ), 0xe7d3fbc8, 20 )
        STEP ( G, a, b, c, d, GET ( 9 ), 0x21e1cde6, 5 )
        STEP ( G, d, a, b, c, GET ( 14 ), 0xc33707d6, 9 )
        STEP ( G, c, d, a, b, GET ( 3 ), 0xf4d50d87, 14 )
        STEP ( G, b, c, d, a, GET ( 8 ), 0x455a14ed, 20 )
        STEP ( G, a, b, c, d, GET ( 13 ), 0xa9e3e905, 5 )
        STEP ( G, d, a, b, c, GET ( 2 ), 0xfcefa3f8, 9 )
        STEP ( G, c, d, a, b, GET ( 7 ), 0x676f02d9, 14 )
        STEP ( G, b, c, d, a, GET ( 12 ), 0x8d2a4c8a, 20 )

        /* Round 3 */
        STEP ( H, a, b, c, d, GET ( 5 ), 0xfffa3942, 4 )
        STEP ( H, d, a, b, c, GET ( 8 ), 0x8771f681, 11 )
        STEP ( H, c, d, a, b, GET ( 11 ), 0x6d9d6122, 16 )
        STEP ( H, b, c, d, a, GET ( 14 ), 0xfde5380c, 23 )
        STEP ( H, a, b, c, d, GET ( 1 ), 0xa4beea44, 4 )
        STEP ( H, d, a, b, c, GET ( 4 ), 0x4bdecfa9, 11 )
        STEP ( H, c, d, a, b, GET ( 7 ), 0xf6bb4b60, 16 )
        STEP ( H, b, c, d, a, GET ( 10 ), 0xbebfbc70, 23 )
        STEP ( H, a, b, c, d, GET ( 13 ), 0x289b7ec6, 4 )
        STEP ( H, d, a, b, c, GET ( 0 ), 0xeaa127fa, 11 )
        STEP ( H, c, d, a, b, GET ( 3 ), 0xd4ef3085, 16 )
        STEP ( H, b, c, d, a, GET ( 6 ), 0x04881d05, 23 )
        STEP ( H, a, b, c, d, GET ( 9 ), 0xd9d4d039, 4 )
        STEP ( H, d, a, b, c, GET ( 12 ), 0xe6db99e5, 11 )
        STEP ( H, c, d, a, b, GET ( 15 ), 0x1fa27cf8, 16 )
        STEP ( H, b, c, d, a, GET ( 2 ), 0xc4ac5665, 23 )

        /* Round 4 */
        STEP ( I, a, b, c, d, GET ( 0 ), 0xf4292244, 6 )
        STEP ( I, d, a, b, c, GET ( 7 ), 0x432aff97, 10 )
        STEP ( I, c, d, a, b, GET ( 14 ), 0xab9423a7, 15 )
        STEP ( I, b, c, d, a, GET ( 5 ), 0xfc93a039, 21 )
        STEP ( I, a, b, c, d, GET ( 12 ), 0x655b59c3, 6 )
        STEP ( I, d, a, b, c, GET ( 3 ), 0x8f0ccc92, 10 )
        STEP ( I, c, d, a, b, GET ( 10 ), 0xffeff47d, 15 )
        STEP ( I, b, c, d, a, GET ( 1 ), 0x85845dd1, 21 )
        STEP ( I, a, b, c, d, GET ( 8 ), 0x6fa87e4f, 6 )
        STEP ( I, d, a, b, c, GET ( 15 ), 0xfe2ce6e0, 10 )
        STEP ( I, c, d, a, b, GET ( 6 ), 0xa3014314, 15 )
        STEP ( I, b, c, d, a, GET ( 13 ), 0x4e0811a1, 21 )
        STEP ( I, a, b, c, d, GET ( 4 ), 0xf7537e82, 6 )
        STEP ( I, d, a, b, c, GET ( 11 ), 0xbd3af235, 10 )
        STEP ( I, c, d, a, b, GET ( 2 ), 0x2ad7d2bb, 15 )
        STEP ( I, b, c, d, a, GET ( 9 ), 0xeb86d391, 21 )

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        ptr += 64;

        assert ( size >= 64 );
    }
    while ( size -= 64 );

    _ctx.a = a;
    _ctx.b = b;
    _ctx.c = c;
    _ctx.d = d;

    return ptr;
}
