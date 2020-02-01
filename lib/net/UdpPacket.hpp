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

#pragma once

#include "IpPacket.hpp"

namespace Pravala
{
/// @brief Contains UDP-related functions
class UdpPacket: public IpPacket
{
    public:
        static const Proto::Number ProtoNumber = Proto::UDP;

#pragma pack(push, 1)
        struct Header
        {
            uint16_t source_port;
            uint16_t dest_port;
            uint16_t length;
            uint16_t checksum;

            inline uint16_t getSrcPort() const
            {
                return ntohs ( source_port );
            }

            inline uint16_t getDestPort() const
            {
                return ntohs ( dest_port );
            }

            inline uint16_t getLength() const
            {
                return ntohs ( length );
            }

            inline uint16_t getChecksum() const
            {
                return ntohs ( checksum );
            }

            inline uint8_t getHeaderSize() const
            {
                return sizeof ( Header );
            }

            /// @brief Sets the source port in the header.
            /// @param [in] sourcePort The new source port.
            /// @note It will adjust the checksum when run.
            void setSrcPort ( uint16_t sourcePort );

            /// @brief Sets the destination port in the header.
            /// @param [in] destPort The new destination port.
            /// @note It will adjust the checksum when run.
            void setDestPort ( uint16_t destPort );
        };

#pragma pack(pop)

        /// @brief Creates a new UDP packet.
        /// This packet should be valid for sending.
        /// It uses the IpPacket(srcAddr, destAddr, Proto::UDP, ...) constructor,
        /// so for the description of the IP fields set look at that constructor's comment.
        /// After configuring this UDP packet the checksum is calculated, so this packet should be ready
        /// for sending.
        /// Any other modifications (through Header's set*() functions) will adjust the checksum accordingly.
        /// @param [in] srcAddr The source address to set
        /// @param [in] srcPort The source port to set
        /// @param [in] destAddr The destination address to set. Has to be of the same size as the source address
        /// @param [in] destPort The destination port to set
        /// @param [in] payload The data to be used as this packet's payload.
        /// @param [in] tos Value for the DSCP/ECN (IPv4) or Traffic Class (IPv6) field in the IpPacket
        /// @param [in] ttl Value for the TTL (IPv4) or Hop Limit (IPv6) field in the IpPacket
        UdpPacket (
            const IpAddress & srcAddr, uint16_t srcPort,
            const IpAddress & destAddr, uint16_t destPort,
            const MemVector & payload = MemVector::EmptyVector,
            uint8_t tos = 0, uint8_t ttl = 255 );

        /// @brief Appends the description of a packet to the buffer.
        /// @param [in] ipPacket The IP packet to describe.
        /// @param [in,out] toBuffer The buffer to append the description to. It is not cleared first.
        static void describe ( const IpPacket & ipPacket, Buffer & toBuffer );
};

/// @brief Streaming operator
/// Appends an UdpPacket's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value UdpPacket for which description should be appended
/// @return Reference to the TextMessage object
inline TextMessage & operator<< ( TextMessage & textMessage, const UdpPacket & value )
{
    return operator<< ( textMessage, ( const IpPacket & ) value );
}
}
