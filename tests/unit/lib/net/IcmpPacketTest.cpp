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
#include "net/IcmpPacket.hpp"

using namespace Pravala;

/// @brief IcmpPacket tests
class IcmpPacketTest: public ::testing::Test, public ::testing::WithParamInterface<bool>
{
    public:
        const bool UseV6; ///< True if we should use IPv6 for the addresses generated in each test.

        /// @brief Default constructor.
        inline IcmpPacketTest(): UseV6 ( GetParam() )
        {
        }
};

TEST_P ( IcmpPacketTest, IcmpPacketCreateInvalid )
{
    // Test that we cannot create a packet with at least one invalid IP address:

    IcmpPacket p ( UseV6 ? "::" : "127.0.0.1", "", 0, 0 );

    EXPECT_FALSE ( p.isValid() );
}

TEST_P ( IcmpPacketTest, IcmpPacketCreation )
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
        const uint8_t type = idx % 256;
        const uint8_t code = ( idx + 3 ) % 256;
        const uint16_t id = ( idx + 5 ) % 65536;
        const uint16_t seq = ( idx + 7 ) % 65536;

        IcmpPacket p ( srcAddr, dstAddr, type, code, id, seq, payload );

        ASSERT_TRUE ( p.isValid() );

        MemHandle mh;

        ASSERT_TRUE ( p.getPacketData().storeContinuous ( mh ) );

        pCopy = IpPacket ( mh );

        EXPECT_TRUE ( pCopy.isValid() );

        MemVector tmpVec;

        ASSERT_TRUE ( pCopy.getProtoPayload<IcmpPacket> ( tmpVec ) );

        ASSERT_EQ ( payload.size(), tmpVec.getDataSize() );

        ASSERT_TRUE ( tmpVec.storeContinuous ( mh ) );

        ASSERT_EQ ( payload.size(), mh.size() );

        EXPECT_STREQ ( payload.get(), mh.get() );

        EXPECT_TRUE ( pCopy.getAddr ( sAddr, dAddr ) );

        EXPECT_STREQ ( srcAddr.c_str(), sAddr.toString ( false ).c_str() );
        EXPECT_STREQ ( dstAddr.c_str(), dAddr.toString ( false ).c_str() );

        ASSERT_NE ( ( const IcmpPacket::Header * ) 0, pCopy.getProtoHeader<IcmpPacket>() );

        EXPECT_EQ ( type, pCopy.getProtoHeader<IcmpPacket>()->type );
        EXPECT_EQ ( code, pCopy.getProtoHeader<IcmpPacket>()->code );
        EXPECT_EQ ( id, pCopy.getProtoHeader<IcmpPacket>()->getId() );
        EXPECT_EQ ( seq, pCopy.getProtoHeader<IcmpPacket>()->getSequence() );
    }
}

INSTANTIATE_TEST_CASE_P ( IPv4, IcmpPacketTest, ::testing::Values ( false ) );
INSTANTIATE_TEST_CASE_P ( IPv6, IcmpPacketTest, ::testing::Values ( true ) );
