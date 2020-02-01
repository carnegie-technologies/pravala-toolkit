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

#include "basic/IpAddress.hpp"

#include "UdpPacket.hpp"
#include "DnsPacket.hpp"

using namespace Pravala;

UdpPacket::UdpPacket (
        const IpAddress & srcAddr, uint16_t srcPort,
        const IpAddress & destAddr, uint16_t destPort,
        const MemVector & payload, uint8_t tos, uint8_t ttl )
{
    Header * const header = reinterpret_cast<Header *> (
        initProtoPacket ( srcAddr, destAddr, ProtoNumber, sizeof ( Header ), payload, tos, ttl ) );

    if ( !header )
        return;

    memset ( header, 0, sizeof ( Header ) );

    header->source_port = htons ( srcPort );
    header->dest_port = htons ( destPort );
    header->length = htons ( sizeof ( Header ) + payload.getDataSize() );

    // For correct payload checksum calculation, this has to be set to 0.
    // It should be anyway (thanks to memset), but for clarity we set it anyway...
    header->checksum = 0;
    header->checksum = calcPseudoHeaderPayloadChecksum();
}

void UdpPacket::describe ( const IpPacket & ipPacket, Buffer & toBuffer )
{
    const Header * header = ipPacket.getProtoHeader<UdpPacket>();

    if ( !header )
        return;

    assert ( header != 0 );
    assert ( ipPacket.is ( Proto::UDP ) );

    toBuffer.append ( "SrcPort: " );
    toBuffer.append ( String::number ( header->getSrcPort() ) );
    toBuffer.append ( "; DestPort: " );
    toBuffer.append ( String::number ( header->getDestPort() ) );
    toBuffer.append ( "; Length: " );
    toBuffer.append ( String::number ( header->getLength() ) );
    toBuffer.append ( "; Checksum: " );
    toBuffer.append ( String::number ( header->getChecksum() ) );

    if ( header->getSrcPort() == 53 || header->getDestPort() == 53 )
    {
        MemVector vec;

        if ( ipPacket.getProtoPayload<UdpPacket> ( vec ) )
        {
            DnsPacket dnsPacket;

            if ( dnsPacket.setupPacket ( vec ) )
            {
                toBuffer.append ( "; " );
                dnsPacket.describe ( toBuffer );
            }
        }
    }
}

void UdpPacket::Header::setSrcPort ( uint16_t sourcePort )
{
    uint16_t netOrderSrcPort = htons ( sourcePort );

    if ( source_port == netOrderSrcPort )
        return;

    IpPacket::adjustChecksum ( checksum, source_port, netOrderSrcPort );

    source_port = netOrderSrcPort;
}

void UdpPacket::Header::setDestPort ( uint16_t destPort )
{
    uint16_t netOrderDestPort = htons ( destPort );

    if ( dest_port == netOrderDestPort )
        return;

    IpPacket::adjustChecksum ( checksum, dest_port, netOrderDestPort );

    dest_port = netOrderDestPort;
}
