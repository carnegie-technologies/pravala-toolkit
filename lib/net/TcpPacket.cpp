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
#include "TcpPacket.hpp"

using namespace Pravala;

TcpPacket::TcpPacket()
{
}

TcpPacket::TcpPacket (
        const IpAddress & srcAddr, uint16_t srcPort, const IpAddress & destAddr, uint16_t destPort,
        uint8_t flagsToSet,
        uint32_t seqNum, uint32_t ackNum, uint16_t winSize,
        const MemVector & payload,
        Option * options, uint8_t optCount )
{
    const uint8_t optSize = getOptLen ( options, optCount );

    Header * const header = reinterpret_cast<Header *> (
        initProtoPacket ( srcAddr, destAddr, ProtoNumber, sizeof ( Header ) + optSize, payload ) );

    assert ( options != 0 || optCount < 1 );

    if ( !header || ( !options && optCount > 0 ) )
    {
        clear();
        return;
    }

    uint8_t * const optPtr = ( uint8_t * ) ( header + 1 );
    size_t optIdx = 0;

    if ( optCount > 0 )
    {
        for ( uint8_t i = 0; i < optCount; ++i )
        {
            if ( optIdx >= optSize || options[ i ].type == OptEnd )
            {
                // optIdx >= optSize will also trigger when getOptLen() returned zero on error.

                clear();
                return;
            }

            optPtr[ optIdx++ ] = options[ i ].type;

            if ( options[ i ].type == OptNop )
            {
                continue;
            }

            if ( optIdx + 1 + options[ i ].dataLength > optSize )
            {
                clear();
                return;
            }

            // Total option length: data-len + 1 for type + 1 for length:
            optPtr[ optIdx++ ] = options[ i ].dataLength + 2;

            if ( options[ i ].dataLength > 0 )
            {
                if ( !options[ i ].data )
                {
                    clear();
                    return;
                }

                memcpy ( optPtr + optIdx, options[ i ].data, options[ i ].dataLength );
                optIdx += options[ i ].dataLength;
            }
        }
    }

    memset ( header, 0, sizeof ( Header ) );

    if ( optIdx < optSize )
    {
        // Let's set all remaining bytes reserved for options with 'NOP' option (which has no length field):
        memset ( optPtr + optIdx, OptNop, optSize - optIdx );
    }

    // We set the fields manually, to avoid having set*() functions recalculate the checksum multiple times.
    // We will have to calculate it from scratch anyway at the end!
    header->source_port = htons ( srcPort );
    header->dest_port = htons ( destPort );
    header->seq_num = htonl ( seqNum );
    header->ack_num = htonl ( ( ( flagsToSet & FlagAck ) != 0 ) ? ackNum : 0 );
    header->window = htons ( winSize );
    header->flags = flagsToSet;

    if ( ( optSize % 4 ) != 0 || optSize > 40 )
    {
        clear();
        return;
    }

    assert ( ( sizeof ( Header ) + optSize ) % 4 == 0 );
    assert ( ( ( sizeof ( Header ) + optSize ) << 2 ) <= 0xF0 );
    assert ( ( ( ( sizeof ( Header ) + optSize ) << 2 ) & 0x0F ) == 0 );

    // sized in words, not bytes.
    // We take the size of header and options, divide by 4, and shift left by 4 bits (which are reserved).
    // Since dividing by 4 is simply shifting right by 2, all we need to do is shift left by 2,
    // and bitmask it with 0xF0 (just in case).
    // Normally we would bitsum this with the content of those 4 reserved bits,
    // but we are setting it from scratch, so they are 0 anyway.
    header->data_off_res = ( ( sizeof ( Header ) + optSize ) << 2 ) & 0xF0;

    // For correct payload checksum calculation, this has to be set to 0.
    // It should be anyway (thanks to memset), but for clarity we set it anyway...
    header->checksum = 0;
    header->setChecksum ( calcPseudoHeaderPayloadChecksum() );
}

