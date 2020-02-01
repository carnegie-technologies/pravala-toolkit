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

/**
 * A class containing a buffer, an offset and max size that can be used
 *
 * This class is used for storing the pointer to the buffer's memory,
 * as well as the size that can be consumed. It is not necessarily
 * the same as buffer.length!
 *
 * The whole class is used to be able to efficiently pass buffer and its offset
 * and max usable size between different messages while deserializing data.
 *
 * It is used because Java doesn't have references that allow for function
 * parameters to be modified.
 */
public class ReadBuffer
{
    /**
     * The actual buffer
     */
    private byte[] buffer = null;

    /**
     * The offset from the start of the buffer.
     *
     *        To be modified by deserialization methods
     */
    private int offset = 0;

    /**
     * size of the buffer that can be used.
     *
     *        Not necessarily the same as buffer.length. It should not be
     *        greater than that, but it could be smaller in case we only
     *        want to deal with part of the buffer.
     */
    private int size = 0;

    /**
     * Constructor
     *
     * The buffer used will be null, with 0 offset and size
     */
    public ReadBuffer()
    {
    }

    /**
     * Creates a new ReadBuffer pointing to a given memory.
     *
     * It uses offset = 0 and the size equal to the size of the memory passed.
     *
     * @param buffer The memory to use
     * @param offset The offset in that memory
     * @param size The total size of the memory (starting at 0, not at the offset given)
     */
    public ReadBuffer ( byte[] buffer, int offset, int size )
    {
        this.buffer = buffer;
        this.offset = offset;
        this.size = size;
    }

    /**
     * Creates a new ReadBuffer pointing to a given memory.
     *
     * It uses offset = 0 and the size equal to the size of the memory passed.
     *
     * @param buffer The memory to use
     */
    public ReadBuffer ( byte[] buffer )
    {
        this.buffer = buffer;
        this.offset = 0;
        this.size = buffer.length;
    }

    /**
     * Creates a new ReadBuffer pointing to the same buffer as the original Read Buffer.
     *
     * Also the offset and the size is copied.
     *
     * @param readBuffer The original buffer to use
     */
    public ReadBuffer ( ReadBuffer readBuffer )
    {
        this.buffer = readBuffer.buffer;
        this.offset = readBuffer.offset;
        this.size = readBuffer.size;
    }

    /**
     * Returns buffer's memory (the original, not the copy)
     * @return the buffer
     */
    public byte[] getBuffer()
    {
        return buffer;
    }

    /**
     * Returns the byte at the current offset, and increments the offset.
     * @return The byte at the current offset.
     */
    public byte readByte()
    {
        return this.buffer[ this.offset++ ];
    }

    /**
     * Returns an offset at which the data to be used starts.
     * @return the offset in the buffer
     */
    public int getOffset()
    {
        return offset;
    }

    /**
     * Increases the offset by the given value
     *
     * @param byValue The value by which the offset should be increased
     */
    public void incOffset ( int byValue )
    {
        this.offset += byValue;
    }

    /**
     * Returns the max usable buffer's size
     *
     * @return the size
     */
    public int getSize()
    {
        return size;
    }

    /**
     * Sets the usable buffer's size to given value
     * @param size the size to set
     */
    public void setSize ( int size )
    {
        this.size = size;
    }
}
