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

namespace Pravala.Protocol
{
    using System.Diagnostics;
    using System.Text;

    /// <summary>
    /// A class that contains code for encoding and decoding different values using Pravala's protocol.
    /// </summary>
    public static class Codec
    {
        /// <summary>
        /// Wire (encoding) types.
        /// </summary>
        /// 000 - zero - the field has no length, the value should be zero or empty.
        /// 001 - 1 byte of data
        /// 010 - 2 bytes of data
        /// 011 - 4 bytes of data
        /// 100 - 8 bytes of data
        /// 101 - length
        /// 110 - variable length A
        /// 111 - variable length B
        public enum WireTypes : byte
        {
            /// <summary>
            /// Invalid type.
            /// </summary>
            TypeInvalid = 255,

            /// <summary>
            /// The field is empty (no data to follow) and the field value should be 0 (or "empty")
            /// </summary>
            TypeZero = 0,

            /// <summary>
            /// The data consist of only a single byte, there is no 'field length' encoded
            /// </summary>
            Type1Byte = 1,

            /// <summary>
            /// The data consist of two bytes, there is no 'field length' encoded
            /// </summary>
            Type2Bytes = 2,

            /// <summary>
            /// The data consist of four bytes, there is no 'field length' encoded
            /// </summary>
            Type4Bytes = 3,

            /// <summary>
            /// The data consist of eight bytes, there is no 'field length' encoded
            /// </summary>
            Type8Bytes = 4,

            /// <summary>
            /// The data has some other length, which is encoded using variable length encoding.
            /// </summary>
            /// The whole field consist of the field type (encoded using varint
            /// algorithm; includes the 'wire type'), followed by the data length entry
            /// (encoded using varint), followed by the actual data of the given size.
            TypeLengthDelim = 5,

            /// <summary>
            /// Variable length encoding "A".
            /// </summary>
            /// Following (after the header) bytes should be considered part of this field
            /// up to the first byte with MSB set to 0. So the field consists of all the bytes with MSB set to 1
            /// and a single byte with MSB set to 0.
            TypeVariableLengthA = 6,

            /// <summary>
            /// Variable length encoding "B".
            /// </summary>
            /// Bytes should be read the same way as in the "A" version, but the data should be interpreted differently.
            /// Right now it is only used by numbers, and means that the value was negative,
            /// and the result should be multiplied by -1.
            TypeVariableLengthB = 7
        }

        /// <summary>
        /// Reads the header of the field.
        /// </summary>
        /// <param name="buffer">The read buffer to read the data from</param>
        /// <param name="offset">Initial offset in the buffer. It is modified as the data is consumed.
        /// After calling this function it will point to the first byte of payload data.
        /// (Unless there is an error, in which case it MAY point to a byte beyond the length)</param>
        /// <param name="bufSize">
        /// The size of the buffer than can be read (starting from 0, not the offset passed)
        /// </param>
        /// <param name="fieldId">Field ID read from the header</param>
        /// <param name="payloadSize">Field's payload size read from the header. It is NOT checked for validity
        /// (it could describe data past the end of the buffer)</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <returns>
        /// <c>true</c>, if there is enough data to decode field's ID and payload size,
        /// <c>false</c> if there is not enough data.
        /// </returns>
        /// <exception cref="ProtoException">Thrown when arguments are invalid or the data is incorrect</exception>
        public static bool ReadHeader (
                byte[] buffer,
                ref uint offset,
                uint bufSize,
                out uint fieldId,
                out uint payloadSize,
                out WireTypes wireType )
        {
            if ( buffer == null || offset >= bufSize || bufSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidParameter,
                        "No buffer provided, or incorrect buffer size" );
            }

            payloadSize = 0;

            byte byteVal = buffer[ offset++ ];

            // We read wire type - first 3 bits
            wireType = ( WireTypes ) ( byteVal & 0x07U );

