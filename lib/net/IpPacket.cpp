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

#include <cassert>

#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"
#include "basic/IpAddress.hpp"
#include "socket/PacketDataStore.hpp"

#include "IpPacket.hpp"
#include "TcpPacket.hpp"
#include "UdpPacket.hpp"
#include "IcmpPacket.hpp"
#include "FlowDesc.hpp"
#include "IpHeaders.hpp"
#include "IpChecksum.hpp"

#define CASE_PROTO_NAME( proto ) \
    case Proto::proto: \
        return #proto; break

using namespace Pravala;

/// @brief A helper union to handle both IPv4 and IPv6 headers using a single pointer.
union DualIpHeader
{
    struct ip v4;      ///< IPv4 header.
    struct ip6_hdr v6; ///< IPv6 header.
};

const size_t IpPacket::IPv4HeaderSize ( sizeof ( struct ip ) );
const size_t IpPacket::IPv6HeaderSize ( sizeof ( struct ip6_hdr ) );

TextLog IpPacket::_log ( "ip_packet" );

IpPacket::IpPacket()
{
}

IpPacket::IpPacket ( const MemHandle & data )
{
    if ( data.isEmpty() )
        return;

    // To make sure the alignment is correct.
    if ( ( ( size_t ) data.get() ) % 4U != 0 )
    {
        LOG ( L_FATAL_ERROR, "Unaligned IP memory received!" );

        assert ( false );

        return;
    }

    // We need at least base IPv4 header (w/o options) to determine anything about the packet,
    // including saying whether it's an IPv4 or IPv6 packet.

    if ( data.size() < sizeof ( struct ip ) )
    {
        LOG ( L_ERROR, "Packet is too small (" << data.size() << "B)" );
        return;
    }

    const DualIpHeader * ipHdrPtr = reinterpret_cast<const DualIpHeader *> ( data.get() );

    size_t packetSize = 0;

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        // ip_hl is the length of IP header in 4-byte words. The minimum length is 20 bytes (5 words).
        // If ip_hl is set to anything less than 5, the data is wrong.
        if ( ipHdrPtr->v4.ip_hl < 5 )
        {
            LOG ( L_ERROR, "IPv4 header length (" << ipHdrPtr->v4.ip_hl << ") < 5 words" );
            return;
        }

        packetSize = ntohs ( ipHdrPtr->v4.ip_len );

        if ( packetSize < sizeof ( struct ip ) )
        {
            LOG ( L_ERROR, "Packet with invalid IP header size set (" << packetSize << "B)" );
            return;
        }
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        if ( data.size() < sizeof ( struct ip6_hdr ) )
        {
            LOG ( L_ERROR, "IPv6 packet is too small (" << data.size() << "B)" );
            return;
        }

        if ( ipHdrPtr->v6.ip6_plen == 0 )
        {
            LOG ( L_ERROR, "Unsupported IPv6 Jumbo packet received" );
            return;
        }

        packetSize = ntohs ( ipHdrPtr->v6.ip6_plen ) + sizeof ( struct ip6_hdr );
    }
    else
    {
        LOG ( L_ERROR, "Unsupported IPv? packet received: " << ipHdrPtr->v4.ip_v );
        return;
    }

    if ( data.size() < packetSize )
    {
        LOG ( L_ERROR, "Incomplete IP packet received; Required " << packetSize
              << "B; Received: " << data.size() << "B" );
        return;
    }

    if ( data.size() > packetSize )
    {
        // This buffer contains something after the IP packet...
        _buffer = data.getHandle ( 0, packetSize );
    }
    else
    {
        _buffer = data;
    }
}

