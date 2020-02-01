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

#include <cassert>
#include <cstddef>

extern "C"
{
#include <stdint.h>
}

#include "basic/MsvcSupport.hpp"
#include "basic/IpAddress.hpp"
#include "basic/Buffer.hpp"
#include "basic/Timestamp.hpp"

#include "ProtoError.hpp"

namespace Pravala
{
class ProtocolCodec
{
    public:
        // "Wire types" - the type stored in the first 3 bits of the 'field ID' byte,
        // they describe the encoding used for each field.

        // Encoding types:
        // 000 - zero - the field has no length, the value should be zero or empty.
        // 001 - 1 byte of data
        // 010 - 2 bytes of data
        // 011 - 4 bytes of data
        // 100 - 8 bytes of data
        // 101 - length
        // 110 - variable length A
        // 111 - variable length B

        /// The field is empty (no data to follow) and the field value should be 0 (or "empty")
        static const uint8_t WireTypeZero = 0;

        /// The data consist of only a single byte, there is no 'field length' encoded
        static const uint8_t WireType1Byte = 1;

        /// The data consist of two bytes, there is no 'field length' encoded
        static const uint8_t WireType2Bytes = 2;

        /// The data consist of four bytes, there is no 'field length' encoded
        static const uint8_t WireType4Bytes = 3;

        /// The data consist of eight bytes, there is no 'field length' encoded
        static const uint8_t WireType8Bytes = 4;

        /// The data has some other length, which is encoded using variable length encoding.
        /// The whole field consist of the field type (encoded using variable length int encoding algorithm;
        /// includes the 'wire type'), followed by the data length entry (encoded using variable length int encoding),
        /// followed by the actual data of the given size.
        static const uint8_t WireTypeLengthDelim = 5;

        /// Variable length encoding "A". Following (after the header) bytes should be considered part of this field
        /// up to the first byte with MSB set to 0. So the field consists of all the bytes with MSB set to 1
        /// and a single byte with MSB set to 0.
        static const uint8_t WireTypeVariableLengthA = 6;

        /// Variable length encoding "B". Bytes should be read the same way as in the "A" version,
        /// but the data should be interpreted differently. Right now it is only used by numbers,
        /// and means that the value was negative, and the result should be multiplied by -1.
        static const uint8_t WireTypeVariableLengthB = 7;

        /// @brief Extracts basic elements from field's header
        ///
        /// It reads data from the buffer, starting at given offset, extracts the wire type
        /// and data length (either based on the wire type, or on the length field in case
        /// wire type says it's length-delimited field). If wire type is varint, dataSize is set to 0.
        /// This function modifies offset to point at the first byte of data payload (so after
        /// field id, wire type and field length if it's present.
        /// After this is called offset can point beyond buffer's size!!! (but in that case
        /// an error is returned).
        ///
        /// @param [in] buffer Buffer to read data from,
        /// @param [in] bufSize Size of the buffer (from offset 0, not from the passed offset)
        /// @param [in,out] offset Initial offset in the buffer. It is modified as the data is consumed.
        ///                        After calling this function it will point to the first byte of payload data.
        ///                        (Unless there is an error, in which case it MAY point to a byte beyond buffer size)
        /// @param [out] wireType Extracted wire type.
        /// @param [out] fieldId The field ID read from the buffer.
        /// @param [out] fieldSize Extracted field size (or assumed using wire type).
        /// @return Standard error code. It also compares data length and the available buffer size
        ///         and returns error if there is no enough data in the buffer given fieldSize decoded,
        ///         even without actually moving the offset
        static ProtoError readFieldHeader (
            const char * buffer, size_t bufSize, size_t & offset,
            uint8_t & wireType, uint32_t & fieldId, size_t & fieldSize );

        /// @brief Decodes a single 'bool' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, bool & value );

        /// @brief Decodes a single 'int8_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, int8_t & value );

        /// @brief Reads a single 'uint8_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint8_t & value );

