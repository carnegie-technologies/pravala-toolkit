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

#include "basic/FloatingPointUtils.hpp"

#include "ProtocolCodec.hpp"

using namespace Pravala;

ProtoError ProtocolCodec::readFieldHeader (
        const char * buffer, size_t bufSize, size_t & offset,
        uint8_t & wireType, uint32_t & fieldId, size_t & fieldSize )
{
    assert ( buffer != 0 );

    fieldSize = 0;

    if ( !buffer )
        return ProtoError::InvalidParameter;

    // Not enough data in the buffer
    if ( bufSize <= offset )
        return ProtoError::IncompleteData;

    uint8_t byteVal = buffer[ offset++ ];

    // We read wire type - first 3 bits
    wireType = byteVal & 0x7;
    fieldId = ( uint32_t ) ( ( byteVal >> 3 ) & ( 0x0F ) );

    uint8_t shift = 4;

    // Overflow bit is set
    while ( ( byteVal & 0x80 ) != 0 )
    {
        // Not enough data in the buffer
        if ( bufSize <= offset )
            return ProtoError::IncompleteData;

        // shifts:
        // 4, 11, 18, 25, 32
        if ( shift > 25 )
            return ProtoError::ProtocolError;

        byteVal = buffer[ offset++ ];

        // We use current byte, take 7 bits from it (the eight one is
        // used as overflow bit), and shift it to add it to already read bits
        fieldId |= ( ( ( uint32_t ) ( byteVal & 0x7F ) ) << shift );

        shift += 7;
    }

    // Here byteVal should be at the LAST byte of field "header".
    assert ( ( byteVal & 0x80 ) == 0 );

    // Offset points at the first byte of the payload (or the length field)

    switch ( wireType )
    {
        case ProtocolCodec::WireTypeZero:
            fieldSize = 0;
            break;

        case ProtocolCodec::WireType1Byte:
            fieldSize = 1;
            break;

        case ProtocolCodec::WireType2Bytes:
            fieldSize = 2;
            break;

        case ProtocolCodec::WireType4Bytes:
            fieldSize = 4;
            break;

        case ProtocolCodec::WireType8Bytes:
            fieldSize = 8;
            break;

        case ProtocolCodec::WireTypeLengthDelim:
            {
                fieldSize = 0;
                shift = 0;

                do
                {
                    if ( bufSize <= offset )
                        return ProtoError::IncompleteData;

                    // shifts:
                    // 0, 7, 14, 21, 28, 37
                    if ( shift > 28 )
                        return ProtoError::ProtocolError;

                    byteVal = buffer[ offset++ ];

                    // The last byte can have only up to 4 bits set!
                    if ( shift == 28 && ( byteVal & 0x0F ) != byteVal )
                        return ProtoError::ProtocolError;

                    // We use current byte, take 7 bits from it (the eight one is
                    // used as overflow bit), and shift it to add it to already read bits
                    fieldSize |= ( ( ( uint32_t ) ( byteVal & 0x7F ) ) << shift );

                    // We just read 7 bits, next read will be in front of them
                    shift += 7;
                }
                while ( ( byteVal & 0x80 ) != 0 );
            }
            break;

        case ProtocolCodec::WireTypeVariableLengthA:
        case ProtocolCodec::WireTypeVariableLengthB:
            {
                fieldSize = 0;

                // We can't modify the original offset, since we are not consuming the header anymore.
                // This is the actual payload.

                size_t tmpOffset = offset;

                do
                {
                    // Not enough data in the buffer
                    if ( bufSize <= tmpOffset )
                        return ProtoError::IncompleteData;

                    ++fieldSize;
                }
                while ( ( ( ( uint8_t ) buffer[ tmpOffset++ ] ) & 0x80 ) != 0 );
            }
            break;

        default:
            return ProtoError::ProtocolError;
            break;
    }

    if ( offset + fieldSize > bufSize )
        return ProtoError::IncompleteData;

    return ProtoError::Success;
}