char * IpPacket::initProtoPacket (
        const IpAddress & srcAddr, const IpAddress & destAddr,
        Proto::Number payloadProto, uint16_t payloadHdrSize, const MemVector & payloadData,
        uint8_t tos, uint8_t ttl )
{
    if ( payloadHdrSize < 1 )
    {
        LOG ( L_ERROR, "Payload header size cannot be 0" );
        return 0;
    }

    uint16_t hdrSize = 0;

    if ( srcAddr.isIPv4() && destAddr.isIPv4() )
    {
        hdrSize = sizeof ( struct ip );
    }
    else if ( srcAddr.isIPv6() && destAddr.isIPv6() )
    {
        hdrSize = sizeof ( struct ip6_hdr );
    }
    else
    {
        LOG ( L_ERROR, "Invalid source-dest address configuration specified ("
              << srcAddr << " - " << destAddr << ")" );
        return 0;
    }

    const size_t totalSize = hdrSize + payloadHdrSize + payloadData.getDataSize();

    if ( totalSize > 0xFFFF )
    {
        LOG ( L_ERROR, "Too large IpPacket: " << totalSize << "B; IP header: " << hdrSize
              << "B; Proto (" << getProtoName ( payloadProto ) << ") header: " << payloadHdrSize
              << "B; Payload size: " << payloadData.getDataSize() << "B" );
        return 0;
    }

    {
        MemHandle hdrData ( PacketDataStore::getPacket ( hdrSize + payloadHdrSize ) );

        hdrData.truncate ( hdrSize + payloadHdrSize );

        if ( hdrData.size() < hdrSize + payloadHdrSize )
        {
            LOG ( L_ERROR, "Too small header buffer generated (" << hdrData.size() << "); IP header: " << hdrSize
                  << "B; Proto (" << getProtoName ( payloadProto ) << ") header: " << payloadHdrSize
                  << "B; Payload size: " << payloadData.getDataSize() << "B" );
            return 0;
        }

        // Let's create an empty buffer with enough slots reserved:
        _buffer = MemVector ( 1 + payloadData.getNumChunks() );

        if ( !_buffer.append ( hdrData ) || !_buffer.append ( payloadData ) )
        {
            LOG ( L_ERROR, "Error appending data to IP buffer" );
            return 0;
        }

        // hdrData goes out-of-scope, so no need to clear it
    }

    char * const hdrMem = _buffer.getContinuousWritable ( hdrSize + payloadHdrSize );

    if ( !hdrMem || ( ( ( size_t ) hdrMem ) % 4U ) != 0 || _buffer.getDataSize() != totalSize )
    {
        // This shouldn't happen...
        LOG ( L_ERROR, "Error configuring IP packet's memory" );

        assert ( false );

        _buffer.clear();
        return 0;
    }

    assert ( ( ( size_t ) hdrMem ) % 4U == 0 );

    memset ( hdrMem, 0, hdrSize );

    DualIpHeader * const ipHdrPtr = reinterpret_cast<DualIpHeader *> ( hdrMem );

    if ( srcAddr.isIPv4() )
    {
        assert ( destAddr.isIPv4() );
        assert ( hdrSize == sizeof ( struct ip ) );

        ipHdrPtr->v4.ip_v = 4;

        // ip_hl is the length of IP header in 4-byte words. The minimum length is 20 bytes (5 words).
        // We don't use additional options, so we use the minimum value.
        ipHdrPtr->v4.ip_hl = 5;

        ipHdrPtr->v4.ip_tos = tos;
        ipHdrPtr->v4.ip_len = htons ( totalSize );
        ipHdrPtr->v4.ip_ttl = ttl;
        ipHdrPtr->v4.ip_p = ( uint8_t ) payloadProto;
        ipHdrPtr->v4.ip_src = srcAddr.getV4();
        ipHdrPtr->v4.ip_dst = destAddr.getV4();
        ipHdrPtr->v4.ip_sum = IpChecksum::getChecksum ( hdrMem, sizeof ( struct ip ) );

        return hdrMem + hdrSize;
    }

    assert ( srcAddr.isIPv6() );
    assert ( destAddr.isIPv6() );
    assert ( hdrSize == sizeof ( struct ip6_hdr ) );

    // Easier to use the bitmasking of the v4 version than to bitmask off ip6_ctlun.ip6_un2_vfc manually.
    // ipHdrPtr->v4 and ipHdrPtr->v6 point to the same memory, and ipHdrPtr->v4.ip_v points to 4 bits in the IP header.
    // Those are the same 4 bits as in IPv6 case, but the IPv6 header doesn't have a convenient field
    // for setting it. IPv4 does (and effectively it sets the exact same bits), so we use that instead:

    ipHdrPtr->v4.ip_v = 6;

    // This field actually "4 bits version, 8 bits TC, 20 bits flow-ID" as per the header.
    // We want to OR it so that we don't overwrite the version that was just set above.
    ipHdrPtr->v6.ip6_flow |= static_cast<uint32_t> ( tos ) << 20;

    ipHdrPtr->v6.ip6_plen = htons ( payloadHdrSize + payloadData.getDataSize() );
    ipHdrPtr->v6.ip6_nxt = ( uint8_t ) payloadProto;
    ipHdrPtr->v6.ip6_hlim = ttl;
    ipHdrPtr->v6.ip6_src = srcAddr.getV6();
    ipHdrPtr->v6.ip6_dst = destAddr.getV6();

    return hdrMem + hdrSize;
}

