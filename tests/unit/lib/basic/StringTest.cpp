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

#include "basic/WString.hpp"
#include "basic/IpAddress.hpp"

using namespace Pravala;

// This is for compatibility with the old List that provided findIndex function (returning -1 on failure)
// Now the indexes are unsigned. findIndex is rarely needed in the real code, but here it is useful.
static int findIndex ( const List<String> & list, const String & what )
{
    for ( size_t idx = 0; idx < list.size(); ++idx )
    {
        if ( list[ idx ] == what )
            return ( int ) idx;
    }

    return -1;
}

/// @brief String class test
/// @todo Add more tests to StringTest

class StringTest: public ::testing::Test
{
};

/// @brief Constructor test
TEST_F ( StringTest, Constructor )
{
    String a;

    EXPECT_EQ ( 0, a.length() );

    String b ( "abcd", 2 );
    String c ( "ab", 3 );

    EXPECT_EQ ( 2, b.length() );
    EXPECT_EQ ( 'a', b[ 0 ] );
    EXPECT_EQ ( 'b', b[ 1 ] );
    EXPECT_EQ ( '\0', b[ 2 ] );

    EXPECT_EQ ( 3, c.length() );
    EXPECT_EQ ( 'a', c[ 0 ] );
    EXPECT_EQ ( 'b', c[ 1 ] );
    EXPECT_EQ ( '\0', c[ 2 ] );

    String d ( "xyz" );

    EXPECT_EQ ( 3, d.length() );
    EXPECT_EQ ( 'x', d[ 0 ] );
    EXPECT_EQ ( 'y', d[ 1 ] );
    EXPECT_EQ ( 'z', d[ 2 ] );
    EXPECT_EQ ( '\0', d[ 3 ] );

    String e ( "qw" );
    String f ( e );

    EXPECT_EQ ( 2, e.length() );
    EXPECT_EQ ( 'q', e[ 0 ] );
    EXPECT_EQ ( 'w', e[ 1 ] );
    EXPECT_EQ ( '\0', e[ 2 ] );

    EXPECT_EQ ( 2, f.length() );
    EXPECT_EQ ( 'q', f[ 0 ] );
    EXPECT_EQ ( 'w', f[ 1 ] );
    EXPECT_EQ ( '\0', f[ 2 ] );
}

/// @brief Operator test
TEST_F ( StringTest, Operators )
{
    String a ( "xyz" );

    // String & operator= ( const String & other );
    // String & operator= ( const char * str );

    String b;

    EXPECT_EQ ( 3, a.length() );
    EXPECT_EQ ( 0, b.length() );

    b = ( a = "a" );

    EXPECT_EQ ( 1, a.length() );
    EXPECT_EQ ( 'a', a[ 0 ] );
    EXPECT_EQ ( '\0', a[ 1 ] );

    a = "cd";

    EXPECT_EQ ( 1, b.length() );
    EXPECT_EQ ( 'a', b[ 0 ] );
    EXPECT_EQ ( '\0', b[ 1 ] );

    EXPECT_EQ ( 2, a.length() );
    EXPECT_EQ ( 'c', a[ 0 ] );
    EXPECT_EQ ( 'd', a[ 1 ] );
    EXPECT_EQ ( '\0', a[ 2 ] );

    String c ( "xyz" );
    String d ( "xyz" );

    // We're testing built-in operators, so we DON'T want to use EXPECT_STREQ( x.c_str(), y.c_str() )

    EXPECT_TRUE ( c == d );
    EXPECT_TRUE ( c == "xyz" );

    String w ( "xyzw" );

    EXPECT_TRUE ( !( c == w ) );
    EXPECT_TRUE ( !( c == "x" ) );

    String e ( "abc" );

    EXPECT_TRUE ( e != d );
    EXPECT_TRUE ( e != c );

    EXPECT_TRUE ( e != "xyz" );
    EXPECT_TRUE ( e != "abcd" );

    e = c;

    EXPECT_TRUE ( e == d );
    EXPECT_TRUE ( e == c );

    EXPECT_TRUE ( e == d.c_str() );
    EXPECT_TRUE ( e == c.c_str() );

    EXPECT_TRUE ( e == "xyz" );

    String l ( "ab" );
    String r ( "cd" );

    EXPECT_TRUE ( l < r );
    EXPECT_TRUE ( !( r < l ) );

    l = "cd";

    EXPECT_TRUE ( !( l < r ) );

    l = "abc";
    r = "abcd";

    EXPECT_TRUE ( l < r );

    l = "cd";
    r = "ab";

    EXPECT_TRUE ( l > r );
    EXPECT_TRUE ( !( r > l ) );

    r = "cd";

    EXPECT_TRUE ( !( l > r ) );

    l = "abcd";
    r = "abc";

    EXPECT_TRUE ( l > r );

    String f ( "0123" );

    int i;

    for ( i = 0; i < 5; i++ )
    {
        EXPECT_EQ ( f[ i ], f[ i ] );
    }

    EXPECT_EQ ( '0', f[ 0 ] );

    EXPECT_EQ ( '0', f[ 0 ] );
    EXPECT_EQ ( '1', f[ 1 ] );
    EXPECT_EQ ( '1', f[ 1 ] );
    EXPECT_EQ ( '2', f[ 2 ] );
    EXPECT_EQ ( '2', f[ 2 ] );
    EXPECT_EQ ( '3', f[ 3 ] );
    EXPECT_EQ ( '3', f[ 3 ] );
    EXPECT_EQ ( '\0', f[ 4 ] );
    EXPECT_EQ ( '\0', f[ 4 ] );

    EXPECT_EQ ( '\0', f[ 4 ] );
    EXPECT_EQ ( '\0', f[ 4 ] );

    f[ 0 ] = 'a';
    f[ 1 ] = 'b';
    f[ 2 ] = 'c';
    f[ 3 ] = 'd';

    EXPECT_EQ ( 'a', f[ 0 ] );
    EXPECT_EQ ( 'a', f[ 0 ] );
    EXPECT_EQ ( 'b', f[ 1 ] );
    EXPECT_EQ ( 'b', f[ 1 ] );
    EXPECT_EQ ( 'c', f[ 2 ] );
    EXPECT_EQ ( 'c', f[ 2 ] );
    EXPECT_EQ ( 'd', f[ 3 ] );
    EXPECT_EQ ( 'd', f[ 3 ] );
    EXPECT_EQ ( '\0', f[ 4 ] );
    EXPECT_EQ ( '\0', f[ 4 ] );
}