TcpPacket TcpPacket::generateResetResponse ( const IpPacket & packet )
{
    const TcpPacket::Header * tcpHeader = packet.getProtoHeader<TcpPacket>();

    IpAddress src;
    IpAddress dst;

    // Not a TCP packet, a TCP RESET packet (do not respond to a reset with a reset - RFC 793),
    // or an incomplete packet, or we cant' read the addresses (for whatever reason).
    if ( !tcpHeader || tcpHeader->isRST() || !packet.getAddr ( src, dst ) )
    {
        return TcpPacket();
    }

    uint32_t seqNum = 0;
    uint32_t ackNum = 0;
    uint8_t flagsToSet = TcpPacket::FlagRst;

    // RFC 793 - Reset Generation - page 36
    // If the incoming segment has an ACK field, the reset takes its
    // sequence number from the ACK field of the segment, otherwise the
    // reset has sequence number zero and the ACK field is set to the sum
    // of the sequence number and segment length of the incoming segment.
    if ( tcpHeader->isACK() )
    {
        // If the ACK is set, the sequence number of the reset is the ACK number of the original packet.
        seqNum = tcpHeader->getAckNum();
    }
    else if ( tcpHeader->isSYN() )
    {
        // If this is the initial SYN packet (SYN set, ACK not set),
        // the ACK number of the reset is the sequence number of the original packet + 1.
        // See page 16 of RFC 793.
        ackNum = tcpHeader->getSeqNum() + 1;
        flagsToSet |= TcpPacket::FlagAck;
    }
    else
    {
        const size_t tcpPayloadSize = packet.getProtoPayloadSize<TcpPacket>();

        // Otherwise the ACK number of the reset is sequence number + TCP payload size.
        ackNum = tcpHeader->getSeqNum() + tcpPayloadSize;
        flagsToSet |= TcpPacket::FlagAck;
    }

    return TcpPacket ( dst, tcpHeader->getDestPort(), // source of the response = destination of the original packet
                       src, tcpHeader->getSrcPort(),  // destination of the response = source of the original packet
                       flagsToSet,
                       seqNum,
                       ackNum );
}

bool TcpPacket::Header::getOptData ( uint8_t optType, const char * & data, uint8_t & dataLen ) const
{
    const uint8_t * ptr = ( const uint8_t * ) this;
    const size_t hdrSize = getHeaderSize();

    data = 0;
    dataLen = 0;

    for ( size_t i = sizeof ( *this ); i < hdrSize; )
    {
        // ptr[i] is the option type:
        const uint8_t curType = ptr[ i++ ];

        // 'i' points to the first byte AFTER the first option type.
        // It could be the next option (in case of NOP), or option's length.

        if ( curType == 0 )
        {
            // End of options, there is no point checking anything else...
            // However, if that's what we were asked to find, we should report it.
            return ( optType == curType );
        }

        if ( curType == 1 )
        {
            // NOP option (single byte), we can just skip it, unless that's what we were asked to find,
            // in which case we're done.

            if ( optType == curType )
                return true;

            continue;
        }

        // 'i' points to the byte with this option's length.

        if ( i >= hdrSize )
        {
            // Not enough bytes, something's wrong...
            return false;
        }

        uint8_t optLen = ptr[ i++ ];

        // 'i' points to the byte after this option's length.
        // It could be the next option's type, or this option's value.

        if ( optLen < 2 )
        {
            // Option len should be at least 2 (it includes the type and length bytes).
            return false;
        }

        optLen -= 2;

        if ( i + optLen > hdrSize )
        {
            // Not enough option bytes to read that length...
            return false;
        }

        if ( optType == curType )
        {
            // This is the option that we were asked to find.
            data = ( const char * ) ( ptr + i );
            dataLen = optLen;
            return true;
        }

        // Some other option. Let's skip it!
        i += optLen;
    }

    return false;
}

bool TcpPacket::Header::getOptMss ( uint16_t & value ) const
{
    const char * optVal = 0;
    uint8_t optLen = 0;
    uint16_t mssVal = 0;

    if ( !isSYN() || !getOptData ( OptMss, optVal, optLen ) || !optVal || optLen != sizeof ( value ) )
    {
        return false;
    }

    // There should be no issues with alignment, but just in case:
    memcpy ( &mssVal, optVal, optLen );
    value = ntohs ( mssVal );

    return true;
}

bool TcpPacket::Header::getOptWindowScale ( uint8_t & value ) const
{
    const char * optVal = 0;
    uint8_t optLen = 0;

    if ( !isSYN() || !getOptData ( OptWScale, optVal, optLen ) || !optVal || optLen != sizeof ( value ) )
    {
        return false;
    }

    value = *optVal;
    return true;
}

uint8_t TcpPacket::getOptLen ( const Option * options, uint8_t optCount )
{
    if ( !options || optCount < 1 )
    {
        return 0;
    }

    size_t totalLen = 0;

    for ( uint8_t i = 0; i < optCount; ++i )
    {
        // We always include at least the type:
        ++totalLen;

        if ( options[ i ].type == OptEnd )
        {
            return 0;
        }

        if ( options[ i ].type != OptNop )
        {
            // Anything other than NOP will include the length field and the data length (which could be 0).
            totalLen += 1 + options[ i ].dataLength;

            if ( options[ i ].dataLength > 0 && !options[ i ].data )
            {
                // No data provided...
                return 0;
            }
        }
    }

    // Let's add required padding:
    totalLen += ( 4 - ( totalLen & 3 ) ) & 3;

    // If there are too many options, we return 0:
    return ( uint8_t ) ( totalLen <= 40 ) ? totalLen : 0;
}

