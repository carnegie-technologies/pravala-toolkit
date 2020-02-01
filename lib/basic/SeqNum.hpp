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

#pragma once

extern "C"
{
#include <stdint.h>
#include <sys/types.h>
}

#include <cassert>

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
}
#else
extern "C"
{
#include <arpa/inet.h>
}
#endif

namespace Pravala
{
/// @brief Compares two sequence numbers
/// @param [in] seqNumA First sequence number for comparison
/// @param [in] seqNumB Second sequence number for comparison
/// @return The difference between two sequence numbers.
///         Positive value means that seqNumA > seqNumB, negative value means that seqNumA < seqNumB.
inline int32_t compareSeqNums ( uint16_t seqNumA, uint16_t seqNumB )
{
    if ( seqNumA == seqNumB )
    {
        return 0;
    }
    else if ( seqNumA > seqNumB )
    {
        // Difference if we treat 'A' as the greater one (regular difference):
        int32_t diffA = seqNumA - seqNumB;

        // Difference if we treat 'B' as the greater one
        // It is equal to the difference between A and max plus B.
        // If A = MAX, then the first part is 0. If 'B' is 1 (it's the smallest legal seq number)
        // Then the difference is 0 + 1 = 1. All is correct, because we would skip 0 while incrementing the seq number.
        int32_t diffB = 0xFFFF - seqNumA + seqNumB;

        if ( diffA <= diffB )
            return diffA;

        return -diffB;
    }
    else
    {
        assert ( seqNumA < seqNumB );

        // Difference if we treat 'B' as the greater one (regular difference):
        int32_t diffA = seqNumB - seqNumA;

        // Difference if we treat 'A' as the greater one
        int32_t diffB = 0xFFFF - seqNumB + seqNumA;

        if ( diffA <= diffB )
            return -diffA;

        return diffB;
    }
}

/// @brief Increments the sequence number properly
/// @param [in,out] seqNum Reference (which is modified!) of the sequence number to increment
inline void incrementSeqNum ( uint16_t & seqNum )
{
    if ( seqNum >= 0xFFFF )
    {
        /// @note This is based on assumption, that seq number = 0 is invalid (or special)
        seqNum = 1;
    }
    else
    {
        ++seqNum;
    }

    assert ( seqNum > 0 );
}

/// @brief Increments the sequence number properly and returns the incremented value
/// @param [in] seqNum The sequence number to increment
/// @return The incremented value of the sequence number
inline uint16_t incrementedSeqNum ( uint16_t seqNum )
{
    incrementSeqNum ( seqNum );

    return seqNum;
}

/// @brief Decrements the sequence number properly.
/// @param [in,out] seqNum Reference (which is modified!) of the sequence number to decrement
inline void decrementSeqNum ( uint16_t & seqNum )
{
    if ( seqNum <= 1 )
    {
        /// @note This is based on assumption, that seq number = 0 is invalid (or special)
        seqNum = 0xFFFF;
    }
    else
    {
        --seqNum;
    }
}

/// @brief Converts a seqNum from network to host byte order
/// @param [in] seqNum Sequence number to convert from network to host endianness
/// @return Converted sequence number
inline uint16_t ntohSeqNum ( uint16_t seqNum )
{
    return ntohs ( seqNum );
}

/// @brief Converts a seqNum from host to network byte order
/// @param [in] seqNum Sequence number to convert from host to network endianness
/// @return Converted sequence number
inline uint16_t htonSeqNum ( uint16_t seqNum )
{
    return htons ( seqNum );
}
}
