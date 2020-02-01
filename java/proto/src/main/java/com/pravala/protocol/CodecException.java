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
 * A class representing an exception thrown when the codec/generated protocol classes
 * experience an error.
 */
public class CodecException extends Exception
{
    /**
     * The serial version UID.
     */
    private static final long serialVersionUID = -3117198554105792005L;

    /**
     * The errorCode carried by this exception
     */
    private final ErrorCode errorCode;

    /**
     * Constructor
     * @param errorCode The code of the error experienced
     */
    public CodecException ( ErrorCode errorCode )
    {
        super ( );
        this.errorCode = errorCode;
    }

    /**
     * @return The error code
     */
    public ErrorCode getErrorCode()
    {
        return errorCode;
    }

    /**
     * The description of the CodecException
     *
     * @return The description of the exception, which is the name of the error
     *         code used when throwing it.
     */
    @Override
    public String toString()
    {
        return errorCode.toString();
    }
}
