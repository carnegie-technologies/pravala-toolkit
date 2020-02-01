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

#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"

using namespace Pravala;

/// @brief Buffer class test
class BufferTest: public ::testing::Test
{
};

TEST_F ( BufferTest, SimpleTest )
{
    Buffer buf;

    buf.append ( "0123456789" );

    Buffer buf2 ( buf );
    Buffer buf3 ( buf );

    ASSERT_EQ ( ( size_t ) 10, buf.size() );
    ASSERT_EQ ( ( size_t ) 10, buf2.size() );
    ASSERT_EQ ( ( size_t ) 10, buf3.size() );

    buf3.append ( "abc" );

    ASSERT_EQ ( ( size_t ) 13, buf3.size() );

    for ( int i = 0; i < 10; ++i )
    {
        EXPECT_EQ ( '0' + i, *buf.get ( i ) );
        EXPECT_EQ ( '0' + i, *buf2.get ( i ) );
        EXPECT_EQ ( '0' + i, *buf3.get ( i ) );
    }

    MemHandle mh = buf.getHandle();
    MemHandle mh2 = buf2.getHandle();
    MemHandle mh3 = buf3.getHandle();
    MemHandle mh4 = mh.getHandle ( 2 );

    ASSERT_EQ ( ( size_t ) 10, mh.size() );
    ASSERT_EQ ( ( size_t ) 10, mh2.size() );
    ASSERT_EQ ( ( size_t ) 13, mh3.size() );
    ASSERT_EQ ( ( size_t ) 8, mh4.size() );

    for ( int i = 0; i < 10; ++i )
    {
        EXPECT_EQ ( '0' + i, *mh.get ( i ) );
        EXPECT_EQ ( '0' + i, *mh2.get ( i ) );
        EXPECT_EQ ( '0' + i, *mh3.get ( i ) );

        if ( i < 8 )
        {
            EXPECT_EQ ( '0' + 2 + i, *mh4.get ( i ) );
        }
    }

    for ( int i = 0; i < 10; ++i )
    {
        MemHandle tmp = buf.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );

        tmp = buf2.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );

        tmp = buf3.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );
    }

    for ( int i = 0; i < 10; ++i )
    {
        MemHandle tmp = mh.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );

        tmp = mh2.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );

        tmp = mh3.getHandle ( i, 1 );

        ASSERT_EQ ( ( size_t ) 1, tmp.size() );
        EXPECT_EQ ( '0' + i, *tmp.get ( 0 ) );
    }
}

TEST_F ( BufferTest, StringListTest )
{
    StringList strList;
    Buffer buf;

    buf.clear();
    buf.append ( "" );

    strList = buf.toStringList ( " \t", true );

    EXPECT_EQ ( 1U, strList.size() );
    EXPECT_STREQ ( "", strList[ 0 ].c_str() );

    buf.clear();
    buf.append ( "ab c" );

    strList = buf.toStringList ( " \t", true );

    EXPECT_EQ ( 2U, strList.size() );
    EXPECT_STREQ ( "ab", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 1 ].c_str() );

    buf.clear();
    buf.append ( "a   b\tc" );

    strList = buf.toStringList ( " \t", true );

    EXPECT_EQ ( 5U, strList.size() );
    EXPECT_STREQ ( "a", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "", strList[ 2 ].c_str() );
    EXPECT_STREQ ( "b", strList[ 3 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 4 ].c_str() );

    buf.clear();
    buf.append ( "\r\n" );

    strList = buf.toStringList ( "\r\n", true );

    EXPECT_EQ ( 3U, strList.size() );
    EXPECT_STREQ ( "", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "", strList[ 2 ].c_str() );
}
