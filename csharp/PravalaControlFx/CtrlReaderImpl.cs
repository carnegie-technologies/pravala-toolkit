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

namespace Pravala.Control.Fx
{
    using System;
    using System.IO;
    using System.Net.Sockets;
    using System.Threading.Tasks;

    /// <summary>
    /// .Net implementation of CtrlReader.
    /// </summary>
    /// <remarks>
    /// TODO: Eliminate wasteful memory copy to MemoryStream on receive/read.
    /// </remarks>
    internal class CtrlReaderImpl: CtrlReader
    {
        #region Fields
        /// <summary>
        /// Socket to read from.
        /// </summary>
        private readonly Socket socket;

        /// <summary>
        /// The network stream used for reading from the socket.
        /// </summary>
        private MemoryStream stream = new MemoryStream();
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the CtrlReaderImpl class.
        /// </summary>
        /// <param name="socket">The socket to read from.</param>
        public CtrlReaderImpl ( Socket socket )
        {
            this.socket = socket;
        }

        #endregion

        #region Properties
        /// <summary>
        /// Returns the number of bytes available in the stream.
        /// </summary>
        public override uint AvailableBytes
        {
            get
            {
                if ( this.stream == null )
                {
                    return 0;
                }

                return ( uint ) ( this.stream.Length - this.stream.Position );
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Reads a single byte from the stream.
        /// </summary>
        /// <returns>The byte read.</returns>
        public override byte ReadByte()
        {
            return ( byte ) this.stream.ReadByte();
        }

        /// <summary>
        /// Reads multiple bytes from the stream.
        /// </summary>
        /// <param name="output">The buffer to fill with the data from the stream.</param>
        public override void ReadBytes ( byte[] output )
        {
            this.stream.Read ( output, 0, ( int ) this.AvailableBytes );
        }

        /// <summary>
        /// Loads data from the input stream.
        /// </summary>
        /// <param name="count">The number of bytes to read.</param>
        /// <returns>The asynchronous load data request.</returns>
        public override async Task<uint> LoadAsync ( uint count )
        {
            // Receive buffer
            byte[] buffer = new byte[ count ];

            // Await-able task for waiting on receive completion
            var asyncReceiveSource = new TaskCompletionSource<uint>();

            // Callback when receive completes
            AsyncCallback receiveCallback = result =>
            {
                int receivedBytes = 0;

                // Retrieve the CtrlClient instance from the state object
                CtrlReaderImpl instance = ( CtrlReaderImpl ) result.AsyncState;

                try
                {
                    // Complete the receive if the socket hasn't been closed
                    if ( instance.socket != null && instance.socket.Connected )
                    {
                        receivedBytes = instance.socket.EndReceive ( result );
                    }
                }
                catch ( ObjectDisposedException )
                {
                    // If we're being stopped, the socket may already have
                    // been disposed before the callback runs
                }

                // Finish connect task
                asyncReceiveSource.SetResult ( ( uint ) receivedBytes );
            };

            // Wait for receive
            this.socket.BeginReceive ( buffer, 0, ( int ) count, 0, receiveCallback, this );
            uint byteCount = await asyncReceiveSource.Task;

            // Write the data to the stream as CtrlReader provides a stream-oriented API.
            this.stream.Write ( buffer, 0, ( int ) byteCount );
            this.stream.Seek ( this.stream.Length - byteCount, SeekOrigin.Begin );

            return byteCount;
        }

        /// <summary>
        /// Dispose the MemoryStream backing stream.
        /// </summary>
        public override void Dispose()
        {
            if ( this.stream != null )
            {
                this.stream.Dispose();
                this.stream = null;
            }
        }

        #endregion
    }
}
