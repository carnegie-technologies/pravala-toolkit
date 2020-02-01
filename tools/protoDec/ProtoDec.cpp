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

#include "proto/ProtocolCodec.hpp"
#include "proto/Serializable.hpp"
#include "config/ConfigSwitch.hpp"
#include "config/ConfigNumber.hpp"
#include "ProtoDec.hpp"

using namespace Pravala;

static ConfigSwitch swIdPath ( "id-path", 'p', "Use the whole ID 'path' for each field." );
static ConfigSwitch swNoInline ( "no-inline-values", 'I', "Never put values in the same line as the field ID." );
static ConfigSwitch swDecAll ( "decode-all", 'a',
        "Decode all fields using all working decoding types, except for binary dumps." );
static ConfigSwitch swDecAllBin ( "decode-all-bin", 'A',
        "Decode all fields using all working decoding types AND binary dumps." );
static ConfigSwitch swOnlyHdrOff ( "only-header-offset", 'O',
        "Only include the offset in the field header line." );
static ConfigSwitch swFieldHdrOff ( "use-header-offset", 'H',
        "Use field header's offset for both header and payload." );
static ConfigSwitch swFieldPayloadOff ( "use-payload-offset", 'P',
        "Use field payload's offset for both header and payload." );

static ConfigLimitedNumber<uint16_t> optMinStrLen (
        0,
        "min-str-len",
        's',
        "",
        "The minimum length of string that will prevent the field from being deserialized as something else "
        "(unless one of the 'decode-all' options is used as well)",
        1, 0xFFFF, 5 );

static ConfigLimitedNumber<uint8_t> optBinDumpColSize (
        0,
        "bin-dump-col-size",
        'D',
        "",
        "The number of bytes in each column of the binary dump",
        1, 0xFF, 8 );

static ConfigLimitedNumber<uint8_t> optBinDumpColNum (
        0,
        "bin-dump-col-num",
        'N',
        "",
        "The number of columns of the binary dump",
        1, 0xFF, 4 );

ProtoDec::ProtoDec ( const MemHandle & buf ):
    _buf ( buf ),
    _fieldIdWidth ( 0 ),
    _fieldSizeWidth ( 0 )
{
    if ( optBinDumpColNum.value() < 1 )
        optBinDumpColNum.setValue ( 4 );

    if ( optBinDumpColSize.value() < 1 )
        optBinDumpColSize.setValue ( 8 );
}

ERRCODE ProtoDec::decode ( StringList & output )
{
    output.clear();

    size_t offset = 0;
    size_t maxFieldId = 0;
    size_t maxFieldSize = 0;

    while ( offset < _buf.size() )
    {
        uint8_t wireType = 0;
        uint32_t fieldId = 0;
        size_t fieldSize = 0;

        ERRCODE eCode = ProtocolCodec::readFieldHeader (
            _buf.get(), _buf.size(), offset, wireType, fieldId, fieldSize );

        // We don't return, we're just trying to figure out the max ID and size!
        if ( NOT_OK ( eCode ) )
            break;

        offset += fieldSize;

        // We don't return, we're just trying to figure out the max ID and size!
        if ( offset > _buf.size() )
            break;

        if ( maxFieldId < fieldId )
            maxFieldId = fieldId;

        if ( maxFieldSize < fieldSize )
            maxFieldSize = fieldSize;
    }

    _fieldIdWidth = 1;

    while ( ( maxFieldId /= 10 ) != 0 )
    {
        ++_fieldIdWidth;
    }

    _fieldSizeWidth = 1;

    while ( ( maxFieldSize /= 10 ) != 0 )
    {
        ++_fieldSizeWidth;
    }

    List<Entry> entries;

    ERRCODE eCode = decodeData ( "", 0, _buf.size(), 0, entries );

    if ( entries.isEmpty() )
        return eCode;

    size_t max = entries.last().offTo;
    size_t offWidth = 1;

    while ( ( max /= 10 ) != 0 )
    {
        ++offWidth;
    }

    size_t lastIndent = 0;
    size_t lastFrom = 0;
    size_t lastTo = 0;
    bool lastIsHdr = false;
    String prefix;

    String prefixFiller ( "    " ); // For '[:] '

    for ( size_t i = 0; i < offWidth; ++i )
    {
        prefixFiller.append ( "  " ); // For each digit in both 'from' and 'to' offset
    }

    for ( size_t i = 0; i < entries.size(); ++i )
    {
        const Entry & e = entries.at ( i );

        if ( prefix.isEmpty()
             || e.indent != lastIndent
             || e.offFrom != lastFrom
             || e.offTo != lastTo
             || ( swOnlyHdrOff.isSet() && e.isHdr != lastIsHdr ) )
        {
            lastFrom = e.offFrom;
            lastTo = e.offTo;
            lastIndent = e.indent;
            lastIsHdr = e.isHdr;

            if ( !lastIsHdr && swOnlyHdrOff.isSet() )
            {
                prefix = prefixFiller;
            }
            else
            {
                prefix = String ( "[%1:%2] " )
                         .arg ( String::number ( lastFrom, String::Int_Dec, offWidth, true ) )
                         .arg ( String::number ( lastTo - 1, String::Int_Dec, offWidth, true ) );
            }

            for ( size_t j = 0; j < lastIndent; ++j )
            {
                prefix.append ( " " );
            }
        }

        String str = prefix;
        str.append ( e.data );

        output.append ( str );
    }

    return eCode;
}

