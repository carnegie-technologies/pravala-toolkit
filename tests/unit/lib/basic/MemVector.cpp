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

#include "basic/Math.hpp"
#include "basic/MemHandle.hpp"
#include "basic/MemVector.hpp"
#include "basic/Random.hpp"
#include "basic/String.hpp"

using namespace Pravala;

class TestMemVector: public ::testing::Test
{
    public:
        TestMemVector():
            orgData ( genData ( 1024 ) ),
            orgStr ( orgData.get(), orgData.size() ),
            totalSize ( 0 ),
            numChunks ( 0 )
        {
        }

    protected:
        const MemHandle orgData;
        const String orgStr;

        MemVector vec;
        MemHandle data;
        size_t totalSize;
        size_t numChunks;

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
};

TEST_F ( TestMemVector, Basic )
{
    EXPECT_EQ ( orgData.size(), orgStr.length() );
    EXPECT_TRUE ( orgStr == orgData.toString() );

    vec = orgData;

    EXPECT_EQ ( 1, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    EXPECT_TRUE ( vec.storeContinuous ( data ) );
    EXPECT_EQ ( orgData.size(), data.size() );
    EXPECT_TRUE ( orgStr == data.toString() );

    vec.clear();

    EXPECT_TRUE ( vec.isEmpty() );

    numChunks = totalSize = 0;

    for ( size_t i = 0; i < orgData.size(); i += 50 )
    {
        MemHandle mh = orgData.getHandle ( i, 50 );

        vec.append ( orgData.getHandle ( i, 50 ) );

        totalSize += mh.size();
        ++numChunks;

        EXPECT_EQ ( totalSize, vec.getDataSize() );
        EXPECT_EQ ( numChunks, vec.getNumChunks() );
    }

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );
    EXPECT_TRUE ( vec.storeContinuous ( data ) );
    EXPECT_EQ ( orgData.size(), data.size() );
    EXPECT_TRUE ( orgStr == data.toString() );

    vec.clear();
    numChunks = totalSize = 0;

    for ( size_t i = orgData.size() / 2, j = orgData.size() / 2; i < orgData.size(); i += 50 )
    {
        MemHandle mh = orgData.getHandle ( i, 50 );

        vec.append ( mh );
        totalSize += mh.size();
        ++numChunks;

        EXPECT_EQ ( totalSize, vec.getDataSize() );
        EXPECT_EQ ( numChunks, vec.getNumChunks() );

        if ( j > 0 )
        {
            if ( j >= 50 )
            {
                j -= 50;
                mh = orgData.getHandle ( j, 50 );
            }
            else
            {
                mh = orgData.getHandle ( 0, j );
                j = 0;
            }

            vec.prepend ( mh );
            totalSize += mh.size();
            ++numChunks;

            EXPECT_EQ ( totalSize, vec.getDataSize() );
            EXPECT_EQ ( numChunks, vec.getNumChunks() );
        }
    }

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_TRUE ( vec.storeContinuous ( data ) );
    EXPECT_EQ ( orgData.size(), data.size() );
    EXPECT_TRUE ( orgStr == data.toString() );
}

TEST_F ( TestMemVector, WritableBasic )
{
    EXPECT_EQ ( orgData.size(), orgStr.length() );
    EXPECT_TRUE ( orgStr == orgData.toString() );

    numChunks = totalSize = 0;
    vec.clear();

    for ( size_t i = 0; i < orgData.size(); i += 50 )
    {
        MemHandle mh = orgData.getHandle ( i, 50 );

        vec.append ( orgData.getHandle ( i, 50 ) );

        totalSize += mh.size();
        ++numChunks;

        EXPECT_EQ ( totalSize, vec.getDataSize() );
        EXPECT_EQ ( numChunks, vec.getNumChunks() );
    }

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );
    EXPECT_TRUE ( vec.storeContinuous ( data ) );
    EXPECT_EQ ( orgData.size(), data.size() );
    EXPECT_TRUE ( orgStr == data.toString() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // And this should combine the whole thing:
    EXPECT_TRUE ( vec.getContinuousWritable ( vec.getDataSize() ) != 0 );

    EXPECT_EQ ( orgData.size(), vec.getDataSize() );
    EXPECT_EQ ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 1024, vec.getChunks()[ 0 ].iov_len );

    vec.clear();

    EXPECT_TRUE ( !vec.getContinuousWritable() );
    EXPECT_EQ ( 0, vec.getNumChunks() );
}

