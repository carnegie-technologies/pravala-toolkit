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

namespace Pravala.Control.Rt
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Pravala.Control.Subscriptions;
    using Windows.Networking;
    using Windows.Networking.Sockets;
    using Windows.Storage.Streams;

    /// <summary>
    /// A test control server.
    /// </summary>
    /// <remarks>Allows the application to talk to itself using the control protocol.</remarks>
    public class TestCtrlServer: CtrlLink
    {
        #region Fields
        /// <summary>
        /// Listener waiting for connections.
        /// </summary>
        private StreamSocketListener listener;

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
        /// Initializes a new instance of the TestCtrlServer class.
        /// </summary>
        /// <param name="msgReceivers">Message receivers to dispatch responses from the server.</param>
        /// <param name="ctrlSubscriptions">List of supported subscriptions on this link.</param>
        public TestCtrlServer ( List<ICtrlMessageReceiver> msgReceivers, List<CtrlSubscription> ctrlSubscriptions ):
            base ( msgReceivers, ctrlSubscriptions )
        {
        }

        #endregion

        #region Methods
        /// <summary>
        /// Starts the test server.
        /// </summary>
        /// <param name="hostName">Hostname to the remote host.</param>
        /// <param name="port">Remote port.</param>
        /// <returns>Task that can be awaited on.</returns>
        public async Task StartAsync ( HostName hostName, string port )
        {
            this.CleanupCtrlServer();

            this.listener = new StreamSocketListener();
            this.listener.ConnectionReceived += this.ListenerConnectionReceived;

            await this.listener.BindEndpointAsync ( hostName, port );
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
                return false;
            }

            this.dataWriter.WriteBytes ( buf );

            await this.dataWriter.StoreAsync();

            await this.dataWriter.FlushAsync();

            return true;
        }

        /// <summary>
        /// Stops and cleans up the link before disposal or re-use.
        /// </summary>
        private void CleanupCtrlServer()
        {
            if ( this.listener != null )
            {
                this.listener.Dispose();
                this.listener = null;
            }

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

        /// <summary>
        /// Receives notifications about incoming connections.
        /// </summary>
        /// <param name="sender">The StreamSocketListener that is responsible for this notification.</param>
        /// <param name="args">Parameters of the connection.</param>
        private async void ListenerConnectionReceived (
                StreamSocketListener sender,
                StreamSocketListenerConnectionReceivedEventArgs args )
        {
            this.socket = args.Socket;
            this.dataReader = new DataReader ( this.socket.InputStream );
            this.dataWriter = new DataWriter ( this.socket.OutputStream );

            await this.StartLinkAsync ( new CtrlReaderImpl ( this.dataReader ) );
        }

        #endregion
    }
}
