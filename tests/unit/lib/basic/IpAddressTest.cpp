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
#include "basic/IpAddress.hpp"
#include "basic/String.hpp"

#ifndef SYSTEM_WINDOWS
extern "C"
{
#include <arpa/inet.h>
}
#endif

using namespace Pravala;

/// @brief Tests the address/netmask conversion methods in IpAddress
class IpAddressTest: public ::testing::Test
{
};

/// @brief Tests built-in IP addresses
TEST_F ( IpAddressTest, IPvBuiltIns )
{
    EXPECT_TRUE ( IpAddress::Ipv4ZeroAddress.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv4ZeroAddress.isIPv4() );

    EXPECT_TRUE ( IpAddress::Ipv6ZeroAddress.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv6ZeroAddress.isIPv6() );

    EXPECT_TRUE ( IpAddress::Ipv4HostNetmask.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv4HostNetmask.isIPv4() );

    EXPECT_TRUE ( IpAddress::Ipv6HostNetmask.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv6HostNetmask.isIPv6() );

    EXPECT_TRUE ( IpAddress::Ipv4LocalhostAddress.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv4LocalhostAddress.isIPv4() );

    EXPECT_TRUE ( IpAddress::Ipv6LocalhostAddress.isValid() );
    EXPECT_TRUE ( IpAddress::Ipv6LocalhostAddress.isIPv6() );
}

/// @brief Tests IPv4 to/from string conversion
TEST_F ( IpAddressTest, IPv4StringConversion )
{
    EXPECT_STREQ ( "11.22.33.44", IpAddress ( "11.22.33.44" ).toString().c_str() );

    EXPECT_FALSE ( IpAddress ( "1.1.1" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1.1.1." ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1.1.1.256" ).isValid() );
}

#ifndef SYSTEM_WINDOWS
#define TEST_INET_PTON( str ) \
    do { \
        struct in6_addr in6Addr; \
        ASSERT_EQ ( 1, inet_pton ( AF_INET6, str, &in6Addr ) ); \
        EXPECT_EQ ( IpAddress ( in6Addr ), IpAddress ( str ) ); \
    } while ( false )
#else
#define TEST_INET_PTON( input_str )
#endif

#define STR_ADDR_TEST( expected_str, input_str ) \
    do { \
        EXPECT_STREQ ( expected_str, IpAddress ( input_str ).toString().c_str() ); \
        EXPECT_STREQ ( expected_str, IpAddress ( expected_str ).toString().c_str() ); \
        EXPECT_EQ ( IpAddress ( expected_str ), IpAddress ( input_str ) ); \
        TEST_INET_PTON ( input_str ); \
        TEST_INET_PTON ( expected_str ); \
    } while ( false )