template<typename TYPE> ProtoError tDecodeInt (
        const char * buffer, size_t dataSize, uint8_t wireType, TYPE & value )
{
    value = 0;
    uint8_t shift = 0;
    size_t offset = 0;

    if ( wireType == ProtocolCodec::WireTypeVariableLengthA
         || wireType == ProtocolCodec::WireTypeVariableLengthB )
    {
        // Each byte of data carries 7 bits of the actual value.
        // Also, up to 6 bits of the last byte can be wasted.
        if ( dataSize * 7 > sizeof ( value ) * 8 + 6 )
        {
            return ProtoError::InvalidDataSize;
        }

        for ( size_t i = 0; i < dataSize; ++i, shift += 7 )
        {
            value |= ( ( ( TYPE ) ( buffer[ offset++ ] & 0x7F ) ) << shift );
        }

        if ( wireType == ProtocolCodec::WireTypeVariableLengthB )
        {
            value *= -1;
        }
    }
    else
    {
        if ( dataSize > sizeof ( value ) )
            return ProtoError::InvalidDataSize;

        for ( size_t i = 0; i < dataSize; ++i, shift += 8 )
        {
            value |= ( ( ( TYPE ) ( buffer[ offset++ ] & 0xFF ) ) << shift );
        }
    }

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, bool & value )
{
    uint8_t tmpVal = 0;

    const ProtoError err = tDecodeInt<uint8_t> ( buffer, dataSize, wireType, tmpVal );

    if ( err == ProtoError::Success )
    {
        if ( tmpVal > 1 )
            return ProtoError::InvalidDataSize;

        value = ( tmpVal != 0 );
    }

    return err;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, int8_t & value )
{
    // To avoid signed overflow issues we force the decoding to happen on unsigned type.
    return tDecodeInt<uint8_t> ( buffer, dataSize, wireType, ( uint8_t & ) value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint8_t & value )
{
    return tDecodeInt<uint8_t> ( buffer, dataSize, wireType, value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, int16_t & value )
{
    // To avoid signed overflow issues we force the decoding to happen on unsigned type.
    return tDecodeInt<uint16_t> ( buffer, dataSize, wireType, ( uint16_t & ) value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint16_t & value )
{
    return tDecodeInt<uint16_t> ( buffer, dataSize, wireType, value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, int32_t & value )
{
    // To avoid signed overflow issues we force the decoding to happen on unsigned type.
    return tDecodeInt<uint32_t> ( buffer, dataSize, wireType, ( uint32_t & ) value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint32_t & value )
{
    return tDecodeInt<uint32_t> ( buffer, dataSize, wireType, value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, int64_t & value )
{
    // To avoid signed overflow issues we force the decoding to happen on unsigned type.
    return tDecodeInt<uint64_t> ( buffer, dataSize, wireType, ( uint64_t & ) value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint64_t & value )
{
    return tDecodeInt<uint64_t> ( buffer, dataSize, wireType, value );
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, float & value )
{
    value = 0;
    uint32_t tmpVal = 0;
    ProtoError ret = tDecodeInt<uint32_t> ( buffer, dataSize, wireType, tmpVal );

    if ( NOT_OK ( ret ) )
        return ret;

    value = FloatingPointUtils::unpack754 ( tmpVal );

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, double & value )
{
    value = 0;
    uint64_t tmpVal = 0;
    ProtoError ret = tDecodeInt<uint64_t> ( buffer, dataSize, wireType, tmpVal );

    if ( NOT_OK ( ret ) )
        return ret;

    value = FloatingPointUtils::unpack754 ( tmpVal );

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, String & value )
{
    value.clear();

    if ( wireType == WireTypeVariableLengthA || wireType == WireTypeVariableLengthB )
        return ProtoError::ProtocolError;

    if ( dataSize > 0 )
        value.append ( buffer, ( int ) dataSize );

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, Buffer & toBuffer )
{
    if ( wireType == WireTypeVariableLengthA || wireType == WireTypeVariableLengthB )
        return ProtoError::ProtocolError;

    if ( dataSize > 0 )
        toBuffer.appendData ( buffer, dataSize );

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, IpAddress & value )
{
    value.clear();

    if ( wireType == WireTypeVariableLengthA || wireType == WireTypeVariableLengthB )
        return ProtoError::ProtocolError;

    switch ( dataSize )
    {
        case 4:
            // IPv4
            value.setupV4Memory ( buffer );
            break;

        case 16:
            // IPv6
            value.setupV6Memory ( buffer );
            break;

        case 0:
            // We 'read' incorrect IpAddress
            break;

        default:
            return ProtoError::ProtocolError;
            break;
    }

    return ProtoError::Success;
}

ProtoError ProtocolCodec::decode ( const char * buffer, size_t dataSize, uint8_t wireType, Timestamp & value )
{
    uint64_t binValue = 0;

    ProtoError ret = tDecodeInt<uint64_t> ( buffer, dataSize, wireType, binValue );

    if ( ret != ProtoError::Success )
        return ret;

    if ( !value.setBinValue ( binValue ) )
        return ProtoError::ProtocolError;

    return ProtoError::Success;
}

ProtoError ProtocolCodec::encodeFieldHeader (
        Buffer & buffer, uint32_t fieldId, uint8_t wireType, size_t dataSize )
{
    if ( ( wireType & 0x07 ) != wireType )
        return ProtoError::InvalidParameter;

    assert ( ( wireType & 0x07 ) == wireType );

    // Max amount of memory:
    // 1 byte for wire type
    // up to 5 bytes for field ID
    // up to dataSize bytes for the value
    // up to 5 bytes for the length (if wireType = WireTypeLengthDelim)
    const size_t bufSize = 1 + 5 + dataSize + ( ( wireType == WireTypeLengthDelim ) ? 5 : 0 );

    uint8_t * bufMem = ( uint8_t * ) buffer.getAppendable ( bufSize );

    if ( !bufMem )
        return ProtoError::MemoryError;

    size_t off = 0;

    // First 3 bits carry the wire type.
    // Next 4 bits carry the first 4 bits of the field ID.
    bufMem[ off ] = ( wireType & 0x07 ) | ( ( fieldId & 0x0F ) << 3 );

    // Remove those 4 bits from the field ID:
    fieldId >>= 4;

    while ( fieldId > 0 )
    {
        // Field ID is longer - set the overflow bit
        bufMem[ off++ ] |= 0x80;

        assert ( off < bufSize );

        if ( off >= bufSize )
            return ProtoError::InternalError;

        // Next byte will contain next 7 bits of the field ID
        bufMem[ off ] = ( fieldId & 0x7F );

        // Remove those 7 bits of the field ID:
        fieldId >>= 7;
    }

    ++off;

    if ( wireType == WireTypeLengthDelim )
    {
        assert ( off < bufSize );

        if ( off >= bufSize )
            return ProtoError::InternalError;

        bufMem[ off ] = ( dataSize & 0x7F );
        dataSize >>= 7;

        while ( dataSize > 0 )
        {
            // Length value is longer - set the overflow bit
            bufMem[ off++ ] |= 0x80;

            assert ( off < bufSize );

            if ( off >= bufSize )
                return ProtoError::InternalError;

            // Next byte will contain next 7 bits of the length value
            bufMem[ off ] = ( dataSize & 0x7F );

            dataSize >>= 7;
        }

        ++off;
    }

    buffer.markAppended ( off );

    return ProtoError::Success;
}

/// @brief Integer encoding modes.
enum EncodingMode
{
    /// @brief Data is encoding using the best method for the given positive value.
    EncNormal = 0,

    /// @brief The value given is treated as positive, but WireTypeVariableLengthB will be used.
    /// This means that the decoder will multiply it by -1.
    EncNegative = 1
};

// Clang complaints about invalid shift value in tEncodeInt when the argument of the template is too small.
// That code, however, will not run in that case.
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif

template<typename TYPE> ProtoError tEncodeInt (
        Buffer & buffer, TYPE value, uint32_t fieldId, EncodingMode encMode )
{
    if ( value == 0 )
    {
        // Zero!
        return ProtocolCodec::encodeFieldHeader ( buffer, fieldId, ProtocolCodec::WireTypeZero, 0 );
    }

    // We shouldn't be seeing negative numbers here!
    assert ( value > 0 );

    size_t dataSize = 0;
    uint8_t wireType = 0;

    if ( encMode == EncNegative )
    {
        // Special case for negative numbers.
        // We don't know the data size, so let's assume it's max possible.
        // max uint64_t encoded using variable length encoding will use 10 bytes.
        dataSize = 10;
        wireType = ProtocolCodec::WireTypeVariableLengthB;
    }
    else if ( ( value & ( ( TYPE ) 0xFF ) ) == value )
    {
        // We can fit the value in 1 byte
        dataSize = 1;
        wireType = ProtocolCodec::WireType1Byte;
    }
    else if ( ( value & ( ( TYPE ) 0xFFFF ) ) == value )
    {
        // We can fit the value in 2 bytes
        dataSize = 2;
        wireType = ProtocolCodec::WireType2Bytes;
    }
    else if ( ( value & ( ( TYPE ) 0x1FFFFF ) ) == value )
    {
        // We can fit the value in 3 bytes using variable length encoding
        dataSize = 3;
        wireType = ProtocolCodec::WireTypeVariableLengthA;
    }
    else if ( ( value & ( ( TYPE ) 0xFFFFFFFF ) ) == value )
    {
        // We can fit the value in 4 bytes
        dataSize = 4;
        wireType = ProtocolCodec::WireType4Bytes;
    }
    else if ( ( value & ( ( TYPE ) 0x1FFFFFFFFFFFFULL ) ) == value )
    {
        // We can fit the value in 7 bytes (or less) using variable length encoding
        dataSize = 7;
        wireType = ProtocolCodec::WireTypeVariableLengthA;
    }
    else if ( ( value & ( ( TYPE ) 0xFFFFFFFFFFFFFFFFULL ) ) == value )
    {
        // We can fit the value in 8 bytes
        dataSize = 8;
        wireType = ProtocolCodec::WireType8Bytes;
    }
    else
    {
        return ProtoError::TooBigValue;
    }

    // Since we don't use WireTypeLengthDelim wire type, encodeFieldHeader will only use the dataSize
    // for preallocating the correct buffer size - this is a good thing, so let's pass the correct value here!
    ProtoError ret = ProtocolCodec::encodeFieldHeader ( buffer, fieldId, wireType, dataSize );

    if ( NOT_OK ( ret ) )
        return ret;

    // Size of the data (this time without the header):
    // just size of the actual data.
    // encodeFieldHeader, if successful, marked
    // the header as appended data already.
    uint8_t * bufMem = ( uint8_t * ) buffer.getAppendable ( dataSize );

    if ( !bufMem )
        return ProtoError::MemoryError;

    size_t off = 0;

    if ( wireType == ProtocolCodec::WireTypeVariableLengthA
         || wireType == ProtocolCodec::WireTypeVariableLengthB )
    {
        assert ( value > 0 );

        bufMem[ off ] = ( value & 0x7F );
        value >>= 7;

        while ( value > 0 )
        {
            // Value is longer - set the overflow bit
            bufMem[ off++ ] |= 0x80;

            assert ( off < dataSize );

            if ( off >= dataSize )
                return ProtoError::InternalError;

            // Next byte will contain next 7 bits of the value
            bufMem[ off ] = ( value & 0x7F );

            value >>= 7;
        }

        ++off;
    }
    else
    {
        while ( off < dataSize )
        {
            assert ( off < dataSize );

            bufMem[ off++ ] = ( value & 0xFF );
            value >>= 8;
        }
    }

    buffer.markAppended ( off );

    return ProtoError::Success;
}

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

uint8_t ProtocolCodec::getWireTypeForSize ( size_t dataSize )
{
    switch ( dataSize )
    {
        case 0:
            return WireTypeZero;
            break;

        case 1:
            return WireType1Byte;
            break;

        case 2:
            return WireType2Bytes;
            break;

        case 4:
            return WireType4Bytes;
            break;

        case 8:
            return WireType8Bytes;
            break;

        default:
            return WireTypeLengthDelim;
            break;
    }
}

ProtoError ProtocolCodec::encodeRaw ( Buffer & buffer, const char * data, size_t dataSize, uint32_t fieldId )
{
    if ( dataSize > 0 && !data )
        return ProtoError::InvalidParameter;

    ProtoError ret = encodeFieldHeader ( buffer, fieldId, getWireTypeForSize ( dataSize ), dataSize );

    if ( NOT_OK ( ret ) )
        return ret;

    if ( dataSize > 0 )
    {
        assert ( data != 0 );

        buffer.appendData ( data, dataSize );
    }

    return ProtoError::Success;
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, const String & value, uint32_t fieldId )
{
    return encodeRaw ( buffer, value.c_str(), value.length(), fieldId );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, bool value, uint32_t fieldId )
{
    return tEncodeInt<uint8_t> ( buffer, value ? 1 : 0, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, uint8_t value, uint32_t fieldId )
{
    return tEncodeInt<uint8_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, uint16_t value, uint32_t fieldId )
{
    return tEncodeInt<uint16_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, uint32_t value, uint32_t fieldId )
{
    return tEncodeInt<uint32_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, uint64_t value, uint32_t fieldId )
{
    return tEncodeInt<uint64_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, int8_t value, uint32_t fieldId )
{
    if ( value < 0 )
    {
        // We negate the value AFTER casting it to unsigned type to avoid signed overflow
        return tEncodeInt<uint8_t> ( buffer, -( ( uint8_t ) value ), fieldId, EncNegative );
    }

    return tEncodeInt<uint8_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, int16_t value, uint32_t fieldId )
{
    if ( value < 0 )
    {
        // We negate the value AFTER casting it to unsigned type to avoid signed overflow
        return tEncodeInt<uint16_t> ( buffer, -( ( uint16_t ) value ), fieldId, EncNegative );
    }

    return tEncodeInt<uint16_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, int32_t value, uint32_t fieldId )
{
    if ( value < 0 )
    {
        // We negate the value AFTER casting it to unsigned type to avoid signed overflow
        return tEncodeInt<uint32_t> ( buffer, -( ( uint32_t ) value ), fieldId, EncNegative );
    }

    return tEncodeInt<uint32_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, int64_t value, uint32_t fieldId )
{
    if ( value < 0 )
    {
        // We negate the value AFTER casting it to unsigned type to avoid signed overflow
        return tEncodeInt<uint64_t> ( buffer, -( ( uint64_t ) value ), fieldId, EncNegative );
    }

    return tEncodeInt<uint64_t> ( buffer, value, fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, float value, uint32_t fieldId )
{
    return tEncodeInt<uint32_t> ( buffer, FloatingPointUtils::pack754 ( value ), fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, double value, uint32_t fieldId )
{
    return tEncodeInt<uint64_t> ( buffer, FloatingPointUtils::pack754 ( value ), fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, const IpAddress & value, uint32_t fieldId )
{
    if ( value.isIPv4() )
        return encodeRaw ( buffer, ( const char * ) &( value.getV4() ), 4, fieldId );

    if ( value.isIPv6() )
        return encodeRaw ( buffer, ( const char * ) &( value.getV6() ), 16, fieldId );

    return encodeRaw ( buffer, 0, 0, fieldId );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, const Timestamp & value, uint32_t fieldId )
{
    return tEncodeInt<uint64_t> ( buffer, value.getBinValue(), fieldId, EncNormal );
}

ProtoError ProtocolCodec::encode ( Buffer & buffer, const Buffer & fromBuffer, uint32_t fieldId )
{
    return encodeRaw ( buffer, fromBuffer.get(), fromBuffer.size(), fieldId );
}
