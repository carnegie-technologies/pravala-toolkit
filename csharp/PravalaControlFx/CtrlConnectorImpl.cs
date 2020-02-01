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
//// #define DEBUG2

namespace Pravala.Control.Fx
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Net;
    using System.Net.Sockets;
    using System.Threading.Tasks;
    using Pravala.Control.Subscriptions;

    /// <summary>
    /// CtrlLink for connecting to a control protocol server.
    /// </summary>
    public class CtrlConnectorImpl: CtrlConnector
    {
        #region Fields
        /// <summary>
        /// Socket used to communicate with the server.
        /// </summary>
        private Socket socket = null;
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
            IPEndPoint endpoint = new IPEndPoint (
                    Dns.GetHostAddresses ( this.HostName )
                    .Where ( ip => ip.AddressFamily == AddressFamily.InterNetwork ).First(),
                    int.Parse ( this.Port ) );

            this.socket = new Socket ( SocketType.Stream, ProtocolType.Tcp );
            this.socket.NoDelay = true;

            // Await-able task for waiting on connect completion
            var asyncConnectSource = new TaskCompletionSource<object>();

            // Callback when connect completes
            AsyncCallback connectCallback = result =>
            {
                // Retrieve the NewCtrlConnectorImpl instance from the state object
                CtrlConnectorImpl instance = ( CtrlConnectorImpl ) result.AsyncState;

                try
                {
                    // Complete the connection if the socket hasn't been closed
                    if ( instance.socket != null )
                    {
                        instance.socket.EndConnect ( result );
                    }
                }
                catch ( SocketException )
                {
                    asyncConnectSource.SetResult ( false );
                    return;
                }
                catch ( ObjectDisposedException )
                {
                    // If we're being stopped, the socket may already have
                    // been disposed before the callback runs
                }

                // Finish connect task
                asyncConnectSource.SetResult ( instance.socket != null && instance.socket.Connected );
            };

#if DEBUG
            Debug.WriteLine ( "Connecting to " + endpoint.ToString() );
#endif

            // Wait for connect
            this.socket.BeginConnect ( endpoint, connectCallback, this );
            bool success = ( bool ) await asyncConnectSource.Task;

            if ( !success )
            {
#if DEBUG
                Debug.WriteLine ( "Failed to connect to " + endpoint.ToString() );
#endif

                return false;
            }

#if DEBUG
            Debug.WriteLine ( "Connected to " + endpoint.ToString() );
#endif

            // Start link once connected
            await this.StartLinkAsync ( new CtrlReaderImpl ( this.socket ) );

            return true;
        }

        /// <summary>
        /// Disconnect from the protocol server
        /// </summary>
        /// <returns>Await-able task.</returns>
        protected override async Task DisconnectAsync()
        {
            if ( this.socket != null && this.socket.Connected )
            {
                // Await-able task for waiting on connect completion
                var asyncDisconnectSource = new TaskCompletionSource<object>();

                // Callback when connect completes
                AsyncCallback disconnectCallback = result =>
                {
                    // Retrieve the NewCtrlConnectorImpl instance from the state object
                    CtrlConnectorImpl instance = ( CtrlConnectorImpl ) result.AsyncState;

                    try
                    {
                        // Complete the connection if the socket hasn't been closed
                        if ( instance.socket != null )
                        {
                            instance.socket.EndDisconnect ( result );
                        }
                    }
                    catch ( ObjectDisposedException )
                    {
                        // If we're being stopped, the socket may already have
                        // been disposed before the callback runs
                    }

                    // Finish connect task
                    asyncDisconnectSource.SetResult ( null );
                };

#if DEBUG
                Debug.WriteLine ( "Disconnecting" );
#endif

                // Wait for disconnect
                this.socket.BeginDisconnect ( false, disconnectCallback, this );
                await asyncDisconnectSource.Task;

#if DEBUG
                Debug.WriteLine ( "Disconnected" );
#endif
            }

            this.CleanupConnectorImpl();
        }

        /// <summary>
        /// Sends provided bytes.
        /// </summary>
        /// <param name="buf">Serialized message bytes to send.</param>
        /// <returns>Whether the send was successful.</returns>
        protected override async Task<bool> SendBytesAsync ( byte[] buf )
        {
            if ( this.socket == null )
            {
                Debug.WriteLine ( "Cannot write to a disconnected link" );
                return false;
            }

            // Await-able task for waiting on send completion
            var asyncSendSource = new TaskCompletionSource<int>();

            // Callback when send completes
            AsyncCallback sendCallback = result =>
            {
                int bytesSent = 0;

                // Retrieve the NewCtrlConnectorImpl instance from the state object
                CtrlConnectorImpl instance = ( CtrlConnectorImpl ) result.AsyncState;

                try
                {
                    // Complete the send if the socket hasn't been closed
                    if ( instance.socket != null )
                    {
                        bytesSent = instance.socket.EndSend ( result );
                    }
                }
                catch ( ObjectDisposedException )
                {
                    // If we're being stopped, the socket may already have
                    // been disposed before the callback runs
                }

                asyncSendSource.SetResult ( bytesSent );
            };

#if DEBUG2
            Debug.WriteLine ( "Sending " + buf.Length + " bytes" );
#endif

            // Wait for send
            this.socket.BeginSend ( buf, 0, buf.Length, 0, sendCallback, this );
            await asyncSendSource.Task;

#if DEBUG2
            Debug.WriteLine ( "Sent " + buf.Length + " bytes" );
#endif

            return true;
        }

        /// <summary>
        /// Cleans up the connector socket.
        /// </summary>
        private void CleanupConnectorImpl()
        {
            if ( this.socket != null )
            {
                this.socket.Dispose();
                this.socket = null;
            }
        }

        #endregion
    }
}