            // And beginning of the field ID - the next 4 bits
            fieldId = ( uint ) ( ( byteVal >> 3 ) & 0x0FU );

            byte shift = 4;

            // While overflow bit is set...
            while ( ( byteVal & 0x80U ) != 0 )
            {
                // Not enough data in the buffer
                if ( offset >= bufSize )
                {
                    return false;
                }

                // shifts:
                // 4, 11, 18, 25, 32
                if ( shift > 25 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.ProtocolError,
                            "Too many bytes have the MSB set while decoding the field ID" );
                }

                byteVal = buffer[ offset++ ];

                // We use current byte, take 7 bits from it (the eight one is
                // used as overflow bit), and shift it to add it to already read bits
                fieldId |= ( uint ) ( ( byteVal & 0x7FU ) << shift );

                // We just read 7 bits, next read will be in front of them
                shift += 7;
            }

            // Now the offset should point to the first byte AFTER the header field
            switch ( wireType )
            {
                case WireTypes.TypeZero:
                    payloadSize = 0;
                    break;

                case WireTypes.Type1Byte:
                    payloadSize = 1;
                    break;

                case WireTypes.Type2Bytes:
                    payloadSize = 2;
                    break;

                case WireTypes.Type4Bytes:
                    payloadSize = 4;
                    break;

                case WireTypes.Type8Bytes:
                    payloadSize = 8;
                    break;

                case WireTypes.TypeLengthDelim:
                    payloadSize = 0;
                    shift = 0;

                    do
                    {
                        // Not enough data in the buffer
                        if ( offset >= bufSize )
                        {
                            return false;
                        }

                        // shifts:
                        // 0, 7, 14, 21, 28, 37
                        if ( shift > 28 )
                        {
                            throw new ProtoException (
                                    ProtoException.ErrorCodes.ProtocolError,
                                    "Too many bytes have the MSB set while decoding the field's variable length" );
                        }

                        byteVal = buffer[ offset++ ];

                        // The last byte can have only up to 4 bits set!
                        if ( shift == 28 && ( byteVal & 0x0FU ) != byteVal )
                        {
                            throw new ProtoException (
                                    ProtoException.ErrorCodes.ProtocolError,
                                    "Too large value while decoding the field's variable length" );
                        }

                        // We use current byte, take 7 bits from it (the eight one is
                        // used as overflow bit), and shift it to add it to already read bits
                        payloadSize |= ( uint ) ( ( ( uint ) ( byteVal & 0x7FU ) ) << shift );

                        // We just read 7 bits, next read will be in front of them
                        shift += 7;
                    }
                    while ( ( byteVal & 0x80U ) != 0 );

                    break;

                case WireTypes.TypeVariableLengthA:
                case WireTypes.TypeVariableLengthB:
                    payloadSize = 0;

                    // We can't modify the original offset, since we are not consuming the header anymore.
                    // This is the actual payload.

                    uint tmpOffset = offset;

                    do
                    {
                        // Not enough data in the buffer
                        if ( offset >= bufSize )
                        {
                            return false;
                        }

                        ++payloadSize;
                    }
                    while ( ( buffer[ tmpOffset++ ] & 0x80U ) != 0 );

                    break;

                default:
                    throw new ProtoException (
                        ProtoException.ErrorCodes.ProtocolError,
                        "Unknown wire type received: " + wireType );
            }

