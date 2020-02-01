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

#include <cstdio>

#include "basic/MsvcSupport.hpp"

#include "DnsPacket.hpp"

#define CASE_ID( cat, code ) \
    case cat ## code: \
        return ( #code ); break

using namespace Pravala;

bool DnsPacket::Query::operator!= ( const DnsPacket::Query & other ) const
{
    return !( operator== ( other ) );
}

bool DnsPacket::Query::operator== ( const DnsPacket::Query & other ) const
{
    return ( data.rType == other.data.rType
             && data.rClass == other.data.rClass
             && name == other.name );
}

size_t Pravala::getHash ( const DnsPacket::Query & key )
{
    return Pravala::getHash ( key.name ) ^ Pravala::getHash ( key.data.rType, key.data.rClass );
}

const DnsPacket::Header * DnsPacket::getHeader() const
{
    if ( _data.size() < sizeof ( Header ) )
        return 0;

    // To make sure the alignment is correct!
    assert ( ( ( size_t ) _data.get() ) % 4U == 0 );

    return reinterpret_cast<const Header *> ( _data.get() );
}

bool DnsPacket::setupPacket ( const MemHandle & data )
{
    if ( data.size() < sizeof ( Header ) )
        return false;

    // When UDP is used it shouldn't be more than 512. Just to be safe lets use a little more.
    // We don't support TCP at this point anyway
    if ( data.size() > 768 )
        return false;

    clear();
    _data = data;

    return parsePacket();
}

bool DnsPacket::setupPacket ( const MemVector & data )
{
    if ( data.getDataSize() < sizeof ( Header ) )
        return false;

    // When UDP is used it shouldn't be more than 512. Just to be safe lets use a little more.
    // We don't support TCP at this point anyway
    if ( data.getDataSize() > 768 )
        return false;

    clear();

    if ( !data.storeContinuous ( _data ) )
        return false;

    return parsePacket();
}

void DnsPacket::clear()
{
    _data.clear();
    _queries.clear();
    _cnames.clear();
    _addrs.clear();
    _shortestTtl = 0xFFFFFFFF;
}

bool DnsPacket::parsePacket()
{
    const Header * header = getHeader();

    if ( !header )
        return false;

    uint16_t index = sizeof ( Header );

    List<ResRecord> tmpRecords;

    // We read queries, followed by answers
    // We don't care about authority or additional records, so we read them to a temporary list
    // At the end we make sure that we "consumed" the entire packet.
    // If anything is wrong, we clear the state and return an error:

    if ( !readQueries ( header->getNumQuestions(), index ) )
        return false;

    const uint16_t answerRecords = header->getNumAnswerRecords();
    const uint16_t totalRecords = answerRecords
                                  + header->getNumAuthorityRecords()
                                  + header->getNumAdditionalRecords();

    if ( !readRecords ( tmpRecords, totalRecords, index ) || index != _data.size() )
        return false;

    for ( size_t i = 0; i < tmpRecords.size(); ++i )
    {
        const ResRecord & rec = tmpRecords.at ( i );

        if ( rec.data.rTTL < _shortestTtl )
            _shortestTtl = rec.data.rTTL;

        if ( i >= answerRecords || rec.data.rClass != ClassIN )
            continue;

        if ( rec.data.rType == RRTypeA )
        {
            if ( rec.data.rDataSize != 4U || rec.rDataOffset + 4U > _data.size() )
                return false;

            IpAddress addr;

            addr.setupV4Memory ( _data.get() + rec.rDataOffset );

            if ( !addr.isValid() )
                return false;

            _addrs.insert ( addr );
        }
        else if ( rec.data.rType == RRTypeAAAA )
        {
            if ( rec.data.rDataSize != 16U || rec.rDataOffset + 16U > _data.size() )
                return false;

            IpAddress addr;

            addr.setupV6Memory ( _data.get() + rec.rDataOffset );

            if ( !addr.isValid() )
                return false;

            _addrs.insert ( addr );
        }
        else if ( rec.data.rType == RRTypeCNAME )
        {
            String str;

            uint16_t tmpIndex = rec.rDataOffset;

            if ( !readDomainName ( _data.get(), _data.size(), str, tmpIndex )
                 || tmpIndex != rec.rDataOffset + rec.data.rDataSize )
                return false;

            _cnames.append ( str );
        }
    }

    return true;
}

void DnsPacket::describe ( Buffer & toBuffer ) const
{
    // To make sure the alignment is correct!
    assert ( ( ( size_t ) _data.get() ) % 4U == 0 );

    const Header * header = reinterpret_cast<const Header *> ( _data.get() );

    if ( !header )
        return;

    toBuffer.append ( "Identifier: 0x" );
    toBuffer.append ( String::number ( header->getIdentifier(), String::Int_HEX, 4, true ) );
    toBuffer.append ( ( header->flagsA & FlagAResponse ) ? "; DNS-Response [" : "; DNS-Query [" );

    if ( header->flagsA & FlagATruncated )
        toBuffer.append ( "T" );

    if ( header->flagsA & FlagAAuthorativeAnswer )
        toBuffer.append ( "A" );

    if ( header->flagsB & FlagBRecursionAvailable )
        toBuffer.append ( "Ra" );

    toBuffer.append ( "]; Type: " );
    toBuffer.append ( opCode2Str ( header->getOperationCode() ) );
    toBuffer.append ( "; RespCode: " );
    toBuffer.append ( respCode2Str ( header->getResponseCode() ) );

    if ( _shortestTtl != 0xFFFFFFFF )
    {
        toBuffer.append ( "; ShortestTTL: " );
        toBuffer.append ( String::number ( _shortestTtl ) );
    }

    toBuffer.append ( "; Questions: " );
    toBuffer.append ( String::number ( header->getNumQuestions() ) );
    toBuffer.append ( "; AnswerRRs: " );
    toBuffer.append ( String::number ( header->getNumAnswerRecords() ) );
    toBuffer.append ( "; AuthRRs: " );
    toBuffer.append ( String::number ( header->getNumAuthorityRecords() ) );
    toBuffer.append ( "; AddRRs: " );
    toBuffer.append ( String::number ( header->getNumAdditionalRecords() ) );

    for ( size_t i = 0; i < _queries.size(); ++i )
    {
        toBuffer.append ( "; Query " );
        toBuffer.append ( String::number ( i ) );
        toBuffer.append ( "; Name: '" );
        toBuffer.append ( _queries.at ( i ).name );
        toBuffer.append ( "'; Type: " );
        toBuffer.append ( rrType2Str ( _queries.at ( i ).data.rType ) );
        toBuffer.append ( "; Class: " );
        toBuffer.append ( class2Str ( _queries.at ( i ).data.rClass ) );
    }

    toBuffer.append ( "; " );

    if ( !_cnames.isEmpty() )
    {
        toBuffer.append ( "CNAMEs: " );

        for ( size_t i = 0; i < _cnames.size(); ++i )
        {
            if ( i > 0 )
                toBuffer.append ( ", " );

            toBuffer.append ( _cnames.at ( i ) );
        }

        toBuffer.append ( "; " );
    }

    if ( !_addrs.isEmpty() )
    {
        toBuffer.append ( "ADDRs: " );
        bool added = false;

        for ( HashSet<IpAddress>::Iterator it ( _addrs ); it.isValid(); it.next() )
        {
            if ( added )
                toBuffer.append ( ", " );

            added = true;
            toBuffer.append ( it.value().toString() );
        }

        toBuffer.append ( "; " );
    }
}

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const DnsPacket & value )
{
    value.describe ( textMessage.getInternalBuf() );

    return textMessage;
}

