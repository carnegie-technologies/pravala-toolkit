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
#include "net/UdpPacket.hpp"

using namespace Pravala;

/// @brief UdpPacket tests
class UdpPacketTest: public ::testing::Test, public ::testing::WithParamInterface<bool>
{
    public:
        const bool UseV6; ///< True if we should use IPv6 for the addresses generated in each test.

        /// @brief Default constructor.
        inline UdpPacketTest(): UseV6 ( GetParam() )
        {
        }
};

TEST_P ( UdpPacketTest, UdpPacketCreateInvalid )
{
    // Test that we cannot create a packet with at least one invalid IP address:

    UdpPacket p ( UseV6 ? "::" : "127.0.0.1", 1, "", 2 );

    EXPECT_FALSE ( p.isValid() );
}

TEST_P ( UdpPacketTest, UdpPacketCreation )
{
    const char str[] = "ABCDEF";

    MemHandle payload ( sizeof ( str ) );

    memcpy ( payload.getWritable(), str, sizeof ( str ) );

    IpAddress sAddr;
    IpAddress dAddr;

    IpPacket pCopy;

    // Test that we can create a packet with arbitrary content and the content survives setting that memory
    // in a brand new IpPacket. Testing 1000 values takes about 13 milliseconds.

    for ( uint32_t idx = 0; idx < 1000; ++idx )
    {
        // Use some arbitrary values.
        const String srcAddr = UseV6 ? "::1" : "127.0.0.1";
        const String dstAddr = UseV6 ? "::2" : "127.0.0.2";
        const uint16_t srcPort = idx % 65536;
        const uint16_t dstPort = ( idx + 3 ) % 65536;

        UdpPacket p ( srcAddr, srcPort, dstAddr, dstPort, payload );

        ASSERT_TRUE ( p.isValid() );

        MemHandle mh;

        ASSERT_TRUE ( p.getPacketData().storeContinuous ( mh ) );

        pCopy = IpPacket ( mh );

        EXPECT_TRUE ( pCopy.isValid() );

        MemVector tmpVec;

        ASSERT_TRUE ( pCopy.getProtoPayload<UdpPacket> ( tmpVec ) );

        ASSERT_EQ ( payload.size(), tmpVec.getDataSize() );

        ASSERT_TRUE ( tmpVec.storeContinuous ( mh ) );

        ASSERT_EQ ( payload.size(), mh.size() );

        EXPECT_STREQ ( payload.get(), mh.get() );

        EXPECT_TRUE ( pCopy.getAddr ( sAddr, dAddr ) );

        EXPECT_STREQ ( srcAddr.c_str(), sAddr.toString ( false ).c_str() );
        EXPECT_STREQ ( dstAddr.c_str(), dAddr.toString ( false ).c_str() );

        ASSERT_NE ( ( const UdpPacket::Header * ) 0, pCopy.getProtoHeader<UdpPacket>() );

        EXPECT_EQ ( srcPort, pCopy.getProtoHeader<UdpPacket>()->getSrcPort() );
        EXPECT_EQ ( dstPort, pCopy.getProtoHeader<UdpPacket>()->getDestPort() );
    }
}

INSTANTIATE_TEST_CASE_P ( IPv4, UdpPacketTest, ::testing::Values ( false ) );
INSTANTIATE_TEST_CASE_P ( IPv6, UdpPacketTest, ::testing::Values ( true ) );