void TcpPacket::describe ( const IpPacket & ipPacket, Buffer & toBuffer )
{
    const Header * header = ipPacket.getProtoHeader<TcpPacket>();

    if ( !header )
        return;

    assert ( header != 0 );
    assert ( ipPacket.is ( Proto::TCP ) );

    toBuffer.append ( "SrcPort: " );
    toBuffer.append ( String::number ( header->getSrcPort() ) );
    toBuffer.append ( "; DestPort: " );
    toBuffer.append ( String::number ( header->getDestPort() ) );
    toBuffer.append ( "; Flags [" );

    if ( ( header->flags & FlagFin ) != 0 ) toBuffer.append ( "F" );
    if ( ( header->flags & FlagSyn ) != 0 ) toBuffer.append ( "S" );
    if ( ( header->flags & FlagRst ) != 0 ) toBuffer.append ( "R" );
    if ( ( header->flags & FlagPsh ) != 0 ) toBuffer.append ( "P" );
    if ( ( header->flags & FlagAck ) != 0 ) toBuffer.append ( "A" );
    if ( ( header->flags & FlagUrg ) != 0 ) toBuffer.append ( "U" );
    if ( ( header->flags & FlagEce ) != 0 ) toBuffer.append ( "E" );
    if ( ( header->flags & FlagCwr ) != 0 ) toBuffer.append ( "C" );

    toBuffer.append ( "]; DataOff: " );
    toBuffer.append ( String::number ( ( unsigned int ) header->getHeaderSize() ) );
    toBuffer.append ( "; SeqNum: " );
    toBuffer.append ( String::number ( header->getSeqNum() ) );
    toBuffer.append ( "; AckNum: " );
    toBuffer.append ( String::number ( header->getAckNum() ) );
    toBuffer.append ( "; WinSize: " );
    toBuffer.append ( String::number ( header->getWindow() ) );
    toBuffer.append ( "; Cksum: 0x" );
    toBuffer.append ( String::number ( header->getChecksum(), String::Int_HEX, 4, true ) );

    // hdrSize is the base header + all the options.
    const uint8_t hdrSize = header->getHeaderSize();
    const uint8_t * mem = ( const uint8_t * ) header;

    // idx starts pointed at the first byte of options segment (if one exists)
    // it is 20 bytes after beginning of TCP header

    assert ( sizeof ( Header ) == 20 );

    for ( size_t idx = sizeof ( Header ); idx < hdrSize; )
    {
        uint8_t optCode = ( uint8_t ) mem[ idx ];
        uint8_t optLen = 1;

        if ( optCode == OptEnd ) // End of options
        {
            toBuffer.append ( "; Opt: 0 (END)" );
            optLen = 1;

            // NOTE: We don't interrupt processing options, to display what's after 'end' option...
        }
        else if ( optCode == OptNop ) // NOP - No-Operation, no length, just operation's code
        {
            toBuffer.append ( "; Opt: 1 (NO-OP)" );
            optLen = 1;
        }
        else // Any other operation, next byte is a length
        {
            // not enough data!
            if ( idx + 1 >= hdrSize ) return;

            optLen = ( uint8_t ) mem[ idx + 1 ];

            // We can't read the whole option
            if ( idx + optLen > hdrSize ) return;

            toBuffer.append ( String ( "; Opt: %1" ).arg ( optCode ) );

            switch ( optCode )
            {
                case OptMss:
                    toBuffer.append ( " (MSS)" );
                    break;

                case OptWScale:
                    toBuffer.append ( " (Window Scale)" );
                    break;

                case OptSAckPerm:
                    toBuffer.append ( " (SACK Permit)" );
                    break;

                case OptSAck:
                    toBuffer.append ( " (SACK)" );
                    break;

                case 6:
                    toBuffer.append ( " (Echo [obsolete])" );
                    break;

                case 7:
                    toBuffer.append ( " (Echo Reply [obsolete])" );
                    break;

                case OptTStamp:
                    toBuffer.append ( " (Time Stamp Opt)" );
                    break;

                case 9:
                    toBuffer.append ( " (Partial Order Connection Permitted)" );
                    break;

                case 10:
                    toBuffer.append ( " (Partial Order Service Profile)" );
                    break;

                case 11:
                    toBuffer.append ( " (CC)" );
                    break;

                case 12:
                    toBuffer.append ( " (CC.NEW)" );
                    break;

                case 13:
                    toBuffer.append ( " (CC.ECHO)" );
                    break;

                case 14:
                    toBuffer.append ( " (TCP Alternate Checksum Request)" );
                    break;

                case 15:
                    toBuffer.append ( " (TCP Alternate Checksum Data)" );
                    break;

                case 16:
                    toBuffer.append ( " (Skeeter)" );
                    break;

                case 17:
                    toBuffer.append ( " (Bubba)" );
                    break;

                case 18:
                    toBuffer.append ( " (Trailer Checksum Option)" );
                    break;

                case 19:
                    toBuffer.append ( " (MD5 Signature Option)" );
                    break;

                case 20:
                    toBuffer.append ( " (SCPS Capabilities)" );
                    break;

                case 21:
                    toBuffer.append ( " (Selective Negative Acknowledgements)" );
                    break;

                case 22:
                    toBuffer.append ( " (Record Boundaries)" );
                    break;

                case 23:
                    toBuffer.append ( " (Corruption experienced)" );
                    break;

                case 24:
                    toBuffer.append ( " (SNAP)" );
                    break;

                case 25:
                    toBuffer.append ( " (Unassigned)" );
                    break;

                case 26:
                    toBuffer.append ( " (TCP Compression Filter)" );
                    break;

                case 27:
                    toBuffer.append ( " (Quick - Start Response)" );
                    break;

                case 28:
                    toBuffer.append ( " (User Timeout Option)" );
                    break;

                case 253:
                    toBuffer.append ( " (RFC3692 Experiment 1 !)" );
                    break;

                case 254:
                    toBuffer.append ( " (RFC3692 Experiment 2 !)" );
                    break;

                default:
                    toBuffer.append ( " (UNASSIGNED!!)" );
                    break;
            }

            toBuffer.append ( " Len: " );
            toBuffer.append ( String::number ( ( int ) optLen ) );

            switch ( optLen )
            {
                case 0:
                case 1:
                    toBuffer.append ( " [Incorrect Length]" );
                    break;

                case 2:
                    toBuffer.append ( " No-Val" );
                    break;

                case 3:
                    toBuffer.append ( " Val: " );
                    toBuffer.append ( String::number ( mem[ idx + 2 ] & 0xFF ) );
                    break;

                case 4:
                    {
                        uint16_t v;
                        memcpy ( &v, mem + idx + 2, sizeof ( v ) );
                        toBuffer.append ( " Val: " );
                        toBuffer.append ( String::number ( ntohs ( v ) ) );
                    }
                    break;

                case 6:
                    {
                        uint32_t v;
                        memcpy ( &v, mem + idx + 2, sizeof ( v ) );
                        toBuffer.append ( " Val: " );
                        toBuffer.append ( String::number ( ntohl ( v ) ) );
                    }
                    break;

                default:
                    toBuffer.append ( " Val: " );

                    for ( uint8_t i = 2; i < optLen; ++i )
                    {
                        toBuffer.append ( String::number ( mem[ idx + i ] & 0xFF, String::Int_HEX, 2, true ) );
                    }

                    break;
            }

            if ( optLen < 2 ) return;
        }

        if ( optLen < 1 ) return;

        // Skip entire option (code, length, and data - all that is included
        // in option's length)
        idx += optLen;
    }
}