bool DnsPacket::readQueries ( uint16_t count, uint16_t & index )
{
    _queries.clear();

    while ( count > 0 )
    {
        --count;

        if ( index >= _data.size() )
            return false;

        Query q;

        if ( !readDomainName ( _data.get(), _data.size(), q.name, index ) )
            return false;

        if ( index + sizeof ( Query::Data ) > _data.size() )
            return false;

        Query::Data data;

        memcpy ( &data, _data.get ( index ), sizeof ( Query::Data ) );
        index += sizeof ( Query::Data );

        q.data.rType = ntohs ( data.rType );
        q.data.rClass = ntohs ( data.rClass );

        _queries.append ( q );
    }

    return true;
}

bool DnsPacket::readRecords ( List<ResRecord> & records, uint16_t count, uint16_t & index )
{
    records.clear();

    while ( count > 0 )
    {
        --count;

        if ( index >= _data.size() )
            return false;

        ResRecord rec;

        if ( !readDomainName ( _data.get(), _data.size(), rec.name, index ) )
            return false;

        if ( index + sizeof ( ResRecord::Data ) > _data.size() )
            return false;

        ResRecord::Data data;

        memcpy ( &data, _data.get ( index ), sizeof ( ResRecord::Data ) );

        index += sizeof ( data );

        rec.data.rType = ntohs ( data.rType );
        rec.data.rClass = ntohs ( data.rClass );
        rec.data.rTTL = ntohl ( data.rTTL );
        rec.data.rDataSize = ntohs ( data.rDataSize );

        if ( ( size_t ) ( index + rec.data.rDataSize ) > _data.size() )
            return false;

        rec.rDataOffset = index;

        index += rec.data.rDataSize;

        records.append ( rec );
    }

    return true;
}

