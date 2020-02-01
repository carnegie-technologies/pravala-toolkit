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

package com.pravala.protocol;

import com.pravala.protocol.auto.ErrorCode;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * A class that contains several methods for data serialization and deserialization
 *
 */
public final class Codec
{
    // Encoding types:
    // 000 - zero - the field has no length, the value should be zero or empty.
    // 001 - 1 byte of data
    // 010 - 2 bytes of data
    // 011 - 4 bytes of data
    // 100 - 8 bytes of data
    // 101 - length
    // 110 - variable length A
    // 111 - variable length B

    /**
     * The field is empty (no data to follow) and the field value should be 0 (or "empty")
     */
    public static final byte WIRE_TYPE_ZERO = 0;

    /**
     * The data consist of only a single byte, there is no 'field length'
     * encoded
     */
    public static final byte WIRE_TYPE_1_BYTE = 1;

    /**
     * The data consist of two bytes, there is no 'field length' encoded
     */
    public static final byte WIRE_TYPE_2_BYTES = 2;

    /**
     * The data consist of four bytes, there is no 'field length' encoded
     */
    public static final byte WIRE_TYPE_4_BYTES = 3;

    /**
     * The data consist of eight bytes, there is no 'field length' encoded
     */
    public static final byte WIRE_TYPE_8_BYTES = 4;

    /**
     * The data has some other length, which is encoded using variable length
     * encoding.
     *
     * The whole field consist of the field type (encoded using varint
     * algorithm; includes the 'wire type'), followed by the data length entry
     * (encoded using varint), followed by the actual data of the given size.
     */
    public static final byte WIRE_TYPE_LENGTH_DELIM = 5;

    /**
     * Variable length encoding "A". Following (after the header) bytes should be considered
     * part of this field up to the first byte with MSB set to 0.
     * So the field consists of all the bytes with MSB set to 1 and a single byte with MSB set to 0.
     */
    public static final byte WIRE_TYPE_VAR_LENGTH_A = 6;

    /**
     * Variable length encoding "B". Bytes should be read the same way as in the "A" version,
     * but the data should be interpreted differently. Right now it is only used by numbers,
     * and means that the value was negative, and the result should be multiplied by -1.
     */
    public static final byte WIRE_TYPE_VAR_LENGTH_B = 7;

    /**
     * Private constructor to prevent instantiating of this class
     */
    private Codec()
    {
    }

    /**
     * Describes a single field.
     */
    public static class FieldHeader
    {
        /**
         * The ID of the field.
         */
        public final int fieldId;

        /**
         * The size of the field (in bytes).
         */
        public final int fieldSize;

        /**
         * The wire type used by the field.
         */
        public final byte wireType;

        /**
         * Constructor
         * @param wireType Wire type of the field.
         * @param fieldSize Size of the field (in bytes).
         */
        public FieldHeader ( int fieldId, int fieldSize, byte wireType )
        {
            this.fieldId = fieldId;
            this.fieldSize = fieldSize;
            this.wireType = wireType;
        }

        /**
         * Checks if this field uses variable length encoding.
         * @return True if the field uses variable length encoding; False otherwise.
         */
        public boolean isVarLen()
        {
            return ( wireType == WIRE_TYPE_VAR_LENGTH_A || wireType == WIRE_TYPE_VAR_LENGTH_B );
        }
    }

