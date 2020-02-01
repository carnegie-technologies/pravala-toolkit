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

#include <cassert>

#include "basic/MemVector.hpp"

namespace Pravala
{
/// @brief A helper class for calculating the IP checksums (RFC 791 and others)
/// The memory passed to this function does not need to be aligned properly, or be in chunks of specific sizes.
/// It calculates the checksum properly whether the memory is passed using a single call, or in several calls,
/// one chunk at a time. The IP checksum is stored in 2 bytes.
/// It is "16-bit one's complement of the one's complement sum of all 16-bit words".
/// To speed things up, instead of dealing with 2B values, this algorithm uses 64-bit words to compute a 64-bit sum,
/// which is later reduced to 16-bit value. However, to perform 64-bit operations the memory has to be aligned properly.
/// Also, if the individual chunks are not sized properly, the later segments, even if they start at aligned
/// addresses, may follow a segment that ended not at the end of a 64-bit word.
/// The idea behind this algorithm is based on the observation, that the 16-bit sum can be calculated by adding up
/// all bytes in the memory by storing them in two columns - one for all even bytes, and one for all odd bytes.
/// "even" and "odd" are about the byte position in the memory, so bytes at 0,2,4,6, etc. offset are "even",
/// and bytes at offset 1,3,5, etc. are "odd". So in the first column we would have bytes from offsets 0,2,4, etc.
/// The second column would include bytes at offsets 1,3,5, etc.
/// Whenever addition of one of them causes data overflow, the opposite column is incremented by one.
/// To be able to update checksum with chunks of different sizes, it supports updating it with only a single byte.
/// That operation updates the checksum, and flips a flag that determines how the next byte should be treated
/// (either as an "even" or "odd" byte). When adding another chunk to the calculation, first all bytes
/// that are not aligned properly are appended - one at a time. Once we get to the position which is aligned properly,
/// we calculate the checksum for the remaining data - 8 bytes at a time - to obtain a 64-bit sum of that value.
/// Once 8 byte operations are no longer possible, we update it with 4B and 2B words at the end (if any),
/// ignoring the last byte (if present) for now.
/// Then this sum is reduced to 16-bit value, by adding halves of it to each other (twice), while following
/// the overflow rules. That 64-bit sum consists of 8 bytes, each one contains the sum of all bytes of the original
/// memory if we divided it into 8 columns. That reduction means we are just adding up values in bytes 0,2,4,6
/// to each other, and we do the same for bytes 1,3,5,7. We just do that in two steps, first going from 8 to 4 columns,
/// and then from 4 to 2 - which gives us a 16-bit sum.
/// Finally, that sum is added to the current (running) sum. If the even/odd flag is set to "even", it means that
/// the first byte of the reduced sum belongs to the first column (and second to the second one). In that case
/// We can just add the new sum to the previously stored one.
/// If the flag is set to "odd", it means that all the data calculated in the previous step was off by one byte,
/// and our columns should be swapped before being added to the existing sum.
/// Finally, the remaining 1 byte (if present) is appended to the checksum using the basic 'one byte append' operation
/// (which flips the flag again).
/// Note that for this to be possible, the checksum is stored in not-negated version, as a basic sum.
/// The 'getChecksum()' method returns negated value (without modifying the sum itself).
/// This algorithm is able to deal with unaligned memory of "weird" sizes properly, while still performing
/// fast 64-bit operations on most of the data in larger chunks.
class IpChecksum
{
    public:
        /// @brief Default constructor.
        inline IpChecksum(): _nextIdx ( 0 )
        {
            _sum.u16 = 0;
        }

        /// @brief Clears the checksum.
        inline void clear()
        {
            _sum.u16 = 0;
            _nextIdx = 0;
        }

        /// @brief Returns the IP checksum of the data passed so far.
        /// It does not modify the internal values, so the current checksum can be modified by passing more data to it.
        /// @return IP checksum of the data passed so far (negated format that can be stored in the IP packet).
        inline uint16_t getChecksum() const
        {
            return ~_sum.u16;
        }

        /// @brief Appends another byte to the data being checksummed.
        /// @param [in] c The next byte of memory to checksum.
        inline void addMemory ( char c )
        {
            ChecksumValue v;

            v.s8[ _nextIdx ] = c;
            v.s8[ ( _nextIdx = 1 - _nextIdx ) ] = 0;

            ( ( _sum.u16 += v.u16 ) < v.u16 ) && ++_sum.u16;
        }

        /// @brief A convenience wrapper for passing arbitrary pointer types.
        /// @param [in] data The data to be checksummed. Does NOT need to be aligned in any specific way.
        /// @param [in] size The size of the data passed.
        inline void addMemory ( const void * data, size_t size )
        {
            addMemory ( ( const char * ) data, size );
        }

        /// @brief A convenience wrapper for passing MemHandle object.
        /// @param [in] data The data to be checksummed. Does NOT need to be aligned in any specific way.
        inline void addMemory ( const MemHandle & data )
        {
            if ( !data.isEmpty() )
            {
                addMemory ( data.get(), data.size() );
            }
        }