bool DnsPacket::readDomainName ( const char * packetData, uint16_t packetSize, String & output, uint16_t & index )
{
    if ( !packetData )
        return false;

    const uint16_t initIndex = index;

    while ( index < packetSize )
    {
        // top 2 bits: flags
        const uint8_t flags = ( packetData[ index ] >> 6 ) & 0x03U;

        // next 6 bits: length (or part of the offset)
        const uint8_t lenOrOffset = packetData[ index++ ] & 0x3FU;

        if ( flags == 3 )
        {
            // This (and the next byte) contains pointer offset (11 flags)

            if ( index >= packetSize )
                return false;

            uint16_t ptrOffset = ( lenOrOffset << 8 ) | ( ( uint8_t ) packetData[ index++ ] );

            // If there is anything wrong with the ptrOffset, internal readDomainName will return 'false'.
            // Also, we use initIndex as the new packet size to prevent infinite recursions caused by evil pointers.
            // This will ensure that:
            // - we only jump back
            // - we never process any part of the data that was already processed
            // In theory forward pointers could be just fine. For example, wireshark is happy with them.
            // But other tools, like 'host' and 'nslookup' complain about messed up pointers.
            // If we wanted to allow them, detecting loops would have to be more complicated...
            // We pass ptrOffset as the last argument (index that gets modified) and we don't need to modify
            // our own index, since the data read here doesn't affect how much memory we "consume" at this level.
            return readDomainName ( packetData, initIndex, output, ptrOffset );
        }
        else if ( flags != 0 )
        {
            // Some other flags (01 or 10) - incorrect!
            return false;
        }

        // This is a length field (00 flags)

        if ( lenOrOffset == 0 )
        {
            // End of string!
            return true;
        }

        if ( index + lenOrOffset > packetSize )
        {
            // Not enough bytes left!
            return false;
        }

        if ( !output.isEmpty() )
            output.append ( '.' );

        output.append ( packetData + index, lenOrOffset );
        index += lenOrOffset;
    }

    // index is too large!
    return false;
}

const char * DnsPacket::opCode2Str ( uint8_t code )
{
    switch ( ( OpCodes ) code )
    {
        CASE_ID ( OpCode, Query );
        CASE_ID ( OpCode, Notify );
        CASE_ID ( OpCode, Status );
        CASE_ID ( OpCode, Update );
    }

    static char descBuf[ 32 ];

    snprintf ( descBuf, 32, "Unknown: %d\n", code );
    descBuf[ 31 ] = 0;

    return descBuf;
}

const char * DnsPacket::respCode2Str ( uint8_t code )
{
    switch ( ( RespCodes ) code )
    {
        CASE_ID ( Resp, NoError );
        CASE_ID ( Resp, FormatError );
        CASE_ID ( Resp, ServerFailure );
        CASE_ID ( Resp, NameError );
        CASE_ID ( Resp, NotImplemented );
        CASE_ID ( Resp, Refused );
        CASE_ID ( Resp, YXDomain );
        CASE_ID ( Resp, YXRRSet );
        CASE_ID ( Resp, NXRRSet );
        CASE_ID ( Resp, NotAuth );
        CASE_ID ( Resp, NotZone );
    }

    static char descBuf[ 32 ];

    snprintf ( descBuf, 32, "Unknown: %d\n", code );
    descBuf[ 31 ] = 0;

    return descBuf;
}

const char * DnsPacket::rrType2Str ( uint16_t code )
{
    switch ( ( RRTypes ) code )
    {
        CASE_ID ( RRType, A );
        CASE_ID ( RRType, AAAA );
        CASE_ID ( RRType, CNAME );
        CASE_ID ( RRType, LOC );
        CASE_ID ( RRType, MX );
        CASE_ID ( RRType, NS );
        CASE_ID ( RRType, PTR );
        CASE_ID ( RRType, SOA );
        CASE_ID ( RRType, SRV );
        CASE_ID ( RRType, TXT );

        CASE_ID ( RRType, QAll );
    }

    static char descBuf[ 32 ];

    snprintf ( descBuf, 32, "Unknown: %d\n", code );
    descBuf[ 31 ] = 0;

    return descBuf;
}

const char * DnsPacket::class2Str ( uint16_t code )
{
    switch ( ( ClassTypes ) code )
    {
        CASE_ID ( Class, IN );
        CASE_ID ( Class, QAny );
    }

    static char descBuf[ 32 ];

    snprintf ( descBuf, 32, "Unknown: %d\n", code );
    descBuf[ 31 ] = 0;

    return descBuf;
}