    /**
     * Extracts the field size from the buffer
     *
     *        It reads data from the buffer, starting at given offset, extracts
     *        the wire type and data length (either based on the wire type, or
     *        on the length field in case wire type says it's length-delimited
     *        field). This function modifies offset to point at the first byte of data
     *        payload (so after field id, wire type and field length if it's
     *        present. After this is called offset can point beyond buffer's
     *        size!!! (but in that case an error is thrown).
     *
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of payload (or past the field when
     *        there is no payload)
     * @return The size of the field's payload
     * @throws CodecException
     */
    public static FieldHeader readFieldHeader ( ReadBuffer readBuffer )
    throws CodecException
    {
        if ( readBuffer == null || readBuffer.getBuffer() == null )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        // Not enough data in the buffer
        if ( readBuffer.getSize() <= readBuffer.getOffset() )
        {
            throw new CodecException ( ErrorCode.IncompleteData );
        }

        byte byteVal = readBuffer.readByte();

        // We read wire type - first 3 bits
        final byte wireType = ( byte ) ( byteVal & 0x7 );
        int fieldSize = 0;
        int fieldId = ( byteVal >> 3 ) & 0x0F;

        int shift = 4;

        // Overflow bit is set
        while ( ( byteVal & 0x80 ) != 0 )
        {
            // Not enough data in the buffer
            if ( readBuffer.getSize() <= readBuffer.getOffset() )
            {
                throw new CodecException ( ErrorCode.IncompleteData );
            }

            // shifts:
            // 4, 11, 18, 25, 32
            if ( shift > 25 )
            {
                throw new CodecException ( ErrorCode.ProtocolError );
            }

            byteVal = readBuffer.readByte();

            // We use current byte, take 7 bits from it (the eight one is
            // used as overflow bit), and shift it to add it to already read bits
            fieldId |= ( byteVal & 0x7F ) << shift;

            shift += 7;
        }

        // Here byteVal should be at the LAST byte of field "header".
        assert ( ( byteVal & 0x80 ) == 0 );

        // Offset points at the first byte of the payload (or the length field)

        switch ( wireType )
        {
            case WIRE_TYPE_ZERO:
                fieldSize = 0;
                break;

            case WIRE_TYPE_1_BYTE:
                fieldSize = 1;
                break;

            case WIRE_TYPE_2_BYTES:
                fieldSize = 2;
                break;

            case WIRE_TYPE_4_BYTES:
                fieldSize = 4;
                break;

            case WIRE_TYPE_8_BYTES:
                fieldSize = 8;
                break;

            case WIRE_TYPE_LENGTH_DELIM:
                {
                    fieldSize = 0;
                    shift = 0;

                    do
                    {
                        if ( readBuffer.getSize() <= readBuffer.getOffset() )
                        {
                            throw new CodecException ( ErrorCode.IncompleteData );
                        }

                        // shifts:
                        // 0, 7, 14, 21, 28, 37
                        if ( shift > 28 )
                        {
                            throw new CodecException ( ErrorCode.ProtocolError );
                        }

                        byteVal = readBuffer.readByte();

                        // The last byte can have only up to 4 bits set!
                        if ( shift == 28 && ( byteVal & 0x0F ) != byteVal )
                        {
                            throw new CodecException ( ErrorCode.ProtocolError );
                        }

                        // We use current byte, take 7 bits from it (the eight one is
                        // used as overflow bit), and shift it to add it to already read bits
                        fieldSize |= ( byteVal & 0x7F ) << shift;

                        // We just read 7 bits, next read will be in front of them
                        shift += 7;
                    }
                    while ( ( byteVal & 0x80 ) != 0 );
                }
                break;

            case WIRE_TYPE_VAR_LENGTH_A:
            case WIRE_TYPE_VAR_LENGTH_B:
                {
                    fieldSize = 0;

                    // We can't modify the original offset, since we are not consuming the header anymore.
                    // This is the actual payload.

                    int tmpOffset = readBuffer.getOffset();

                    do
                    {
                        // Not enough data in the buffer
                        if ( readBuffer.getSize() <= tmpOffset )
                        {
                            throw new CodecException ( ErrorCode.IncompleteData );
                        }

                        ++fieldSize;

                        byteVal = readBuffer.getBuffer()[ tmpOffset++ ];
                    }
                    while ( ( byteVal & 0x80 ) != 0 );
                }
                break;

            default:
                throw new CodecException ( ErrorCode.ProtocolError );
        }

        assert fieldSize >= 0;

        if ( readBuffer.getOffset() + fieldSize > readBuffer.getSize() )
        {
            throw new CodecException ( ErrorCode.IncompleteData );
        }

        assert readBuffer.getOffset() + fieldSize <= readBuffer.getSize();

        return new FieldHeader ( fieldId, fieldSize, wireType );
    }

