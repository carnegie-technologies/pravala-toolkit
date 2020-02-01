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

//// #define DEBUG2

namespace Pravala.Protocol
{
    using System.Diagnostics;

    /// <summary>
    /// Base object to be inherited by all "base message" protocol objects
    /// </summary>
    public abstract class BaseSerializable: Serializable
    {
        /// <summary>
        /// The ID of the 'length' field - always 0
        /// </summary>
        internal const uint LengthFieldId = 0;

        /// <summary>
        /// The original buffer the base message used when it was deserialized.
        /// </summary>
        private byte[] baseBuffer;

        /// <summary>
        /// Serializes the entire base message, including the 'length' field at the beginning.
        /// </summary>
        /// <remarks>The result can be directly sent over the network using stream protocol</remarks>
        /// <returns>Serialized message as a byte array</returns>
        /// <exception cref="Protocol.ProtoException">When serializing fails
        /// (for example if some of the required fields are not set).</exception>
        public byte[] SerializeMessageWithLength()
        {
            Buffer buf = new Buffer();

            this.SerializeMessageData ( buf );

            int payloadSize = buf.Size;

            if ( payloadSize < 1 )
            {
                return new byte[ 0 ];
            }

            // We append the header at the end of the 'buf' - just after the payload.
            // We need it at the beginning, but until we actually serialize the data,
            // we don't know what the payload size is. We could also put it in a separate Buffer,
            // but we already have one that likely has some empty space left.
            // We would have to move things around anyway.
            Codec.AppendValue ( buf, LengthFieldId, ( uint ) payloadSize );

            byte[] ret = new byte[ buf.Size ];

            int idx = 0;

            // Let's copy the header (which was appended to the end of 'buf')
            // to the beginning of the buffer that we actually are about to return.
            for ( int i = payloadSize; i < buf.Size; ++i )
            {
                ret[ idx++ ] = buf.Bytes[ i ];
            }

            // And now append the actual payload.
            for ( int i = 0; i < payloadSize; ++i )
            {
                ret[ idx++ ] = buf.Bytes[ i ];
            }

            return ret;
        }

        /// <summary>
        /// Used to deserialize base message with the length field at the beginning
        /// </summary>
        /// <remarks>It should be called multiple times until it returns 0.</remarks>
        /// <param name="inputStream">The input stream to read the message from.</param>
        /// <returns>The number of bytes missing.</returns>
        /// <exception cref="Protocol.ProtoException">When de-serializing fails.</exception>
        public int DeserializeBaseMessage ( DeserializerStream inputStream )
        {
            int missingBytes;

            byte[] messagePayload = inputStream.ReadMessage ( out missingBytes );

            if ( messagePayload == null )
            {
                return missingBytes;
            }

            this.DeserializeMessageData ( messagePayload, 0, ( uint ) messagePayload.Length );

            // If all is good, let's store it for later!
            this.baseBuffer = messagePayload;

            return 0;
        }

        /// <summary>
        /// Deserializes the data from the buffer.
        /// </summary>
        /// <param name="buffer">Buffer to deserialize data from.</param>
        /// <param name="offset">Starting offset.</param>
        /// <param name="length">The number of bytes that can be read (starting at offset, not 0).</param>
        /// <returns>
        /// <c>true</c>, if all fields in this message were known,
        /// <c>false</c> if some of the fields were unknown.
        /// </returns>
        /// <exception cref="ProtoException">
        /// On error or if the message doesn't validate after being deserialized.
        /// </exception>
        internal override bool DeserializeMessageData ( byte[] buffer, uint offset, uint length )
        {
            bool ret = base.DeserializeMessageData ( buffer, offset, length );

            // If it worked we need to create a copy of the buffer:
            byte[] newBuf = new byte[ length ];

            for ( uint i = 0; i < length; ++i )
            {
                newBuf[ i ] = buffer[ offset + i ];
            }

            this.baseBuffer = newBuf;

            return ret;
        }

        /// <summary>
        /// Used to deserialize a message based on its base message.
        /// </summary>
        /// <remarks>It will deserialize the message, and if it works it will validate it.</remarks>
        /// <param name="baseMessage">The base message to de-serialize this message based on.</param>
        /// <returns><c>true</c>, if all the fields were recognized, <c>false</c>, if de-serializing succeeded,
        /// but some of the fields were not recognized.</returns>
        /// <exception cref="Protocol.ProtoException">When the message cannot be properly de-serialized,
        /// or when it doesn't validate properly
        /// (for example, if required fields are missing or have incorrect values.</exception>
        protected bool DeserializeFromBaseMessage ( BaseSerializable baseMessage )
        {
            if ( baseMessage == null )
            {
                throw new ProtoException ( ProtoException.ErrorCodes.InvalidParameter, "No base message provided" );
            }

            bool ret
                = this.DeserializeMessageData ( baseMessage.baseBuffer, 0U, ( uint ) baseMessage.baseBuffer.Length );

            // We can just use the same buffer!
            this.baseBuffer = baseMessage.baseBuffer;

            return ret;
        }

        /// <summary>
        /// A helper function that returns the string with a dump of bytes passed
        /// </summary>
        /// <param name="data">The byte array to dump</param>
        /// <param name="dataSize">Number of bytes to dump</param>
        /// <returns>The list of bytes (as numbers) as a string.</returns>
        private string DumpBytes ( byte[] data, int dataSize )
        {
            string ret = string.Empty;

            for ( int i = 0; i < dataSize; ++i )
            {
                ret += " " + ( ( int ) data[ i ] );
            }

            return ret;
        }
    }
}
