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

#include "IcmpPacket.hpp"
#include "IpChecksum.hpp"

using namespace Pravala;

IcmpPacket::IcmpPacket (
        const IpAddress & srcAddr, const IpAddress & destAddr,
        uint8_t icmpType, uint8_t icmpCode,
        uint16_t icmpId, uint16_t icmpSequence,
        const MemVector & payload )
{
    Header * const header = reinterpret_cast<Header *> (
        initProtoPacket ( srcAddr, destAddr, ProtoNumber, sizeof ( Header ), payload ) );

    if ( !header )
        return;

    memset ( header, 0, sizeof ( Header ) );

    header->type = icmpType;
    header->code = icmpCode;
    header->id = htons ( icmpId );
    header->sequence = htons ( icmpSequence );

    // For correct payload checksum calculation, this has to be set to 0.
    // It should be anyway (thanks to memset), but for clarity we set it anyway...
    header->checksum = 0;

    // ICMP doesn't use the IP pseudo header, the checksum only includes the ICMP header + data

    IpChecksum cSum;

    cSum.addMemory ( ( char * ) header, sizeof ( Header ) );
    cSum.addMemory ( payload );

    header->checksum = cSum.getChecksum();
}

void IcmpPacket::describe ( const IpPacket & ipPacket, Buffer & toBuffer )
{
    const Header * header = ipPacket.getProtoHeader<IcmpPacket>();

    if ( !header )
        return;

    assert ( header != 0 );
    assert ( ipPacket.is ( Proto::ICMP ) );

    const uint8_t type = header->type;
    const uint8_t code = header->code;

    const char * tDesc = "";
    const char * cDesc = "";

    /// @todo These should be defined in an enum, for easier creation of ICMP packets - if required!
    switch ( type )
    {
        case 0:
            tDesc = " (Echo Reply)";
            break;

        case 1:
        case 2:
            tDesc = " (Reserved)";
            break;

        case 3:
            {
                tDesc = " (Destination Unreachable)";

                switch ( code )
                {
                    case 0:
                        cDesc = " (Destination network unreachable)";
                        break;

                    case 1:
                        cDesc = " (Destination host unreachable)";
                        break;

                    case 2:
                        cDesc = " (Destination protocol unreachable)";
                        break;

                    case 3:
                        cDesc = " (Destination port unreachable)";
                        break;

                    case 4:
                        cDesc = " (Fragmentation required, and DF flag set)";
                        break;

                    case 5:
                        cDesc = " (Source route failed)";
                        break;

                    case 6:
                        cDesc = " (Destination network unknown)";
                        break;

                    case 7:
                        cDesc = " (Destination host unknown)";
                        break;

                    case 8:
                        cDesc = " (Source host isolated)";
                        break;

                    case 9:
                        cDesc = " (Network administratively prohibited)";
                        break;

                    case 10:
                        cDesc = " (Host administratively prohibited)";
                        break;

                    case 11:
                        cDesc = " (Network unreachable for TOS)";
                        break;

                    case 12:
                        cDesc = " (Host unreachable for TOS)";
                        break;

                    case 13:
                        cDesc = " (Communication administratively prohibited)";
                        break;
                }
            }
            break;

        case 4:
            tDesc = " (Source Quench)";
            if ( code == 0 ) cDesc = " (Congestion Control)";
            break;

        case 5:
            {
                tDesc = " (Redirect Message)";

                switch ( code )
                {
                    case 0:
                        cDesc = " (Redirect Datagram for the Network)";
                        break;

                    case 1:
                        cDesc = " (Redirect Datagram for the Host)";
                        break;

                    case 2:
                        cDesc = " (Redirect Datagram for the TOS & network)";
                        break;

                    case 3:
                        cDesc = " (Redirect Datagram for the TOS & host)";
                        break;
                }
            }
            break;

        case 6:
            tDesc = " (Alternate Host Address)";
            break;

        case 7:
            tDesc = " (Reserved)";
            break;

        case 8:
            tDesc = " (Echo Request)";
            break;

        case 9:
            tDesc = " (Router Advertisement)";
            break;

        case 10:
            tDesc = " (Router Solicitation)";
            break;

        case 11:
            {
                tDesc = " (Time Exceeded)";

                switch ( code )
                {
                    case 0:
                        cDesc = " (TTL expired in transit)";
                        break;

                    case 1:
                        cDesc = " (Fragment reassembly time exceeded)";
                        break;
                }
            }
            break;

        case 12:
            {
                tDesc = " (Parameter Problem: Bad IP header)";

                switch ( code )
                {
                    case 0:
                        cDesc = " (Pointer indicates the error)";
                        break;

                    case 1:
                        cDesc = " (Missing a required option)";
                        break;

                    case 2:
                        cDesc = " (Bad length)";
                        break;
                }
            }
            break;

        case 13:
            tDesc = " (Timestamp)";
            break;

        case 14:
            tDesc = " (Timestamp Reply)";
            break;

        case 15:
            tDesc = " (Information Request)";
            break;

        case 16:
            tDesc = " (Information Reply)";
            break;

        case 17:
            tDesc = " (Address Mask Request)";
            break;

        case 18:
            tDesc = " (Address Mask Reply)";
            break;

        case 19:
            tDesc = " (Reserved for security)";
            break;

        case 30:
            tDesc = " (Traceroute)";
            break;

        case 31:
            tDesc = " (Datagram Conversion Error)";
            break;

        case 32:
            tDesc = " (Mobile Host Redirect)";
            break;

        case 33:
            tDesc = " (Where-Are-You)";
            break;

        case 34:
            tDesc = " (Here-I-Am)";
            break;

        case 35:
            tDesc = " (Mobile Registration Request)";
            break;

        case 36:
            tDesc = " (Mobile Registration Reply)";
            break;

        case 37:
            tDesc = " (Domain Name Request)";
            break;

        case 38:
            tDesc = " (Domain Name Reply)";
            break;

        case 39:
            tDesc = " (SKIP Algorithm Discovery Protocol)";
            break;

        case 40:
            tDesc = " (Photuris, Security failures)";
            break;

        case 41:
            tDesc = " (ICMP for experimental mobility protocols)";
            break;

        default:
            if ( type >= 20 && type <= 29 ) { tDesc = " (Reserved for robustness experiment)"; }
            else if ( type >= 42 )
            {
                tDesc = " (Reserved)";
            }
            break;
    }

    toBuffer.append ( "Type: " );
    toBuffer.append ( String::number ( ( unsigned int ) type ) );
    toBuffer.append ( tDesc );
    toBuffer.append ( " Code: " );
    toBuffer.append ( String::number ( ( unsigned int ) code ) );
    toBuffer.append ( cDesc );
    toBuffer.append ( " Checksum: " );
    toBuffer.append ( String::number ( header->getChecksum() ) );
    toBuffer.append ( " Id: " );
    toBuffer.append ( String::number ( header->getId() ) );
    toBuffer.append ( " Seq: " );
    toBuffer.append ( String::number ( header->getSequence() ) );
}

/// @todo matches() method
// bool ProtoICMP::matches ( const char * /* icmpData */, size_t /* size */, TrafficFlow flow )
// {
//     if ( flow.getProtocol() != TrafficFlow::Any )
//     {
//         if ( flow.getProtocol() != TrafficFlow::ICMP ) return false;
//     }
//
//     // ICMP doesn't have ports!
//     if ( flow.getSourcePort() != 0 || flow.getDestPort() != 0 ) return false;
//
//     /// @todo - Add ICMP type/code matching?
//
// #if 0
//
//     const Header * hdr = getHeader ( icmpData, size );
//
//     // Since ICMP's code/type = 0 is valid,
//     // 'Any' value has to be -1
//
//     if ( flow.getICMPType() != -1 )
//     {
//         if ( !hdr || flow.getICMPType() != ( int ) hdr->type ) return false;
//     }
//
//     if ( flow.getICMPCode() != -1 )
//     {
//         if ( !hdr || flow.getICMPCode() != ( int ) hdr->code ) return false;
//     }
//
// #endif
//
//     return true;
// }