            return true;
        }

        /// <summary>
        /// Decodes a byte array from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out byte[] value )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.IncompleteData,
                        "Invalid buffer or not enough bytes in the buffer to read " + payloadSize + " bytes" );
            }

            if ( wireType == WireTypes.TypeVariableLengthA || wireType == WireTypes.TypeVariableLengthA )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.ProtocolError,
                        "Byte array cannot be encoded using variable length encoding (" + wireType + ")" );
            }

            value = new byte[ payloadSize ];

            for ( uint i = 0; i < payloadSize; ++i )
            {
                Debug.Assert ( offset < buffer.Length, "To make sure we are not reading past the buffer size. " );

                value[ i ] = buffer[ offset++ ];
            }
        }

        /// <summary>
        /// Decodes a single byte value from the buffer.
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out byte value )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.IncompleteData,
                        "Invalid buffer or not enough bytes in the buffer to read " + payloadSize + " bytes" );
            }

            if ( wireType == WireTypes.TypeVariableLengthA
                 || wireType == WireTypes.TypeVariableLengthB )
            {
                if ( payloadSize > 2 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize
                            + ") to store in a (s)byte value (using variable length encoding)" );
                }

                value = 0;
                byte shift = 0;

                for ( uint i = 0; i < payloadSize; ++i, shift += 7 )
                {
                    value |= ( byte ) ( ( buffer[ offset++ ] & 0x7FU ) << shift );
                }

                if ( wireType == WireTypes.TypeVariableLengthB )
                {
                    value = ( byte ) ( ( ( sbyte ) value ) * -1 );
                }
            }
            else
            {
                if ( payloadSize > 1 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize + ") to store in a (s)byte value" );
                }

                value = ( payloadSize > 0 ) ? buffer[ offset ] : ( ( byte ) 0 );
            }
        }

        /// <summary>
        /// Decodes a single signed byte value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out sbyte value )
        {
            byte tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = ( sbyte ) tmpValue;
        }

        /// <summary>
        /// Decodes a single unsigned short value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out ushort value )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.IncompleteData,
                        "Invalid buffer or not enough bytes in the buffer to read " + payloadSize + " bytes" );
            }

            value = 0;
            byte shift = 0;

            if ( wireType == WireTypes.TypeVariableLengthA
                 || wireType == WireTypes.TypeVariableLengthB )
            {
                if ( payloadSize > 3 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize
                            + ") to store in a (u)short value (using variable length encoding)" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 7 )
                {
                    value |= ( ushort ) ( ( buffer[ offset++ ] & 0x7FU ) << shift );
                }

                if ( wireType == WireTypes.TypeVariableLengthB )
                {
                    value = ( ushort ) ( ( ( short ) value ) * -1 );
                }
            }
            else
            {
                if ( payloadSize > sizeof ( ushort ) )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize + ") to store in a (u)short value" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 8 )
                {
                    value |= ( ushort ) ( ( buffer[ offset++ ] & 0xFFU ) << shift );
                }
            }
        }

        /// <summary>
        /// Decodes a single short value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out short value )
        {
            ushort tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = ( short ) tmpValue;
        }

        /// <summary>
        /// Decodes a single unsigned integer value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out uint value )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.IncompleteData,
                        "Invalid buffer or not enough bytes in the buffer to read " + payloadSize + " bytes" );
            }

            value = 0;
            byte shift = 0;

            if ( wireType == WireTypes.TypeVariableLengthA
                 || wireType == WireTypes.TypeVariableLengthB )
            {
                if ( payloadSize > 5 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize
                            + ") to store in a (u)int value (using variable length encoding)" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 7 )
                {
                    value |= ( ( uint ) ( buffer[ offset++ ] & 0x7FU ) ) << shift;
                }

                if ( wireType == WireTypes.TypeVariableLengthB )
                {
                    value = ( uint ) ( ( ( int ) value ) * -1 );
                }
            }
            else
            {
                if ( payloadSize > sizeof ( uint ) )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize + ") to store in a (u)int value" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 8 )
                {
                    value |= ( ( uint ) ( buffer[ offset++ ] & 0xFFU ) ) << shift;
                }
            }
        }

        /// <summary>
        /// Decodes a single integer value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out int value )
        {
            uint tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = ( int ) tmpValue;
        }

        /// <summary>
        /// Decodes a single unsigned long value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out ulong value )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.IncompleteData,
                        "Invalid buffer or not enough bytes in the buffer to read " + payloadSize + " bytes" );
            }

            value = 0;
            byte shift = 0;

            if ( wireType == WireTypes.TypeVariableLengthA
                 || wireType == WireTypes.TypeVariableLengthB )
            {
                if ( payloadSize > 10 )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize
                            + ") to store in a (u)long value (using variable length encoding)" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 7 )
                {
                    value |= ( ( ulong ) ( buffer[ offset++ ] & 0x7FU ) ) << shift;
                }

                if ( wireType == WireTypes.TypeVariableLengthB )
                {
                    value = ( ulong ) ( ( ( long ) value ) * -1 );
                }
            }
            else
            {
                if ( payloadSize > sizeof ( ulong ) )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Too many bytes (" + payloadSize + ") to store in a (u)long value" );
                }

                for ( uint i = 0; i < payloadSize; ++i, shift += 8 )
                {
                    value |= ( ( ulong ) ( buffer[ offset++ ] & 0xFFU ) ) << shift;
                }
            }
        }

        /// <summary>
        /// Decodes a single 'long' value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out long value )
        {
            ulong tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = ( long ) tmpValue;
        }

        /// <summary>
        /// Decodes a single 'float' value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out float value )
        {
            uint tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = FloatingPointUtils.unpack754 ( tmpValue );
        }

        /// <summary>
        /// Decodes a single 'double' value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out double value )
        {
            ulong tmpValue;

            Decode ( buffer, offset, payloadSize, wireType, out tmpValue );
            value = FloatingPointUtils.unpack754 ( tmpValue );
        }

        /// <summary>
        /// Decodes a single string value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode ( byte[] buffer, uint offset, uint payloadSize, WireTypes wireType, out string value )
        {
            if ( wireType == WireTypes.TypeZero )
            {
                value = string.Empty;
                return;
            }

            byte[] data;
            Decode ( buffer, offset, payloadSize, wireType, out data );

            if ( data == null || data.Length < 1 )
            {
                value = string.Empty;
            }
            else
            {
                value = Encoding.UTF8.GetString ( data, 0, data.Length );
            }
        }

        /// <summary>
        /// Decodes a singleIP address value from the buffer
        /// </summary>
        /// <param name="buffer">Buffer to read data from.</param>
        /// <param name="offset">Initial offset in the buffer.</param>
        /// <param name="payloadSize">The size od the payload to decode.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <param name="value">Decoded value.</param>
        /// <exception cref="ProtoException">
        /// When one of the parameters is invalid, the data incorrect, or there is not enough data.
        /// </exception>
        public static void Decode (
                byte[] buffer,
                uint offset,
                uint payloadSize,
                WireTypes wireType,
                out IpAddress value )
        {
            byte[] data;
            Decode ( buffer, offset, payloadSize, wireType, out data );

            if ( data != null && ( data.Length == 4 || data.Length == 16 ) )
            {
                value = new IpAddress ( data );
            }
            else
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidDataSize,
                        "Invalid number of bytes (" + payloadSize + ") to store in an IpAddress value" );
            }
        }

        /// <summary>
        /// Encodes the header of the field (and appends it to the buffer)
        /// </summary>
        /// <param name="buffer">Buffer to append the header to. The buffer size is modified</param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="wireType">The "wire type" of the field to store in the header</param>
        /// <param name="dataSize">
        /// The size of the data that will be stored in that field.
        /// If wireType is set to WireTypes.TypeLengthDelim then this function encodes the length value in the header as
        /// well.
        /// </param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendFieldHeader ( Buffer buffer, uint fieldId, WireTypes wireType, uint dataSize )
        {
            if ( ( ( ( byte ) wireType ) & 0x07 ) != ( ( byte ) wireType ) )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidParameter,
                        "Invalid wire type: " + ( ( int ) wireType ) );
            }

            Debug.Assert ( ( ( ( byte ) wireType ) & 0x07 ) == ( ( byte ) wireType ),
                    "To make sure the wire type makes sense." );

            // Max amount of memory:
            // 1 byte for wire type
            // up to 5 bytes for field ID
            // up to 5 bytes for the length of the field
            // up to dataSize bytes for the value
            buffer.ReserveSpace ( 1 + 5 + 5 + ( int ) dataSize );

            // First 3 bits carry the wire type
            byte byteVal = ( byte ) ( ( ( byte ) wireType ) & 0x07U );

            // Next 4 bits carry the first 4 bits of the field ID:
            byteVal |= ( byte ) ( ( fieldId & 0x0FU ) << 3 );

            // Remove those 4 bits from the field ID:
            fieldId >>= 4;

            while ( fieldId > 0 )
            {
                // Field ID is longer - set the 'byteVal' value AND the overflow bit
                buffer.AppendByte ( ( byte ) ( byteVal | 0x80U ) );

                // Next byte will contain next 7 bits of the field ID:
                byteVal = ( byte ) ( fieldId & 0x7FU );

                // Remove those 7 bits of the field ID:
                fieldId >>= 7;
            }

            // Last byte - no overflow bit set!
            buffer.AppendByte ( byteVal );

            if ( wireType == WireTypes.TypeLengthDelim )
            {
                // In a single byte we can store 7 bits of the data length
                byteVal = ( byte ) ( dataSize & 0x7FU );

                // Remove those 7 bits
                uint remainingLen = dataSize >> 7;

                while ( remainingLen > 0 )
                {
                    // data length is longer - set the 'byteVal' value AND the overflow bit
                    buffer.AppendByte ( ( byte ) ( byteVal | 0x80U ) );

                    // Next byte will contain next 7 bits of the data length:
                    byteVal = ( byte ) ( remainingLen & 0x7FU );

                    // Remove those 7 bits:
                    remainingLen >>= 7;
                }

                // Last byte - no overflow bit set!
                buffer.AppendByte ( byteVal );
            }
        }

        /// <summary>
        /// Appends a field carrying 'byte' value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, byte value )
        {
            AppendFieldHeader ( buffer, fieldId, WireTypes.Type1Byte, 1 );
            buffer.AppendByte ( value );
        }

        /// <summary>
        /// Appends a field carrying signed byte value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, sbyte value )
        {
            if ( value >= 0 )
            {
                AppendValue ( buffer, fieldId, ( byte ) value );
                return;
            }

            value = ( sbyte ) ( -value );

            AppendFieldHeader ( buffer, fieldId, WireTypes.TypeVariableLengthB, 1 );

            byte byteVal = ( byte ) value;

            buffer.AppendByte ( byteVal );

            if ( ( byteVal & 0x80U ) != 0 )
            {
                buffer.AppendByte ( ( byte ) 0x01 );
            }
        }

        /// <summary>
        /// Appends a field carrying unsigned short value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, ushort value )
        {
            uint dataSize = 0;
            WireTypes wireType = WireTypes.TypeInvalid;

            if ( ( value & 0xFFU ) == value )
            {
                // We can fit the value in 1 byte
                dataSize = 1;

                // The wire type to be used:
                wireType = WireTypes.Type1Byte;
            }
            else if ( ( value & 0xFFFFU ) == value )
            {
                // We can fit the value in 2 bytes
                dataSize = 2;

                // The wire type to be used:
                wireType = WireTypes.Type2Bytes;
            }
            else
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.TooBigValue,
                        "Value too big to store as a (u)short: " + value );
            }

            Debug.Assert ( dataSize == 1 || dataSize == 2, "To make sure we're working with correct data size" );
            Debug.Assert (
                    wireType == WireTypes.Type1Byte || wireType == WireTypes.Type2Bytes,
                    "To make sure that the wire type makes sense." );

            AppendFieldHeader ( buffer, fieldId, wireType, dataSize );

            uint offset = 0;
            byte shift = 0;

            while ( offset < dataSize )
            {
                buffer.AppendByte ( ( byte ) ( ( value >> shift ) & 0xFFU ) );
                shift += 8;
                ++offset;
            }
        }

        /// <summary>
        /// Appends a field carrying short value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, short value )
        {
            AppendValue ( buffer, fieldId, ( ushort ) value );
        }

        /// <summary>
        /// Appends a field carrying unsigned integer value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, uint value )
        {
            uint dataSize = 0;
            WireTypes wireType = 0;

            if ( ( value & 0xFFU ) == value )
            {
                // We can fit the value in 1 byte
                dataSize = 1;

                // The wire type to be used:
                wireType = WireTypes.Type1Byte;
            }
            else if ( ( value & 0xFFFFU ) == value )
            {
                // We can fit the value in 2 bytes
                dataSize = 2;

                // The wire type to be used:
                wireType = WireTypes.Type2Bytes;
            }
            else if ( ( value & 0xFFFFFFFFU ) == value )
            {
                // 3 bytes - no difference between 3 bytes + length vs 4 bytes
                // We can fit the value in 4 bytes
                dataSize = 4;

                // The wire type to be used:
                wireType = WireTypes.Type4Bytes;
            }
            else
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.TooBigValue,
                        "Value too big to store as a (u)short: " + value );
            }

            Debug.Assert ( dataSize == 1 || dataSize == 2 || dataSize == 4, "To make sure data size makes sense." );
            Debug.Assert (
                    wireType == WireTypes.Type1Byte
                    || wireType == WireTypes.Type2Bytes
                    || wireType == WireTypes.Type4Bytes,
                    "To make sure wire type makes sense." );

            AppendFieldHeader ( buffer, fieldId, wireType, dataSize );

            uint offset = 0;
            byte shift = 0;

            while ( offset < dataSize )
            {
                buffer.AppendByte ( ( byte ) ( ( value >> shift ) & 0xFFU ) );
                shift += 8;
                ++offset;
            }
        }

        /// <summary>
        /// Appends a field carrying integer value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, int value )
        {
            AppendValue ( buffer, fieldId, ( uint ) value );
        }

        /// <summary>
        /// Appends a field carrying unsigned long value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, ulong value )
        {
            uint dataSize = 0;
            WireTypes wireType = 0;

            if ( ( value & 0xFFU ) == value )
            {
                // We can fit the value in 1 byte
                dataSize = 1;

                // The wire type to be used:
                wireType = WireTypes.Type1Byte;
            }
            else if ( ( value & 0xFFFFU ) == value )
            {
                // We can fit the value in 2 bytes
                dataSize = 2;

                // The wire type to be used:
                wireType = WireTypes.Type2Bytes;
            }
            else if ( ( value & 0xFFFFFFFFU ) == value )
            {
                // 3 bytes - no difference between 3 bytes + length vs 4 bytes
                // We can fit the value in 4 bytes
                dataSize = 4;

                // The wire type to be used:
                wireType = WireTypes.Type4Bytes;
            }
            else if ( ( value & 0xFFFFFFFFFFUL ) == value )
            {
                // We can fit the value in 5 bytes (+1 byte for length)
                dataSize = 5;

                // The wire type to be used:
                wireType = WireTypes.TypeLengthDelim;
            }
            else if ( ( value & 0xFFFFFFFFFFFFUL ) == value )
            {
                // We can fit the value in 6 bytes (+1 byte for length)
                dataSize = 6;

                // The wire type to be used:
                wireType = WireTypes.TypeLengthDelim;
            }
            else if ( ( value & 0xFFFFFFFFFFFFFFFFUL ) == value )
            {
                // 7 bytes - no difference between 7 bytes + length vs 8 bytes
                // We can fit the value in 8 bytes
                dataSize = 8;

                // The wire type to be used:
                wireType = WireTypes.Type8Bytes;
            }
            else
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.TooBigValue,
                        "Value too big to store as a (u)short: " + value );
            }

            Debug.Assert ( dataSize >= 1 && dataSize <= 8, "To make sure data size makes sense." );
            Debug.Assert (
                    wireType == WireTypes.Type1Byte
                    || wireType == WireTypes.Type2Bytes
                    || wireType == WireTypes.Type4Bytes
                    || wireType == WireTypes.Type8Bytes
                    || wireType == WireTypes.TypeLengthDelim,
                    "To make sure wire type makes sense." );

            AppendFieldHeader ( buffer, fieldId, wireType, dataSize );

            uint offset = 0;
            byte shift = 0;

            while ( offset < dataSize )
            {
                buffer.AppendByte ( ( byte ) ( ( value >> shift ) & 0xFFU ) );
                shift += 8;
                ++offset;
            }
        }

        /// <summary>
        /// Appends a field carrying long value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, long value )
        {
            AppendValue ( buffer, fieldId, ( ulong ) value );
        }

        /// <summary>
        /// Appends a field carrying float value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, float value )
        {
            AppendValue ( buffer, fieldId, FloatingPointUtils.pack754 ( value ) );
        }

        /// <summary>
        /// Appends a field carrying double value to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, double value )
        {
            AppendValue ( buffer, fieldId, FloatingPointUtils.pack754 ( value ) );
        }

        /// <summary>
        /// Appends a field carrying a byte array to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="data">The data to serialize</param>
        /// <param name="dataSize">The size of data to serialize</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, byte[] data, int dataSize )
        {
            if ( dataSize > 0 )
            {
                if ( data == null )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidParameter,
                            "Empty byte array passed to AppendValue; Expected size: " + dataSize );
                }

                if ( dataSize > data.Length )
                {
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidParameter,
                            "Too small byte array sent to AppendValue; Expected size: " + dataSize + "; Received: "
                            + data.Length );
                }
            }

            WireTypes wireType;

            switch ( dataSize )
            {
                case 0:
                    wireType = WireTypes.TypeZero;
                    break;

                case 1:
                    wireType = WireTypes.Type1Byte;
                    break;

                case 2:
                    wireType = WireTypes.Type2Bytes;
                    break;

                case 4:
                    wireType = WireTypes.Type4Bytes;
                    break;

                case 8:
                    wireType = WireTypes.Type8Bytes;
                    break;

                default:
                    wireType = WireTypes.TypeLengthDelim;
                    break;
            }

            AppendFieldHeader ( buffer, fieldId, wireType, ( uint ) dataSize );

            if ( dataSize > 0 )
            {
                buffer.AppendBytes ( data, dataSize );
            }
        }

        /// <summary>
        /// Appends a field carrying a byte array to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="data">The data to serialize</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, byte[] data )
        {
            AppendValue ( buffer, fieldId, data, ( data != null ) ? data.Length : 0 );
        }

        /// <summary>
        /// Appends a field carrying a byte array to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="data">The data to serialize</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, Buffer data )
        {
            AppendValue ( buffer, fieldId, ( data != null ) ? data.Bytes : null, ( data != null ) ? data.Size : 0 );
        }

        /// <summary>
        /// Appends a field carrying a string to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, string value )
        {
            AppendValue ( buffer, fieldId, Encoding.UTF8.GetBytes ( value ) );
        }

        /// <summary>
        /// Appends a field carrying an IP address to the buffer
        /// </summary>
        /// This function appends the header followed by the actual field value.
        /// <param name="buffer">
        /// Buffer to append the data field to. The buffer size is modified as data is appended.
        /// </param>
        /// <param name="fieldId">The ID of the field to store in the header</param>
        /// <param name="value">The value to be encoded</param>
        /// <exception cref="ProtoException">When one of the parameters is invalid</exception>
        public static void AppendValue ( Buffer buffer, uint fieldId, IpAddress value )
        {
            if ( value == null || ( !value.IsIpv4 && !value.IsIpv6 ) )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidParameter,
                        "Invalid value to store as an IpAddress: " + value );
            }

            AppendValue ( buffer, fieldId, value.Bytes );
        }
    }
}
