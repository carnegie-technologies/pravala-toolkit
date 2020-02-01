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

#include <gtest/gtest.h>

#include "basic/MemHandle.hpp"
#include "basic/List.hpp"
#include "basic/Random.hpp"
#include "basic/Math.hpp"
#include "net/IpChecksum.hpp"

using namespace Pravala;

class TestIpChecksum: public ::testing::TestWithParam<int>
{
    public:
        TestIpChecksum(): _data ( genData ( GetParam() ) ), _dataChecksum ( getBaseChecksum ( _data ) )
        {
        }

    protected:
        const MemHandle _data;        ///< Random data to use during the test.
        const uint16_t _dataChecksum; ///< The checksum of the data to compare against.

        /// @brief A helper function that changes data alignment.
        /// It returns a handle with the same data as passed, but the beginning of the data will
        /// 'bytesOff' bytes above regular memory alignment.
        /// @param [in] data The data to copy.
        /// @param [in] bytesOff The number of bytes to shift the data by.
        /// @return Copy of the data passed, but with modified alignment.
        static MemHandle aligned ( const MemHandle & data, uint8_t bytesOff )
        {
            MemHandle ret ( bytesOff + data.size() );

            memcpy ( ret.getWritable ( bytesOff ), data.get(), data.size() );

            return ret.getHandle ( bytesOff );
        }

        /// @brief Calculates a checksum using data split into smaller chunks.
        /// @param [in] data The data to use.
        /// @param [in] ranges A string with a list of ranges, comma and space separated.
        ///                    For a list "1, 2,3" first a single byte will be added to checksum.
        ///                    Then, 2 bytes will be added, followed by 3 bytes, followed by the rest of the data.
        /// @param [in] alignments Similar to 'ranges', but with the list of chunk alignments to use.
        ///                        May be empty or have a different length.
        /// @return The checksum.
        static uint16_t rangeChecksum (
            const MemHandle & data,
            const String & ranges, const String & alignments = String::EmptyString )
        {
            StringList list = ranges.split ( ",; \t-" );
            StringList aList = alignments.split ( ",; \t-" );

            IpChecksum cSum;

            MemHandle d ( data );

            for ( size_t i = 0; i < list.size(); ++i )
            {
                size_t len = 0;

                if ( list.at ( i ).toNumber ( len ) && len > 0 )
                {
                    uint8_t aOff = 0;

                    if ( !aList.isEmpty() && aList.first().toNumber ( aOff ) )
                    {
                        cSum.addMemory ( aligned ( d.getHandle ( 0, len ), aOff ) );
                    }
                    else
                    {
                        cSum.addMemory ( d.getHandle ( 0, len ) );
                    }

                    d.consume ( len );
                }
                else if ( !aList.isEmpty() )
                {
                    aList.removeFirst();
                }
            }

            cSum.addMemory ( d );

            return cSum.getChecksum();
        }

    private:
        /// @brief Generates random data.
        /// @param [in] size The size of data (in bytes) to generate.
        /// @return Generated data.
        static MemHandle genData ( int size )
        {
            MemHandle data ( size );

            size_t off = 0;

            while ( off < data.size() )
            {
                int v = Random::rand();

                const size_t s = min<size_t> ( sizeof ( v ), data.size() - off );

                memcpy ( data.getWritable ( off ), &v, s );
                off += s;
            }

            return data;
        }

        /// @brief Calculates the IP checksum using the safe, traditional method - using 2 bytes at a time.
        /// It also needs the memory to be aligned properly.
        /// @param [in] data The data to calculate the checksum for.
        /// @return Calculated IP checksum value.
        static uint16_t getBaseChecksum ( const MemHandle & data )
        {
            uint16_t sum = 0;

            for ( size_t i = 1; i < data.size(); i += 2 )
            {
                const uint16_t v = *( ( const uint16_t * ) data.get ( i - 1 ) );

                if ( ( sum += v ) < v )
                {
                    ++sum;
                }
            }

            if ( ( data.size() % 2 ) > 0 )
            {
                union
                {
                    char c[ 2 ];
                    uint16_t s;
                } u;

                u.c[ 0 ] = data.get()[ data.size() - 1 ];
                u.c[ 1 ] = 0;

                if ( ( sum += u.s ) < u.s )
                {
                    ++sum;
                }
            }

            return ~sum;
        }
};

TEST_P ( TestIpChecksum, Basic )
{
    for ( uint8_t alignment = 0; alignment < 4; ++alignment )
    {
        const MemHandle data = aligned ( _data, alignment );

        IpChecksum cSum;

        cSum.addMemory ( data );

        EXPECT_EQ ( _dataChecksum, cSum.getChecksum() );

        cSum.clear();

        for ( size_t i = 0; i < data.size(); ++i )
        {
            cSum.addMemory ( data.get()[ i ] );
        }

        EXPECT_EQ ( _dataChecksum, cSum.getChecksum() );

        cSum.clear();

        for ( size_t i = 0; i < data.size(); ++i )
        {
            cSum.addMemory ( data.get ( i ), 1 );
        }

        EXPECT_EQ ( _dataChecksum, cSum.getChecksum() );
    }
}

TEST_P ( TestIpChecksum, TwoChunks )
{
    for ( uint8_t alignment = 0; alignment < 4; ++alignment )
    {
        const MemHandle data = aligned ( _data, alignment );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "17" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "18" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "19" ) );
    }
}

TEST_P ( TestIpChecksum, TwoRealignedChunks )
{
    for ( uint8_t alignment = 0; alignment < 4; ++alignment )
    {
        const MemHandle data = aligned ( _data, alignment );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15", "3" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16", "0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16", "1" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16", "2" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16", "3" ) );
    }
}

TEST_P ( TestIpChecksum, MultipleChunks )
{
    for ( uint8_t alignment = 0; alignment < 4; ++alignment )
    {
        const MemHandle data = aligned ( _data, alignment );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6,7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7,8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8,9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9,10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10,11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11,12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12,13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13,14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14,15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15,16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16,17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "17,18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "18,19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "19,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "20" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2,4,6,8,10,12,14,16,18,20,22,24" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1,2,4,200,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2,4,200,20" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "200,20,1" ) );
    }
}

TEST_P ( TestIpChecksum, MultipleRealignedChunks )
{
    for ( uint8_t alignment = 0; alignment < 4; ++alignment )
    {
        const MemHandle data = aligned ( _data, alignment );

        const char * al = "0,0,0,0,1,3,0,2,1,0,2,0,0,1,1,2,2,3,1,3";

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "7,8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "8,9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "9,10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "10,11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "11,12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "12,13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "13,14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "14,15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "15,16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "16,17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "17,18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "18,19,20", al ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "19,20", al ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2,4,6,8,10,12,14,16,18,20,22,24", al ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1,2,4,200,20", "0,0,0,0,0" ) );
        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "1,2,4,200,20", "1,0,1,3,0" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "2,4,200,20", "1,1,1,1" ) );

        EXPECT_EQ ( _dataChecksum, rangeChecksum ( data, "200,20,1", "2,1,0" ) );
    }
}

// Parameter for the test is the size of the test memory (255-280).
// We want to use differently sized memory ranges.
INSTANTIATE_TEST_CASE_P ( TestSizes, TestIpChecksum, ::testing::Range ( 255, 280 ) );
