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

#include "basic/String.hpp"
#include "basic/List.hpp"
#include "basic/HashSet.hpp"
#include "basic/Random.hpp"

using namespace Pravala;

/// @brief List class test
class ListTest: public ::testing::Test
{
    protected:

        /// @brief Generates a random string.
        /// It makes sure that this string has never been used before.
        /// Max length of the string is 1024 characters, so if this is supposed to be called
        /// too many times, it may take longer and longer to generate strings.
        /// @param [out] len The random length chosen for the string.
        /// @return The random string generated.
        String getRandomString ( int & len )
        {
            String str;

            do
            {
                len = Random::rand ( 1024 );

                str.clear();
                str.reserve ( len );

                for ( int i = 0; i < len; i++ )
                {
                    str.append ( static_cast<char> ( 'a' + Random::rand ( 'z' - 'a' + 1 ) ) );
                }
            }
            while ( _generatedStrings.contains ( str ) );

            _generatedStrings.insert ( str );
            return str;
        }

    private:
        /// @brief All previously generated strings.
        HashSet<String> _generatedStrings;
};

TEST_F ( ListTest, Remove )
{
    StringList list;

    String a ( "abc" );
    String b ( "def" );
    String c ( "jkl" );
    String d ( "xyz" );

    EXPECT_EQ ( 1, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 1, d.getRefCount() );

    list.append ( a );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );

    list.append ( b );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );

    list.append ( c );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 2, c.getRefCount() );

    list.append ( d );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 2, c.getRefCount() );
    EXPECT_EQ ( 2, d.getRefCount() );

    EXPECT_TRUE ( list.removeValue ( "jkl" ) );
    EXPECT_FALSE ( list.removeValue ( "jkl" ) );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 2, d.getRefCount() );

    list.append ( "qwe" );
    list.append ( "qwe" );
    list.append ( "qwe" );

    EXPECT_TRUE ( list.removeValue ( "qwe" ) );
    EXPECT_TRUE ( list.removeValue ( "qwe" ) );
    EXPECT_TRUE ( list.removeValue ( "qwe" ) );
    EXPECT_FALSE ( list.removeValue ( "qwe" ) );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 2, d.getRefCount() );

    EXPECT_EQ ( 3U, list.size() );

    list.removeIndex ( 2 );

    EXPECT_EQ ( 2U, list.size() );

    list.removeIndex ( 1 );

    EXPECT_EQ ( 1U, list.size() );

    list.removeIndex ( 0 );

    EXPECT_EQ ( 0U, list.size() );
}

TEST_F ( ListTest, Find )
{
    // This generates a list of random length,
    // and fills it with random strings.
    // Some of them we want to remember (strings and their position in the list)
    // and verify that they are in the right spots.

    const size_t listLen ( 5000 + Random::rand ( 5000 ) );
    const size_t numItems = 20;

    // "special" random strings and their positions.
    String strings[ numItems ];
    size_t indexes[ numItems ];

    for ( size_t i = 0; i < numItems; ++i )
    {
        bool generateNew = true;

        // Let's find a random index for the string in the range: [0,list-length)
        while ( generateNew )
        {
            generateNew = false;

            indexes[ i ] = Random::rand ( listLen );

            // We want to make sure that previously generated indexes are all different:
            for ( size_t j = 0; j < i; ++j )
            {
                if ( indexes[ i ] == indexes[ j ] )
                {
                    generateNew = true;
                    break;
                }
            }
        }
    }

    StringList list;

    for ( size_t i = 0; i < listLen; ++i )
    {
        int randLen = 0;

        // Let's generate a random string.
        // getRandomString() generates a different one every time.
        const String str = getRandomString ( randLen );

        EXPECT_EQ ( randLen, str.length() );

        for ( size_t j = 0; j < numItems; ++j )
        {
            if ( i == indexes[ j ] )
            {
                // This is one of our "special" indexes. Store that string.
                strings[ j ] = str;
                break;
            }
        }

        list.append ( str );
    }

    for ( size_t i = 0; i < numItems; ++i )
    {
        size_t idx;
        EXPECT_TRUE ( list.findValue ( strings[ i ], &idx ) );
        EXPECT_EQ ( indexes[ i ], idx );
        EXPECT_STREQ ( strings[ i ].c_str(), list[ idx ].c_str() );
    }

    EXPECT_EQ ( listLen, list.size() );

    list.clear();

    EXPECT_EQ ( 0U, list.size() );
}