const char * ProtoDec::getWireType ( uint8_t wireType )
{
    switch ( wireType )
    {
        case ProtocolCodec::WireTypeZero:
            return " Z";

        case ProtocolCodec::WireType1Byte:
            return "1B";

        case ProtocolCodec::WireType2Bytes:
            return "2B";

        case ProtocolCodec::WireType4Bytes:
            return "4B";

        case ProtocolCodec::WireType8Bytes:
            return "8B";

        case ProtocolCodec::WireTypeLengthDelim:
            return " L";

        case ProtocolCodec::WireTypeVariableLengthA:
            return "VA";

        case ProtocolCodec::WireTypeVariableLengthB:
            return "VB";

        default:
            return "INVALID";
    }
}

ERRCODE ProtoDec::decodeData (
        const String & idPath,
        size_t offset, size_t dataSize, size_t indent, List<Entry> & output )
{
    ERRCODE eCode;

    const size_t bufSize = offset + dataSize;

    if ( bufSize > _buf.size() )
        return Error::InternalError;

    while ( offset < bufSize )
    {
        uint8_t wireType = 0;
        uint32_t fieldId = 0;
        size_t fieldSize = 0;

        const size_t hdrOffset = offset;

        eCode = ProtocolCodec::readFieldHeader ( _buf.get(), bufSize, offset, wireType, fieldId, fieldSize );

        if ( NOT_OK ( eCode ) )
            return eCode;

        String path;

        if ( !swIdPath.isSet() )
        {
            path = String::number ( fieldId, String::Int_Dec, _fieldIdWidth );
        }
        else
        {
            path = idPath;

            if ( !path.isEmpty() )
                path.append ( "." );

            path.append ( String::number ( fieldId ) );
        }

        dumpField ( path, hdrOffset, offset, fieldId, fieldSize, wireType, indent, output );

        offset += fieldSize;

        if ( offset > bufSize )
            return Error::InternalError;
    }

    return Error::Success;
}

