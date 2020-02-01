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

#include "RtpParser.hpp"
#include "UdpPacket.hpp"

using namespace Pravala;

RtpParser::PacketType RtpParser::parseRtp (
        const IpPacket & packet, uint8_t & payloadType, uint32_t & ssrcId, uint32_t & timestamp, uint16_t & seqNum )
{
    MemVector udpPayload;

    if ( !packet.getProtoPayload<UdpPacket> ( udpPayload ) || udpPayload.getDataSize() <= sizeof ( Header ) )
    {
        return TypeInvalid;
    }

    // If the RTP header is split between different chunks, this will fail.
    // For now, we don't care.

    assert ( udpPayload.getNumChunks() > 0 );

    const struct iovec * chunks = udpPayload.getChunks();
    const Header * const rtpHeader = static_cast<const Header *> ( chunks[ 0 ].iov_base );

    if ( chunks[ 0 ].iov_len < sizeof ( Header ) || !rtpHeader || rtpHeader->getVersion() != 2 )
    {
        return TypeInvalid;
    }

    payloadType = rtpHeader->getPayloadType();
    ssrcId = rtpHeader->getSsrcId();
    timestamp = rtpHeader->getTimestamp();
    seqNum = rtpHeader->getSeqNum();

    // These types are based on:
    // https://www.ietf.org/assignments/rtp-parameters/rtp-parameters.xml#rtp-parameters-1

    if ( payloadType == 0 || ( payloadType >= 3 && payloadType <= 18 ) )
    {
        // Known audio codecs
        return TypeAudio;
    }

    if ( payloadType >= 96 && payloadType <= 127 )
    {
        // 96-127 is the range for dynamic IDs
        return TypeDynamic;
    }

    if ( payloadType == 33 )
    {
        // 33 is the only audio-video codec
        return TypeAudioVideo;
    }

    if ( payloadType >= 25 && payloadType <= 34 )
    {
        // 25-34 (except 33 handled above) are video codecs
        return TypeVideo;
    }

    if ( payloadType >= 72 && payloadType <= 76 && rtpHeader->hasMarker() )
    {
        // This is an RTCP packet.
        // RTCP doesn't use the marker field, but full 8 bits for the payload type.
        // The payload types used by RTCP are in the 200-204 range, which means that if we only look
        // at the 7 bits that are used by the RTP, it will result in types 72-76.
        // So this is an RTCP packet, but only if the marker bit is set.
        // If the marker bit is not set, it is an invalid packet, becadatause RTP packets are not allowed
        // to use that range (72-76 IDs are reserved by IETF for RTCP conflict avoidance).

        // We want to return the RTCP type, which uses one extra bit (the same one as the RTP marker).
        // Since we already know that the marker is set, we simply add that bit:

        payloadType |= 0x80;

        return TypeRtcp;
    }

    return TypeInvalidCodec;
}
