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
    /// Represents an IP v4/v6 address in its numeric form.
    /// </summary>
    public class IpAddress
    {
        /// <summary>
        /// The content of the IP Address. 4 bytes for IP v4; 16 bytes for IP v6
        /// </summary>
        private byte[] buffer;

        /// <summary>
        /// Initializes a new instance of the <see cref="Pravala.Protocol.IpAddress" /> class.
        /// </summary>
        /// <param name="b1">First byte of the IP v4</param>
        /// <param name="b2">Second byte of the IP v4</param>
        /// <param name="b3">Third byte of the IP v4</param>
        /// <param name="b4">Fourth byte of the IP v4</param>
        public IpAddress ( byte b1, byte b2, byte b3, byte b4 )
        {
            this.buffer = new byte[ 4 ];

            this.buffer[ 0 ] = b1;
            this.buffer[ 1 ] = b2;
            this.buffer[ 2 ] = b3;
            this.buffer[ 3 ] = b4;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Pravala.Protocol.IpAddress" /> class.
        /// </summary>
        /// <param name="data">Data; It has to have 4 (for IP v4) or 16 (for IP v6) bytes.</param>
        /// <exception cref="System.ArgumentException">Thrown if the data has neither 4 nor 16 bytes.</exception>
        public IpAddress ( byte[] data )
        {
            if ( data == null || ( data.Length != 4 && data.Length != 16 ) )
            {
                throw new ArgumentException ( "Invalid memory size" );
            }

            this.buffer = new byte[ data.Length ];

            for ( int i = 0; i < data.Length; ++i )
            {
                this.buffer[ i ] = data[ i ];
            }
        }

        /// <summary>
        /// Gets the memory associated with this IP address
        /// </summary>
        public byte[] Bytes
        {
            get
            {
                return this.buffer;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this instance is an IP v4 address
        /// </summary>
        /// <value><c>true</c> if this instance is an IP v4 address; otherwise, <c>false</c>.</value>
        public bool IsIpv4
        {
            get
            {
                return this.buffer != null && this.buffer.Length == 4;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this instance is an IP v6 address
        /// </summary>
        /// <value><c>true</c> if this instance is an IP v6 address; otherwise, <c>false</c>.</value>
        public bool IsIpv6
        {
            get
            {
                return this.buffer != null && this.buffer.Length == 16;
            }
        }

        /// <summary>
        /// Returns the string description of the IP Address
        /// </summary>
        /// <returns>The string description of the IP Address</returns>
        public override string ToString()
        {
            if ( this.buffer == null )
            {
                return "Empty Address";
            }

            if ( this.buffer.Length == 4 )
            {
                return ( int ) this.buffer[ 0 ] + "." + ( int ) this.buffer[ 1 ] + "." + ( int ) this.buffer[ 2 ] + "."
                       + ( int ) this.buffer[ 3 ];
            }
            else if ( this.buffer.Length == 16 )
            {
                string str = string.Empty;

                for ( uint i = 0; i < 8; i += 2 )
                {
                    if ( str.Length > 0 )
                    {
                        str += ":";
                    }

                    str += ( ( int ) this.buffer[ i ] ).ToString ( "X2" );
                    str += ( ( int ) this.buffer[ i + 1 ] ).ToString ( "X2" );
                }

                return str;
            }
            else
            {
                return "Invalid Address";
            }
        }
    }
}