/// @brief Test matching of strings that are equal
TEST_F ( StringTest, EqualityMatch )
{
    String z ( "xyz" );
    EXPECT_TRUE ( String ( z ).matches ( "?yz" ) );
    EXPECT_TRUE ( String ( "" ).matches ( "" ) );
    EXPECT_TRUE ( String ( "a" ).matches ( "a" ) );
}

/// @brief Test that strings that are not equal do not match
TEST_F ( StringTest, NonEqualityMatch )
{
    String z ( "xyz" );
    EXPECT_FALSE ( String ( z ).matches ( "axyz" ) );
    EXPECT_FALSE ( String ( z ).matches ( "xyza" ) );
    EXPECT_FALSE ( String ( z ).matches ( " xyz" ) );
    EXPECT_FALSE ( String ( z ).matches ( "xyz " ) );
    EXPECT_FALSE ( String ( "" ).matches ( "a" ) );
    EXPECT_FALSE ( String ( "a" ).matches ( "" ) );
    EXPECT_FALSE ( String ( "b" ).matches ( "a" ) );
}

/// @brief Test the use of the ? optional match character
TEST_F ( StringTest, OptionalMatch )
{
    String z ( "xyz" );
    EXPECT_TRUE ( String ( z ).matches ( "?yz" ) );
    EXPECT_TRUE ( !String ( "" ).matches ( "?" ) );
    EXPECT_TRUE ( !String ( "" ).matches ( "??" ) );
    EXPECT_TRUE ( !String ( "ab" ).matches ( "?" ) );

    EXPECT_TRUE ( String ( "a" ).matches ( "?" ) );
    EXPECT_TRUE ( String ( "bc" ).matches ( "??" ) );

    EXPECT_TRUE ( !String ( "a" ).matches ( "??" ) );

    EXPECT_TRUE ( String ( "abc" ).matches ( "???" ) );
    EXPECT_TRUE ( !String ( "ab" ).matches ( "???" ) );
}

/// @brief Test the use of the . prefix match character
TEST_F ( StringTest, DotMatch )
{
    EXPECT_TRUE ( !String ( "a.b.c" ).matches ( "a.b" ) );
    EXPECT_TRUE ( !String ( "a.b" ).matches ( "a.b.c" ) );
}

/// @brief Test the use of the * wildcard match character
TEST_F ( StringTest, WildcardMatch )
{
    EXPECT_TRUE ( String ( "" ).matches ( "*" ) );
    EXPECT_TRUE ( String ( "" ).matches ( "**" ) );
    EXPECT_TRUE ( String ( "a" ).matches ( "*" ) );
    EXPECT_TRUE ( String ( "ab" ).matches ( "*" ) );
    EXPECT_TRUE ( String ( "a" ).matches ( "a*" ) );
    EXPECT_TRUE ( String ( "ab" ).matches ( "a*" ) );
    EXPECT_TRUE ( String ( "ab" ).matches ( "a*b" ) );
    EXPECT_TRUE ( String ( "acb" ).matches ( "a*b" ) );
}