bool TcpPacket::fixMSS ( IpPacket & ipPacket, int modifMSS, int & oldMSS, int & newMSS )
{
    Header * header = ipPacket.getWritableProtoHeader<TcpPacket>();

    // MSS can only appear in SYN segments.
    if ( !header || !header->isSYN() )
    {
        return false;
    }

    char * mem = ( char * ) header;

    assert ( mem != 0 );
    assert ( header != 0 );
    assert ( ipPacket.is ( Proto::TCP ) );

    // hdrSize is the base header + all the options.
    const uint8_t hdrSize = header->getHeaderSize();

    // idx starts pointed at the first byte of options segment (if one exists)
    // it is 20 bytes after beginning of TCP header

    assert ( sizeof ( Header ) == 20 );

    for ( size_t idx = sizeof ( Header ); idx < hdrSize; )
    {
        uint8_t optCode = ( uint8_t ) mem[ idx ];
        uint8_t optLen = 1;

        if ( optCode == OptEnd )
        {
            return false;
        }
        else if ( optCode == OptNop )
        {
            optLen = 1;
        }
        else
        {
            // Any other operation, next byte is a length

            // not enough data!
            if ( idx + 1 >= hdrSize ) return false;

            optLen = ( uint8_t ) mem[ idx + 1 ];

            // We can't read the whole option
            if ( idx + optLen > hdrSize ) return false;

            if ( optCode == OptMss && optLen == 4 )
            {
                uint16_t * ptrMss = ( uint16_t * ) ( mem + ( idx + 2 ) );
                uint16_t hostMss = ntohs ( *ptrMss );

                oldMSS = hostMss;

                if ( modifMSS < 0 )
                    modifMSS += oldMSS;

                if ( modifMSS > 128 && hostMss > modifMSS )
                {
                    const uint16_t oldMssNetOrder = *ptrMss;

                    hostMss = modifMSS;
                    *ptrMss = htons ( hostMss );
                    newMSS = hostMss;

                    IpPacket::adjustChecksum ( header->checksum, oldMssNetOrder, *ptrMss );

                    return true;
                }
            }

            if ( optLen < 2 ) return false;
        }

        if ( optLen < 1 ) return false;

        // Skip entire option (code, length, and data - all that is included
        // in option's length)
        idx += optLen;
    }

    return false;
}

