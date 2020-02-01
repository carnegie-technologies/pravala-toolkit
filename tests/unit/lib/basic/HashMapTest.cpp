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
#include "basic/HashMap.hpp"

using namespace Pravala;

/// @brief HashMap class test

class HashMapTest: public ::testing::Test
{
};

/// @brief Test "A"
TEST_F ( HashMapTest, TestA )
{
    HashMap<String, String> map;

    EXPECT_EQ ( 0U, map.size() );

    // This is a null map, so refcount will always be 1
    EXPECT_EQ ( 1U, map.getRefCount() );

    String a ( "abc" );
    String b ( "def" );
    String c ( "xyz" );

    EXPECT_EQ ( 1, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );

    map.insert ( a, b );
    EXPECT_EQ ( 1U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );

    map.insert ( a, c );
    EXPECT_EQ ( 1U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 2, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 2, c.getRefCount() );

    EXPECT_STREQ ( c.c_str(), map.value ( a ).c_str() );

    String d ( "qwerty" );

    map.insert ( b, d );
    EXPECT_EQ ( 2U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 2, b.getRefCount() );
    EXPECT_EQ ( 2, d.getRefCount() );

    EXPECT_STREQ ( d.c_str(), map.value ( b ).c_str() );

    {
        // We create this iterator inside it's own block.
        // This way it will go out of scope at the end of this block
        // and will release a reference to the map!

        HashMap<String, String>::Iterator cIt ( map );

        EXPECT_TRUE ( cIt.isValid() );

        EXPECT_TRUE ( cIt.key() == a || cIt.key() == b );

        if ( cIt.key() == a )
        {
            EXPECT_STREQ ( c.c_str(), cIt.value().c_str() );
        }
        else
        {
            EXPECT_STREQ ( d.c_str(), cIt.value().c_str() );
        }

        EXPECT_TRUE ( cIt.isValid() );
        EXPECT_TRUE ( cIt.next() );
        EXPECT_TRUE ( cIt.isValid() );

        EXPECT_TRUE ( cIt.key() == a || cIt.key() == b );

        if ( cIt.key() == a )
        {
            EXPECT_STREQ ( c.c_str(), cIt.value().c_str() );
        }
        else
        {
            EXPECT_STREQ ( d.c_str(), cIt.value().c_str() );
        }

        EXPECT_TRUE ( cIt.isValid() );
        EXPECT_TRUE ( !cIt.next() );
        EXPECT_TRUE ( !cIt.isValid() );
    }

    HashMap<String, String>::MutableIterator it ( map );

    EXPECT_TRUE ( it.isValid() );

    EXPECT_TRUE ( it.key() == a || it.key() == b );

    if ( it.key() == a )
    {
        EXPECT_STREQ ( c.c_str(), it.value().c_str() );
    }
    else
    {
        EXPECT_STREQ ( d.c_str(), it.value().c_str() );
    }

    EXPECT_TRUE ( it.isValid() );
    EXPECT_TRUE ( it.remove() );
    EXPECT_TRUE ( it.isValid() );

    EXPECT_EQ ( 1U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_TRUE ( it.key() == a || it.key() == b );

    if ( it.key() == a )
    {
        EXPECT_STREQ ( c.c_str(), it.value().c_str() );
    }
    else
    {
        EXPECT_STREQ ( d.c_str(), it.value().c_str() );
    }

    EXPECT_TRUE ( it.isValid() );
    EXPECT_TRUE ( !it.remove() );
    EXPECT_TRUE ( !it.isValid() );

    EXPECT_EQ ( 0U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 1, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 1, d.getRefCount() );

    map.insert ( a, b );
    map.insert ( b, c );
    map.insert ( c, d );
    map.insert ( d, a );

    EXPECT_EQ ( 4U, map.size() );
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 3, a.getRefCount() );
    EXPECT_EQ ( 3, b.getRefCount() );
    EXPECT_EQ ( 3, c.getRefCount() );
    EXPECT_EQ ( 3, d.getRefCount() );

    map.clear();

    EXPECT_EQ ( 0U, map.size() );

    // Back to null map
    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 1, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 1, d.getRefCount() );
}