/// @brief Test matching on a set of dotted notation strings
TEST_F ( StringTest, DottedNotationMatch )
{
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a1.*.*" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a1.*" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "*.b2.*" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "*.*.c3" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "*.c3" ) );

    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a1.*.c3" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a1.b2.*" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "*.b2.c3" ) );

    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a*.b*.c*" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "*1.*2.*3" ) );
    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a*1.b*2.c*3" ) );

    EXPECT_TRUE ( !String ( "a1.b2.c3" ).matches ( "*a.b2.c3" ) );

    EXPECT_TRUE ( String ( "a1.b2.c3" ).matches ( "a?.b?.c?" ) );
    EXPECT_TRUE ( !String ( "a1.b2.c3" ).matches ( "a?.b?2.c?" ) );
}

/// @brief Test matching on a set of sequence strings
TEST_F ( StringTest, SequenceMatch )
{
    EXPECT_TRUE ( String ( "12345678" ).matches ( "*1*2*3*4*5*6*7*8*" ) );
    EXPECT_TRUE ( String ( "a1b2a3b4a5b6a7b8a" ).matches ( "*1*2*3*4*5*6*7*8*" ) );

    EXPECT_FALSE ( String ( "12345678" ).matches ( "?1?2?3?4?5?6?7?8?" ) );
    EXPECT_TRUE ( String ( "a1b2a3b4a5b6a7b8a" ).matches ( "?1?2?3?4?5?6?7?8?" ) );

    EXPECT_FALSE ( String ( "1b2a3b4a5b6a7b8a" ).matches ( "?1?2?3?4?5?6?7?8?" ) );
    EXPECT_FALSE ( String ( "a1b2a3b4a5b6a7b8" ).matches ( "?1?2?3?4?5?6?7?8?" ) );
    EXPECT_FALSE ( String ( "a1b2a3b45b6a7b8" ).matches ( "?1?2?3?4?5?6?7?8?" ) );
    EXPECT_FALSE ( String ( "a1b2a3b4cc5b6a7b8" ).matches ( "?1?2?3?4?5?6?7?8?" ) );

    EXPECT_TRUE ( String ( "abc-def:123:456" ).matches ( "abc-def:*:*" ) );
    EXPECT_FALSE ( String ( "abc-def:123" ).matches ( "abc-def:*:*" ) );
}

/// @brief String splitting test - with default values of optional parameters.
TEST_F ( StringTest, DefaultSplit )
{
    StringList strList;

    strList = String ( "" ).split ( " \t" );

    EXPECT_EQ ( 0U, strList.size() );

    strList = String ( "abc" ).split ( " \t" );

    EXPECT_EQ ( 1U, strList.size() );
    EXPECT_STREQ ( "abc", strList[ 0 ].c_str() );

    strList = String ( "ab c" ).split ( " \t" );

    EXPECT_EQ ( 2U, strList.size() );

    EXPECT_STREQ ( "ab", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 1 ].c_str() );

    strList = String ( "a   b\tc" ).split ( " \t" );

    EXPECT_EQ ( 3U, strList.size() );
    EXPECT_STREQ ( "a", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "b", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 2 ].c_str() );

    strList = String ( "x y z w" ).split ( " \t" );

    EXPECT_EQ ( 4U, strList.size() );
    EXPECT_STREQ ( "x", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "y", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "z", strList[ 2 ].c_str() );
    EXPECT_STREQ ( "w", strList[ 3 ].c_str() );
}

/// @brief String splitting test - with 'keep empty' = true
TEST_F ( StringTest, KeepEmptySplit )
{
    StringList strList;

    strList = String ( "" ).split ( " \t", true );

    EXPECT_EQ ( 1U, strList.size() );
    EXPECT_STREQ ( "", strList[ 0 ].c_str() );

    strList = String ( "ab c" ).split ( " \t", true );

    EXPECT_EQ ( 2U, strList.size() );
    EXPECT_STREQ ( "ab", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 1 ].c_str() );

    strList = String ( "a   b\tc" ).split ( " \t", true );

    EXPECT_EQ ( 5U, strList.size() );
    EXPECT_STREQ ( "a", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "", strList[ 2 ].c_str() );
    EXPECT_STREQ ( "b", strList[ 3 ].c_str() );
    EXPECT_STREQ ( "c", strList[ 4 ].c_str() );

    strList = String ( "\r\n" ).split ( "\r\n", true );

    EXPECT_EQ ( 3U, strList.size() );
    EXPECT_STREQ ( "", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "", strList[ 1 ].c_str() );
    EXPECT_STREQ ( "", strList[ 2 ].c_str() );
}

