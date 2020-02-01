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
/// @brief Contains ICMP-related functions
class IcmpPacket: public IpPacket
{
    public:
        static const Proto::Number ProtoNumber = Proto::ICMP;

#pragma pack(push, 1)
        struct Header
        {
            uint8_t type;
            uint8_t code;
            uint16_t checksum;
            uint16_t id;
            uint16_t sequence;

            inline uint16_t getChecksum() const
            {
                return ntohs ( checksum );
            }

            inline uint16_t getId() const
            {
                return ntohs ( id );
            }

            inline uint16_t getSequence() const
            {
                return ntohs ( sequence );
            }

            inline uint8_t getHeaderSize() const
            {
                return sizeof ( Header );
            }
        };

#pragma pack(pop)

        /// @brief Creates a new ICMP packet
        /// This packet should be valid for sending.
        /// It uses the IpPacket(srcAddr, destAddr, Proto::ICMP, ...) constructor,
        /// so for the description of the IP fields set look at that constructor's comment.
        /// After configuring this ICMP packet the checksum is calculated, so this packet should be ready
        /// for sending.
        /// Any other modifications (through Header's set*() functions) will adjust the checksum accordingly.
        /// @param [in] srcAddr The source address to set
        /// @param [in] destAddr The destination address to set. Has to be of the same size as the source address
        /// @param [in] icmpType The 'type' of the ICMP packet
        /// @param [in] icmpCode The 'code' of the ICMP packet
        /// @param [in] icmpId The 'id' of the ICMP packet (used by some of the messages)
        /// @param [in] icmpSequence The 'sequence' of the ICMP packet (used by some of the messages)
        /// @param [in] payload The payload data to store in the packet
        IcmpPacket (
            const IpAddress & srcAddr, const IpAddress & destAddr,
            uint8_t icmpType, uint8_t icmpCode,
            uint16_t icmpId = 0, uint16_t icmpSequence = 0,
            const MemVector & payload = MemVector::EmptyVector );

        static void describe ( const IpPacket & ipPacket, Buffer & toBuffer );
};
}
