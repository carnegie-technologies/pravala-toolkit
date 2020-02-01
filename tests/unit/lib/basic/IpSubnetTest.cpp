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

#include "basic/IpSubnet.hpp"
#include "basic/String.hpp"

using namespace Pravala;

/// @brief Tests the address/netmask conversion methods in IpAddress
class IpSubnetTest: public ::testing::Test
{
};

/// @brief IPv4 string conversion test
TEST_F ( IpSubnetTest, IPv4StringConversion )
{
    IpSubnet subNet;

    EXPECT_TRUE ( subNet.setFromString ( "0.0.0.0/0" ) );
    EXPECT_TRUE ( subNet.setFromString ( "0.0.0.0/1" ) );
    EXPECT_TRUE ( subNet.setFromString ( "128.0.0.0/1" ) );
    EXPECT_TRUE ( subNet.setFromString ( "192.0.0.0/2" ) );
    EXPECT_TRUE ( subNet.setFromString ( "255.0.0.0/8" ) );
    EXPECT_TRUE ( subNet.setFromString ( "100.255.0.0/16" ) );
    EXPECT_TRUE ( subNet.setFromString ( "255.255.255.255/32" ) );

    EXPECT_FALSE ( subNet.setFromString ( "128.0.0.0/0" ) );
    EXPECT_FALSE ( subNet.setFromString ( "64.0.0.0/1" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0.128.0.0/8" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0.0.128.0/16" ) );

    EXPECT_FALSE ( subNet.setFromString ( "0.0.0.0" ) );
    EXPECT_FALSE ( subNet.setFromString ( "1.2.3.4" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0.0.0.0/" ) );
    EXPECT_FALSE ( subNet.setFromString ( "/8" ) );
    EXPECT_FALSE ( subNet.setFromString ( "/" ) );

    EXPECT_FALSE ( subNet.setFromString ( "abc/8" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0.0.0./8" ) );
    EXPECT_FALSE ( subNet.setFromString ( "0.0.0.0/a" ) );
}

/// @brief IPv6 string conversion test
TEST_F ( IpSubnetTest, IPv6StringConversion )
{
    IpSubnet subNet;

    EXPECT_TRUE ( subNet.setFromString ( "::/0" ) );
    EXPECT_TRUE ( subNet.setFromString ( "::/1" ) );
    EXPECT_TRUE ( subNet.setFromString ( "::/8" ) );
    EXPECT_TRUE ( subNet.setFromString ( "::/32" ) );
    EXPECT_TRUE ( subNet.setFromString ( "::/64" ) );
    EXPECT_TRUE ( subNet.setFromString ( "::/128" ) );

    EXPECT_TRUE ( subNet.setFromString ( "::0/8" ) );

    /// @todo TODO: More tests needed.
}
