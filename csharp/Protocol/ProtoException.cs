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
    using System;

    /// <summary>
    /// Protocol exception.
    /// </summary>
    public class ProtoException: Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Pravala.Protocol.ProtoException"/> class.
        /// </summary>
        /// <param name="errorCode">Error code.</param>
        public ProtoException ( ErrorCodes errorCode )
        {
            this.ErrorCode = errorCode;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Pravala.Protocol.ProtoException"/> class.
        /// </summary>
        /// <param name="errorCode">Error code.</param>
        /// <param name="message">The error message.</param>
        public ProtoException ( ErrorCodes errorCode, string message ):
            base ( message )
        {
            this.ErrorCode = errorCode;
        }

        /// <summary>
        /// Possible error codes.
        /// </summary>
        public enum ErrorCodes
        {
            /// <summary>One of the parameters passed to the function is invalid.</summary>
            InvalidParameter,

            /// <summary>Some of the data is missing.</summary>
            IncompleteData,

            /// <summary>There was a protocol error. Possibly an incompatible protocol version.</summary>
            ProtocolError,

            /// <summary>Unexpected data.</summary>
            InvalidData,

            /// <summary>Unexpected data size.</summary>
            InvalidDataSize,

            /// <summary>An internal error in the protocol code. This should not happen.</summary>
            InternalError,

            /// <summary>One of the values set/passed is too big to be encoded.</summary>
            TooBigValue,

            /// <summary>One of the values is invalid.</summary>
            IllegalValue,

            /// <summary>At least one of the required fields was not set.</summary>
            RequiredFieldNotSet,

            /// <summary>A field value is different than the message expects.</summary>
            DefinedValueMismatch,

            /// <summary>A field value doesn't satisfy the restrictions.</summary>
            FieldValueOutOfRange,

            /// <summary>A list field value doesn't satisfy the restrictions.</summary>
            ListSizeOutOfRange,

            /// <summary>A string value doesn't satisfy the restrictions.</summary>
            StringLengthOutOfRange
        }

        /// <summary>
        /// Gets or Sets the error code.
        /// </summary>
        public ErrorCodes ErrorCode
        {
            get;

            private set;
        }
    }
}
