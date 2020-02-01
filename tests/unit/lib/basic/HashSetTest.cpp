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

#include "basic/HashSet.hpp"
#include "basic/String.hpp"
#include "basic/IpAddress.hpp"

using namespace Pravala;

/// @brief HashSet class test
/// @todo Add more tests to HashSetTest
class HashSetTest: public ::testing::Test
{
};

/// @brief Test "A"
TEST_F ( HashSetTest, TestA )
{
    HashSet<String> set;

    String a ( "abc" );
    String b ( "abc" );

    EXPECT_EQ ( 1U, set.getRefCount() );

    set.insert ( a );

    EXPECT_EQ ( 1U, set.getRefCount() );
    EXPECT_EQ ( 1U, set.size() );

    set.insert ( b );

    EXPECT_EQ ( 1U, set.getRefCount() );
    EXPECT_EQ ( 1U, set.size() );

    set.remove ( a );

    EXPECT_EQ ( 1U, set.getRefCount() );
    EXPECT_EQ ( 0U, set.size() );

    set.insert ( "abc" );
    set.insert ( "defsdf" );
    set.insert ( "ghi" );
    set.insert ( "jklm" );

    EXPECT_EQ ( 4U, set.size() );

    EXPECT_TRUE ( set.contains ( "abc" ) );
    EXPECT_TRUE ( set.contains ( "defsdf" ) );
    EXPECT_TRUE ( set.contains ( "ghi" ) );
    EXPECT_TRUE ( set.contains ( "jklm" ) );

    EXPECT_FALSE ( set.contains ( "something else" ) );
}

TEST_F ( HashSetTest, IpAddress )
{
    HashSet<IpAddress> set;
    IpAddress a = IpAddress ( "1.2.3.4" );
    IpAddress b = IpAddress ( "4.3.2.1" );
    IpAddress c = IpAddress ( "1.1.1.1" );
    IpAddress d = IpAddress ( "1.2.3.4" );

    set.insert ( a );
    set.insert ( b );
    set.insert ( c );

    EXPECT_TRUE ( set.contains ( a ) );
    EXPECT_TRUE ( set.contains ( b ) );
    EXPECT_TRUE ( set.contains ( c ) );
    EXPECT_TRUE ( set.contains ( d ) );
}

TEST_F ( HashSetTest, Remove )
{
    HashSet<IpAddress> setA;
    HashSet<IpAddress> setB;
    IpAddress a = IpAddress ( "1.2.3.4" );
    IpAddress b = IpAddress ( "4.3.2.1" );
    IpAddress c = IpAddress ( "1.1.1.1" );
    IpAddress d = IpAddress ( "1.2.3.4" );
    IpAddress e = IpAddress ( "4.3.2.1" );

    setA.insert ( a );
    setA.insert ( b );
    setA.insert ( c );
    setB.insert ( a );
    setB.insert ( c );

    EXPECT_TRUE ( setA.contains ( a ) );
    EXPECT_TRUE ( setA.contains ( b ) );
    EXPECT_TRUE ( setA.contains ( c ) );
    EXPECT_TRUE ( setA.contains ( d ) );

    EXPECT_TRUE ( setB.contains ( a ) );
    EXPECT_TRUE ( setB.contains ( c ) );
    EXPECT_TRUE ( setB.contains ( d ) );

    EXPECT_EQ ( a, d );
    EXPECT_EQ ( b, e );

    setA.remove ( a );

    EXPECT_EQ ( a, d );
    EXPECT_EQ ( b, e );

    setA.remove ( b );

    EXPECT_EQ ( b, e );
}

TEST_F ( HashSetTest, InsertAllTest )
{
    HashSet<String> a;
    HashSet<String> b;

    a.insert ( "a" );

    b.insert ( "b" );
    b.insert ( "c" );

    a.insertAll ( b );

    EXPECT_TRUE ( a.contains ( "a" ) );
    EXPECT_TRUE ( a.contains ( "b" ) );
    EXPECT_TRUE ( a.contains ( "c" ) );

    HashMap<String, bool> mb;
    HashMap<String, int> mi;
    HashMap<String, String> ms;

    mb.insert ( "0", true );
    mb.insert ( "1", false );

    mi.insert ( "a", 10 );
    mi.insert ( "x", 11 );
    mi.insert ( "y", 12 );

    ms.insert ( "k", "q" );
    ms.insert ( "l", "o" );
    ms.insert ( "m", "p" );

    a.insertAll ( mi ).insertAll ( mb );
    a.insertAll ( ms );

    EXPECT_TRUE ( a.contains ( "a" ) );
    EXPECT_TRUE ( a.contains ( "b" ) );
    EXPECT_TRUE ( a.contains ( "c" ) );
    EXPECT_TRUE ( a.contains ( "x" ) );
    EXPECT_TRUE ( a.contains ( "y" ) );
    EXPECT_TRUE ( a.contains ( "k" ) );
    EXPECT_TRUE ( a.contains ( "l" ) );
    EXPECT_TRUE ( a.contains ( "m" ) );

    EXPECT_TRUE ( a.contains ( "0" ) );
    EXPECT_TRUE ( a.contains ( "1" ) );

    List<String> la;
    List<String> lb;

    la.append ( "q" );
    la.append ( "w" );

    lb.append ( "r" );
    lb.append ( "t" );

    a.insertAll ( la ).insertAll ( lb );

    EXPECT_TRUE ( a.contains ( "q" ) );
    EXPECT_TRUE ( a.contains ( "w" ) );
    EXPECT_TRUE ( a.contains ( "r" ) );
    EXPECT_TRUE ( a.contains ( "t" ) );

    EXPECT_TRUE ( a.contains ( "a" ) );
    EXPECT_TRUE ( a.contains ( "b" ) );
    EXPECT_TRUE ( a.contains ( "c" ) );
    EXPECT_TRUE ( a.contains ( "x" ) );
    EXPECT_TRUE ( a.contains ( "y" ) );
    EXPECT_TRUE ( a.contains ( "k" ) );
    EXPECT_TRUE ( a.contains ( "l" ) );
    EXPECT_TRUE ( a.contains ( "m" ) );
}