/// @brief String splitting test - with custom separators
TEST_F ( StringTest, CustomSeparatorsSplit )
{
    StringList strList;

    strList = String ( "," ).split ( ",", false );

    EXPECT_EQ ( 0U, strList.size() );

    strList = String ( "," ).split ( ",", true );

    EXPECT_EQ ( 2U, strList.size() );

    EXPECT_STREQ ( "", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "", strList[ 1 ].c_str() );

    strList = String ( "a,b c" ).split ( ",", false );

    EXPECT_EQ ( 2U, strList.size() );
    EXPECT_STREQ ( "a", strList[ 0 ].c_str() );
    EXPECT_STREQ ( "b c", strList[ 1 ].c_str() );

    strList = String ( "a , ,;, b;\tc" ).split ( ",;", false );

    EXPECT_EQ ( 4U, strList.size() );
    EXPECT_STREQ ( "a ", strList[ 0 ].c_str() );
    EXPECT_STREQ ( " ", strList[ 1 ].c_str() );
    EXPECT_STREQ ( " b", strList[ 2 ].c_str() );
    EXPECT_STREQ ( "\tc", strList[ 3 ].c_str() );
}

/// @brief String trimming test
TEST_F ( StringTest, Trim )
{
    EXPECT_STREQ ( "", String ( "" ).trimmed().c_str() );
    EXPECT_STREQ ( "xyz", String ( "xyz" ).trimmed().c_str() );
    EXPECT_STREQ ( "", String ( " " ).trimmed().c_str() );
    EXPECT_STREQ ( "", String ( " \t " ).trimmed().c_str() );
    EXPECT_STREQ ( "", String ( " \t \n " ).trimmed().c_str() );
    EXPECT_STREQ ( "a", String ( "a \t " ).trimmed().c_str() );
    EXPECT_STREQ ( "b", String ( " \t b" ).trimmed().c_str() );
    EXPECT_STREQ ( "ab c", String ( "ab c" ).trimmed().c_str() );
    EXPECT_STREQ ( "a  b  c", String ( " a  b  c " ).trimmed().c_str() );
    EXPECT_STREQ ( "a \t b c", String ( " \t \n a \t b c\n" ).trimmed().c_str() );
    EXPECT_STREQ ( "x \t \n a \t b c", String ( "x \t \n a \t b c\n" ).trimmed().c_str() );
}

/// @brief String reversing test
TEST_F ( StringTest, Reverse )
{
    EXPECT_STREQ ( "", String ( "" ).reversed().c_str() );
    EXPECT_STREQ ( "x", String ( "x" ).reversed().c_str() );
    EXPECT_STREQ ( "yx", String ( "xy" ).reversed().c_str() );
    EXPECT_STREQ ( "zyx", String ( "xyz" ).reversed().c_str() );
    EXPECT_STREQ ( " z y x ", String ( " x y z " ).reversed().c_str() );
}

/// @brief String simplifying test
TEST_F ( StringTest, Simplify )
{
    String z (
            "                 \t            asdf   asdf  asdf    asdf asd hello there this "
            "is  a        tests  a... .\t \t  \t\t\n   asd  f f    d        sdfsdf \t   " );

    String zsimp ( "asdf asdf asdf asdf asd hello there this is a tests a... . asd f f d sdfsdf" );

    EXPECT_STREQ ( "", String ( "" ).simplified().c_str() );
    EXPECT_STREQ ( "xyz", String ( "xyz" ).simplified().c_str() );
    EXPECT_STREQ ( "", String ( " " ).simplified().c_str() );
    EXPECT_STREQ ( "", String ( " \t " ).simplified().c_str() );
    EXPECT_STREQ ( "", String ( " \t \n " ).simplified().c_str() );
    EXPECT_STREQ ( "a", String ( "a \t " ).simplified().c_str() );
    EXPECT_STREQ ( "b", String ( " \t b" ).simplified().c_str() );
    EXPECT_STREQ ( "ab c", String ( "ab c" ).simplified().c_str() );
    EXPECT_STREQ ( "a b c", String ( " a  b  c " ).simplified().c_str() );
    EXPECT_STREQ ( "a b c", String ( " \t \n a \t b c\n" ).simplified().c_str() );
    EXPECT_STREQ ( "x a b c", String ( "x \t \n a \t b c\n" ).simplified().c_str() );
    EXPECT_STREQ ( zsimp.c_str(), String ( z ).simplified().c_str() );
}

