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

/**
 * A class representing an exception thrown when the codec tries to
 * deserialize data that has wrong size for the expected value type.
 *
 */
public class DataSizeException extends CodecException
{
    /**
     * The serial version UID.
     */
    private static final long serialVersionUID = -393600750342891885L;

    /**
     * The expected data size.
     *
     *  It could also be a minimum/maximum allowed data size,
     *  or one of several allowed data sizes.
     */
    private final int expectedSize;

    /**
     * The data size that was received instead of the expected/allowed value
     */
    private final int actualSize;

    /**
     * Constructor
     * @param expectedSize The data size that was expected (or maximal)
     * @param actualSize The size that was received
     */
    public DataSizeException ( int expectedSize, int actualSize )
    {
        super ( ErrorCode.InvalidDataSize );

        this.expectedSize = expectedSize;
        this.actualSize = actualSize;
    }

    /**
     * @return the expectedSize
     */
    public int getExpectedSize()
    {
        return expectedSize;
    }

    /**
     * @return the actualSize
     */
    public int getActualSize()
    {
        return actualSize;
    }

    /**
     * The description of the DataSizeException
     *
     * @return The description of the exception, which contains the
     *         received data size, and the expected/allowed value
     */
    @Override
    public String toString()
    {
        return "Expected size: " + expectedSize + "; Actual size: " + actualSize;
    }
}
