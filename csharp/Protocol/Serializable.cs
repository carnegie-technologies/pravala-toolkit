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
    /// A base class that should be inherited by all messages.
    /// </summary>
    public abstract class Serializable
    {
        /// <summary>
        /// Clears all the fields.
        /// </summary>
        public virtual void ClearMessage()
        {
        }

        /// <summary>
        /// Validates the content of the object.
        /// </summary>
        /// <exception cref="ProtoException">If the message is incorrect.</exception>
        public virtual void ValidateMessage()
        {
        }

        /// <summary>
        /// Configures all "defined" fields.
        /// </summary>
        /// Checks if all required fields in this and all inherited objects are present and have legal values.
        /// If this is used by external code on messages that are to be sent
        /// it is probably a good idea to call setupDefines() first.
        public virtual void SetupMessageDefines()
        {
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
        internal virtual bool DeserializeMessageData ( byte[] buffer, uint offset, uint length )
        {
            this.ClearMessage();

            uint maxSize = offset + length;

            if ( buffer == null || maxSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidParameter,
                        "No buffer provided, or incorrect buffer size" );
            }

            bool ret = true;

            uint desOffset = offset;

            while ( offset < maxSize )
            {
                uint fieldId;
                uint payloadSize;
                Codec.WireTypes wireType;
                uint fieldOffset = offset;

#if DEBUG2
                Debug.WriteLine ( "Field; Offset: " + offset + "; MaxSize: " + maxSize );
#endif

                if ( !Codec.ReadHeader ( buffer, ref offset, maxSize, out fieldId, out payloadSize, out wireType ) )
                {
                    // There is not enough data. In this function we should have enough data!
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Not enough data to decode next field's header" );
                }

#if DEBUG2
                Debug.WriteLine ( "FieldId: " + fieldId + "; WireType: " + wireType + "; Offset: "
                        + offset + "; PayloadSize: " + payloadSize + "; MaxSize: " + maxSize );
#endif

                if ( offset + payloadSize > maxSize )
                {
                    // There was enough data to decode the header,
                    // but the payload size in this header describes data that we don't have...
                    throw new ProtoException (
                            ProtoException.ErrorCodes.InvalidDataSize,
                            "Not enough data to decode next field's payload; Field Id: " + fieldId
                            + "; WireType: " + ( ( int ) wireType )
                            + "; PayloadSize: " + payloadSize + "; DesOffset: " + desOffset
                            + "; FieldOffset: " + fieldOffset + "; CurOffset: " + offset
                            + "; MaxSize: " + maxSize + "; Length: " + length
                            + "; BufSize: " + buffer.Length );
                }

                if ( !this.DeserializeMessageField ( buffer, offset, fieldId, payloadSize, wireType ) )
                {
                    // Unknown field
                    ret = false;
                }

                // If we are still here (no exception),
                // it means that the field was handled properly (even if it was unknown)!
                offset += payloadSize;
            }

            this.ValidateMessage();

            return ret;
        }

        /// <summary>
        /// Serializes the object into the buffer.
        /// </summary>
        /// It first configures the defined values and then it tries to validate the message.
        /// If it validates correctly it is serialized into the buffer.
        /// <param name="buffer">Buffer to serialize data to.</param>
        /// <exception cref="ProtoException">On error or if the message doesn't validate.</exception>
        internal void SerializeMessageData ( Buffer buffer )
        {
            this.SetupMessageDefines();
            this.ValidateMessage();

            if ( buffer == null )
            {
                throw new ProtoException ( ProtoException.ErrorCodes.InvalidParameter, "No buffer provided" );
            }

            this.SerializeMessageFields ( buffer );
        }

        /// <summary>
        /// Deserializes a single field.
        /// </summary>
        /// <param name="buffer">Buffer to deserialize the payload from.</param>
        /// <param name="offset">Offset in the buffer where the payload starts.</param>
        /// <param name="fieldId">Field identifier.</param>
        /// <param name="payloadSize">Payload size.</param>
        /// <param name="wireType">The wire type used by the field.</param>
        /// <returns><c>true</c>, if field was deserialized, <c>false</c> otherwise (if it was unknown).</returns>
        /// <exception cref="ProtoException">If the decoding failed.</exception>
        protected virtual bool DeserializeMessageField (
                byte[] buffer,
                uint offset,
                uint fieldId,
                uint payloadSize,
                Codec.WireTypes wireType )
        {
            if ( buffer == null || offset + payloadSize > buffer.Length )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.InvalidParameter,
                        "Invalid buffer, or incorrect offset/payload size" );
            }

            return false;
        }

        /// <summary>
        /// Serializes all fields into the buffer.
        /// </summary>
        /// <param name="buffer">Buffer to serialize data to.</param>
        protected virtual void SerializeMessageFields ( Buffer buffer )
        {
        }
    }
}
