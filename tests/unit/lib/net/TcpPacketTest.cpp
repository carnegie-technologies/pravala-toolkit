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
#include "net/TcpPacket.hpp"

using namespace Pravala;

/// @brief TcpPacket tests
class TcpPacketTest: public ::testing::Test, public ::testing::WithParamInterface<bool>
{
    public:
        const bool UseV6; ///< True if we should use IPv6 for the addresses generated in each test.

        /// @brief Default constructor.
        inline TcpPacketTest(): UseV6 ( GetParam() )
        {
        }
};

TEST_P ( TcpPacketTest, TcpPacketCreateInvalid )
{
    // Test that we cannot create a packet with at least one invalid IP address:

    TcpPacket p ( UseV6 ? "::" : "127.0.0.1", 1, "", 2, TcpPacket::FlagSyn, 0, 0 );

    EXPECT_FALSE ( p.isValid() );
}

TEST_P ( TcpPacketTest, TcpPacketCreation )
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
        const uint8_t flags = ( idx + 5 ) % 256;
        const uint32_t seqNum = idx + 7;
        const uint32_t ackNum = ( ( flags & TcpPacket::FlagAck ) != 0 ) ? ( idx + 11 ) : 0;
        const uint16_t window = ( idx + 13 ) % 65536;

        TcpPacket p ( srcAddr, srcPort, dstAddr, dstPort, flags, seqNum, ackNum, window, payload );

        ASSERT_TRUE ( p.isValid() );

        MemHandle mh;

        ASSERT_TRUE ( p.getPacketData().storeContinuous ( mh ) );

        pCopy = IpPacket ( mh );

        EXPECT_TRUE ( pCopy.isValid() );

        MemVector tmpVec;

        ASSERT_TRUE ( pCopy.getProtoPayload<TcpPacket> ( tmpVec ) );

        ASSERT_EQ ( payload.size(), tmpVec.getDataSize() );

        ASSERT_TRUE ( tmpVec.storeContinuous ( mh ) );

        ASSERT_EQ ( payload.size(), mh.size() );

        EXPECT_STREQ ( payload.get(), mh.get() );

        EXPECT_TRUE ( pCopy.getAddr ( sAddr, dAddr ) );

        EXPECT_STREQ ( srcAddr.c_str(), sAddr.toString ( false ).c_str() );
        EXPECT_STREQ ( dstAddr.c_str(), dAddr.toString ( false ).c_str() );

        ASSERT_NE ( ( const TcpPacket::Header * ) 0, pCopy.getProtoHeader<TcpPacket>() );

        EXPECT_EQ ( srcPort, pCopy.getProtoHeader<TcpPacket>()->getSrcPort() );
        EXPECT_EQ ( dstPort, pCopy.getProtoHeader<TcpPacket>()->getDestPort() );
        EXPECT_EQ ( flags, pCopy.getProtoHeader<TcpPacket>()->flags );
        EXPECT_EQ ( seqNum, pCopy.getProtoHeader<TcpPacket>()->getSeqNum() );
        EXPECT_EQ ( ackNum, pCopy.getProtoHeader<TcpPacket>()->getAckNum() );
        EXPECT_EQ ( window, pCopy.getProtoHeader<TcpPacket>()->getWindow() );
    }
}

INSTANTIATE_TEST_CASE_P ( IPv4, TcpPacketTest, ::testing::Values ( false ) );
INSTANTIATE_TEST_CASE_P ( IPv6, TcpPacketTest, ::testing::Values ( true ) );