TEST_F ( HashSetTest, CopyConstrTest )
{
    HashMap<String, bool> mb;

    mb.insert ( "0", true );
    mb.insert ( "1", false );

    HashSet<String> a ( mb );

    EXPECT_TRUE ( a.contains ( "0" ) );
    EXPECT_TRUE ( a.contains ( "1" ) );

    HashMap<String, int> mi;

    mi.insert ( "a", 10 );
    mi.insert ( "x", 11 );
    mi.insert ( "y", 12 );

    HashSet<String> b ( mi );

    EXPECT_TRUE ( b.contains ( "a" ) );
    EXPECT_TRUE ( b.contains ( "x" ) );
    EXPECT_TRUE ( b.contains ( "y" ) );

    HashMap<String, String> ms;

    ms.insert ( "k", "q" );
    ms.insert ( "l", "o" );
    ms.insert ( "m", "p" );

    HashSet<String> c ( ms );

    EXPECT_TRUE ( c.contains ( "k" ) );
    EXPECT_TRUE ( c.contains ( "l" ) );
    EXPECT_TRUE ( c.contains ( "m" ) );

    List<String> la;

    la.append ( "q" );
    la.append ( "w" );

    HashSet<String> d ( la );

    EXPECT_TRUE ( d.contains ( "q" ) );
    EXPECT_TRUE ( d.contains ( "w" ) );
}

TEST_F ( HashSetTest, EqOperatorTest )
{
    HashMap<String, bool> mb;

    mb.insert ( "0", true );
    mb.insert ( "1", false );

    HashSet<String> a;

    a = mb;

    EXPECT_TRUE ( a.contains ( "0" ) );
    EXPECT_TRUE ( a.contains ( "1" ) );

    HashMap<String, int> mi;

    mi.insert ( "a", 10 );
    mi.insert ( "x", 11 );
    mi.insert ( "y", 12 );

    HashSet<String> b;

    b = mi;

    EXPECT_TRUE ( b.contains ( "a" ) );
    EXPECT_TRUE ( b.contains ( "x" ) );
    EXPECT_TRUE ( b.contains ( "y" ) );

    HashMap<String, String> ms;

    ms.insert ( "k", "q" );
    ms.insert ( "l", "o" );
    ms.insert ( "m", "p" );

    HashSet<String> c;

    c = ms;

    EXPECT_TRUE ( c.contains ( "k" ) );
    EXPECT_TRUE ( c.contains ( "l" ) );
    EXPECT_TRUE ( c.contains ( "m" ) );

    List<String> la;

    la.append ( "q" );
    la.append ( "w" );

    HashSet<String> d;

    d = la;

    EXPECT_TRUE ( d.contains ( "q" ) );
    EXPECT_TRUE ( d.contains ( "w" ) );
}

TEST_F ( HashSetTest, ListCastTest )
{
    HashSet<String> a;

    a.insert ( "a" );
    a.insert ( "b" );
    a.insert ( "c" );
    a.insert ( "d" );
    a.insert ( "e" );

    List<String> la ( a );
    List<String> lb;

    lb = a;

    EXPECT_EQ ( a.size(), la.size() );
    EXPECT_EQ ( a.size(), lb.size() );

    EXPECT_TRUE ( la.findValue ( "a" ) );
    EXPECT_TRUE ( la.findValue ( "b" ) );
    EXPECT_TRUE ( la.findValue ( "c" ) );
    EXPECT_TRUE ( la.findValue ( "d" ) );
    EXPECT_TRUE ( la.findValue ( "e" ) );

    EXPECT_TRUE ( lb.findValue ( "a" ) );
    EXPECT_TRUE ( lb.findValue ( "b" ) );
    EXPECT_TRUE ( lb.findValue ( "c" ) );
    EXPECT_TRUE ( lb.findValue ( "d" ) );
    EXPECT_TRUE ( lb.findValue ( "e" ) );
}

TEST_F ( HashSetTest, EqualityOperatorTest )
{
    HashSet<String> a;
    HashSet<String> b;
    HashSet<String> c;

    a.insert ( "a" );
    a.insert ( "b" );

    b.insert ( "a" );
    b.insert ( "b" );

    c.insert ( "c" );
    c.insert ( "d" );

    EXPECT_TRUE ( a == b );
    EXPECT_FALSE ( a != b );

    EXPECT_TRUE ( a != c );
    EXPECT_TRUE ( b != c );
    EXPECT_FALSE ( a == c );
    EXPECT_FALSE ( b == c );

    b.insert ( "x" );

    EXPECT_TRUE ( a != b );
    EXPECT_FALSE ( a == b );
}