/// @brief Tests IPv6 to/from string conversion
TEST_F ( IpAddressTest, IPv6StringConversion )
{
    STR_ADDR_TEST ( "::", "::" );
    STR_ADDR_TEST ( "::", "::0" );
    STR_ADDR_TEST ( "::", "0:0:0:0:0:0:0:0" );

    STR_ADDR_TEST ( "1:2:3:4:5:6:7:8", "1:2:3:4:5:6:7:8" );

    STR_ADDR_TEST ( "::1:2:3:4:5:6:7", "0:1:2:3:4:5:6:7" );
    STR_ADDR_TEST ( "::1:2:3:4:5:6", "0:0:1:2:3:4:5:6" );

    STR_ADDR_TEST ( "1:2:3:4:5:6:7::", "1:2:3:4:5:6:7:0" );
    STR_ADDR_TEST ( "1:2:3:4:5:6::", "1:2:3:4:5:6:0:0" );

    STR_ADDR_TEST ( "::2:3:4:5:6:7:8", "0:2:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1::3:4:5:6:7:8", "1:0:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1:2::4:5:6:7:8", "1:2:0:4:5:6:7:8" );
    STR_ADDR_TEST ( "1:2:3::5:6:7:8", "1:2:3:0:5:6:7:8" );
    STR_ADDR_TEST ( "1:2:3:4::6:7:8", "1:2:3:4:0:6:7:8" );
    STR_ADDR_TEST ( "1:2:3:4:5::7:8", "1:2:3:4:5:0:7:8" );
    STR_ADDR_TEST ( "1:2:3:4:5:6::8", "1:2:3:4:5:6:0:8" );
    STR_ADDR_TEST ( "1:2:3:4:5:6:7::", "1:2:3:4:5:6:7:0" );

    STR_ADDR_TEST ( "::3:4:5:6:7:8", "0:0:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1::4:5:6:7:8", "1:0:0:4:5:6:7:8" );

    STR_ADDR_TEST ( "1:2:3:4:5:6:7:8", "1:2:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1::5:6:7:8", "1:0:0:0:5:6:7:8" );
    STR_ADDR_TEST ( "1:2::7:8", "1:2:0:0:0:0:7:8" );
    STR_ADDR_TEST ( "1::8", "1:0:0:0:0:0:0:8" );

    STR_ADDR_TEST ( "::2:3:4:5:6:7:8", "0:2:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1::3:4:5:6:7:8", "1:0:3:4:5:6:7:8" );
    STR_ADDR_TEST ( "1:2::4:5:6:7:8", "1:2:0:4:5:6:7:8" );
    STR_ADDR_TEST ( "1:2:3::5:6:7:8", "1:2:3:0:5:6:7:8" );
    STR_ADDR_TEST ( "1:2:3:4::6:7:8", "1:2:3:4:0:6:7:8" );
    STR_ADDR_TEST ( "1:2:3:4:5::7:8", "1:2:3:4:5:0:7:8" );
    STR_ADDR_TEST ( "1:2:3:4:5:6::8", "1:2:3:4:5:6:0:8" );
    STR_ADDR_TEST ( "1:2:3:4:5:6:7::", "1:2:3:4:5:6:7:0" );

    STR_ADDR_TEST ( "1:2::5:6:7:8", "1:2:0:0:5:6:7:8" );
    STR_ADDR_TEST ( "1:2:3::6:7:8", "1:2:3:0:0:6:7:8" );
    STR_ADDR_TEST ( "1:2:3:4::7:8", "1:2:3:4:0:0:7:8" );
    STR_ADDR_TEST ( "1:2:3:4:5::8", "1:2:3:4:5:0:0:8" );

    // Treated as IPv4:
    STR_ADDR_TEST ( "::ffff:11.22.33.44", "::ffff:11.22.33.44" );
    STR_ADDR_TEST ( "::ffff:11.22.33.44", "::0:0:ffff:11.22.33.44" );
    STR_ADDR_TEST ( "::ffff:11.22.33.44", "::0:ffff:11.22.33.44" );
    STR_ADDR_TEST ( "::ffff:0.22.33.44", "::ffff:0.22.33.44" );
    STR_ADDR_TEST ( "::ffff:0.0.33.44", "::ffff:0.0.33.44" );
    STR_ADDR_TEST ( "::ffff:0.0.0.44", "::ffff:0.0.0.44" );
    STR_ADDR_TEST ( "::ffff:0.0.0.0", "::ffff:0.0.0.0" );
    STR_ADDR_TEST ( "::ffff:0.0.0.0", "::0:0:ffff:0.0.0.0" );

    // Treated as IPv4:
    STR_ADDR_TEST ( "::11.22.33.44", "::11.22.33.44" );
    STR_ADDR_TEST ( "::11.22.33.44", "::0:11.22.33.44" );
    STR_ADDR_TEST ( "::11.22.33.44", "::0:0:11.22.33.44" );
    STR_ADDR_TEST ( "::0.22.33.44", "::0.22.33.44" );

    // Treated as IPv6:
    STR_ADDR_TEST ( "::212c", "::0.0.33.44" );
    STR_ADDR_TEST ( "::2c", "::0.0.0.44" );
    STR_ADDR_TEST ( "::", "::0.0.0.0" );
    STR_ADDR_TEST ( "::", "::0:0:0:0.0.0.0" );

    // Invalid strings:
    EXPECT_FALSE ( IpAddress ( ":" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":0" ).isValid() );

    EXPECT_TRUE ( IpAddress ( "1:2:3:4:5:6:7:8" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:7" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":1:2:3:4:5:6:7" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:7:" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:7:8:" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":1:2:3:4:5:6:7:8" ).isValid() );

    EXPECT_TRUE ( IpAddress ( "1::2:3:4:5" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1::2:3:4:5:" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":1::2:3:4:5" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1::2::4:5" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2::3:4::5" ).isValid() );

    EXPECT_FALSE ( IpAddress ( ":::1:2:3:4:5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::2:3:4:5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::3:4:5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::4:5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:7:8:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:7:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:6:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:::2:3:4:5:6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2:3:4:5:6:7" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2:3:4:5:6" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2:3:4:5" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2:3:4" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2:3" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:::2" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::6:7" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::6" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "2:3:4:5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "3:4:5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":::6:7:8" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1:2:3:4:5:::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "2:3:4:5:::6:7" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "3:4:5:::6" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "4:5:::6" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "5:::6" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1::::2" ).isValid() );

    EXPECT_FALSE ( IpAddress ( ":1" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:" ).isValid() );
    EXPECT_FALSE ( IpAddress ( ":1:" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "::2:3:4:5:6::8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1::3:4:5:6::8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2::4::6:7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2:3::5:6:7::" ).isValid() );

    EXPECT_FALSE ( IpAddress ( "1::3::5::7:8" ).isValid() );
    EXPECT_FALSE ( IpAddress ( "1:2::4::6::8" ).isValid() );
}

// @brief Tests isZero() function.
TEST_F ( IpAddressTest, IsZero )
{
    EXPECT_TRUE ( IpAddress ( "0.0.0.0" ).isZero() );
    EXPECT_TRUE ( IpAddress ( "::" ).isZero() );
    EXPECT_TRUE ( IpAddress ( "::ffff:0.0.0.0" ).isZero() );

    EXPECT_FALSE ( IpAddress().isZero() );
    EXPECT_FALSE ( IpAddress ( "1.2.3.4" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "0.0.0.1" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "1.0.0.0" ).isZero() );

    EXPECT_FALSE ( IpAddress ( "::1" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "1::" ).isZero() );

    EXPECT_FALSE ( IpAddress ( "::ffff:0.0.0.1" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "::ffff:1.0.0.0" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "1::ffff:0.0.0.0" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "::1:ffff:0.0.0.0" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "::fff:1.0.0.0" ).isZero() );
    EXPECT_FALSE ( IpAddress ( "::ff0:1.0.0.0" ).isZero() );
}

/// @brief Tests that IPv4 conversion from IP address to prefix is correct.
TEST_F ( IpAddressTest, V4NetmaskToPrefix )
{
    IpAddress ipAddr ( "255.255.255.0" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv4() );
    EXPECT_EQ ( 24, ipAddr.toPrefix() );

    IpAddress ipAddr2 ( "255.255.255.1" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv4() );
    EXPECT_EQ ( -1, ipAddr2.toPrefix() );
}

/// @brief Tests that IPv6 conversion from IP address to prefix is correct.
TEST_F ( IpAddressTest, V6NetmaskToPrefix )
{
    IpAddress ipAddr ( "ffff::" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv6() );
    EXPECT_EQ ( 16, ipAddr.toPrefix() );

    IpAddress ipAddr2 ( "ffff::1" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv6() );
    EXPECT_EQ ( -1, ipAddr2.toPrefix() );
}

/// @brief Tests that IPv4 conversion from prefix to IP address is correct.
TEST_F ( IpAddressTest, V4PrefixToNetmask )
{
    const IpAddress ipAddr = IpAddress::Ipv4ZeroAddress.getNetmaskAddress ( 24 );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv4() );

    const IpAddress otherAddr ( "255.255.255.0" );

    EXPECT_TRUE ( otherAddr.isValid() );
    EXPECT_TRUE ( otherAddr.isIPv4() );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( ipAddr.toString().c_str(), otherAddr.toString().c_str() );
}

/// @brief Tests that IPv6 conversion from prefix to IP address is correct.
TEST_F ( IpAddressTest, V6PrefixToNetmask )
{
    const IpAddress ipAddr = IpAddress::Ipv6ZeroAddress.getNetmaskAddress ( 16 );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv6() );

    const IpAddress otherAddr ( "ffff::" );

    EXPECT_TRUE ( otherAddr.isValid() );
    EXPECT_TRUE ( otherAddr.isIPv6() );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( ipAddr.toString().c_str(), otherAddr.toString().c_str() );

    const IpAddress ipAddr2 = IpAddress::Ipv6ZeroAddress.getNetmaskAddress ( 31 );

    EXPECT_TRUE ( ipAddr2.isValid() );
    EXPECT_TRUE ( ipAddr2.isIPv6() );

    const IpAddress otherAddr2 ( "ffff:fffe::" );

    EXPECT_TRUE ( otherAddr2.isValid() );
    EXPECT_TRUE ( otherAddr2.isIPv6() );

    EXPECT_STREQ ( ipAddr2.toString().c_str(), otherAddr2.toString().c_str() );
}

/// @brief Tests that IPv4 network calculation is correct.
TEST_F ( IpAddressTest, V4Netmasking )
{
    const IpAddress goodNetAddr ( "192.168.33.0" );
    const IpAddress ipAddr ( "192.168.33.55" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv4() );

    const IpAddress netAddr = ipAddr.getNetworkAddress ( 24 );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( goodNetAddr.toString().c_str(), netAddr.toString().c_str() );
}

/// @brief Tests that IPv6 network calculation is correct.
TEST_F ( IpAddressTest, V6Netmasking )
{
    const IpAddress goodNetAddr ( "2001::" );
    const IpAddress ipAddr ( "2001::cc" );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv6() );

    const IpAddress netAddr = ipAddr.getNetworkAddress ( 24 );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( goodNetAddr.toString().c_str(), netAddr.toString().c_str() );
}

/// @brief Tests that IPv6 incrementBy function works
TEST_F ( IpAddressTest, V6IncrementBy )
{
    IpAddress ipAddr ( "2001::1" );

    ipAddr.incrementBy ( 1 );

    EXPECT_TRUE ( ipAddr.isValid() );

    IpAddress goodAddr ( "2001::2" );

    EXPECT_STREQ ( goodAddr.toString().c_str(), ipAddr.toString().c_str() );

    ipAddr = "2001::ffff";

    ipAddr.incrementBy ( 1 );

    EXPECT_TRUE ( ipAddr.isValid() );

    goodAddr = "2001::1:0";

    EXPECT_STREQ ( goodAddr.toString().c_str(), ipAddr.toString().c_str() );

    ipAddr = "2001::ffff";

    ipAddr.incrementBy ( 2 );

    EXPECT_TRUE ( ipAddr.isValid() );

    goodAddr = "2001::1:1";

    EXPECT_STREQ ( goodAddr.toString().c_str(), ipAddr.toString().c_str() );

    ipAddr = "2001::eeff";

    ipAddr.incrementBy ( 1 );

    EXPECT_TRUE ( ipAddr.isValid() );

    goodAddr = "2001::ef00";

    EXPECT_STREQ ( goodAddr.toString().c_str(), ipAddr.toString().c_str() );
}

/// @brief Tests that IPv4 to IPv6 mapped network conversion is correct.
TEST_F ( IpAddressTest, V4ToV6Mapped )
{
    const IpAddress goodAddr ( "::ffff:192.168.5.5" );
    IpAddress ipAddr ( "192.168.5.5" );

    EXPECT_TRUE ( goodAddr.isValid() );
    EXPECT_TRUE ( goodAddr.isIPv6() );
    EXPECT_TRUE ( goodAddr.isIPv6MappedIPv4() );

    EXPECT_TRUE ( ipAddr.isValid() );
    EXPECT_TRUE ( ipAddr.isIPv4() );

    EXPECT_TRUE ( ipAddr.convertToV4MappedV6() );
    EXPECT_TRUE ( ipAddr.isIPv6MappedIPv4() );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( goodAddr.toString().c_str(), ipAddr.toString().c_str() );
}

// @brief Tests that IPv6 'is link local' function works.
TEST_F ( IpAddressTest, V6LinkLocal )
{
    IpAddress ipAddr ( "fe80::fa32:e4ff:fe70:1348" );

    EXPECT_TRUE ( ipAddr.isIPv6() );
    EXPECT_TRUE ( ipAddr.isLinkLocal() );

    ipAddr = "ff80::fa32:e4ff:fe70:1348";

    EXPECT_TRUE ( ipAddr.isIPv6() );
    EXPECT_FALSE ( ipAddr.isLinkLocal() );
}

// @brief Tests that IPv4 'is link local' function works.
TEST_F ( IpAddressTest, V4LinkLocal )
{
    // IPv4 link-local addresses use 169.254.0.0/16 prefix (RFC 3927)

    IpAddress ipAddr ( "169.254.1.2" );

    EXPECT_TRUE ( ipAddr.isIPv4() );
    EXPECT_TRUE ( ipAddr.isLinkLocal() );

    ipAddr = "169.253.1.2";

    EXPECT_TRUE ( ipAddr.isIPv4() );
    EXPECT_FALSE ( ipAddr.isLinkLocal() );
}

/// @brief Tests handling of IPv6 brackets
TEST_F ( IpAddressTest, V6Brackets )
{
    IpAddress addr ( "2001:1978:1300:19:3:1:2:3" );

    EXPECT_TRUE ( addr.isValid() );
    EXPECT_TRUE ( addr.isIPv6() );

    // Use STREQ so that it displays what the string values are (if it fails)
    EXPECT_STREQ ( "2001:1978:1300:19:3:1:2:3", addr.toString().c_str() );
    EXPECT_STREQ ( "[2001:1978:1300:19:3:1:2:3]", addr.toString ( true ).c_str() );

    addr = "[2001:1978:1300:19:3:1:4:5]";

    EXPECT_TRUE ( addr.isValid() );
    EXPECT_TRUE ( addr.isIPv6() );

    EXPECT_STREQ ( "2001:1978:1300:19:3:1:4:5", addr.toString().c_str() );
    EXPECT_STREQ ( "[2001:1978:1300:19:3:1:4:5]", addr.toString ( true ).c_str() );
}