TEST_F ( TestMemVector, WritableExt )
{
    EXPECT_EQ ( orgData.size(), orgStr.length() );
    EXPECT_TRUE ( orgStr == orgData.toString() );

    numChunks = totalSize = 0;
    vec.clear();

    for ( size_t i = 0; i < orgData.size(); i += 50 )
    {
        MemHandle mh = orgData.getHandle ( i, 50 );

        vec.append ( orgData.getHandle ( i, 50 ) );

        totalSize += mh.size();
        ++numChunks;

        EXPECT_EQ ( totalSize, vec.getDataSize() );
        EXPECT_EQ ( numChunks, vec.getNumChunks() );
    }

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );
    EXPECT_TRUE ( vec.storeContinuous ( data ) );
    EXPECT_EQ ( orgData.size(), data.size() );
    EXPECT_TRUE ( orgStr == data.toString() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // All the chunks should be shared, so this should split up the first chunk:
    EXPECT_TRUE ( vec.getContinuousWritable ( 40 ) != 0 );

    ++numChunks;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 2, vec.getNumChunks() );
    EXPECT_EQ ( 40, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 10, vec.getChunks()[ 1 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 2 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Now the first 40 bytes are not shared. So this shouldn't create a new chunk:
    EXPECT_TRUE ( vec.getContinuousWritable ( 30 ) != 0 );

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 2, vec.getNumChunks() );
    EXPECT_EQ ( 40, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 10, vec.getChunks()[ 1 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 2 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Now this will merge them together again:
    EXPECT_TRUE ( vec.getContinuousWritable ( 50 ) != 0 );
    --numChunks;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Now the first 50 bytes are no longer shared. So this shouldn't create a new chunk:
    EXPECT_TRUE ( vec.getContinuousWritable ( 30 ) != 0 );

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Let's reference the first chunk:
    MemHandle tmp = vec.getChunk ( 0 );

    // And let's try to use the entire first chunk. It should replace the chunk that's there.
    EXPECT_TRUE ( vec.getContinuousWritable ( 50 ) != 0 );

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // And now the first chunk is not shared. So this shouldn't create a new chunk:
    EXPECT_TRUE ( vec.getContinuousWritable ( 30 ) != 0 );

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Let's repeat it, but forcing it to use special memory:
    tmp = vec.getChunk ( 0 );
    MemHandle tmp2 ( 100 );

    EXPECT_TRUE ( vec.getContinuousWritable ( 50, &tmp2 ) != 0 );

    EXPECT_TRUE ( tmp2.isEmpty() );
    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 50, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // This should replace the first chunk and consume half of the second one:
    EXPECT_TRUE ( vec.getContinuousWritable ( 75 ) != 0 );

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 2, vec.getNumChunks() );
    EXPECT_EQ ( 75, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 25, vec.getChunks()[ 1 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 2 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // This should replace first two chunks and consume half of the third one (so there will be one less chunk):
    EXPECT_TRUE ( vec.getContinuousWritable ( 125 ) != 0 );
    --numChunks;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 2, vec.getNumChunks() );
    EXPECT_EQ ( 125, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 25, vec.getChunks()[ 1 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 2 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // Now we have chunks: 125, 25, 50, 50, 50, 50, 50, 50, ...
    // This should replace first seven chunks with one and consume 5 bytes from the eight one:
    EXPECT_TRUE ( vec.getContinuousWritable ( 405 ) != 0 );
    numChunks -= 6;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 2, vec.getNumChunks() );
    EXPECT_EQ ( 405, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 45, vec.getChunks()[ 1 ].iov_len );
    EXPECT_EQ ( 50, vec.getChunks()[ 2 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // This should replace all chunks but one, and consume all but 1 byte from the last one:
    EXPECT_TRUE ( vec.getContinuousWritable ( vec.getDataSize() - 1 ) != 0 );
    numChunks = 2;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 1, vec.getNumChunks() );
    EXPECT_EQ ( 1023, vec.getChunks()[ 0 ].iov_len );
    EXPECT_EQ ( 1, vec.getChunks()[ 1 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );

    // And this should combine the whole thing:
    EXPECT_TRUE ( vec.getContinuousWritable() != 0 );
    numChunks = 1;

    EXPECT_EQ ( numChunks, vec.getNumChunks() );
    EXPECT_EQ ( orgData.size(), vec.getDataSize() );

    ASSERT_LT ( 0, vec.getNumChunks() );
    EXPECT_EQ ( 1024, vec.getChunks()[ 0 ].iov_len );

    // This should always fail:
    EXPECT_EQ ( 0, vec.getContinuousWritable ( vec.getDataSize() + 1 ) );
}
