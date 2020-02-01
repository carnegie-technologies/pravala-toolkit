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
    /// A helper class that contains a buffer to append data to. This buffer is dynamically resized when needed.
    /// </summary>
    public class Buffer
    {
        /// <summary>
        /// The actual data.
        /// </summary>
        private byte[] buffer;

        /// <summary>
        /// The size that is actually used.
        /// </summary>
        private int size = 0;

        /// <summary>
        /// Gets or Sets the entire data buffer.
        /// </summary>
        public byte[] Bytes
        {
            get
            {
                return this.buffer;
            }
        }

        /// <summary>
        /// Gets or Sets the size of data that is actually used.
        /// </summary>
        public int Size
        {
            get
            {
                return this.size;
            }
        }

        /// <summary>
        /// Appends a single byte to the buffer.
        /// </summary>
        /// <param name="value">The byte to append.</param>
        public void AppendByte ( byte value )
        {
            this.ReserveSpace ( 1 );

            this.buffer[ this.size++ ] = value;
        }

        /// <summary>
        /// Appends a number of bytes to the buffer.
        /// </summary>
        /// <param name="data">The byte array to use.</param>
        /// <param name="dataSize">The number of bytes to append.</param>
        /// <exception cref="System.ArgumentNullException">
        /// When the dataSize is > 0, but the buffer is null.
        /// </exception>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// When the dataSize is greater than the buffer size.
        /// </exception>
        public void AppendBytes ( byte[] data, int dataSize )
        {
            if ( dataSize > 0 )
            {
                if ( data == null )
                {
                    throw new ArgumentNullException();
                }

                if ( dataSize > data.Length )
                {
                    throw new ArgumentOutOfRangeException (
                            "Data size used (" + dataSize + ") is greater than the size of the array: " + data.Length );
                }

                this.ReserveSpace ( dataSize );
            }

            for ( int i = 0; i < dataSize; ++i )
            {
                this.buffer[ this.size++ ] = data[ i ];
            }
        }

        /// <summary>
        /// Appends the entire byte array to the buffer.
        /// </summary>
        /// <param name="data">The data to append.</param>
        public void AppendBytes ( byte[] data )
        {
            if ( data == null )
            {
                return;
            }

            this.AppendBytes ( data, data.Length );
        }

        /// <summary>
        /// Appends another Buffer to this instance.
        /// </summary>
        /// <param name="buf">The buffer to append.</param>
        public void AppendBuffer ( Buffer buf )
        {
            if ( buf == null )
            {
                return;
            }

            this.AppendBytes ( buf.Bytes, buf.Size );
        }

        /// <summary>
        /// Reserves (at least) given number of bytes in the buffer.
        /// </summary>
        /// <param name="value">The number of bytes to reserve.</param>
        public void ReserveSpace ( int value )
        {
            if ( this.buffer != null && this.size + value <= this.buffer.Length )
            {
                return;
            }

            if ( value < 16 )
            {
                value = 16;
            }

            Array.Resize<byte> ( ref this.buffer, this.size + value );
        }
    }
}
