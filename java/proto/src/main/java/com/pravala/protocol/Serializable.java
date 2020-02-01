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

import java.io.IOException;
import java.io.OutputStream;

/**
 * A class that contains base methods for serialization and deserialization
 */
public abstract class Serializable
{
    /**
     * Serializes content of the message
     *
     * It just appends content to the buffer.
     * First the serializeData() from inherited object is called.
     * Next, all present, local fields are serialized -
     * either by serializing them directly, or by calling their serializeData()
     * This function does not encode base message's length.
     * To encode a complete message you need to call serialize()
     * from the base message class (and that is the only function that should
     * be used in most cases - usually there is no need to call serializeData() directly)
     * @param writeTo Where to serialize data
     * @throws IOException, CodecException
     */
    public void serializeData ( OutputStream writeTo ) throws IOException, CodecException
    {
        // If we 'define' and alias, we are safe to do this first and call 'serialize'
        // in our ancestor later (which could, potentially, define the storage type itself
        // overwriting our value). However, the protocol compiler should not allow
        // storage (aliased) types to be defined (and vice versa).
        setupDefines();

        validate();

        serializeAllFields ( writeTo );
    }

    /**
     * Deserializes data from the buffer
     *
     * First the deserialize() from inherited object is called.
     * Next, all present, local fields are deserialized -
     * either by deserializing them directly, or by calling their deserialize()
     * @param readBuffer The buffer to deserialize the data from. Its offset is NOT modified.
     * @return True if the message has been successfully deserialized,
     *         False if an unknown field was found, and throws an exception on errors
     * @throws CodecException
     */
    public boolean deserializeData ( ReadBuffer readBuffer ) throws CodecException
    {
        if ( readBuffer == null || readBuffer.getOffset() < 0
             || readBuffer.getSize() < 1 || readBuffer.getOffset() > readBuffer.getSize() )
        {
            throw new CodecException ( ErrorCode.InvalidParameter );
        }

        clear();

        boolean ret = true;

        int offset = readBuffer.getOffset();

        while ( offset < readBuffer.getSize() )
        {
            ReadBuffer tmpBuf = new ReadBuffer ( readBuffer.getBuffer(), offset, readBuffer.getSize() );

            Codec.FieldHeader hdr = Codec.readFieldHeader ( tmpBuf );

            // The new offset (it doesn't affect or depend on the deserializeField!):
            offset = tmpBuf.getOffset() + hdr.fieldSize;

            if ( offset > tmpBuf.getSize() )
            {
                throw new CodecException ( ErrorCode.IncompleteData );
            }

            tmpBuf.setSize ( offset );

            if ( !deserializeField ( hdr, tmpBuf ) )
            {
                ret = false;
            }
        }

        // Regardless of the 'ret' value we want to check if everything is OK
        validate();

        return ret;
    }

    /**
     * Clears the content
     *
     * All fields will either be set to their default values (or 0 if not set) or their clear()
     * method will be called and they will be set as not present.
     */
    public abstract void clear();

    /**
     * Checks validity of the data
     *
     * Checks if all required fields in this and all inherited objects are present and have legal values.
     * If this is used by external code on messages that are to be sent
     * (NOT on received ones!) it is probably a good idea to call setupDefines() first.
     *
     * @throws CodecException
     */
    public abstract void validate() throws CodecException;

    /**
     * Sets the values of all the fields 'defined' by this and all inherited objects (if any)
     */
    public abstract void setupDefines();

    /**
     * Serializes all fields to the buffer. It is called by serializeData.
     * @param writeTo Where to serialize data
     * @throws IOException, CodecException
     */
    protected abstract void serializeAllFields ( OutputStream writeTo ) throws IOException, CodecException;

    /**
     * Deserializes a single field from the buffer.
     * @param hdr Field's header description.
     * @param readBuffer The buffer to deserialize the data from. The offset should be pointing at the first byte
     *                   of the actual payload (just after the header).
     *                   This function may modify this buffer's offset. It may also be invalid after calling it,
     *                   or in the middle of the field's payload.
     * @return True if the field has been successfully deserialized,
     *         False if an unknown field was found, and throws an exception on errors
     * @throws CodecException
     */
    protected abstract boolean deserializeField ( Codec.FieldHeader hdr, ReadBuffer readBuffer ) throws CodecException;
}
