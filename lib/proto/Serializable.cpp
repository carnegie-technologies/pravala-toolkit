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

#include "basic/Buffer.hpp"
#include "ExtProtoError.hpp"
#include "Serializable.hpp"
#include "ProtocolCodec.hpp"

#include <cassert>

using namespace Pravala;

// These should *NOT* be changed!
const size_t Serializable::MaxLengthHeaderSize = 5;
const uint32_t Serializable::LengthVarFieldId = 0;

Serializable::~Serializable()
{
}

ProtoError Serializable::serialize ( Buffer & buf, ExtProtoError * extError )
{
    setupDefines();

    ProtoError ret = validate ( extError );

    if ( NOT_OK ( ret ) )
    {
        return ret;
    }

    return serializeFields ( buf, extError );
}

ProtoError Serializable::serialize ( Json & json, ExtProtoError * extError )
{
    setupDefines();

    ProtoError ret = validate ( extError );

    if ( NOT_OK ( ret ) )
    {
        return ret;
    }

    return serializeFields ( json, extError );
}

ProtoError Serializable::serializeFields ( Json &, ExtProtoError * extError )
{
    if ( extError != 0 )
    {
        extError->add ( ProtoError::Unsupported, "This object does not support JSON serialization" );
    }

    return ProtoError::Unsupported;
}

ProtoError Serializable::deserialize (
        const MemHandle & buf, size_t offset, size_t dataSize, ExtProtoError * extError )
{
    clear();

    bool wasWarning = false;

    ProtoError eCode;

    const size_t bufSize = offset + dataSize;

    if ( bufSize > buf.size() )
        return ProtoError::IncompleteData;

    while ( offset < bufSize )
    {
        uint8_t wireType = 0;
        uint32_t fieldId = 0;
        size_t fieldSize = 0;

        eCode = ProtocolCodec::readFieldHeader ( buf.get(), bufSize, offset, wireType, fieldId, fieldSize );

        if ( NOT_OK ( eCode ) )
        {
            if ( extError != 0 )
            {
                extError->add ( eCode, "Error reading field ID" );
            }

            return eCode;
        }

        eCode = deserializeField ( fieldId, wireType, buf, offset, fieldSize, extError );

        if ( eCode == ProtoError::ProtocolWarning )
        {
            wasWarning = true;
        }
        else if ( eCode != ProtoError::Success )
        {
            if ( extError != 0 )
            {
                extError->add ( eCode, String ( "Error deserializing field with ID %1" ).arg ( fieldId ) );
            }

            return eCode;
        }

        offset += fieldSize;

        if ( offset > bufSize )
        {
            if ( extError != 0 )
            {
                extError->add ( ProtoError::InternalError,
                                String ( "Error deserializing data - offset is greater "
                                         "than buffer size; last field ID: %1" ).arg ( fieldId ) );
            }

            return ProtoError::InternalError;
        }
    }

    eCode = validate ( extError );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    return wasWarning ? ( ProtoError::ProtocolWarning ) : ( ProtoError::Success );
}

ProtoError Serializable::serializeWithLength ( Buffer & buf, ExtProtoError * extError )
{
    ProtoError ret = ProtocolCodec::encodeFieldHeader ( buf, LengthVarFieldId,
                                                        ProtocolCodec::getWireTypeForSize ( sizeof ( LengthVarType ) ),
                                                        sizeof ( LengthVarType ) );

    if ( NOT_OK ( ret ) )
    {
        return ret;
    }

    size_t lenOffset = buf.size();
    char * mem = buf.getAppendable ( sizeof ( LengthVarType ) );

    if ( !mem )
    {
        return ProtoError::MemoryError;
    }

    buf.markAppended ( sizeof ( LengthVarType ) );

    size_t payloadOffset = buf.size();

    ret = serialize ( buf, extError );

    if ( NOT_OK ( ret ) )
    {
        return ret;
    }

    LengthVarType payloadSize = ( LengthVarType ) ( buf.size() - payloadOffset );

    if ( buf.size() < payloadOffset || payloadSize < 0 )
    {
        return ProtoError::TooMuchData;
    }

    // This is somewhat ugly, since we're modifying the content of the append-only buffer.
    // But we just appended this data, and nothing else should be using it...
    // Alternatively we could pass an RwBuffer to this function, but we are really only appending
    // to it - but we need to append the length before we actually know what it is!
    // We could keep the pointer to the original memory, but it may be no longer valid - inside serialize()
    // the buffer's memory may have been reallocated several times already!
    mem = const_cast<char *> ( buf.get ( lenOffset ) );

    if ( !mem )
    {
        return ProtoError::InvalidParameter;
    }

    memset ( mem, 0, sizeof ( LengthVarType ) );

    lenOffset = 0;

    while ( payloadSize > 0 )
    {
        assert ( lenOffset < sizeof ( LengthVarType ) );

        if ( lenOffset >= sizeof ( LengthVarType ) )
        {
            return ProtoError::InternalError;
        }

        mem[ lenOffset++ ] = ( payloadSize & 0xFF );
        payloadSize >>= 8;
    }

    return ret;
}