bool IpPacket::examinePacket ( IpPacket::PacketDesc & pDesc ) const
{
    if ( _buffer.isEmpty() )
    {
        return false;
    }

    assert ( _buffer.getNumChunks() > 0 );

    const struct iovec * const chunks = _buffer.getChunks();

    assert ( chunks != 0 );
    assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( chunks[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        pDesc.protoType = ipHdrPtr->v4.ip_p;

        pDesc.ipHeaderSize = ( 4 * ipHdrPtr->v4.ip_hl );

        assert ( pDesc.ipHeaderSize >= sizeof ( struct ip ) );

        // If the fragment offset is != 0, this IP packet is not the first
        // fragment on an IP packet. So it can't really be treated the same way!
        // Note that ip_off contains also fragmentation flags. To get just the offset,
        // we need to use the IP_OFFMASK mask. This mask has to be applied to the number
        // in local endianness!
        if ( ( ntohs ( ipHdrPtr->v4.ip_off ) & IP_OFFMASK ) != 0 )
        {
            pDesc.protoType |= ProtoBitNextIpFragment;
        }
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        pDesc.ipHeaderSize = sizeof ( struct ip6_hdr );

        // We only consider the first 'next header' field.
        // It could be some extension header(s), followed by the actual IP data.
        // For now, we treat the first 'next header' as the internal protocol number and don't look for the actual data.
        // So, basically, we treat the first extension header as the IP data of the packet,
        // whether it is the actual data or an extension header.

        /// @todo Add support for IPv6 extension headers! Also, the ipHeaderSize will be affected.
        pDesc.protoType = ipHdrPtr->v6.ip6_nxt;
    }
    else
    {
        return false;
    }

    if ( pDesc.ipHeaderSize < chunks[ 0 ].iov_len )
    {
        pDesc.protoHeaderSize = chunks[ 0 ].iov_len - pDesc.ipHeaderSize;
        pDesc.protoHeader = static_cast<const char * > ( chunks[ 0 ].iov_base ) + pDesc.ipHeaderSize;

        return true;
    }

    if ( pDesc.ipHeaderSize > chunks[ 0 ].iov_len || _buffer.getNumChunks() < 2 )
    {
        // IP header size is larger than the first segment, or it is the same, but there are no more segments...
        return false;
    }

    // We have another chunk, protocol header could be as big as that chunk.

    pDesc.protoHeaderSize = chunks[ 1 ].iov_len;
    pDesc.protoHeader = static_cast<const char * > ( chunks[ 1 ].iov_base );

    return true;
}

uint8_t IpPacket::getIpVersion() const
{
    if ( _buffer.isEmpty() )
        return 0;

    assert ( _buffer.getChunks() != 0 );
    assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( _buffer.getChunks()[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    return ipHdrPtr->v4.ip_v;
}

bool IpPacket::getAddr ( IpAddress & srcAddr, IpAddress & dstAddr ) const
{
    if ( _buffer.isEmpty() )
        return false;

    assert ( _buffer.getChunks() != 0 );
    assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( _buffer.getChunks()[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        srcAddr = ipHdrPtr->v4.ip_src;
        dstAddr = ipHdrPtr->v4.ip_dst;

        return true;
    }

    if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        srcAddr = ipHdrPtr->v6.ip6_src;
        dstAddr = ipHdrPtr->v6.ip6_dst;

        return true;
    }

    return false;
}

uint16_t IpPacket::calcPseudoHeaderPayloadChecksum() const
{
    if ( _buffer.isEmpty() )
        return 0;

    const struct iovec * const chunks = _buffer.getChunks();

    assert ( chunks != 0 );
    assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( chunks[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    // Let's calculate the checksum.
    IpChecksum ipChecksum;

    size_t ipHdrSize = 0;

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        // We start with IPv4 "pseudo header". It contains following fields, in order:
        //  srcAddr (4 bytes)
        //  destAddr (4 bytes)
        //  null (1 byte)
        //  protocol number (1 byte)
        //  dataSize (2 bytes), total IP data size (so internal protocol header and payload)

        // Instead of copying data to a new pseudo header structure, we can just use the existing header:

        // The header contains ip_src (4 bytes), followed by ip_dst (4 bytes).
        // We can treat them as a single 8 byte value:
        ipChecksum.addMemory ( &ipHdrPtr->v4.ip_src, sizeof ( ipHdrPtr->v4.ip_src ) + sizeof ( ipHdrPtr->v4.ip_dst ) );

        uint16_t v = 0;

        // null (1), ip_p (1), data_size (2)
        ipChecksum.addMemory ( &v, 1 );
        ipChecksum.addMemory ( &ipHdrPtr->v4.ip_p, 1 );

        ipHdrSize = 4 * ipHdrPtr->v4.ip_hl;

        v = htons ( _buffer.getDataSize() - ipHdrSize );

        ipChecksum.addMemory ( &v, 2 );
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        // We start with IPv6 "pseudo header". It contains following fields, in order:
        //  srcAddr (16 bytes)
        //  dstAddr (16 bytes)
        //  dataSize (4 bytes), data size (internal protocol header and payload, w/o IP header)
        //  zeros (3 bytes)
        //  next header = protocol number (1 byte)

        // again, 'dst' is right after 'src':
        ipChecksum.addMemory (
            &ipHdrPtr->v6.ip6_src, sizeof ( ipHdrPtr->v6.ip6_src ) + sizeof ( ipHdrPtr->v6.ip6_dst ) );

        ipHdrSize = sizeof ( struct ip6_hdr );

        // This is stored in 4 bytes in the pseudo-header (so we use htonl):
        uint32_t v = htonl ( _buffer.getDataSize() - ipHdrSize );

        ipChecksum.addMemory ( &v, 4 );

        // Then 3 zeros and 'ip6_next':
        v = 0;
        ipChecksum.addMemory ( &v, 3 );
        ipChecksum.addMemory ( &ipHdrPtr->v6.ip6_nxt, 1 );
    }
    else
    {
        // Neither IPv4 nor IPv6
        assert ( false );
        return 0;
    }

    assert ( ipHdrSize <= chunks[ 0 ].iov_len );

    ipChecksum.addMemory (
        static_cast<const char *> ( chunks[ 0 ].iov_base ) + ipHdrSize, chunks[ 0 ].iov_len - ipHdrSize );

    for ( size_t i = 1; i < _buffer.getNumChunks(); ++i )
    {
        ipChecksum.addMemory ( static_cast<const char *> ( chunks[ i ].iov_base ), chunks[ i ].iov_len );
    }

    return ipChecksum.getChecksum();
}

bool IpPacket::setupFlowDesc ( FlowDesc & flowDesc, PacketDirection direction ) const
{
    if ( _buffer.isEmpty() )
        return false;

    flowDesc.clear();

    assert ( _buffer.getChunks() != 0 );
    assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( _buffer.getChunks()[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    uint16_t pType = 0;

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        flowDesc.common.type = 4;

        if ( direction == PacketToClient )
        {
            flowDesc.v4.clientAddr = ipHdrPtr->v4.ip_dst;
            flowDesc.v4.serverAddr = ipHdrPtr->v4.ip_src;
        }
        else
        {
            flowDesc.v4.clientAddr = ipHdrPtr->v4.ip_src;
            flowDesc.v4.serverAddr = ipHdrPtr->v4.ip_dst;
        }

        pType = ipHdrPtr->v4.ip_p;

        if ( ( ntohs ( ipHdrPtr->v4.ip_off ) & IP_OFFMASK ) != 0 )
        {
            pType |= ProtoBitNextIpFragment;
        }
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        flowDesc.common.type = 6;

        if ( direction == PacketToClient )
        {
            flowDesc.v6.clientAddr = ipHdrPtr->v6.ip6_dst;
            flowDesc.v6.serverAddr = ipHdrPtr->v6.ip6_src;
        }
        else
        {
            flowDesc.v6.clientAddr = ipHdrPtr->v6.ip6_src;
            flowDesc.v6.serverAddr = ipHdrPtr->v6.ip6_dst;
        }

        pType = ipHdrPtr->v6.ip6_nxt;
    }
    else
    {
        return false;
    }

    flowDesc.common.foo = 0;

    // We mark the protocol, but without the 'IP fragmented' bit
    // So even if the packet is fragmented, we still see the original protocol type,
    // even if ports and other values are 0.
    flowDesc.common.heProto = pType & ( ~ProtoBitNextIpFragment );

    // Here we want the entire pType, even if it contains the ProtoBitNextIpFragment bit set.
    // We can't get the protocol header information if this is not the first fragment.

    switch ( pType )
    {
        case TcpPacket::ProtoNumber:
            {
                const TcpPacket::Header * hdr = getProtoHeader<TcpPacket>();

                if ( !hdr )
                    return false;

                if ( direction == PacketToClient )
                {
                    flowDesc.common.u.port.client = hdr->dest_port;
                    flowDesc.common.u.port.server = hdr->source_port;
                }
                else
                {
                    flowDesc.common.u.port.client = hdr->source_port;
                    flowDesc.common.u.port.server = hdr->dest_port;
                }
            }
            break;

        case UdpPacket::ProtoNumber:
            {
                const UdpPacket::Header * hdr = getProtoHeader<UdpPacket>();

                if ( !hdr )
                    return false;

                if ( direction == PacketToClient )
                {
                    flowDesc.common.u.port.client = hdr->dest_port;
                    flowDesc.common.u.port.server = hdr->source_port;
                }
                else
                {
                    flowDesc.common.u.port.client = hdr->source_port;
                    flowDesc.common.u.port.server = hdr->dest_port;
                }
            }
            break;
    }

    return true;
}

uint16_t IpPacket::getSeed() const
{
    if ( !isValid() )
    {
        return 0;
    }

    assert ( _buffer.getChunks() != 0 );
    assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * const ipHdrPtr = static_cast<const DualIpHeader *> ( _buffer.getChunks()[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    uint16_t pType = 0;
    const uint16_t * addrA = 0;
    const uint16_t * addrB = 0;
    int addrLen = 0;

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip ) );

        addrA = ( const uint16_t * ) &( ipHdrPtr->v4.ip_src );
        addrB = ( const uint16_t * ) &( ipHdrPtr->v4.ip_dst );
        addrLen = sizeof ( ipHdrPtr->v4.ip_src ) / sizeof ( uint16_t );

        assert ( sizeof ( ipHdrPtr->v4.ip_src ) == sizeof ( ipHdrPtr->v4.ip_dst ) );
        assert ( addrLen * sizeof ( uint16_t ) == sizeof ( ipHdrPtr->v4.ip_src ) );

        pType = ipHdrPtr->v4.ip_p;

        if ( ( ntohs ( ipHdrPtr->v4.ip_off ) & IP_OFFMASK ) != 0 )
        {
            pType |= ProtoBitNextIpFragment;
        }
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( _buffer.getChunks()[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        addrA = ( const uint16_t * ) &( ipHdrPtr->v6.ip6_src );
        addrB = ( const uint16_t * ) &( ipHdrPtr->v6.ip6_dst );
        addrLen = sizeof ( ipHdrPtr->v6.ip6_src ) / sizeof ( uint16_t );

        assert ( sizeof ( ipHdrPtr->v6.ip6_src ) == sizeof ( ipHdrPtr->v6.ip6_dst ) );
        assert ( addrLen * sizeof ( uint16_t ) == sizeof ( ipHdrPtr->v6.ip6_src ) );

        pType = ipHdrPtr->v6.ip6_nxt;
    }
    else
    {
        return 0;
    }

    uint16_t portA = 0;
    uint16_t portB = 0;

    // Here we want the entire pType, even if it contains the ProtoBitNextIpFragment bit set.
    // We can't get the protocol header information if this is not the first fragment.
    switch ( pType )
    {
        case Proto::UDP:
            {
                const UdpPacket::Header * hdr = getProtoHeader<UdpPacket>();

                if ( hdr != 0 )
                {
                    // We use getSrcPort and getDestPort instead of accessing these values directly,
                    // because we want network->host endianness conversion.
                    // This way the seed of the same packet will be the same, regardless of the architecture.
                    portA = hdr->getSrcPort();
                    portB = hdr->getDestPort();
                }
            }
            break;

        case Proto::TCP:
            {
                const TcpPacket::Header * hdr = getProtoHeader<TcpPacket>();

                if ( hdr != 0 )
                {
                    // We use getSrcPort and getDestPort instead of accessing these values directly,
                    // because we want network->host endianness conversion.
                    // This way the seed of the same packet will be the same, regardless of the architecture.
                    portA = hdr->getSrcPort();
                    portB = hdr->getDestPort();
                }
            }
            break;
    }

    if ( portA != portB )
    {
        // They are different. Let's use XOR on them!
        return ( portA ^ portB );
    }
    else if ( portA > 0 )
    {
        // If they are the same we don't want to XOR them (which would give us 0).
        // We can just return one of them, unless they are both 0. In that case we don't use ports at all.
        return portA;
    }

    assert ( addrA != 0 );
    assert ( addrB != 0 );
    assert ( addrLen > 0 );

    // We want this number to be the same for the packets that are part of the same flow
    // in both directions, so we treat source and destination addresses symmetrically.
    // Also, we want it to be the same on architectures with different endiannesses.

    uint16_t result = 0;

    for ( int i = 0; i < addrLen; ++i )
    {
        // For each 16bit part of the address, we take XOR of that fragment
        // of src and dest addresses, and XOR it with the result so far.

        result ^= ( ntohs ( addrA[ i ] ) ^ ntohs ( addrB[ i ] ) );
    }

    return result;
}

bool IpPacket::setAddress ( AddressType whichAddr, const IpAddress & newAddress )
{
    assert ( whichAddr == SourceAddress || whichAddr == DestAddress );

    PacketDesc pDesc;

    if ( !isValid() || !examinePacket ( pDesc ) )
    {
        LOG ( L_ERROR, "Invalid IP packet" );
        return false;
    }

    // We need a writable pointer to the IP header.
    // We may also need a writable pointer to payload protocol's header, which is after the IP header.
    // To do that, we need to call getContinuousWritable().
    // However, we want to call it with the total size of both headers.
    // Otherwise it would first copy just the IP header, and then it would replace it with IP + payload header.

    // The size of the entire payload header:
    uint16_t pHdrSize = 0;

    if ( pDesc.protoType == Proto::TCP )
    {
        const TcpPacket::Header * hdr = getProtoHeader<TcpPacket>();

        if ( hdr != 0 )
        {
            pHdrSize = hdr->getHeaderSize();
        }
    }
    else if ( pDesc.protoType == Proto::UDP )
    {
        const UdpPacket::Header * hdr = getProtoHeader<UdpPacket>();

        if ( hdr != 0 )
        {
            pHdrSize = hdr->getHeaderSize();
        }
    }
    else if ( pDesc.protoType == Proto::ICMP )
    {
        const IcmpPacket::Header * hdr = getProtoHeader<IcmpPacket>();

        if ( hdr != 0 )
        {
            pHdrSize = hdr->getHeaderSize();
        }
    }

    char * const ipMem = _buffer.getContinuousWritable ( pDesc.ipHeaderSize + pHdrSize );
    DualIpHeader * const ipHdrPtr = reinterpret_cast<DualIpHeader *> ( ipMem );

    if ( !ipHdrPtr )
    {
        LOG ( L_ERROR, "Could not get writable pointer to IP header" );
        return false;
    }

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        if ( !newAddress.isIPv4() )
        {
            LOG ( L_ERROR, "Trying to set non-IPv4 address (" << newAddress << ") in IPv4 packet. Ignoring" );
            return false;
        }

        in_addr & addr = ( whichAddr == SourceAddress ) ? ( ipHdrPtr->v4.ip_src ) : ( ipHdrPtr->v4.ip_dst );
        const uint32_t oldAddrU32 = addr.s_addr;

        addr = newAddress.getV4();

        adjustChecksum ( ipHdrPtr->v4.ip_sum, oldAddrU32, addr.s_addr );

        if ( pHdrSize > 0 )
        {
            uint16_t * checksum = 0;

            if ( pDesc.protoType == Proto::TCP )
            {
                checksum = &( reinterpret_cast<TcpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }
            else if ( pDesc.protoType == Proto::UDP )
            {
                checksum = &( reinterpret_cast<UdpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }
            else if ( pDesc.protoType == Proto::ICMP )
            {
                checksum = &( reinterpret_cast<IcmpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }

            assert ( checksum != 0 );

            adjustChecksum ( *checksum, oldAddrU32, addr.s_addr );
        }

        return true;
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( pDesc.ipHeaderSize == sizeof ( struct ip6_hdr ) );

        if ( !newAddress.isIPv6() )
        {
            LOG ( L_ERROR, "Trying to set non-IPv6 address (" << newAddress << ") in IPv6 packet. Ignoring" );
            return false;
        }

        in6_addr & addr = ( whichAddr == SourceAddress ) ? ( ipHdrPtr->v6.ip6_src ) : ( ipHdrPtr->v6.ip6_dst );
        const in6_addr oldAddr = addr;

        addr = newAddress.getV6();

        if ( pHdrSize > 0 )
        {
            uint16_t * checksum = 0;

            if ( pDesc.protoType == Proto::TCP )
            {
                checksum = &( reinterpret_cast<TcpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }
            else if ( pDesc.protoType == Proto::UDP )
            {
                checksum = &( reinterpret_cast<UdpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }
            else if ( pDesc.protoType == Proto::ICMP )
            {
                checksum = &( reinterpret_cast<IcmpPacket::Header *> ( ipMem + pDesc.ipHeaderSize )->checksum );
            }

            assert ( checksum != 0 );

            const uint16_t * const oAddr16 = ( const uint16_t * ) &oldAddr;
            const uint16_t * const nAddr16 = ( const uint16_t * ) &newAddress.getV6();

            adjustChecksum ( *checksum,
                             ( oAddr16[ 0 ] - nAddr16[ 0 ] )
                             + ( oAddr16[ 1 ] - nAddr16[ 1 ] )
                             + ( oAddr16[ 2 ] - nAddr16[ 2 ] )
                             + ( oAddr16[ 3 ] - nAddr16[ 3 ] )
                             + ( oAddr16[ 4 ] - nAddr16[ 4 ] )
                             + ( oAddr16[ 5 ] - nAddr16[ 5 ] )
                             + ( oAddr16[ 6 ] - nAddr16[ 6 ] )
                             + ( oAddr16[ 7 ] - nAddr16[ 7 ] ) );
        }

        return true;
    }

    LOG ( L_ERROR, "Unknown type of IP packet (" << ipHdrPtr->v4.ip_v << "); Ignoring" );

    return false;
}

void IpPacket::adjustChecksum ( uint16_t & checksum, int32_t diff )
{
    int32_t modif = diff + checksum;

    // handle any carries (forward/backwards)
    if ( modif < 0 )
    {
        // the new checksum has borrowed back (rolled backwards)
        // get the magnitude, and...
        modif = -( modif );

        // add any overflow back into the 16-bit range
        modif = ( modif >> 16 ) + ( modif & 0xFFFF );

        // add any more overflow back into the 16-bit range
        // (in case the above operation overflowed it again)
        modif += modif >> 16;

        // invert to make it negative again, and
        // save the new value to the checksum
        checksum = ( uint16_t ) ~( modif );
    }
    else
    {
        // the new checksum may have carried forward and have bits overflowing the 16-bit range
        // add any overflow back into the 16-bit range
        modif = ( modif >> 16 ) + ( modif & 0xFFFF );

        // add any more overflow back into the 16-bit range
        // (in case the above operation overflowed it again)
        modif += modif >> 16;

        // save the new value to the checksum
        checksum = ( uint16_t ) modif;
    }
}

void IpPacket::adjustChecksum ( uint16_t & checksum, uint32_t oldValue, uint32_t newValue )
{
    // diff is the old value - new value
    // it's okay if the overflows the byte since we're doing all
    // the math with values in the same byte order.
    //
    // See RFC1071 "Byte Order Independence"

    adjustChecksum ( checksum, ( oldValue >> 16 ) - ( newValue >> 16 )
                     + ( ( oldValue & 0xFFFF ) - ( newValue & 0XFFFF ) ) );
}

void IpPacket::adjustChecksum ( uint16_t & checksum, uint16_t oldValue, uint16_t newValue )
{
    // diff is the old value - new value
    // it's okay if the overflows the byte since we're doing all
    // the math with values in the same byte order.
    //
    // See RFC1071 "Byte Order Independence"

    adjustChecksum ( checksum, oldValue - newValue );
}

void IpPacket::describe ( Buffer & toBuffer ) const
{
    if ( _buffer.getDataSize() < sizeof ( struct ip ) )
    {
        toBuffer.append ( "Too short packet (" );
        toBuffer.append ( String::number ( _buffer.getDataSize() ) );
        toBuffer.append ( "B); Could not read IP data" );
        return;
    }

    assert ( _buffer.getNumChunks() > 0 );

    const struct iovec * const chunks = _buffer.getChunks();

    assert ( chunks != 0 );
    assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip ) );

    const DualIpHeader * ipHdrPtr = static_cast<const DualIpHeader *> ( chunks[ 0 ].iov_base );

    assert ( ipHdrPtr != 0 );
    assert ( ( ( size_t ) ipHdrPtr ) % 4U == 0 );

    uint16_t pType = 0;

    if ( ipHdrPtr->v4.ip_v == 4 )
    {
        toBuffer.append ( "IPv4;" );
        toBuffer.append ( " Length: " );
        toBuffer.append ( String::number ( ntohs ( ipHdrPtr->v4.ip_len ) ) );
        toBuffer.append ( "; ID: 0x" );
        toBuffer.append ( String::number ( ntohs ( ipHdrPtr->v4.ip_id ), String::Int_HEX ) );
        toBuffer.append ( "; Source: " );
        toBuffer.append ( IpAddress::toString ( ipHdrPtr->v4.ip_src ) );
        toBuffer.append ( "; Dest: " );
        toBuffer.append ( IpAddress::toString ( ipHdrPtr->v4.ip_dst ) );

        pType = ipHdrPtr->v4.ip_p;

        if ( ( ntohs ( ipHdrPtr->v4.ip_off ) & IP_OFFMASK ) != 0 )
        {
            pType |= ProtoBitNextIpFragment;
        }
    }
    else if ( ipHdrPtr->v4.ip_v == 6 )
    {
        assert ( chunks[ 0 ].iov_len >= sizeof ( struct ip6_hdr ) );

        toBuffer.append ( "IPv6;" );
        toBuffer.append ( " IP Data Length: " );
        toBuffer.append ( String::number ( ( unsigned int ) ntohs ( ipHdrPtr->v6.ip6_plen ) ) );

        /// @todo IPv6 jumbo support.?

        if ( ipHdrPtr->v6.ip6_plen == 0 )
            toBuffer.append ( " (jumbo packet, unsupported)" );

        toBuffer.append ( "; Source: " );
        toBuffer.append ( IpAddress::toString ( ipHdrPtr->v6.ip6_src ) );
        toBuffer.append ( "; Dest: " );
        toBuffer.append ( IpAddress::toString ( ipHdrPtr->v6.ip6_dst ) );

        pType = ipHdrPtr->v6.ip6_nxt;
    }
    else
    {
        toBuffer.append ( "Unknown packet type" );
        return;
    }

    if ( pType > 0 )
    {
        toBuffer.append ( "; " );
        toBuffer.append ( getProtoName ( pType ) );

        if ( ( pType & ProtoBitNextIpFragment ) != 0 )
        {
            toBuffer.append ( "-[non-zero fragment offset]" );
            return;
        }

        switch ( pType )
        {
            case Proto::ICMP:
                toBuffer.append ( "; " );
                IcmpPacket::describe ( *this, toBuffer );
                break;

            case Proto::UDP:
                toBuffer.append ( "; " );
                UdpPacket::describe ( *this, toBuffer );
                break;

            case Proto::TCP:
                toBuffer.append ( "; " );
                TcpPacket::describe ( *this, toBuffer );
                break;
        }
    }
}

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const IpPacket & value )
{
    value.describe ( textMessage.getInternalBuf() );

    return textMessage;
}

String IpPacket::getProtoName ( uint16_t protoNum )
{
    // We want to consider only 8 lowest bits of the protocol number
    switch ( protoNum & 0xFF )
    {
        CASE_PROTO_NAME ( ICMP );
        CASE_PROTO_NAME ( IGMP );
        CASE_PROTO_NAME ( TCP );
        CASE_PROTO_NAME ( UDP );
        CASE_PROTO_NAME ( RDP );
        CASE_PROTO_NAME ( IPv6Encaps );
        CASE_PROTO_NAME ( IPv6Route );
        CASE_PROTO_NAME ( IPv6Frag );
        CASE_PROTO_NAME ( RSVP );
        CASE_PROTO_NAME ( GRE );
        CASE_PROTO_NAME ( ESP );
        CASE_PROTO_NAME ( AH );
        CASE_PROTO_NAME ( IPv6ICMP );
        CASE_PROTO_NAME ( IPv6NoNxt );
        CASE_PROTO_NAME ( IPv6Opts );
        CASE_PROTO_NAME ( IPIP );
        CASE_PROTO_NAME ( EtherIP );
        CASE_PROTO_NAME ( Encap );
        CASE_PROTO_NAME ( SCTP );
        CASE_PROTO_NAME ( UDPLite );

        default:
            return String::number ( protoNum );
            break;
    }
}
