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
#ifdef SYSTEM_WINDOWS
// openssl/rand.h pulls in windows.h, but we need WIN32_LEAN_AND_MEAN, so define it first then include rand.h
// portable_endian.h relies on this ordering for the winsock2.h header that it depends on
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <openssl/rand.h>
#include "portable_endian.h"
}

#include <cassert>
#include <cstdlib>

#include "basic/Random.hpp"

#include "WebSocketFrameHeader.hpp"

using namespace Pravala;

WebSocketFrameHeader::WebSocketFrameHeader()
{
    // It would be nice if this was a compile time check, but just enforce nobody screwed up the union,
    // the size of this object should be size of the largest possible frame header.
    assert ( sizeof ( *this ) == sizeof ( WebSocketHeader64Masked ) );
}

uint8_t WebSocketFrameHeader::setupWebSocketFrame ( OpCode opCode, bool finFlag, uint64_t payloadLen, bool setMask )
{
    // Clear the minimum header.
    // Everything else will be overwritten if we need it.
    memset ( this, 0, sizeof ( WebSocketHeader ) );

    setOpCode ( opCode );
    setFin ( finFlag );

    // Set the payload size

    if ( payloadLen < 126 )
    {
        _norm.maskLen1 = ( uint8_t ) payloadLen;
    }
    else if ( payloadLen > 0xFFFFU )
    {
        _norm.maskLen1 = ( uint8_t ) 127;
        _norm64.len64 = htobe64 ( payloadLen );
    }
    else
    {
        _norm.maskLen1 = ( uint8_t ) 126;
        _norm16.len16 = htobe16 ( ( uint16_t ) payloadLen );
    }

    if ( setMask )
    {
        uint32_t maskVal = 0;

        if ( RAND_bytes ( ( unsigned char * ) &maskVal, sizeof ( maskVal ) ) < 1 )
        {
            maskVal = Random::rand();
        }

        // We just set the length above, one of these values has to make sense
        switch ( _norm.maskLen1 )
        {
            case 127:
                _masked64.mask = maskVal;
                break;

            case 126:
                _masked16.mask = maskVal;
                break;

            default:
                _masked.mask = maskVal;
        }

        // Set the mask bit last
        _norm.maskLen1 |= FlagMaskLen1Mask;
    }

    return getHdrSize();
}

bool WebSocketFrameHeader::parseAndConsume ( MemHandle & buf, MemHandle & payload )
{
    if ( buf.size() < sizeof ( WebSocketHeader ) )
    {
        // Not enough data for the minimum header
        return false;
    }

    const WebSocketFrameHeader * hdr = reinterpret_cast<const WebSocketFrameHeader *> ( buf.get() );

    assert ( hdr != 0 );

    const uint8_t hdrSize = hdr->getHdrSize();

    if ( buf.size() < hdrSize )
    {
        // Not enough bytes for the full header
        return false;
    }

    const size_t payloadSize = hdr->getPayloadSize();
    const size_t frameSize = hdrSize + payloadSize;

    if ( buf.size() < frameSize )
    {
        // Not enough bytes for the full frame
        return false;
    }

    assert ( hdrSize <= sizeof ( *this ) );

    // Copy the header from the buffer into this object
    memcpy ( this, hdr, hdrSize );

    // Get the handle for the payload
    payload = buf.getHandle ( hdrSize, payloadSize );

    buf.consume ( frameSize );

    return true;
}

uint8_t WebSocketFrameHeader::getHdrSize() const
{
    switch ( getLen1() )
    {
        case 126:
            return hasMask() ? sizeof ( WebSocketHeader16Masked ) : sizeof ( WebSocketHeader16 );
            break;

        case 127:
            return hasMask() ? sizeof ( WebSocketHeader64Masked ) : sizeof ( WebSocketHeader64 );
            break;

        default:
            return hasMask() ? sizeof ( WebSocketHeaderMasked ) : sizeof ( WebSocketHeader );
    }
}

size_t WebSocketFrameHeader::getPayloadSize() const
{
    const uint8_t len1 = getLen1();

    switch ( len1 )
    {
        case 126:
            return be16toh ( _norm16.len16 );
            break;

        case 127:
            return ( size_t ) be64toh ( _norm64.len64 );
            break;

        default:
            return len1;
    }
}

uint32_t WebSocketFrameHeader::getMask() const
{
    if ( !hasMask() )
        return 0;

    switch ( getHdrSize() )
    {
        case sizeof ( WebSocketHeaderMasked ):
            return _masked.mask;

        case sizeof ( WebSocketHeader16Masked ):
            return _masked16.mask;

        case sizeof ( WebSocketHeader64Masked ):
            return _masked64.mask;

        default:
            return 0;
    }
}

void WebSocketFrameHeader::maskAndCopy ( char * dst, const char * src, size_t len )
{
    const uint32_t maskVal = getMask();

    // We can do the faster 4-byte XOR until there aren't 4 bytes left.
    //
    // Endianness doesn't matter, since each byte of the uint32_t mask value will be masked to the same
    // byte of the src, and put back into the same place, regardless of endianness.
    while ( len >= 4 )
    {
        *reinterpret_cast<uint32_t *> ( dst ) = *reinterpret_cast<const uint32_t *> ( src ) ^ maskVal;

        dst += 4;
        src += 4;
        len -= 4;
    }

    // If there are bytes left over, do a slower byte-by-byte XOR
    if ( len != 0 )
    {
        assert ( len < 4 );

        const uint8_t * mask8 = ( const uint8_t * ) &maskVal;

        for ( ; len != 0; ++src, ++dst, ++mask8, --len )
        {
            *dst = *src ^ *mask8;
        }
    }
}