/// @brief Test "B"
TEST_F ( HashMapTest, TestB )
{
    HashMap<String, String> map;

    EXPECT_EQ ( 0U, map.size() );

    // This is a null map, so refcount will always be 1
    EXPECT_EQ ( 1U, map.getRefCount() );

    String a ( "abc" );
    String b ( "def" );
    String c ( "xyz" );
    String d ( "qwerty" );

    map.insert ( a, b );
    map.insert ( b, c );
    map.insert ( c, d );
    map.insert ( d, a );

    EXPECT_EQ ( 1U, map.getRefCount() );

    EXPECT_EQ ( 3, a.getRefCount() );
    EXPECT_EQ ( 3, b.getRefCount() );
    EXPECT_EQ ( 3, c.getRefCount() );
    EXPECT_EQ ( 3, d.getRefCount() );

    HashMap<String, String> map2 ( map );

    EXPECT_EQ ( 2U, map.getRefCount() );
    EXPECT_EQ ( 2U, map2.getRefCount() );

    HashMap<String, String> map3;

    // null map
    EXPECT_EQ ( 1U, map3.getRefCount() );

    EXPECT_EQ ( 2U, map.getRefCount() );
    EXPECT_EQ ( 2U, map2.getRefCount() );

    map3 = map2;

    EXPECT_EQ ( 3U, map.getRefCount() );
    EXPECT_EQ ( 3U, map2.getRefCount() );
    EXPECT_EQ ( 3U, map3.getRefCount() );

    EXPECT_EQ ( 3, a.getRefCount() );
    EXPECT_EQ ( 3, b.getRefCount() );
    EXPECT_EQ ( 3, c.getRefCount() );
    EXPECT_EQ ( 3, d.getRefCount() );

    String e ( "12345" );
    String f ( "asdf" );

    map3.insert ( e, f );

    EXPECT_EQ ( 2U, map.getRefCount() );
    EXPECT_EQ ( 2U, map2.getRefCount() );
    EXPECT_EQ ( 1U, map3.getRefCount() );

    // 5 - two copies for each string per
    // mapping - one key, one value
    EXPECT_EQ ( 5, a.getRefCount() );
    EXPECT_EQ ( 5, b.getRefCount() );
    EXPECT_EQ ( 5, c.getRefCount() );
    EXPECT_EQ ( 5, d.getRefCount() );

    EXPECT_EQ ( 2, e.getRefCount() );
    EXPECT_EQ ( 2, f.getRefCount() );

    HashMap<String, String>::MutableIterator it2 ( map2 );

    EXPECT_EQ ( 2U, map.getRefCount() );
    EXPECT_EQ ( 2U, map2.getRefCount() );
    EXPECT_EQ ( 1U, map3.getRefCount() );

    // 5 - two copies for each string per
    // mapping - one key, one value
    EXPECT_EQ ( 5, a.getRefCount() );
    EXPECT_EQ ( 5, b.getRefCount() );
    EXPECT_EQ ( 5, c.getRefCount() );
    EXPECT_EQ ( 5, d.getRefCount() );

    EXPECT_EQ ( 2, e.getRefCount() );
    EXPECT_EQ ( 2, f.getRefCount() );

    EXPECT_TRUE ( it2.isValid() );

    EXPECT_TRUE ( it2.remove() );

    EXPECT_EQ ( 1U, map.getRefCount() );
    EXPECT_EQ ( 1U, map2.getRefCount() );
    EXPECT_EQ ( 1U, map3.getRefCount() );

    int rCount = a.getRefCount()
                 + b.getRefCount()
                 + c.getRefCount()
                 + d.getRefCount();

    // We had 5 references per string.
    // We have created extra 2 references per string by
    // detaching map2, but we removed 1 reference from one
    // of the keys, and 1 reference from the associated value
    // by removing it from the map2.
    // So the new total number of references should be:
    // 4*5 + 4*2 - 2 = 26
    EXPECT_EQ ( 26, rCount );

    EXPECT_EQ ( 4U, map.size() );
    EXPECT_EQ ( 3U, map2.size() );
    EXPECT_EQ ( 5U, map3.size() );

    map.clear();
    map2.clear();
    map3.clear();

    EXPECT_EQ ( 1, a.getRefCount() );
    EXPECT_EQ ( 1, b.getRefCount() );
    EXPECT_EQ ( 1, c.getRefCount() );
    EXPECT_EQ ( 1, d.getRefCount() );
    EXPECT_EQ ( 1, e.getRefCount() );
    EXPECT_EQ ( 1, f.getRefCount() );

    // All of them should be null now
    EXPECT_EQ ( 1U, map.getRefCount() );
    EXPECT_EQ ( 1U, map2.getRefCount() );
    EXPECT_EQ ( 1U, map3.getRefCount() );
}