TEST_F ( StringTest, NCaseCompare )
{
    EXPECT_EQ ( 0, String ( "" ).compare ( "", false, 5 ) );
    EXPECT_EQ ( 0, String ( "a" ).compare ( "b", false, 0 ) );

    EXPECT_EQ ( 0, String ( "AaAaB" ).compare ( "aAaAc", false, 0 ) );
    EXPECT_EQ ( 0, String ( "AaAaB" ).compare ( "aAaAc", false, 1 ) );
    EXPECT_EQ ( 0, String ( "AaAaB" ).compare ( "aAaAc", false, 2 ) );
    EXPECT_EQ ( 0, String ( "AaAaB" ).compare ( "aAaAc", false, 3 ) );
    EXPECT_EQ ( 0, String ( "AaAaB" ).compare ( "aAaAc", false, 4 ) );
    EXPECT_EQ ( -1, String ( "AaAaB" ).compare ( "aAaAc", false, 5 ) );
    EXPECT_EQ ( 1, String ( "AaAac" ).compare ( "aAaAb", false, 5 ) );

    EXPECT_EQ ( 0, String ( "abc" ).compare ( "abc", false ) );
    EXPECT_EQ ( 0, String ( "abc" ).compare ( "aBc", false ) );
    EXPECT_EQ ( 1, String ( "acb" ).compare ( "abc", false ) );

    EXPECT_EQ ( -1, String ( "aB" ).compare ( "Aba", false ) );
    EXPECT_EQ ( 1, String ( "aB" ).compare ( "AAa", false ) );

    EXPECT_EQ ( 0, String ( "aB" ).compare ( "Aba", false, 1 ) );
    EXPECT_EQ ( 0, String ( "aB" ).compare ( "AAA", false, 1 ) );

    EXPECT_EQ ( 0, String ( "abc" ).compare ( "abc", false, 5 ) );
    EXPECT_EQ ( 0, String ( "abc" ).compare ( "aBc", false, 5 ) );
    EXPECT_EQ ( 1, String ( "acb" ).compare ( "abc", false, 5 ) );

    EXPECT_EQ ( -1, String ( "aB" ).compare ( "Aba", false, 5 ) );
    EXPECT_EQ ( 1, String ( "aB" ).compare ( "AAa", false, 5 ) );
}

// Inherit String class so we can access private data for testing, sqeeze/reserver
class TestString: public String
{
    public:
        inline int getReservedSize() const
        {
            return getAllocatedSize();
        }
};

TEST_F ( StringTest, Mem )
{
    // reserve()
    TestString ts1, ts2;
    ts1.append ( "hello" );
    ts1.reserve ( 7 );
    ts2.reserve ( 37 );
    EXPECT_LE ( 12, ts1.getReservedSize() );
    EXPECT_LE ( 37, ts2.getReservedSize() );

    // squeeze()
    ts1.squeeze();
    ts2.squeeze();
    EXPECT_EQ ( 5, ts1.getReservedSize() );
    EXPECT_EQ ( 0, ts2.getReservedSize() );
}

TEST_F ( StringTest, Manipulation )
{
    // append()
    String a1 ( "hello" );
    String a2 ( "hello" );
    String a3 ( "hello" );
    String a4 ( "hello" );
    String b ( "goodbye" );
    EXPECT_STREQ ( "hellogoodbye", a1.append ( b ).c_str() );
    EXPECT_STREQ ( "hellofoo", a2.append ( "foo" ).c_str() );
    EXPECT_STREQ ( "hellofo", a3.append ( "foooo", 2 ).c_str() );
    EXPECT_STREQ ( "helloa", a4.append ( 'a' ).c_str() );

    // isEmpty()
    String sempty;
    String sitem ( "foo" );
    EXPECT_TRUE ( sempty.isEmpty() );
    EXPECT_FALSE ( sitem.isEmpty() );

    // clear
    String c ( "hello" );
    c.clear();
    EXPECT_EQ ( 0, c.length() );

    // substr
    String d ( "helloGoodbye" );
    EXPECT_STREQ ( "loGoo", d.substr ( 3, 5 ).c_str() );
    EXPECT_STREQ ( "odbye", d.substr ( 7 ).c_str() );

    // toLower, toUpper
    String e ( "hElLoGooDBYe" );
    String f ( e );
    EXPECT_STREQ ( "hellogoodbye", e.toLower().c_str() );
    EXPECT_STREQ ( "HELLOGOODBYE", f.toUpper().c_str() );

    // replace
    String h ( "abc" );
    String h1;
    h1 = h.replace ( "b", "z" );
    EXPECT_STREQ ( h1.c_str(), "azc" );

    String m = String ( "HelloGoodbyeCanada" ).replace ( "Good", "123" );
    EXPECT_STREQ ( "Hello123byeCanada", m.c_str() );

    m = String ( "eAbcdefeghe" ).replace ( "e", "" );
    EXPECT_STREQ ( "Abcdfgh", m.c_str() );

    m = String ( "aaaaa" ).replace ( "aa", "x" );
    EXPECT_STREQ ( "xxa", m.c_str() );

    m = String ( "ababababa" ).replace ( "aa", "x" );
    EXPECT_STREQ ( "ababababa", m.c_str() );

    m = String ( "aaaa" ).replace ( "aa", "" );
    EXPECT_STREQ ( "", m.c_str() );

    // findFirstOf
    String n ( "abcdeffedcbaabcdeffedcba" );
    EXPECT_EQ ( 5, n.findFirstOf ( "zxf" ) );
    EXPECT_EQ ( 17, n.findFirstOf ( "zxf", 8 ) );

    // find
    String p ( "edcb" );
    EXPECT_EQ ( 7, n.find ( p ) );
    EXPECT_EQ ( 19, n.find ( p, 8 ) );
    EXPECT_EQ ( 7, n.find ( "edcb" ) );
    EXPECT_EQ ( 19, n.find ( "edcb", 8 ) );
    EXPECT_EQ ( 3, n.find ( "d" ) );
    EXPECT_EQ ( 8, n.find ( "d", 6 ) );

    // removeChars
    String r ( "abbbbccdDee" );
    EXPECT_STREQ ( "", r.removeChars ( "abcdDe" ).c_str() );
    EXPECT_STREQ ( "abbbbccdDee", r.removeChars ( "" ).c_str() );
    EXPECT_STREQ ( "abbbbccdDee", r.removeChars ( "xyz" ).c_str() );
    EXPECT_STREQ ( "bbbbccdDee", r.removeChars ( "a" ).c_str() );
    EXPECT_STREQ ( "ccdDee", r.removeChars ( "ab" ).c_str() );
    EXPECT_STREQ ( "bbbbdD", r.removeChars ( "ace" ).c_str() );
    EXPECT_STREQ ( "dD", r.removeChars ( "ecba" ).c_str() );
}