TEST_F ( ListTest, AppendPrepend )
{
    const size_t listLen = 5;
    String strings[ listLen ];

    StringList list;

    for ( size_t i = 0; i < listLen; ++i )
    {
        int randLen = 0;
        const String str = getRandomString ( randLen );

        EXPECT_EQ ( randLen, str.length() );

        strings[ i ] = str;

        if ( i % 2 == 0 )
        {
            // strings 0, 2, and 4 are appended
            list.append ( str );
        }
        else
        {
            // strings 1, 3 are prepended.
            list.prepend ( str );
        }
    }

    // Final order of strings:
    // 3,1,0,2,4

    size_t idx;

    EXPECT_TRUE ( list.findValue ( strings[ 0 ], &idx ) );
    EXPECT_EQ ( 2U, idx );
    EXPECT_STREQ ( strings[ 0 ].c_str(), list[ 2 ].c_str() );

    EXPECT_TRUE ( list.findValue ( strings[ 1 ], &idx ) );
    EXPECT_EQ ( 1U, idx );
    EXPECT_STREQ ( strings[ 1 ].c_str(), list[ 1 ].c_str() );

    EXPECT_TRUE ( list.findValue ( strings[ 2 ], &idx ) );
    EXPECT_EQ ( 3U, idx );
    EXPECT_STREQ ( strings[ 2 ].c_str(), list[ 3 ].c_str() );

    EXPECT_TRUE ( list.findValue ( strings[ 3 ], &idx ) );
    EXPECT_EQ ( 0U, idx );
    EXPECT_STREQ ( strings[ 3 ].c_str(), list[ 0 ].c_str() );

    EXPECT_TRUE ( list.findValue ( strings[ 4 ], &idx ) );
    EXPECT_EQ ( 4U, idx );
    EXPECT_STREQ ( strings[ 4 ].c_str(), list[ 4 ].c_str() );

    EXPECT_EQ ( listLen, list.size() );

    list.clear();

    EXPECT_EQ ( 0U, list.size() );
}

TEST_F ( ListTest, SortTest )
{
    List<int> list;

    list.append ( 1 );
    list.append ( 3 );
    list.append ( 2 );
    list.append ( 5 );
    list.append ( 4 );
    list.append ( 7 );
    list.append ( 6 );
    list.append ( 9 );
    list.append ( 8 );
    list.append ( 0 );

    list.sortAscending();

    for ( int i = 0; i < 10; ++i )
    {
        EXPECT_EQ ( i, list.at ( i ) );
    }

    list.sortDescending();

    for ( int i = 0; i < 10; ++i )
    {
        EXPECT_EQ ( 9 - i, list.at ( i ) );
    }

    HashMap<int, int> vals;

    list.clear();

    for ( int i = 0; i < 10000; ++i )
    {
        int r = Random::rand ( 5 );

        if ( r == 0 && !list.isEmpty() )
        {
            int val = list.first();
            list.removeFirst();

            ASSERT_TRUE ( vals.contains ( val ) );
            ASSERT_TRUE ( vals.value ( val ) > 0 );

            --vals[ val ];
            continue;
        }

        int val = Random::rand();

        if ( r < 4 )
        {
            list.append ( val );
        }
        else
        {
            list.prepend ( val );
        }

        if ( !vals.contains ( val ) )
        {
            vals[ val ] = 1;
        }
        else
        {
            ++vals[ val ];
        }
    }

    HashMap<int, int> vals2 ( vals );

    list.sortAscending();

    int lastVal = 0;

    for ( size_t i = 0; i < list.size(); ++i )
    {
        int val = list.at ( i );

        if ( i == 0 )
        {
            lastVal = val;
        }
        else
        {
            ASSERT_TRUE ( lastVal <= val );
        }

        ASSERT_TRUE ( vals.contains ( val ) );
        ASSERT_TRUE ( vals.value ( val ) > 0 );

        --vals[ val ];
    }

    for ( HashMap<int, int>::Iterator it ( vals ); it.isValid(); it.next() )
    {
        ASSERT_EQ ( 0, it.value() );
    }

    list.sortDescending();

    for ( size_t i = 0; i < list.size(); ++i )
    {
        int val = list.at ( i );

        if ( i == 0 )
        {
            lastVal = val;
        }
        else
        {
            ASSERT_TRUE ( lastVal >= val );
        }

        ASSERT_TRUE ( vals2.contains ( val ) );
        ASSERT_TRUE ( vals2.value ( val ) > 0 );

        --vals2[ val ];
    }

    for ( HashMap<int, int>::Iterator it ( vals2 ); it.isValid(); it.next() )
    {
        ASSERT_EQ ( 0, it.value() );
    }
}
