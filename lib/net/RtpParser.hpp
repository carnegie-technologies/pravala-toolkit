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

// IpPacket could simply be a forward declaration here,
// but this also pulls some other headers we need here (stdint.h, and network headers for ntohl and similar).

#include "IpPacket.hpp"

namespace Pravala
{
/// @brief Contains RTP-related functions
class RtpParser
{
    public:
#pragma pack(push, 1)
        /// @brief Describes an RTP header.
        struct Header
        {
            uint8_t flags; ///< Various flags.
            uint8_t payloadType; ///< Payload type and a marker bit
            uint16_t seqNumber; ///< Sequence number
            uint32_t timestamp; ///< The timestamp.
            uint32_t ssrcId; ///< Synchronization source identifier.

            /// @brief Returns the version of the RTP protocol.
            /// Current version is 2.
            /// @return The version of the protocol.
            inline uint8_t getVersion() const
            {
                return ( flags >> 6 ) & 0x03;
            }

            /// @brief Returns the number of CSRC identifiers that follow the fixed header.
            /// @return The number of CSRC identifiers that follow the fixed header.
            inline uint8_t getCsrcCount() const
            {
                return ( flags & 0x0F );
            }

            /// @brief Checks if there are extra padding bytes at the end of the RTP packet.
            /// @return True if there are extra padding bytes at the end of the RTP packet; False otherwise.
            inline bool hasPadding() const
            {
                return ( flags & 0x20 ) == 0x20;
            }

            /// @brief Checks if there is an extension header between standard header and payload data.
            /// @return True if there is an extension header between standard header and payload data; False otherwise.
            inline bool hasExtension() const
            {
                return ( flags & 0x10 ) == 0x10;
            }

            /// @brief Checks if the marker bit is set.
            /// Marker is application specific.
            /// @return True if the marker bit is set; False otherwise.
            inline bool hasMarker() const
            {
                return ( payloadType & 0x80 ) == 0x80;
            }

            /// @brief Returns the RTP payload type.
            /// @return The RTP payload type.
            inline uint8_t getPayloadType() const
            {
                return ( payloadType & 0x7F );
            }

            /// @brief Returns the sequence number.
            /// @return The sequence number.
            inline uint16_t getSeqNum() const
            {
                return ntohs ( seqNumber );
            }

            /// @brief Returns the timestamp.
            /// @return The timestamp.
            inline uint32_t getTimestamp() const
            {
                return ntohl ( timestamp );
            }

            /// @brief Returns the synchronization source identifier.
            /// Synchronization source identifier uniquely identifies the source of a stream.
            /// The synchronization sources within the same RTP session will be unique.
            /// @return The synchronization source identifier.
            inline uint32_t getSsrcId() const
            {
                return ntohl ( ssrcId );
            }
        };

#pragma pack(pop)

        /// @brief Different RTP packets that the parser recognizes.
        enum PacketType
        {
            TypeInvalid,      ///< Not an RTPv2 packet. Too short header, or a wrong version.
            TypeInvalidCodec, ///< Looks like an RTP packet, but the codec type seems invalid.
            TypeAudio,        ///< RTP packet, one of the known audio codecs.
            TypeVideo,        ///< RTP packet, one of the known video codecs.
            TypeAudioVideo,   ///< RTP packet, one of the known audio/video codecs.
            TypeDynamic,      ///< RTP packet, the type is from the dynamic range.
            TypeRtcp          ///< RTCP packet. In that case the type will be set to the RTCP type.
        };

        /// @brief Tries to parse given IP packet as an RTP UDP packet.
        /// It also verifies it the RTP protocol version is 2.
        /// If it isn't, the data is not considered an RTP packet.
        /// @note The output parameters might be set even if this function returns an error.
        /// @param [in] packet The IP packet to examine.
        /// @param [out] payloadType The RTP payload type extracted.
        ///                          Note that if this returns TypeRtcp, the payload type will be set to
        ///                          one of the RTCP types in 200-204 range (which is not possible for RTP packets).
        /// @param [out] ssrcId The RTP synchronization source identifier extracted.
        /// @param [out] timestamp The RTP timestamp extracted.
        /// @param [out] seqNum The RTP sequence number extracted.
        /// @return The packet type detected.
        static PacketType parseRtp (
            const IpPacket & packet,
            uint8_t & payloadType,
            uint32_t & ssrcId,
            uint32_t & timestamp,
            uint16_t & seqNum );
};
}