void ProtoDec::dumpData (
        size_t hdrOffset, size_t offset, size_t fieldSize,
        size_t indent, List< ProtoDec::Entry > & output )
{
    if ( fieldSize < 1 )
        return;

    Entry e ( swFieldHdrOff.isSet() ? hdrOffset : offset, offset + fieldSize, indent, false );

    StringList hCols;
    StringList sCols;

    hCols.append ( "" );
    sCols.append ( "" );

    for ( size_t i = 0; i < fieldSize; ++i )
    {
        const uint8_t h = ( uint8_t ) _buf.get()[ offset + i ];

        assert ( !hCols.isEmpty() );
        assert ( !sCols.isEmpty() );

        if ( hCols.last().length() >= 2 * optBinDumpColSize.value() )
        {
            hCols.append ( "" );
            sCols.append ( "" );
        }

        hCols.last().append ( String::number ( h, String::Int_HEX, 2, true ) );

        if ( h >= 32 && h <= 126 )
        {
            sCols.last().append ( ( char ) h );
        }
        else
        {
            sCols.last().append ( '.' );
        }
    }

    // Fill the last column.
    while ( hCols.last().length() < 2 * optBinDumpColSize.value() )
    {
        hCols.last().append ( "  " );
        sCols.last().append ( ' ' );
    }

    String emptyHCol;
    String emptySCol;

    for ( unsigned i = 0; i < optBinDumpColSize.value(); ++i )
    {
        emptyHCol.append ( "  " );
        emptySCol.append ( ' ' );
    }

    // Add missing columns.
    while ( hCols.size() % optBinDumpColNum.value() != 0 )
    {
        hCols.append ( emptyHCol );
        sCols.append ( emptySCol );
    }

    size_t offWidth = 1;

    while ( ( fieldSize /= 10 ) != 0 )
    {
        ++offWidth;
    }

    size_t binOffset = 0;
    String line;

    assert ( hCols.size() == sCols.size() );

    if ( hCols.isEmpty() )
        return;

    size_t hCol = 0;
    size_t sCol = 0;

    do
    {
        if ( line.isEmpty() )
        {
            line = String::number ( binOffset, String::Int_Dec, offWidth );
            line.append ( ":" );
        }

        line.append ( ' ' );
        line.append ( hCols.at ( hCol ) );
        binOffset += hCols.at ( hCol ).length() / 2;

        ++hCol;

        if ( ( hCol % optBinDumpColNum.value() ) == 0 )
        {
            line.append ( "  [" );

            while ( sCol < hCol )
            {
                line.append ( sCols.at ( sCol ) );
                sCol++;

                if ( sCol < hCol )
                    line.append ( ' ' );
            }

            line.append ( "]" );
            output.append ( e.set ( line ) );
            line.clear();
        }
    }
    while ( hCol < hCols.size() );
}

template<typename U, typename S> static bool tryNumber (
        const char * buf, size_t size, uint8_t wireType,
        uint64_t & p, int64_t & n )
{
    U tmpPos;
    S tmpNeg;

    if ( IS_OK ( ProtocolCodec::decode ( buf, size, wireType, tmpPos ) )
         && IS_OK ( ProtocolCodec::decode ( buf, size, wireType, tmpNeg ) ) )
    {
        p = tmpPos;
        n = tmpNeg;
        return true;
    }

    return false;
}

void ProtoDec::dumpField (
        const String & idPath, size_t hdrOffset, size_t offset,
        uint8_t fieldId, size_t fieldSize, uint8_t wireType,
        size_t indent, List<Entry> & output )
{
    List<Entry> values;

    bool inlineValue = false;
    dumpFieldValue ( idPath, hdrOffset, offset, fieldId, fieldSize, wireType, indent + 1, values, inlineValue );

    Entry e ( swFieldPayloadOff.isSet() ? offset : hdrOffset, offset + fieldSize, indent, true );

    if ( values.size() == 1 && inlineValue && !swNoInline.isSet() )
    {
        output.append ( e.set ( String ( "ID: %1; Type: %2; Size: %3; %4" )
                                .arg ( idPath )
                                .arg ( getWireType ( wireType ) )
                                .arg ( String::number ( fieldSize, String::Int_Dec, _fieldSizeWidth ) )
                                .arg ( values.first().data ) ) );
        return;
    }

    output.append ( e.set ( String ( "ID: %1; Type: %2; Size: %3" )
                            .arg ( idPath )
                            .arg ( getWireType ( wireType ) )
                            .arg ( String::number ( fieldSize, String::Int_Dec, _fieldSizeWidth ) ) ) );

    for ( size_t i = 0; i < values.size(); ++i )
    {
        output.append ( values.at ( i ) );
    }
}

