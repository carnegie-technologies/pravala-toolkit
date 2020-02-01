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
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Class that should be inherited by all base messages.
 */
public abstract class SerializableBase extends Serializable
{
    /**
     * The ReadBuffer that stores the original data used to deserialize this message
     *
     * It is only set in base messages on which 'deserialize ( ReadBuffer )' is called
     * (and only when the entire message is available - so deserialize returns 0 or -1)
     */
    private ReadBuffer orgDataBuf = null;

    /**
     * Creates a copy of the original read buffer object
     *
     * It can be used by messages inheriting this base message to deserialize.
     * It does NOT copy the data itself, just the 'ReadBuffer' object,
     * which will use the same memory as the original object.
     */
    protected ReadBuffer getBufferDescCopy()
    {
        if ( orgDataBuf == null )
        {
            return null;
        }

        // We want to return a new object, so if anything modifies the offset it
        // happens in the copy, and not in the original!
        return new ReadBuffer ( orgDataBuf );
    }

    /**
     * Serializes entire message to the buffer
     *
     * It calls serializeData from inheriting classes and also encodes the total message's length.
     * @param toStream The stream to serialize the data to
     * @throws java.io.IOException, CodecException
     */
    public void serializeWithLength ( OutputStream toStream ) throws IOException, CodecException
    {
        assert toStream != null;

        if ( toStream == null )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        ByteArrayOutputStream buf = new ByteArrayOutputStream();

        serializeData ( buf );

        if ( buf.size() < 0 )
        {
            throw new CodecException ( ErrorCode.TooMuchData );
        }

        Codec.appendField ( toStream, ( Integer ) buf.size(), 0 );
        buf.writeTo ( toStream );
    }

    /**
     * It reads a single message from the input stream
     *
     * @param fromStream The stream to read the message from
     * @return The number of bytes consumed, or -1 if the end of the stream was reached before
     *  the entire message could be read.
     * @throws IOException, CodecException
     */
    public int deserializeWithLength ( InputStream fromStream ) throws IOException, CodecException
    {
        byte[] buf = null;
        int dataSize = 0;
        int bufSize = 0;

        while ( true )
        {
            // deserialize() function returns a positive value when it has all the bytes
            // needed to deserialize the function. The returned value is the number of consumed
            // bytes.
            // When the value is negative, it means that there was not enough data in the buffer.
            // The number of (at least) missing bytes is the absolute value of the (negative) number
            // returned. Here we focus on the case where there are some bytes missing,
            // so we use 'missingBytes' variable to represent this.
            // However, at first this variable actually stores both the number of consumed bytes
            // (if it's positive), and the number of missing bytes (if it's negative).
            int missingBytes = deserializeWithLength ( buf, 0, dataSize );

            // If missingBytes > 0, then it means that the value stored in missingBytes
            // is actually the number of 'consumed bytes', and that we now have a complete message.
            // Let's report the number of bytes we just consumed!
            if ( missingBytes > 0 )
            {
                return missingBytes;
            }

            // missingBytes is <= 0, which means there were not enough bytes to deserialize the message
            // Let's make the 'missingBytes' to store the number of bytes missing as a positive value.
            missingBytes *= -1;

            // From this point, missingBytes is positive, and stores the number of bytes that
            // are still missing (at least, this number could change in the future - this is because
            // we may not have enough bytes to decode even the message length, so the deserialize
            // could report the number of missing bytes it knows for sure it's going to need,
            // and, in the future, once it has the entire header, it will report the actual number
            // of missing bytes, that will most likely be larger).

            // deserialize above should not return 0!
            assert missingBytes > 0;

            if ( dataSize + missingBytes > bufSize )
            {
                bufSize = dataSize + missingBytes;

                // At the beginning we want to allocate a little more space,
                // just so we could avoid reallocating it after every byte of the message header!
                if ( bufSize < 16 )
                {
                    bufSize = 16;
                }

                byte[] oldBuf = buf;
                buf = new byte[ bufSize ];

                if ( dataSize > 0 )
                {
                    System.arraycopy ( oldBuf, 0, buf, 0, dataSize );
                }
            }

            while ( missingBytes > 0 )
            {
                int r = fromStream.read ( buf, dataSize, missingBytes );

                if ( r < 0 )
                {
                    return -1;
                }

                dataSize += r;
                missingBytes -= r;
            }
        }
    }

    /**
     * Deserializes the entire message from the buffer
     *
     * It calls deserializeData from inheriting classes and also decodes the total message's length.
     * @param buffer The buffer to deserialize the data from
     * @param offset The offset in the buffer to start deserializing from
     * @return Negative value means that more bytes are needed. The number of missing bytes is the absolute
     *         value of the number returned. Positive value means that the entire message was deserialized,
     *         and that the returned number of bytes were consumed
     * @throws CodecException
     */
    public int deserializeWithLength ( byte[] buffer, int offset ) throws CodecException
    {
        if ( buffer == null || offset < 0 || offset > buffer.length )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        return deserializeWithLength ( buffer, offset, buffer.length - offset );
    }

