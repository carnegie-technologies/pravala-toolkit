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

#include <cstring>
#include <cassert>

#include "Math.hpp"
#include "PacketLossCounter.hpp"

// We need minimum to have an array.
#define MIN_BUF_SIZE    ( ( uint16_t ) 4 )

using namespace Pravala;

PacketLossCounter::PacketLossCounter ( uint16_t bufSize ): _bufSize ( max ( bufSize, MIN_BUF_SIZE ) )
{
    assert ( _bufSize >= MIN_BUF_SIZE );

    _buf = new uint8_t[ _bufSize ];

    clear();
}

PacketLossCounter::~PacketLossCounter()
{
    delete[] _buf;
    _buf = 0;
}

void PacketLossCounter::clear()
{
    _curBufPos = 0;
    _totalLossCount = 0;

    if ( _buf != 0 )
    {
        memset ( _buf, 0, sizeof ( uint8_t ) * _bufSize );
    }
}

uint8_t PacketLossCounter::getLossPercentage() const
{
    return ( 100 * _totalLossCount ) / ( _bufSize + _totalLossCount );
}

void PacketLossCounter::intAddLoss ( uint8_t packetLoss )
{
    // Let's increase the current position.
    // If _curBufPos wraps around, it will become 0, and % will still give us 0!
    _curBufPos = ( _curBufPos + 1 ) % _bufSize;

    // Now _curBufPos points to the oldest loss number.
    // We will overwrite it with the one that we just received.

    assert ( _buf[ _curBufPos ] <= _totalLossCount );

    _totalLossCount -= _buf[ _curBufPos ];

    _buf[ _curBufPos ] = packetLoss;
    _totalLossCount += packetLoss;
}

void PacketLossCounter::addLoss ( uint32_t packetLoss )
{
    if ( packetLoss <= 0xFF )
    {
        intAddLoss ( packetLoss & 0xFF );
        return;
    }

    // It is bigger than the number of losses we can store in a single buffer entry.
    // This means we will have to modify multiple entries.

    uint16_t count = 0;

    while ( packetLoss > 0xFF )
    {
        packetLoss -= 0xFF;

        intAddLoss ( 0xFF );

        if ( ++count >= _bufSize )
            return;
    }

    intAddLoss ( packetLoss & 0xFF );
}