void ProtoDec::dumpFieldValue (
        const String & idPath, size_t hdrOffset, size_t offset,
        uint8_t fieldId, size_t fieldSize, uint8_t wireType,
        size_t indent, List< ProtoDec::Entry > & output, bool & inlineValue )
{
    inlineValue = false;

    Entry e ( swFieldHdrOff.isSet() ? hdrOffset : offset, offset + fieldSize, indent, false );

    if ( wireType == ProtocolCodec::WireTypeZero )
    {
        output.append ( e.set ( "Zero" ) );
        inlineValue = true;
        return;
    }

    const char * mem = _buf.get ( offset );
    const bool decAll = ( swDecAll.isSet() || swDecAllBin.isSet() );

    if ( !decAll && fieldId == Serializable::LengthVarFieldId )
    {
        // This is (most likely) a length field. Let's try to decode it as a number first!

        Serializable::LengthVarType value = 0;

        ERRCODE eCode = ProtocolCodec::decode ( mem, fieldSize, wireType, value );

        if ( IS_OK ( eCode ) )
        {
            output.append ( e.set ( String ( "Number : %1" ).arg ( value ) ) );
            inlineValue = true;
            return;
        }
    }

    if ( wireType == ProtocolCodec::WireTypeVariableLengthB )
    {
        // This can only be a negative number (at least for now).

        int64_t value = 0;
        ERRCODE eCode = ProtocolCodec::decode ( mem, fieldSize, wireType, value );

        if ( IS_OK ( eCode ) )
        {
            output.append ( e.set ( String ( "Number : %1" ).arg ( value ) ) );

            if ( swDecAllBin.isSet() )
            {
                // Decode binary data as well.
                output.append ( e.set ( "DATA:" ) );
                dumpData ( hdrOffset, offset, fieldSize, e.indent + 1, output );
            }
            else
            {
                inlineValue = true;
            }

            return;
        }

        output.append ( e.set ( String ( "Invalid value encoded using VAR-LEN-B encoding: %1; DATA:" )
                                .arg ( eCode.toString() ) ) );
        dumpData ( hdrOffset, offset, fieldSize, e.indent + 1, output );
        return;
    }

    // Different things can be encoded in different ways. Let's try different types!
    // If all of them fail, let's just dump data!

    bool decodedValue = false;

    if ( wireType != ProtocolCodec::WireTypeVariableLengthA )
    {
        // This could be a string:
        String str;

        if ( IS_OK ( ProtocolCodec::decode ( mem, fieldSize, wireType, str ) ) && !str.isEmpty() )
        {
            bool allPrintable = true;

            for ( int i = 0; i < str.length(); ++i )
            {
                if ( str[ i ] < 32 || str[ i ] > 126 )
                {
                    allPrintable = false;
                    break;
                }
            }

            if ( allPrintable )
            {
                output.append ( e.set ( String ( "String : '%1'" ).arg ( str ) ) );

                decodedValue = true;

                // This is a pretty long string. Let's just stop here...
                if ( !decAll && str.length() >= optMinStrLen.value() )
                {
                    inlineValue = true;
                    return;
                }
            }
        }

        // Or an embedded structure:
        List<Entry> entries;

        if ( IS_OK ( decodeData ( idPath, offset, fieldSize, e.indent + 1, entries ) ) )
        {
            for ( size_t i = 0; i < entries.size(); ++i )
            {
                output.append ( entries.at ( i ) );
            }

            decodedValue = true;

            if ( !decAll )
                return;
        }

        // Or an IP Address:
        IpAddress ipAddr;

        if ( IS_OK ( ProtocolCodec::decode ( mem, fieldSize, wireType, ipAddr ) ) )
        {
            output.append ( e.set ( String ( "Address: %1" ).arg ( ipAddr ) ) );
            decodedValue = true;
        }
    }

    // Numeric values:

    uint64_t uValue = 0;
    int64_t sValue = 0;

    if ( tryNumber<uint8_t, int8_t> ( mem, fieldSize, wireType, uValue, sValue )
         || tryNumber<uint16_t, int16_t> ( mem, fieldSize, wireType, uValue, sValue )
         || tryNumber<uint64_t, int32_t> ( mem, fieldSize, wireType, uValue, sValue )
         || tryNumber<uint64_t, int64_t> ( mem, fieldSize, wireType, uValue, sValue ) )
    {
        String uStr = String::number ( uValue );
        String sStr = String::number ( sValue );

        if ( uStr == sStr )
        {
            output.append ( e.set ( String ( "Number : %1" ).arg ( uStr ) ) );
        }
        else
        {
            output.append ( e.set ( String ( "Number : %1 / %2" ).arg ( uStr, sStr ) ) );
        }

        inlineValue = true;
        decodedValue = true;
    }
    else if ( wireType == ProtocolCodec::WireTypeVariableLengthA )
    {
        // Variable length encoding and we failed to decode this as a number.
        // This is incorrect!
        output.append ( e.set ( "Invalid value encoded using VAR-LEN-A encoding; DATA:" ) );
        dumpData ( hdrOffset, offset, fieldSize, e.indent + 1, output );
        return;
    }

    if ( !decodedValue || swDecAllBin.isSet() )
    {
        output.append ( e.set ( "DATA:" ) );
        dumpData ( hdrOffset, offset, fieldSize, e.indent + 1, output );
    }
}
