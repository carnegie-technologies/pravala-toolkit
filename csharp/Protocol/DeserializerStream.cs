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
    using System;

    /// <summary>
    /// A class that assists in deserializing BaseSerializable messages from stream.
    /// </summary>
    public abstract class DeserializerStream
    {
        /// <summary>
        /// Returns the number of bytes available in the stream.
        /// </summary>
        public abstract uint AvailableBytes
        {
            get;
        }

        /// <summary>
        /// Reads a single byte from the stream.
        /// </summary>
        /// <returns>The byte read.</returns>
        public abstract byte ReadByte();

        /// <summary>
        /// Reads multiple bytes from the stream.
        /// </summary>
        /// <param name="output">The buffer to fill with the data from the stream.</param>
        public abstract void ReadBytes ( byte[] output );

        /// <summary>
        /// Reads a single message's payload from the stream.
        /// </summary>
        /// <remarks>It should be called multiple times until it returns a payload.</remarks>
        /// <param name="missingBytes">The number of bytes missing.</param>
        /// <returns>The buffer with message's payload (without the header),
        /// or null if there were not enough bytes.</returns>
        /// <exception cref="Protocol.ProtoException">When de-serializing fails.</exception>
        internal byte[] ReadMessage ( out int missingBytes )
        {
            if ( this.header == null )
            {
                this.header = new Buffer();
            }

            // Min length - 2 bytes, one for the field header, one for the 'length' field
            while ( this.header.Size < 2 )
            {
                if ( this.AvailableBytes < 1 )
                {
                    missingBytes = ( 2 - this.header.Size );
                    return null;
                }

                this.header.AppendByte ( this.ReadByte() );
            }

            uint offset = 0;

            uint fieldId;
            uint lengthFieldSize;
            Codec.WireTypes wireType;

            while ( true )
            {
                offset = 0;

                if ( Codec.ReadHeader (
                             this.header.Bytes,
                             ref offset,
                             ( uint ) this.header.Size,
                             out fieldId,
                             out lengthFieldSize,
                             out wireType ) )
                {
#if DEBUG2
                    Debug.WriteLine ( "Read message header properly; FieldId: " + fieldId + "; WireType: "
                            + wireType + "; LengthFieldSize: " + lengthFieldSize
                            + "; Offset: " + offset );
#endif
                    // We had enough data to decode the header (just the header, we don't know about the actual field)
                    break;
                }

                if ( this.AvailableBytes < 1 )
                {
                    // We have no more data, let's ask for more:
                    missingBytes = 1;
                    return null;
                }

                this.header.AppendByte ( this.ReadByte() );
            }

            if ( fieldId != BaseSerializable.LengthFieldId )
            {
                throw new ProtoException (
                        ProtoException.ErrorCodes.ProtocolError,
                        "Expected length field ID: " + BaseSerializable.LengthFieldId + "; Received: " + fieldId );
            }

            while ( this.header.Size < offset + lengthFieldSize )
            {
                if ( this.AvailableBytes < 1 )
                {
                    missingBytes = ( int ) ( offset + lengthFieldSize - this.header.Size );
                    return null;
                }

                this.header.AppendByte ( this.ReadByte() );
            }

            // Here we should have enough data to read the actual content of the length field:
            uint messagePayloadSize;

            Codec.Decode ( this.header.Bytes, offset, lengthFieldSize, wireType, out messagePayloadSize );

            if ( messagePayloadSize > this.AvailableBytes )
            {
                // We need this many bytes to read the actual message content:
                missingBytes = ( int ) ( messagePayloadSize - this.AvailableBytes );
                return null;
            }

            byte[] payload = new byte[ messagePayloadSize ];

            this.ReadBytes ( payload );

#if DEBUG2
            Debug.WriteLine ( "Pending Header: [" + header.Size + "]: "
                    + dumpBytes ( this.header.Bytes, this.header.Size ) );
#endif

            this.header = null;

#if DEBUG2
            Debug.WriteLine ( "Payload Buffer [" + messagePayloadSize + "]: "
                    + dumpBytes ( payload, payload.Length ) );
#endif

            missingBytes = 0;
            return payload;
        }

        /// <summary>
        /// A buffer used for storing the header until the whole message can be read.
        /// </summary>
        private Buffer header;
    }
}
