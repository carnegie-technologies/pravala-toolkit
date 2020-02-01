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

using namespace Pravala;

/// @brief Tests SockAddr class.
class SockAddrTest: public ::testing::Test
{
};

/// @brief Tests built-in SockAddr addresses
TEST_F ( SockAddrTest, IPvBuiltIns )
{
    EXPECT_FALSE ( EmptySockAddress.hasIpAddr() );
    EXPECT_FALSE ( EmptySockAddress.isIPv4() );
    EXPECT_FALSE ( EmptySockAddress.isIPv6() );

    EXPECT_TRUE ( Ipv4ZeroSockAddress.hasIpAddr() );
    EXPECT_TRUE ( Ipv4ZeroSockAddress.isIPv4() );
    EXPECT_FALSE ( Ipv4ZeroSockAddress.isIPv6() );

    EXPECT_TRUE ( Ipv6ZeroSockAddress.hasIpAddr() );
    EXPECT_TRUE ( Ipv6ZeroSockAddress.isIPv6() );
    EXPECT_FALSE ( Ipv6ZeroSockAddress.isIPv4() );
}

/// @brief Tests operator==()
TEST_F ( SockAddrTest, Equality )
{
    EXPECT_TRUE ( SockAddr() == SockAddr() );
    EXPECT_TRUE ( SockAddr ( Ipv4ZeroSockAddress ) == SockAddr ( Ipv4ZeroSockAddress ) );
    EXPECT_TRUE ( SockAddr ( Ipv6ZeroSockAddress ) == SockAddr ( Ipv6ZeroSockAddress ) );

    EXPECT_FALSE ( SockAddr() == SockAddr ( Ipv4ZeroSockAddress ) );
    EXPECT_FALSE ( SockAddr() == SockAddr ( Ipv6ZeroSockAddress ) );

    EXPECT_FALSE ( SockAddr ( Ipv4ZeroSockAddress ) == SockAddr ( Ipv6ZeroSockAddress ) );

    EXPECT_TRUE ( SockAddr ( IpAddress ( "192.168.0.1" ), 15 ) == SockAddr ( IpAddress ( "192.168.0.1" ), 15 ) );

    EXPECT_FALSE ( SockAddr ( IpAddress ( "192.168.0.1" ), 15 ) == SockAddr ( IpAddress ( "192.168.0.1" ), 16 ) );
    EXPECT_FALSE ( SockAddr ( IpAddress ( "192.168.0.1" ), 15 ) == SockAddr ( IpAddress ( "192.168.0.2" ), 15 ) );
}

/// @brief Tests isEquivalent
TEST_F ( SockAddrTest, IsEquivalent )
{
    IpAddress a = "1.2.3.4";
    IpAddress b = a;

    b.convertToV4MappedV6();

    SockAddr sA ( a, 1234 );
    SockAddr sB ( b, 1234 );

    EXPECT_FALSE ( sA == sB );
    EXPECT_TRUE ( sA.isEquivalent ( sB ) );
    EXPECT_TRUE ( sB.isEquivalent ( sA ) );
}