        /// @brief A convenience wrapper for passing MemVector object.
        /// @param [in] data The data to be checksummed. Does NOT need to be aligned in any specific way.
        inline void addMemory ( const MemVector & data )
        {
            const struct iovec * chunks = data.getChunks();
            const size_t numChunks = data.getNumChunks();

            for ( size_t i = 0; i < numChunks; ++i )
            {
                addMemory ( ( const char * ) chunks[ i ].iov_base, chunks[ i ].iov_len );
            }
        }

        /// @brief Appends more data to be checksummed.
        /// @param [in] data The data to be checksummed. Does NOT need to be aligned in any specific way.
        /// @param [in] size The size of the data passed.
        inline void addMemory ( const char * data, size_t size )
        {
            if ( !data || size < 1 )
            {
                return;
            }

            // If the memory is not aligned, we want to add the bytes at the beginning one by one,
            // until 'data' pointer is aligned properly. The number of bytes that need to be treated this
            // way is between 0 (if the data is aligned properly), and 3 (if data % 4 == 1):
            for ( size_t i = 4U - ( ( ( size_t ) data ) % 4U ); i > 0; --i )
            {
                addMemory ( *( data++ ) );

                // We are done if we ever run out of data:
                if ( --size < 1 )
                    return;
            }

            // Now the memory should be aligned properly:
            assert ( ( ( size_t ) data ) % 4U == 0 );
            assert ( size > 0 );

            uint64_t sum64 = 0;

            // Let's add the data, 8 bytes at a time.
            while ( size >= 8 )
            {
                assert ( ( ( size_t ) data ) % 4 == 0 );

                const uint64_t v = *( ( const uint64_t * ) data );

                ( ( sum64 += v ) < v ) && ++sum64;

                data += 8;
                size -= 8;
            }

            assert ( size <= 7 );

            // Now we may have between 0 and 7 bytes left.
            // We don't modify the size anymore, just check individual bits.
            // We still need to modify the data pointer.

            if ( size & 4 )
            {
                // 4-7 bytes left; Let's add 4:

                const uint32_t v = *( ( const uint32_t * ) data );

                ( ( sum64 += v ) < v ) && ++sum64;

                data += 4;
            }

            if ( size & 2 )
            {
                // 2-3 bytes left; Let's add 2:

                const uint16_t v = *( ( const uint16_t * ) data );

                ( ( sum64 += v ) < v ) && ++sum64;

                data += 2;
            }

            // NOTE: There could still be a single byte to deal with here!
            //       But if there is, it means it's an odd byte, should be padded with 0, and modify _nextIdx.
            //       Because of that we simply add that single byte at the end (after changing the checksum),
            //       using addMemory ( char ) version.

            // Now we have a 64-bit checksum.
            // First, we need to reduce it to 16 bits, by adding halves of it to each other (twice):

            ChecksumValue sum16;

            {
                uint32_t sum32 = ( uint32_t ) ( sum64 & 0xFFFFFFFFUL );
                const uint32_t tmp32 = ( uint32_t ) ( sum64 >> 32 );

                ( ( sum32 += tmp32 ) < tmp32 ) && ++sum32;

                sum16.u16 = ( uint16_t ) ( sum32 & 0xFFFFU );
                const uint16_t tmp16 = ( uint16_t ) ( sum32 >> 16 );

                ( ( sum16.u16 += tmp16 ) < tmp16 ) && ++sum16.u16;
            }

            if ( _nextIdx != 0 )
            {
                // We are misaligned, which means that the last modification of the checksum
                // did not end at a 2 byte boundary. We have to switch the bytes:

                ChecksumValue v;

                v.s8[ 0 ] = sum16.s8[ 1 ];
                v.s8[ 1 ] = sum16.s8[ 0 ];

                ( ( _sum.u16 += v.u16 ) < v.u16 ) && ++_sum.u16;
            }
            else if ( ( _sum.u16 += sum16.u16 ) < sum16.u16 )
            {
                ++_sum.u16;
            }

            if ( size & 1 )
            {
                // We have one additional byte to add!
                // Because it's an odd byte it will affect _nextIdx.
                // Let's just use the regular addMemory(char):
                addMemory ( *data );
            }
        }

        /// @brief Convenience wrapper around IpChecksum object.
        /// It calculates a checksum for provided memory segment.
        /// @param [in] mem The pointer to the memory.
        /// @param [in] size The size of the memory passed.
        /// @return IP checksum of the data passed.
        static inline uint16_t getChecksum ( const char * mem, size_t size )
        {
            IpChecksum cSum;
            cSum.addMemory ( mem, size );
            return cSum.getChecksum();
        }

    private:
        /// @brief Storage type for the checksum.
        /// It is stored in this way to simplify the code.
        union ChecksumValue
        {
            char s8[ 2 ];   ///< The checksum as two characters.
            uint16_t u16; ///< The checksum as a single uint16 value.
        };

        ChecksumValue _sum; ///< The checksum so far (it is NOT negated).

        /// @brief The index in _sum.s8[] where the next byte of memory should go (either 1 or 0).
        /// When data is added 2 bytes at a time, this index does not change.
        /// But adding a single byte (or odd number of bytes) flips this index (between 0 and 1),
        /// because following bytes should start at a different index than they would normally do.
        uint8_t _nextIdx;
};
}