void TcpPacket::Header::setDataOffsetInWords ( uint8_t hdrSize )
{
    uint8_t newOffRes = ( hdrSize << 4 ) & 0xF0;
    newOffRes |= ( data_off_res & 0xF );

    if ( newOffRes != data_off_res )
    {
        IpPacket::adjustChecksum ( checksum, ( uint16_t ) data_off_res, ( uint16_t ) newOffRes );
        data_off_res = newOffRes;
    }
}

void TcpPacket::Header::setACK ( bool value )
{
    uint8_t newFlags = flags;

    if ( value )
    {
        newFlags |= FlagAck;
    }
    else
    {
        newFlags &= ( ~FlagAck );
    }

    if ( newFlags != flags )
    {
        IpPacket::adjustChecksum ( checksum, ( uint16_t ) flags, ( uint16_t ) newFlags );
        flags = newFlags;
    }
}

void TcpPacket::Header::setRST ( bool value )
{
    uint8_t newFlags = flags;

    if ( value )
    {
        newFlags |= FlagRst;
    }
    else
    {
        newFlags &= ( ~FlagRst );
    }

    if ( newFlags != flags )
    {
        IpPacket::adjustChecksum ( checksum, ( uint16_t ) flags, ( uint16_t ) newFlags );
        flags = newFlags;
    }
}

void TcpPacket::Header::setSYN ( bool value )
{
    uint8_t newFlags = flags;

    if ( value )
    {
        newFlags |= FlagSyn;
    }
    else
    {
        newFlags &= ( ~FlagSyn );
    }

    if ( newFlags != flags )
    {
        IpPacket::adjustChecksum ( checksum, ( uint16_t ) flags, ( uint16_t ) newFlags );
        flags = newFlags;
    }
}

void TcpPacket::Header::setFIN ( bool value )
{
    uint8_t newFlags = flags;

    if ( value )
    {
        newFlags |= FlagFin;
    }
    else
    {
        newFlags &= ( ~FlagFin );
    }

    if ( newFlags != flags )
    {
        IpPacket::adjustChecksum ( checksum, ( uint16_t ) flags, ( uint16_t ) newFlags );
        flags = newFlags;
    }
}

void TcpPacket::Header::setSrcPort ( uint16_t sourcePort )
{
    uint16_t netOrderSrcPort = htons ( sourcePort );

    if ( source_port == netOrderSrcPort )
        return;

    IpPacket::adjustChecksum ( checksum, source_port, netOrderSrcPort );

    source_port = netOrderSrcPort;
}

void TcpPacket::Header::setDestPort ( uint16_t destPort )
{
    uint16_t netOrderDestPort = htons ( destPort );

    if ( dest_port == netOrderDestPort )
        return;

    IpPacket::adjustChecksum ( checksum, dest_port, netOrderDestPort );

    dest_port = netOrderDestPort;
}

void TcpPacket::Header::setSeqNum ( uint32_t seqNum )
{
    uint32_t netOrderSeqNum = htonl ( seqNum );

    if ( seq_num == netOrderSeqNum )
        return;

    IpPacket::adjustChecksum ( checksum, seq_num, netOrderSeqNum );

    seq_num = netOrderSeqNum;
}

void TcpPacket::Header::setAckNum ( uint32_t ackNum )
{
    uint32_t netOrderAckNum = htonl ( ackNum );

    if ( ack_num == netOrderAckNum )
        return;

    IpPacket::adjustChecksum ( checksum, ack_num, netOrderAckNum );

    ack_num = netOrderAckNum;
}