class Abc
{
    public:
        int value;
};

/// @brief Test "C"
TEST_F ( HashMapTest, TestC )
{
    HashMap<Abc *, String> map;

    Abc * a = new Abc();

    map.insert ( a, "new" );
    map.insert ( a, "again" );

    EXPECT_EQ ( 1U, map.size() );

    EXPECT_STREQ ( "again", map.value ( a ).c_str() );

    HashMap<String, Abc *> map2;

    Abc * b = new Abc();

    map2.insert ( "xyz", a );
    map2.insert ( "def", b );

    EXPECT_EQ ( 2U, map2.size() );

    EXPECT_EQ ( b, map2.value ( "def" ) );
    EXPECT_EQ ( a, map2.value ( "xyz" ) );

    delete a;
    delete b;

    a = b = 0;
}

void someFunc ( HashMap<String, int> & hMap )
{
    // The purpose of this function is to take the reference to hash map
    // from another function and create a const iterator.
    // It was revealing a problem inside the hash map
    // preventing it from compiling properly.
    // It has to be a separate function.
    HashMap<String, int>::Iterator iterator ( hMap );

    ( void ) iterator;
}

/// @brief Test "D"
TEST_F ( HashMapTest, TestD )
{
    HashMap<String, int> map;
    HashMap<String, int>::Iterator iterator ( map );

    EXPECT_FALSE ( iterator.isValid() );

    someFunc ( map );
}

/// @brief Test "E"
TEST_F ( HashMapTest, TestE )
{
    HashMap<String, String> map;

    for ( char c = 'a'; c < 'z'; ++c )
    {
        map[ String ( &c, 1 ) ] = "0";
    }

    for ( char c = 'a'; c < 'z'; ++c )
    {
        char d = c + 1;

        map[ String ( &c, 1 ) ] = String ( &d, 1 );
    }

    for ( char c = 'a'; c < 'z'; ++c )
    {
        char d = c + 1;

        EXPECT_STREQ ( String ( &d, 1 ).c_str(), map[ String ( &c, 1 ) ].c_str() );
    }
}

TEST_F ( HashMapTest, IterTest )
{
    HashMap<int, int> map;
    HashMap<int, int> map2;

    map.insert ( 1, 1 );
    map2.insert ( 2, 2 );

    HashMap<int, int>::Iterator cIt ( map );

    EXPECT_TRUE ( cIt.isValid() );
    EXPECT_EQ ( 1, cIt.key() );
    EXPECT_EQ ( 1, cIt.value() );

    cIt = map2;

    EXPECT_TRUE ( cIt.isValid() );
    EXPECT_EQ ( 2, cIt.key() );
    EXPECT_EQ ( 2, cIt.value() );

    cIt.next();

    EXPECT_FALSE ( cIt.isValid() );

    {
        HashMap<int, int>::Iterator it ( map );

        EXPECT_TRUE ( it.isValid() );
        EXPECT_EQ ( 1, it.key() );
        EXPECT_EQ ( 1, it.value() );
    }

    {
        HashMap<int, int>::Iterator it ( map2 );

        EXPECT_TRUE ( it.isValid() );
        EXPECT_EQ ( 2, it.key() );
        EXPECT_EQ ( 2, it.value() );

        it.next();

        EXPECT_FALSE ( it.isValid() );
    }
}

TEST_F ( HashMapTest, InsertAllTest )
{
    HashMap<String, int> a;
    HashMap<String, int> b;

    a.insert ( "a", 1 );
    a.insert ( "b", 2 );

    b.insert ( "b", 3 );
    b.insert ( "c", 4 );

    a.insertAll ( b );

    EXPECT_EQ ( 1, a[ "a" ] );
    EXPECT_EQ ( 3, a[ "b" ] );
    EXPECT_EQ ( 4, a[ "c" ] );
}