        /// @brief Reads a single 'int16_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, int16_t & value );

        /// @brief Reads a single 'uint16_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint16_t & value );

        /// @brief Reads a single 'int32_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, int32_t & value );

        /// @brief Reads a single 'uint32_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint32_t & value );

        /// @brief Reads a single 'int64_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, int64_t & value );

        /// @brief Reads a single 'uint64_t' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, uint64_t & value );

        /// @brief Reads a single 'float' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, float & value );

        /// @brief Reads a single 'double' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, double & value );

        /// @brief Reads a single 'String' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, String & value );

        /// @brief Reads all field's data to the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, Buffer & toBuffer );

        /// @brief Reads a single 'IpAddress' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, IpAddress & value );

        /// @brief Reads a single 'Timestamp' value from the buffer.
        /// @param [in] buffer Buffer to read the data from
        /// @param [in] dataSize Size of data to read.
        /// @param [in] wireType Wire encoding type.
        /// @param [out] value Value to read the data to.
        /// @return Standard error code.
        static ProtoError decode ( const char * buffer, size_t dataSize, uint8_t wireType, Timestamp & value );

        /// @brief Encodes the header of the field (and appends it to the buffer)
        ///
        /// @param [in,out] buffer Buffer to append the header to. The buffer size is modified using markAppended()
        /// @param [in] fieldId The ID of the field to store in the header
        /// @param [in] wireType The "wire type" of the field to store in the header
        /// @param [in] dataSize The size of the data that will be stored in that field.
        ///                      If wireType is set to WireTypeLengthDelim then this function encodes the
        ///                      length value in the header as well, otherwise this parameter is only used
        ///                      for buffer preallocation, so it is a good idea to pass the correct value!
        /// @return Standard error code
        static ProtoError encodeFieldHeader (
            Buffer & buffer, uint32_t fieldId,
            uint8_t wireType, size_t dataSize );

        /// @brief Appends a field carrying 'bool' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, bool value, uint32_t fieldId );

        /// @brief Appends a field carrying 'int8_t' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, int8_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'uint8_t' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, uint8_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'int16_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, int16_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'uint16_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, uint16_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'int32_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, int32_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'uint32_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, uint32_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'int64_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, int64_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'uint64_t' value to the buffer
        ///
        /// It takes care of endianness, the value should be passed in 'local' byte order (without conversion)
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, uint64_t value, uint32_t fieldId );

        /// @brief Appends a field carrying 'float' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, float value, uint32_t fieldId );

        /// @brief Appends a field carrying 'double' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, double value, uint32_t fieldId );

        /// @brief Appends a field carrying 'String' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, const String & value, uint32_t fieldId );

        /// @brief Appends a field carrying data to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] fromBuffer The buffer which content should be encoded.
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, const Buffer & fromBuffer, uint32_t fieldId );

        /// @brief Appends a field carrying 'IpAddress' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, const IpAddress & value, uint32_t fieldId );

        /// @brief Appends a field carrying 'Timestamp' value to the buffer
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] value The value to be encoded
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encode ( Buffer & buffer, const Timestamp & value, uint32_t fieldId );

        /// @brief Appends a field carrying data to the buffer
        ///
        /// This function is used by all the other 'encode' functions. It does not perform any
        /// endianness conversions.
        ///
        /// @param [in,out] buffer Buffer to append the data field to. The buffer size is modified using markAppended()
        /// @param [in] data Pointer to the data to be encoded
        /// @param [in] dataSize The size of the data to encode
        /// @param [in] fieldId The ID of the field to store in the header
        /// @return Standard error code
        static ProtoError encodeRaw ( Buffer & buffer, const char * data, size_t dataSize, uint32_t fieldId );

        /// @brief Returns the appropriate wire type for the given data size
        /// @param [in] dataSize The size of data
        /// @return The wire type for that data size
        static uint8_t getWireTypeForSize ( size_t dataSize );
};
}