TEST_F ( StringTest, ToNum )
{
    bool isOk = false;

    EXPECT_TRUE ( String ( "yes" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_TRUE ( String ( "Yes" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_TRUE ( String ( "YeS" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_TRUE ( String ( "tRue" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_TRUE ( String ( "1" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_TRUE ( String ( "On" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( bool ), String ( "oN" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );

    EXPECT_FALSE ( String ( "No" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_FALSE ( String ( "NO" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_FALSE ( String ( "falSE" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_FALSE ( String ( "0" ).toBool ( &isOk ) );
    EXPECT_TRUE ( isOk );

    EXPECT_FALSE ( String ( "hello" ).toBool ( &isOk ) );
    EXPECT_FALSE ( isOk );

    // toInt
    EXPECT_EQ ( 1234, String ( "1234" ).toInt32 ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0xffaebc, String ( "0xffAebc" ).toInt32 ( &isOk, 0 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0713, String ( "0713" ).toInt32 ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -0xffaebc, String ( "-0xffaebc" ).toInt32 ( &isOk, 0 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 5334, String ( "+5334" ).toInt32 ( &isOk, 10 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( int ), sizeof ( String ( "+5334" ).toInt32 ( &isOk, 10 ) ) );
    EXPECT_TRUE ( isOk );

    // toLong
    EXPECT_EQ ( 4567, String ( "4567" ).toLong ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0xcead3, String ( "0xceAd3" ).toLong ( &isOk, 16 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0527, String ( "0527" ).toLong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -0315, String ( "-0315" ).toLong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( long ), sizeof ( String ( "-0315" ).toLong ( &isOk, 8 ) ) );
    EXPECT_TRUE ( isOk );

    // toULong
    EXPECT_EQ ( 4567, String ( "4567" ).toULong ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0xcead3, String ( "0xceAd3" ).toULong ( &isOk, 16 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0527, String ( "0527" ).toULong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -0315, String ( "-0315" ).toULong ( &isOk, 8 ) );
    EXPECT_FALSE ( isOk );
    EXPECT_EQ ( sizeof ( long ), sizeof ( String ( "-0315" ).toULong ( &isOk, 8 ) ) );
    EXPECT_FALSE ( isOk );

    // toLongLong
    EXPECT_EQ ( 4567, String ( "4567" ).toLongLong ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0xcead3, String ( "0xceAd3" ).toLongLong ( &isOk, 16 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0527, String ( "0527" ).toLongLong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -0315, String ( "-0315" ).toLongLong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( long long ), sizeof ( String ( "-0315" ).toLongLong ( &isOk, 8 ) ) );
    EXPECT_TRUE ( isOk );

    // toULongLong
    EXPECT_EQ ( 4567, String ( "4567" ).toULongLong ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0xcead3, String ( "0xceAd3" ).toULongLong ( &isOk, 16 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0527, String ( "0527" ).toULongLong ( &isOk, 8 ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -0315, String ( "-0315" ).toULongLong ( &isOk, 8 ) );
    EXPECT_FALSE ( isOk );
    EXPECT_EQ ( sizeof ( long long ), sizeof ( String ( "-0315" ).toULongLong ( &isOk, 8 ) ) );
    EXPECT_FALSE ( isOk );

    // toFloat
    EXPECT_EQ ( 1234, String ( "1234" ).toFloat ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 4567.125, String ( "4567.125" ).toFloat ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -9876.375, String ( "-9876.375" ).toFloat ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 5598.625, String ( "+5598.625" ).toFloat ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0x57893, String ( "0x57893" ).toFloat ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( float ), sizeof ( String ( "0x57893" ).toFloat ( &isOk ) ) );
    EXPECT_TRUE ( isOk );

    // toDouble
    EXPECT_EQ ( 1234, String ( "1234" ).toDouble ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 4567.125, String ( "4567.125" ).toDouble ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( -9876.375, String ( "-9876.375" ).toDouble ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 5598.542, String ( "+5598.542" ).toDouble ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( 0x57893, String ( "0x57893" ).toDouble ( &isOk ) );
    EXPECT_TRUE ( isOk );
    EXPECT_EQ ( sizeof ( double ), sizeof ( String ( "0x57893" ).toDouble ( &isOk ) ) );
    EXPECT_TRUE ( isOk );
}

TEST_F ( StringTest, List )
{
    StringList a1;

    a1.append ( "there" );
    a1.append ( "how" );
    a1.append ( "are" );
    a1.append ( "you" );
    a1.prepend ( "hello" );
    a1.removeValue ( "are" );

    EXPECT_STREQ ( "hello:there:how:you", String::join ( a1, ":" ).c_str() );
    EXPECT_EQ ( 4U, a1.size() );
    EXPECT_EQ ( 2, findIndex ( a1, "how" ) );

    a1.removeIndex ( 2 );
    EXPECT_STREQ ( "hello'there'you", String::join ( a1, "'" ).c_str() );
    EXPECT_STREQ ( "there", a1[ 1 ].c_str() );

    a1.clear();
    EXPECT_STREQ ( "", String::join ( a1, "'" ).c_str() );
}

TEST_F ( StringTest, NumToString )
{
    int n1 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n1 ).c_str() );
    EXPECT_STREQ ( "-12345", String::number ( -n1 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n1, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n1, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n1, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n1, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n1, String::Int_Dec, 12, true ).c_str() );

    long int n2 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n2 ).c_str() );
    EXPECT_STREQ ( "-12345", String::number ( -n2 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n2, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n2, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n2, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n2, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n2, String::Int_Dec, 12, true ).c_str() );

    long long int n3 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n3 ).c_str() );
    EXPECT_STREQ ( "-12345", String::number ( -n3 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n3, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n3, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n3, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n3, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n3, String::Int_Dec, 12, true ).c_str() );

    unsigned int n4 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n4 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n4, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n4, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n4, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n4, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n4, String::Int_Dec, 12, true ).c_str() );

    unsigned long int n5 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n5 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n5, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n5, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n5, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n5, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n5, String::Int_Dec, 12, true ).c_str() );

    unsigned long long int n6 ( 12345 );
    EXPECT_STREQ ( "12345", String::number ( n6 ).c_str() );
    EXPECT_STREQ ( "30071", String::number ( n6, String::Int_Oct ).c_str() );
    EXPECT_STREQ ( "3039", String::number ( n6, String::Int_Hex ).c_str() );
    EXPECT_STREQ ( "   30071", String::number ( n6, String::Int_Oct, 8 ).c_str() );
    EXPECT_STREQ ( "       12345", String::number ( n6, String::Int_Dec, 12 ).c_str() );
    EXPECT_STREQ ( "000000012345", String::number ( n6, String::Int_Dec, 12, true ).c_str() );

    double n7 ( 123456789 );
    EXPECT_STREQ ( "1.23457e+08", String::number ( n7 ).c_str() );
    EXPECT_STREQ ( "-1.23457E+08", String::number ( -n7, String::Double_G ).c_str() );
    EXPECT_STREQ ( "12345", String::number ( double ( 12345 ) ).c_str() );
    EXPECT_STREQ ( "1.234568e+08", String::number ( n7, String::Double_e ).c_str() );
    EXPECT_STREQ ( "1.234568e+19", String::number ( 12345678901234567890.0, String::Double_e ).c_str() );
    EXPECT_STREQ ( "2.345679e-10", String::number ( 0.0000000002345678901234567890, String::Double_e ).c_str() );
    EXPECT_STREQ ( "1234.567800", String::number ( 1234.5678, String::Double_f ).c_str() );
    EXPECT_STREQ ( "0000000001234.567800", String::number ( 1234.5678, String::Double_f, 20, true ).c_str() );
    EXPECT_STREQ ( "123.456780", String::number ( 123.45678, String::Double_F ).c_str() );

    // FIXME, the result isn't Hex, what is it? 10.0=0x1.4p+3  with Double_a
    // EXPECT_STREQ ( "0000000001234.567800", String::number ( 1234.5678, String::Double_a,20,true ) );
    // EXPECT_STREQ ( "10", String::number ( 10.0, String::Double_a) );
    // // add in some Double_A  tests

    long double n8 ( 1 );
    EXPECT_STREQ ( "1", String::number ( n8 ).c_str() );
    // FIXME, add in the extra tests from double above

    EXPECT_STREQ ( "true", String::number ( bool ( 1 ) ).c_str() );
    EXPECT_STREQ ( "false", String::number ( bool ( 0 ) ).c_str() );
}

void StringRef ( const String & ref )
{
    EXPECT_STREQ ( "abc", ref.c_str() );
}

TEST_F ( StringTest, FuncRef )
{
    StringRef ( "abc" );
}

TEST_F ( StringTest, WideChar )
{
    String str ( "abcdef" );

    bool isOk = false;
    WString wStr = str.toWString ( &isOk );

    ASSERT_TRUE ( isOk );
    EXPECT_EQ ( L'a', wStr[ 0 ] );
    EXPECT_EQ ( L'b', wStr[ 1 ] );
    EXPECT_EQ ( L'c', wStr[ 2 ] );
    EXPECT_EQ ( L'd', wStr[ 3 ] );
    EXPECT_EQ ( L'e', wStr[ 4 ] );
    EXPECT_EQ ( L'f', wStr[ 5 ] );
    EXPECT_EQ ( L'\0', wStr[ 6 ] );

    String str2 = wStr.toString ( &isOk );

    ASSERT_TRUE ( isOk );
    EXPECT_TRUE ( str == str2 );

    wStr = L"abcdef";
    WString wStr2 ( L"abcdef" );

    EXPECT_TRUE ( wStr == wStr2 );

    String str3 = wStr2.toString ( &isOk );

    ASSERT_TRUE ( isOk );

    wStr = str3.toWString ( &isOk );

    ASSERT_TRUE ( isOk );
    EXPECT_TRUE ( wStr == wStr2 );
    EXPECT_EQ ( L'a', wStr[ 0 ] );
    EXPECT_EQ ( L'b', wStr[ 1 ] );
    EXPECT_EQ ( L'c', wStr[ 2 ] );
    EXPECT_EQ ( L'd', wStr[ 3 ] );
    EXPECT_EQ ( L'e', wStr[ 4 ] );
    EXPECT_EQ ( L'f', wStr[ 5 ] );
    EXPECT_EQ ( L'\0', wStr[ 6 ] );
}

TEST_F ( StringTest, Format )
{
    String str ( "%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11" );

    EXPECT_STREQ ( "1 %1 %2 %3 %4 %5 %6 %7 %8 %9 %10", str.arg ( 1 ).c_str() );
    EXPECT_STREQ ( "1 2 %1 %2 %3 %4 %5 %6 %7 %8 %9", str.arg ( 2 ).c_str() );
    EXPECT_STREQ ( "1 2 3 %1 %2 %3 %4 %5 %6 %7 %8", str.arg ( 3 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 %1 %2 %3 %4 %5 %6 %7", str.arg ( 4 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 %1 %2 %3 %4 %5 %6", str.arg ( 5 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 %1 %2 %3 %4 %5", str.arg ( 6 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 7 %1 %2 %3 %4", str.arg ( 7 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 7 8 %1 %2 %3", str.arg ( 8 ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 7 8 hello %1 %2", str.arg ( "hello" ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 7 8 hello 1.2.3.4 %1", str.arg ( IpAddress ( "1.2.3.4" ) ).c_str() );
    EXPECT_STREQ ( "1 2 3 4 5 6 7 8 hello 1.2.3.4 x", str.arg ( 'x' ).c_str() );

    str = String ( "%1 %100" ).arg ( "X" );

    // This is because arg only supports up to %99:
    EXPECT_STREQ ( "X %90", str.c_str() );

    str.arg ( "Y" );

    EXPECT_STREQ ( "X %89", str.c_str() );
}

TEST_F ( StringTest, BiggerFormat )
{
    String str;

    for ( int i = 1; i < 100; ++i )
    {
        str.append ( "%" ).append ( String::number ( i ) );
    }

    for ( int i = 1; i < 100; ++i )
    {
        const String replaceWith = String ( "[" ).append ( String::number ( i ) ).append ( "]" );

        str.arg ( replaceWith );

        String compStr;

        for ( int j = 1; j <= i; ++j )
        {
            compStr.append ( "[" ).append ( String::number ( j ) ).append ( "]" );
        }

        for ( int j = 1; j < 100 - i; ++j )
        {
            compStr.append ( "%" ).append ( String::number ( j ) );
        }

        ASSERT_STREQ ( compStr.c_str(), str.c_str() );
    }
}
