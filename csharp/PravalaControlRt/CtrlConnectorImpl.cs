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

#define DEBUG
//// #define DEBUG

namespace Pravala.Control.Rt
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using Pravala.Control.Subscriptions;
    using Windows.Networking;
    using Windows.Networking.Sockets;
    using Windows.Storage.Streams;

    /// <summary>
    /// CtrlLink for connecting to a control protocol server.
    /// </summary>
    public class CtrlConnectorImpl: CtrlConnector
    {
        #region Fields
        /// <summary>
        /// Socket used for reading and writing.
        /// </summary>
        private StreamSocket socket;

        /// <summary>
        /// DataWriter to be used for writing.
        /// </summary>
        private DataWriter dataWriter;

        /// <summary>
        /// DataReader to be used for reading.
        /// </summary>
        private DataReader dataReader;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the CtrlConnectorImpl class.
        /// </summary>
        /// <param name="hostName">Hostname of the server to connect to.</param>
        /// <param name="port">Port to connect to on the server.</param>
        /// <param name="msgReceivers">Message receivers to dispatch responses from the server.</param>
        /// <param name="ctrlSubscriptions">List of supported subscriptions on this link.</param>
        public CtrlConnectorImpl (
                string hostName,
                string port,
                List<ICtrlMessageReceiver> msgReceivers,
                List<CtrlSubscription> ctrlSubscriptions ):
            base ( hostName, port, msgReceivers, ctrlSubscriptions )
        {
        }

        #endregion

        #region Methods
        /// <summary>
        /// Disposes the CtrlConnectorImpl.
        /// </summary>
        public override void Dispose()
        {
            base.Dispose();
            this.CleanupConnectorImpl();
        }

        /// <summary>
        /// Connect to the protocol server.
        /// </summary>
        /// <returns>A value indicating whether the connect was successful.</returns>
        protected override async Task<bool> ConnectAsync()
        {
            this.socket = new StreamSocket();
            this.socket.Control.NoDelay = true;

#if DEBUG
            Debug.WriteLine ( "Connecting to " + this.HostName + ":" + this.Port );
#endif

            try
            {
                await this.socket.ConnectAsync ( new HostName ( this.HostName ), this.Port );
            }
            catch ( Exception )
            {
                // Connect failed; ConnectAsync does not throw a more specific exception
                Debug.WriteLine ( "Failed to connect to " + this.HostName + ":" + this.Port );
                return false;
            }

#if DEBUG
            Debug.WriteLine ( "Connected to " + this.HostName + ":" + this.Port );
#endif

            //// This is left here for reference. Previously we were reading up to 1024 bytes
            //// and had this option set. This was causing messages to be delayed a lot.
            //// So now we are reading in smaller chunks and without this option.
            //// dataReader.InputStreamOptions = InputStreamOptions.Partial;

            this.dataReader = new DataReader ( this.socket.InputStream );
            this.dataWriter = new DataWriter ( this.socket.OutputStream );

            await this.StartLinkAsync ( new CtrlReaderImpl ( this.dataReader ) );

            return true;
        }

        /// <summary>
        /// Disconnect from the protocol server
        /// </summary>
        /// <returns>Await-able task.</returns>
        protected override async Task DisconnectAsync()
        {
            // The only way to disconnect a StreamSocket is to dispose it.
            this.CleanupConnectorImpl();
            await Task.FromResult<object> ( null );
        }

        /// <summary>
        /// Sends provided bytes.
        /// </summary>
        /// <param name="buf">Serialized message bytes to send.</param>
        /// <returns>Whether the send was successful.</returns>
        protected override async Task<bool> SendBytesAsync ( byte[] buf )
        {
            if ( this.socket == null || this.dataWriter == null )
            {
                Debug.WriteLine ( "Cannot write to a disconnected link" );
                return false;
            }

#if DEBUG2
            Debug.WriteLine ( "Sending " + buf.Length + " bytes" );
#endif

            this.dataWriter.WriteBytes ( buf );

            await this.dataWriter.StoreAsync();

            await this.dataWriter.FlushAsync();

#if DEBUG2
            Debug.WriteLine ( "Sent " + buf.Length + " bytes" );
#endif

            return true;
        }

        /// <summary>
        /// Cleans up the connector socket/reader/writer.
        /// </summary>
        private void CleanupConnectorImpl()
        {
            if ( this.socket != null )
            {
                this.socket.Dispose();
                this.socket = null;
            }

            if ( this.dataWriter != null )
            {
                this.dataWriter.Dispose();
                this.dataWriter = null;
            }

            if ( this.dataReader != null )
            {
                this.dataReader.Dispose();
                this.dataReader = null;
            }
        }

        #endregion
    }
}