    /**
     * Reads a single Boolean from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static boolean readBool ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        final byte tmpVal = readByte ( hdr, readBuffer );

        if ( tmpVal == 0 )
            return false;

        if ( tmpVal == 1 )
            return true;

        throw new DataSizeException ( 1, hdr.fieldSize );
    }

    /**
     * Reads a single Byte from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static byte readByte ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        if ( hdr.isVarLen() )
        {
            // If variable length encoding is used, we can accept up to 2 bytes)
            if ( hdr.fieldSize > 2 )
            {
                throw new DataSizeException ( 2, hdr.fieldSize );
            }

            byte shift = 0;
            int value = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 7 )
            {
                value |= ( ( int ) ( readBuffer.readByte() & 0x7F ) ) << shift;
            }

            if ( hdr.wireType == WIRE_TYPE_VAR_LENGTH_B )
            {
                value = -value;
            }

            return ( byte ) value;
        }

        if ( hdr.fieldSize > 1 )
        {
            throw new DataSizeException ( 1, hdr.fieldSize );
        }

        byte value = 0;

        if ( hdr.fieldSize > 0 )
        {
            value = readBuffer.readByte();
        }

        return value;
    }

    /**
     * Reads a single Short from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static short readShort ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        int value = 0;

        if ( hdr.isVarLen() )
        {
            // If variable length encoding is used, we can accept up to 3 bytes)
            if ( hdr.fieldSize > 3 )
            {
                throw new DataSizeException ( 3, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 7 )
            {
                value |= ( ( int ) ( readBuffer.readByte() & 0x7F ) ) << shift;
            }

            if ( hdr.wireType == WIRE_TYPE_VAR_LENGTH_B )
            {
                value = -value;
            }
        }
        else
        {
            if ( hdr.fieldSize > 2 )
            {
                throw new DataSizeException ( 2, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 8 )
            {
                value |= ( ( int ) ( readBuffer.readByte() & 0xFF ) ) << shift;
            }
        }

        return ( short ) value; // NOPMD - PMD doesn't like the 'short' type, but
        // we really really need it!
    }

    /**
     * Reads a single Integer from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static int readInteger ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        int value = 0;

        if ( hdr.isVarLen() )
        {
            // If variable length encoding is used, we can accept up to 5 bytes)
            if ( hdr.fieldSize > 5 )
            {
                throw new DataSizeException ( 5, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 7 )
            {
                value |= ( ( int ) ( readBuffer.readByte() & 0x7F ) ) << shift;
            }

            if ( hdr.wireType == WIRE_TYPE_VAR_LENGTH_B )
            {
                value = -value;
            }
        }
        else
        {
            if ( hdr.fieldSize > 4 )
            {
                throw new DataSizeException ( 4, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 8 )
            {
                value |= ( ( int ) ( readBuffer.readByte() & 0xFF ) ) << shift;
            }
        }

        return value;
    }

    /**
     * Reads a single Long from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static long readLong ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        long value = 0;

        if ( hdr.isVarLen() )
        {
            // If variable length encoding is used, we can accept up to 10 bytes)
            if ( hdr.fieldSize > 10 )
            {
                throw new DataSizeException ( 10, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 7 )
            {
                value |= ( ( long ) ( readBuffer.readByte() & 0x7F ) ) << shift;
            }

            if ( hdr.wireType == WIRE_TYPE_VAR_LENGTH_B )
            {
                value = -value;
            }
        }
        else
        {
            if ( hdr.fieldSize > 8 )
            {
                throw new DataSizeException ( 8, hdr.fieldSize );
            }

            byte shift = 0;

            for ( int i = 0; i < hdr.fieldSize; ++i, shift += 8 )
            {
                value |= ( ( long ) ( readBuffer.readByte() & 0xFF ) ) << shift;
            }
        }

        return value;
    }

    /**
     * Reads a single Float from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static float readFloat ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        return FloatingPointUtils.unpack754 ( readInteger ( hdr, readBuffer ) );
    }

    /**
     * Reads a single Double from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static double readDouble ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        return FloatingPointUtils.unpack754 ( readLong ( hdr, readBuffer ) );
    }

    /**
     * Reads a single InetAddress from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static InetAddress readInetAddress ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        if ( hdr.isVarLen() )
        {
            throw new CodecException ( ErrorCode.ProtocolError );
        }

        if ( hdr.fieldSize != 4 && hdr.fieldSize != 16 )
        {
            throw new DataSizeException ( 4, hdr.fieldSize );
        }

        byte[] buf = readBuffer.getBuffer();

        if ( readBuffer.getOffset() > 0 )
        {
            buf = new byte[ hdr.fieldSize ];
            System.arraycopy ( readBuffer.getBuffer(), readBuffer.getOffset(), buf, 0, hdr.fieldSize );
        }

        readBuffer.incOffset ( hdr.fieldSize );

        try
        {
            return InetAddress.getByAddress ( buf );
        }
        catch ( UnknownHostException e )
        {
            // This should not happen, we checked the dataSize before!
            assert false;
        }

        throw new CodecException ( ErrorCode.ProtocolError );
    }

    /**
     * Reads a single String from the buffer.
     *
     *        If the length of the data is correct for this type (and there is
     *        enough bytes in the buffer), this function reads the data
     *        and moves the 'offset' to the end of that field.
     *
     * @param hdr This field's header description.
     * @param readBuffer The description of the buffer to read the field from.
     *        The offset field should be pointing to the beginning of the field's actual payload
     *        when this function is called. This function modifies this offset
     *        to point at the first byte of the next field's header (or to the index = size
     *        of the buffer when this is the last field).
     *
     * @return The value read
     * @throws CodecException
     */
    public static String readString ( FieldHeader hdr, ReadBuffer readBuffer )
    throws CodecException
    {
        if ( hdr.isVarLen() )
        {
            throw new CodecException ( ErrorCode.ProtocolError );
        }

        String value = null;

        if ( hdr.fieldSize > 0 )
        {
            value = new String ( readBuffer.getBuffer(), readBuffer.getOffset(), hdr.fieldSize );
            readBuffer.incOffset ( hdr.fieldSize );
        }
        else if ( hdr.fieldSize == 0 )
        {
            value = new String();
        }

        return value;
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException, CodecException
     **/
    public static void appendField ( OutputStream toStream, Boolean value, int fieldId )
    throws IOException, CodecException
    {
        final byte tmpVal = ( byte ) ( ( value != null && value == true ) ? 1 : 0 );

        appendField ( toStream, tmpVal, fieldId );
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException, CodecException
     **/
    public static void appendField ( OutputStream toStream, Byte value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( value == null || value == 0 )
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_ZERO, 0 );
            return;
        }

        if ( value < 0 )
        {
            value = ( byte ) -value;

            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_VAR_LENGTH_B, 0 );

            while ( value != 0 )
            {
                byte byteVal = ( byte ) ( value & 0x7F );

                // & part is to get rid of negative bit being shifted and replicated
                value = ( byte ) ( ( value >> 7 ) & 0x01 );

                if ( value != 0 )
                {
                    byteVal |= 0x80;
                }

                toStream.write ( byteVal );
            }
        }
        else
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_1_BYTE, 1 );
            toStream.write ( value );
        }
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, Short value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( value == null || value == 0 )
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_ZERO, 0 );
            return;
        }

        byte wireType;
        int dataSize = 0;

        if ( value < 0 )
        {
            value = ( short ) -value;
            wireType = WIRE_TYPE_VAR_LENGTH_B;
        }
        else if ( ( value & 0xFF ) == value )
        {
            dataSize = 1;
            wireType = WIRE_TYPE_1_BYTE;
        }
        else
        {
            dataSize = 2;
            wireType = WIRE_TYPE_2_BYTES;
        }

        appendFieldHeader ( toStream, fieldId, wireType, dataSize );

        if ( wireType == WIRE_TYPE_VAR_LENGTH_A || wireType == WIRE_TYPE_VAR_LENGTH_B )
        {
            while ( value != 0 )
            {
                byte byteVal = ( byte ) ( value & 0x7F );

                // & part is to get rid of negative bit being shifted and replicated
                value = ( short ) ( ( value >> 7 ) & 0x1FF );

                if ( value != 0 )
                {
                    byteVal |= 0x80;
                }

                toStream.write ( byteVal );
            }
        }
        else
        {
            for ( int i = 0; i < dataSize; ++i )
            {
                toStream.write ( ( byte ) ( value & 0xFF ) );

                value = ( short ) ( value >> 8 );
            }
        }
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, Integer value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( value == null || value == 0 )
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_ZERO, 0 );
            return;
        }

        byte wireType;
        int dataSize = 0;

        if ( value < 0 )
        {
            value = -value;
            wireType = WIRE_TYPE_VAR_LENGTH_B;
        }
        else if ( ( value & 0xFF ) == value )
        {
            dataSize = 1;
            wireType = WIRE_TYPE_1_BYTE;
        }
        else if ( ( value & 0xFFFF ) == value )
        {
            dataSize = 2;
            wireType = WIRE_TYPE_2_BYTES;
        }
        else if ( ( value & 0x1FFFFF ) == value )
        {
            // We can fit the value in 3 bytes using variable length encoding
            dataSize = 3;
            wireType = WIRE_TYPE_VAR_LENGTH_A;
        }
        else
        {
            dataSize = 4;
            wireType = WIRE_TYPE_4_BYTES;
        }

        appendFieldHeader ( toStream, fieldId, wireType, dataSize );

        if ( wireType == WIRE_TYPE_VAR_LENGTH_A || wireType == WIRE_TYPE_VAR_LENGTH_B )
        {
            while ( value != 0 )
            {
                byte byteVal = ( byte ) ( value & 0x7F );

                // & part is to get rid of negative bit being shifted and replicated
                value = ( value >> 7 ) & 0x1FFFFFF;

                if ( value != 0 )
                {
                    byteVal |= 0x80;
                }

                toStream.write ( byteVal );
            }
        }
        else
        {
            for ( int i = 0; i < dataSize; ++i )
            {
                toStream.write ( ( byte ) ( value & 0xFF ) );

                value = value >> 8;
            }
        }
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, Long value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( value == null || value == 0 )
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_ZERO, 0 );
            return;
        }

        byte wireType;
        int dataSize = 0;

        if ( value < 0 )
        {
            value = -value;
            wireType = WIRE_TYPE_VAR_LENGTH_B;
        }
        else if ( ( value & 0xFF ) == value )
        {
            dataSize = 1;
            wireType = WIRE_TYPE_1_BYTE;
        }
        else if ( ( value & 0xFFFF ) == value )
        {
            dataSize = 2;
            wireType = WIRE_TYPE_2_BYTES;
        }
        else if ( ( value & 0x1FFFFF ) == value )
        {
            // We can fit the value in 3 bytes using variable length encoding
            dataSize = 3;
            wireType = WIRE_TYPE_VAR_LENGTH_A;
        }
        else if ( ( value & 0xFFFFFFFFL ) == value )
        {
            dataSize = 4;
            wireType = WIRE_TYPE_4_BYTES;
        }
        else if ( ( value & 0x1FFFFFFFFFFFFL ) == value )
        {
            // We can fit the value in 7 bytes (or less) using variable length encoding
            dataSize = 7;
            wireType = WIRE_TYPE_VAR_LENGTH_A;
        }
        else
        {
            dataSize = 8;
            wireType = WIRE_TYPE_8_BYTES;
        }

        appendFieldHeader ( toStream, fieldId, wireType, dataSize );

        if ( wireType == WIRE_TYPE_VAR_LENGTH_A || wireType == WIRE_TYPE_VAR_LENGTH_B )
        {
            while ( value != 0 )
            {
                byte byteVal = ( byte ) ( value & 0x7F );

                // & part is to get rid of negative bit being shifted and replicated
                value = ( value >> 7 ) & 0x1FFFFFFFFFFFFFFL;

                if ( value != 0 )
                {
                    byteVal |= 0x80;
                }

                toStream.write ( byteVal );
            }
        }
        else
        {
            for ( int i = 0; i < dataSize; ++i )
            {
                toStream.write ( ( byte ) ( value & 0xFF ) );

                value = value >> 8;
            }
        }
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, Float value, int fieldId )
    throws IOException, CodecException
    {
        appendField ( toStream, FloatingPointUtils.pack754 ( value ), fieldId );
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, Double value, int fieldId )
    throws IOException, CodecException
    {
        appendField ( toStream, FloatingPointUtils.pack754 ( value ), fieldId );
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, InetAddress value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        int dataSize = 0;

        if ( value == null )
        {
            dataSize = 0;
        }
        else if ( value instanceof Inet4Address )
        {
            dataSize = 4;
        }
        else if ( value instanceof Inet6Address )
        {
            dataSize = 16;
        }

        appendFieldHeader ( toStream, fieldId, getWireTypeForSize ( dataSize ), dataSize );

        if ( dataSize < 1 )
        {
            return;
        }

        byte[] data = value.getAddress();

        if ( data.length != dataSize )
        {
            throw new CodecException ( ErrorCode.InternalError );
        }

        toStream.write ( data );
    }

    /**
     *  Writes a single value to the stream
     *
     * @param toStream The stream to append the data to
     * @param value The value to append to the stream
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, String value, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( value == null || value.length() < 1 )
        {
            appendFieldHeader ( toStream, fieldId, WIRE_TYPE_ZERO, 0 );
            return;
        }

        byte[] data = value.getBytes();

        appendFieldHeader ( toStream, fieldId, getWireTypeForSize ( data.length ), data.length );
        toStream.write ( data );
    }

    /**
     *  Writes a buffer to the stream
     *
     * @param toStream The stream to append the data to
     * @param fieldId ID of the field to store in the header
     * @param buffer The buffer to encode
     * @param offset The offset in the buffer to start encoding from
     * @param length The number of bytes to encode. offset+length can't be bigger than buffer.length
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, int fieldId, byte[] buffer, int offset, int length )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        if ( length < 0 || offset < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidDataSize );
        }

        if ( buffer != null && offset + length > buffer.length )
        {
            throw new CodecException ( ErrorCode.InvalidDataSize );
        }

        if ( buffer == null && length > 0 )
        {
            throw new CodecException ( ErrorCode.InvalidDataSize );
        }

        appendFieldHeader ( toStream, fieldId, getWireTypeForSize ( length ), length );

        if ( length > 0 )
        {
            toStream.write ( buffer, offset, length );
        }
    }

    /**
     *  Writes a byte output stream to another stream
     *
     * @param toStream The stream to append the data to
     * @param byteStream The buffer stream whose content should be written
     * @param fieldId ID of the field to store in the header
     *
     * @throws IOException Thrown by OutputStream.write, CodecException
     **/
    public static void appendField ( OutputStream toStream, ByteArrayOutputStream byteStream, int fieldId )
    throws IOException, CodecException
    {
        if ( fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        int size = 0;

        if ( byteStream != null )
        {
            size = byteStream.size();
        }

        appendFieldHeader ( toStream, fieldId, getWireTypeForSize ( size ), size );

        if ( size > 0 )
        {
            byteStream.writeTo ( toStream );
        }
    }

    /**
     * Returns the wire type that can be used for the given data length
     * @param dataSize The size of data to encode
     * @return The wire type to use while encoding this data size
     */
    public static byte getWireTypeForSize ( int dataSize )
    {
        switch ( dataSize )
        {
            case 0:
                return WIRE_TYPE_ZERO;

            case 1:
                return WIRE_TYPE_1_BYTE;

            case 2:
                return WIRE_TYPE_2_BYTES;

            case 4:
                return WIRE_TYPE_4_BYTES;

            case 8:
                return WIRE_TYPE_8_BYTES;

            default:
                return WIRE_TYPE_LENGTH_DELIM;
        }
    }

    /**
     * Writes the header of the field (and appends it to the stream)
     *
     * @param toStream The output stream to encode the data to
     * @param fieldId The ID of the field to store in the header
     * @param wireType The wire type to use
     * @param dataSize The size of the data that will be stored in that field. Only used
     *               with WIRE_TYPE_LENGTH_DELIM wire type, it is ignored for all other wire types
     * @throws IOException Thrown by OutputStream.write, CodecException
     */
    public static void appendFieldHeader ( OutputStream toStream, int fieldId, byte wireType, int dataSize )
    throws IOException, CodecException
    {
        if ( dataSize < 0 || fieldId < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        assert ( wireType & 0x7 ) == wireType;

        if ( ( wireType & 0x7 ) != wireType )
        {
            throw new CodecException ( ErrorCode.InternalError );
        }

        // First 3 bits carry the wire type
        byte byteVal = ( byte ) ( wireType & 0x7 );

        // Next 4 bits carry the first 4 bits of the field ID:
        byteVal |= ( byte ) ( ( fieldId & 0xF ) << 3 );

        // Remove those 4 bits from the field ID:
        fieldId >>= 4;

        while ( fieldId > 0 )
        {
            // Field ID is longer - set the 'byteVal' value AND the overflow bit
            toStream.write ( byteVal | 0x80 );

            // Next byte will contain next 7 bits of the field ID:
            byteVal = ( byte ) ( fieldId & 0x7F );

            // Remove those 7 bits of the field ID:
            fieldId >>= 7;
        }

        // Last byte - no overflow bit set!
        toStream.write ( byteVal );

        if ( wireType == WIRE_TYPE_LENGTH_DELIM )
        {
            // In a single byte we can store 7 bits of the data length
            byteVal = ( byte ) ( dataSize & 0x7F );

            // Remove those 7 bits
            int remainingLen = dataSize >> 7;

            assert remainingLen >= 0;

            while ( remainingLen > 0 )
            {
                // data length is longer - set the 'byteVal' value AND the
                // overflow bit
                toStream.write ( byteVal | 0x80 );

                // Next byte will contain next 7 bits of the data length:
                byteVal = ( byte ) ( remainingLen & 0x7F );

                // Remove those 7 bits:
                remainingLen >>= 7;
            }

            // Last byte - no overflow bit set!
            toStream.write ( byteVal );
        }
    }
}