TEST_F ( HashMapTest, EqualityOperatorTest )
{
    HashMap<String, int> a;
    HashMap<String, int> b;
    HashMap<String, int> c;

    a.insert ( "a", 1 );
    a.insert ( "b", 2 );

    b.insert ( "a", 1 );
    b.insert ( "b", 2 );

    c.insert ( "c", 1 );
    c.insert ( "d", 2 );

    EXPECT_TRUE ( a == b );
    EXPECT_FALSE ( a != b );

    EXPECT_TRUE ( a != c );
    EXPECT_TRUE ( b != c );
    EXPECT_FALSE ( a == c );
    EXPECT_FALSE ( b == c );

    b[ "a" ] = 3;

    EXPECT_TRUE ( a != b );
    EXPECT_FALSE ( a == b );

    a = b;

    EXPECT_TRUE ( a == b );
    EXPECT_FALSE ( a != b );

    a[ "x" ] = 99;

    EXPECT_TRUE ( a != b );
    EXPECT_FALSE ( a == b );
}

TEST_F ( HashMapTest, MutableIterTest )
{
    HashMap<String, int> map;

    map.insert ( "a", 1 );
    map.insert ( "b", 2 );
    map.insert ( "c", 3 );

    HashMap<String, int>::MutableIterator itA ( map );

    EXPECT_TRUE ( itA.isValid() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_TRUE ( itA.remove() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_TRUE ( itA.remove() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_FALSE ( itA.remove() ); // The last remove() will return 'false'
    EXPECT_TRUE ( map.isEmpty() );

    EXPECT_FALSE ( itA.remove() );
    EXPECT_TRUE ( map.isEmpty() );

    map.insert ( "a", 1 );
    map.insert ( "b", 2 );
    map.insert ( "c", 3 );

    HashMap<String, int>::MutableIterator itB ( map );

    EXPECT_TRUE ( itB.isValid() );

    map.remove ( "a" );
    map.remove ( "b" );
    map.remove ( "c" );

    EXPECT_FALSE ( itB.isValid() );
}

TEST_F ( HashMapTest, IterResetTest )
{
    HashMap<String, int> map;

    map.insert ( "a", 1 );
    map.insert ( "b", 2 );
    map.insert ( "c", 3 );

    HashMap<String, int>::Iterator it ( map );

    EXPECT_TRUE ( it.isValid() );
    EXPECT_FALSE ( map.isEmpty() );

    map.remove ( "a" );
    map.remove ( "b" );
    map.remove ( "c" );

    EXPECT_TRUE ( it.isValid() );

    int cnt = 0;

    for ( ; it.isValid(); it.next() )
    {
        ++cnt;
    }

    EXPECT_FALSE ( it.isValid() );
    EXPECT_EQ ( 3, cnt );

    EXPECT_TRUE ( it.reset() );
    EXPECT_TRUE ( it.isValid() );

    for ( ; it.isValid(); it.next() )
    {
        ++cnt;
    }

    EXPECT_EQ ( 6, cnt );
}

TEST_F ( HashMapTest, MutableIterResetTest )
{
    HashMap<String, int> map;

    map.insert ( "a", 1 );
    map.insert ( "b", 2 );
    map.insert ( "c", 3 );

    HashMap<String, int>::MutableIterator it ( map );

    EXPECT_TRUE ( it.isValid() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_TRUE ( it.remove() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_TRUE ( it.remove() );
    EXPECT_FALSE ( map.isEmpty() );

    EXPECT_FALSE ( it.remove() ); // The last remove() will return 'false'
    EXPECT_TRUE ( map.isEmpty() );

    EXPECT_FALSE ( it.remove() );
    EXPECT_TRUE ( map.isEmpty() );

    map.insert ( "a", 1 );
    map.insert ( "b", 2 );
    map.insert ( "c", 3 );

    EXPECT_TRUE ( it.reset() );
    EXPECT_TRUE ( it.isValid() );

    int cnt = 0;

    for ( ; it.isValid(); it.next() )
    {
        ++cnt;
    }

    EXPECT_FALSE ( it.isValid() );
    EXPECT_EQ ( 3, cnt );

    EXPECT_TRUE ( it.reset() );
    EXPECT_TRUE ( it.isValid() );

    for ( ; it.isValid(); it.next() )
    {
        ++cnt;
    }

    EXPECT_FALSE ( it.isValid() );
    EXPECT_EQ ( 6, cnt );

    EXPECT_TRUE ( it.reset() );
    EXPECT_TRUE ( it.isValid() );

    map.remove ( "a" );
    map.remove ( "b" );
    map.remove ( "c" );

    EXPECT_FALSE ( it.isValid() );
    EXPECT_FALSE ( it.reset() );
}