    /**
     * Deserializes the entire message from the buffer
     *
     * It calls deserializeData from inheriting classes and also decodes the total message's length.
     * @param buffer The buffer to deserialize the data from.
     * @param offset The offset in the buffer to start deserializing from.
     * @param length The number of bytes (starting from the offset) that can be deserialized.
     * @return Negative value means that more bytes are needed. The number of missing bytes is the absolute
     *         value of the number returned. Positive value means that the entire message was deserialized,
     *         and that the returned number of bytes were consumed
     * @throws CodecException
     */
    public int deserializeWithLength ( byte[] buffer, final int offset, final int length ) throws CodecException
    {
        orgDataBuf = null;

        if ( length < 0 )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        // Min length - 2 bytes, one for the field header, one for the 'length' field
        if ( length < 2 )
        {
            return -1 * ( 2 - length );
        }

        if ( buffer == null || offset < 0 || length < 0 || offset + length > buffer.length )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        assert length >= 0;

        int payloadSize = 0;
        ReadBuffer readBuffer = new ReadBuffer ( buffer, offset, offset + length );

        try
        {
            // This DOES modify the offset
            Codec.FieldHeader hdr = Codec.readFieldHeader ( readBuffer );

            // This is not the code we expected!
            if ( hdr.fieldId != 0 )
            {
                throw new CodecException ( ErrorCode.ProtocolError );
            }

            // This DOES modify the offset
            payloadSize = Codec.readInteger ( hdr, readBuffer );

            assert readBuffer.getOffset() > offset;

            if ( payloadSize < 0 )
            {
                throw new CodecException ( ErrorCode.ProtocolError );
            }
        }
        catch ( CodecException e )
        {
            if ( e.getErrorCode() == ErrorCode.IncompleteData )
            {
                // We still are missing some data. We don't know how much, so
                // lets just say there is one byte missing!
                return -1;
            }

            // Otherwise just pass this error up!
            throw e;
        }

        // We don't have the entire message yet!
        // Report the number of missing bytes.
        if ( readBuffer.getOffset() + payloadSize > offset + length )
        {
            return -1 * ( readBuffer.getOffset() + payloadSize - ( offset + length ) );
        }

        assert offset < readBuffer.getOffset();
        assert readBuffer.getOffset() + payloadSize <= offset + length;
        assert readBuffer.getOffset() + payloadSize <= buffer.length;
        assert readBuffer.getOffset() + payloadSize <= readBuffer.getSize();

        readBuffer.setSize ( readBuffer.getOffset() + payloadSize );

        deserializeBase ( readBuffer );

        // deserializeBase should NOT modify the buffer...
        return payloadSize + ( readBuffer.getOffset() - offset );
    }

    /**
     * Deserializes an entire, single, message from the buffer
     *
     * It calls deserializeData from inheriting classes, but does NOT decode the total message's length.
     * It assumes that the entire data passed for deserializing is a single message.
     *
     * @param readBuffer The buffer to deserialize the data from. Its offset is NOT modified.
     * @return True if the message has been successfully deserialized,
     *         False if an unknown field was found, and throws an exception on errors
     * @throws CodecException
     */
    public boolean deserializeBase ( ReadBuffer readBuffer ) throws CodecException
    {
        orgDataBuf = null;

        // deserializeData should NOT modify the buffer...
        boolean ret = deserializeData ( readBuffer );

        // Lets keep the buffer for later - other messages will be able to deserialize
        // themselves using this base message

        orgDataBuf = new ReadBuffer ( readBuffer );

        return ret;
    }

    /**
     * Deserializes this serializable using its base serializable
     *
     * It is protected and should be used by base message's deserializeFromBase function.
     * It does not perform sanity checks (except for the existence of the original buffer).
     * If it succeeds, a reference to the original buffer from the baseSerializable
     * will be stored in this object as well.
     *
     * @param baseSerializable The base serializable to deserialize the data from
     * @return True if this serializable has been successfully deserialized and it used the entire available data.
     *         False means that although the serializable has been deserialized properly,
     *            there are some additional, unknown fields that have not been deserialized.
     *         If there is a deserialization error it throws one of the exceptions.
     * @throws CodecException
     */
    protected boolean deserializeFromBaseSerializable ( SerializableBase baseSerializable ) throws CodecException
    {
        orgDataBuf = null;

        return deserializeBase ( baseSerializable.getBufferDescCopy() );
    }
}