MemHandle Serializable::serializeWithLength ( ProtoError * errCode, ExtProtoError * extError )
{
    RwBuffer buf;

    buf.getAppendable ( MaxLengthHeaderSize );
    buf.markAppended ( MaxLengthHeaderSize );

    if ( buf.size() < MaxLengthHeaderSize )
    {
        if ( errCode != 0 )
            *errCode = ProtoError::MemoryError;

        // An empty one!
        return MemHandle();
    }

    assert ( MaxLengthHeaderSize == buf.size() );

    ProtoError ret = serialize ( buf, extError );

    if ( NOT_OK ( ret ) )
    {
        if ( errCode != 0 )
            *errCode = ret;

        // An empty one!
        return MemHandle();
    }

    assert ( buf.size() >= MaxLengthHeaderSize );

    LengthVarType payloadSize = ( LengthVarType ) ( buf.size() - MaxLengthHeaderSize );

    if ( buf.size() < MaxLengthHeaderSize || payloadSize < 0 )
    {
        if ( errCode != 0 )
            *errCode = ProtoError::TooMuchData;

        // An empty one!
        return Buffer();
    }

    size_t hdrOffset = 0;
    uint8_t wireType = ProtocolCodec::WireType4Bytes;

    if ( ( payloadSize & 0xFF ) == payloadSize )
    {
        // We need just one byte (instead of 4)
        // So we can skip first 3 bytes
        hdrOffset = 3;
        wireType = ProtocolCodec::WireType1Byte;
    }
    else if ( ( payloadSize & 0xFFFF ) == payloadSize )
    {
        // We need just two bytes (instead of 4)
        // So we can skip first 2 bytes
        hdrOffset = 2;
        wireType = ProtocolCodec::WireType2Bytes;
    }

    // For wireType to fit in 3 bits:
    assert ( ( wireType & 0x07 ) == wireType );

    // For LengthVarFieldId to fit in 4 bits
    // (we have 5 bits left, but the fifth one is used
    // as an overflow bit):
    assert ( ( LengthVarFieldId & 0x0F ) == LengthVarFieldId );

    char * const mem = buf.getWritable();

    assert ( mem != 0 );

    if ( !mem )
    {
        if ( errCode != 0 )
            *errCode = ProtoError::MemoryError;

        // An empty one!
        return Buffer();
    }

    size_t off = hdrOffset;

    assert ( off < MaxLengthHeaderSize );

    mem[ off++ ] = ( wireType & 0x07 ) | ( ( LengthVarFieldId & 0x0F ) << 3 );

    assert ( off < MaxLengthHeaderSize );

    while ( off < MaxLengthHeaderSize )
    {
        mem[ off++ ] = ( uint8_t ) ( payloadSize & 0xFF );
        payloadSize >>= 8;
    }

    assert ( off == MaxLengthHeaderSize );

    if ( errCode != 0 )
        *errCode = ProtoError::Success;

    // We need to skip some bytes from the beginning of the buffer.
    // If hdrOffset > 0, then this means that some of the first bytes are not actually used by the header.
    return buf.getHandle ( hdrOffset );
}

ProtoError Serializable::deserializeWithLength ( Buffer & buf, size_t * missingBytes, ExtProtoError * extError )
{
    MemHandle data ( buf );

    buf.clear();

    size_t offset = 0;

    const ProtoError ret = deserializeWithLength ( data, offset, missingBytes, extError );

    assert ( buf.isEmpty() );

    if ( NOT_OK ( ret ) )
    {
        // There was an error - let's put all the data back in the buffer.
        buf.append ( data );
    }
    else if ( offset < data.size() )
    {
        // It worked, but there is some data that hasn't been consumed.
        // Let's put it back in the buffer.

        buf.append ( data.getHandle ( offset ) );
    }

    return ret;
}

ProtoError Serializable::deserializeWithLength (
        const MemHandle & buf, size_t & offset,
        size_t * missingBytes, ExtProtoError * extError )
{
    // Min message size = 2
    if ( offset + 2 > buf.size() )
    {
        // We need more data!

        if ( missingBytes != 0 )
        {
            *missingBytes = offset + 2 - buf.size();
        }

        return ProtoError::IncompleteData;
    }

    size_t intOffset = offset;
    uint8_t wireType = 0;
    uint32_t fieldId = 0;
    size_t fieldSize = 0;

    ProtoError eCode = ProtocolCodec::readFieldHeader (
        buf.get(), buf.size(), intOffset, wireType, fieldId, fieldSize );

    if ( NOT_OK ( eCode ) )
    {
        // Even though we thought 2 bytes should be enough it apparently isn't
        // We don't know how much we're missing, so let's say there is one byte missing!
        if ( eCode == ProtoError::IncompleteData && missingBytes != 0 )
        {
            *missingBytes = 1;
        }

        return eCode;
    }

    assert ( intOffset > 0 );

    // This should be ensured by readFieldHeader:
    assert ( intOffset + fieldSize <= buf.size() );

    // This is not the code we expected!
    if ( fieldId != LengthVarFieldId )
    {
        return ProtoError::ProtocolError;
    }

    LengthVarType intPayloadSize = 0;

    // Let's try to decode the length field:
    eCode = ProtocolCodec::decode ( buf.get ( intOffset ), fieldSize, wireType, intPayloadSize );

    if ( NOT_OK ( eCode ) )
    {
        // We still need more data and we still don't know how much!
        if ( eCode == ProtoError::IncompleteData && missingBytes != 0 )
        {
            *missingBytes = 1;
        }

        return eCode;
    }

    intOffset += fieldSize;

    if ( intPayloadSize < 0 )
    {
        return ProtoError::ProtocolError;
    }

    const size_t payloadSize = ( size_t ) intPayloadSize;

    // We don't have the entire message yet!

    if ( intOffset + payloadSize > buf.size() )
    {
        if ( missingBytes != 0 )
        {
            *missingBytes = intOffset + payloadSize - buf.size();
        }

        return ProtoError::IncompleteData;
    }

    const ProtoError ret = deserialize ( buf, intOffset, payloadSize, extError );

    if ( IS_OK ( ret ) )
    {
        offset = intOffset + payloadSize;
    }

    return ret;
}
