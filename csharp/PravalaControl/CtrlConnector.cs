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

namespace Pravala.Control
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading;
    using System.Threading.Tasks;
    using Pravala.Control.Subscriptions;

    /// <summary>
    /// CtrlLink for connecting to a control protocol server.
    /// </summary>
    public abstract class CtrlConnector: CtrlLink
    {
        #region Fields
        /// <summary>
        /// Interval (in ms) to check if a reconnect is necessary.
        /// </summary>
        private const int ReconnectCheckInterval = 5 * 1000;

        /// <summary>
        /// The server hostname.
        /// </summary>
        private readonly string hostName;

        /// <summary>
        /// The server port.
        /// </summary>
        private readonly string port;

        /// <summary>
        /// Indicates whether we will try to connect to the control protocol server.
        /// </summary>
        private bool isRunning = false;

        /// <summary>
        /// Cancellation token source for the reconnect loop.
        /// </summary>
        private CancellationTokenSource reconnectCancelSource;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the CtrlConnector class.
        /// </summary>
        /// <param name="hostName">Hostname of the server to connect to.</param>
        /// <param name="port">Port to connect to on the server.</param>
        /// <param name="msgReceivers">Message receivers to dispatch responses from the server.</param>
        /// <param name="ctrlSubscriptions">List of supported subscriptions on this link.</param>
        public CtrlConnector (
                string hostName,
                string port,
                List<ICtrlMessageReceiver> msgReceivers,
                List<CtrlSubscription> ctrlSubscriptions ):
            base ( msgReceivers, ctrlSubscriptions )
        {
            this.hostName = hostName;
            this.port = port;
        }

        #endregion

        #region Properties
        /// <summary>
        /// Gets the server hostname.
        /// </summary>
        public string HostName
        {
            get
            {
                return this.hostName;
            }
        }

        /// <summary>
        /// Gets the server port.
        /// </summary>
        public string Port
        {
            get
            {
                return this.port;
            }
        }

        /// <summary>
        /// Gets a value indicating whether we will try to connect/reconnect to the control protocol server.
        /// </summary>
        public bool IsRunning
        {
            get
            {
                return this.isRunning;
            }

            private set
            {
                this.isRunning = value;
                this.NotifyPropertyChanged ( "IsRunning" );
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Starts the connector.
        /// </summary>
        public async virtual void Start()
        {
            lock ( this )
            {
                if ( this.isRunning )
                {
                    return;
                }

                this.IsRunning = true;
                this.reconnectCancelSource = new CancellationTokenSource();
            }

            // Check connection status every 5 seconds
            // while manager started
            while ( this.isRunning )
            {
#if DEBUG
                Debug.WriteLine ( "Check connection state: " + ( this.IsConnected ? "connected" : "not connected" ) );
#endif

                if ( !this.IsConnected )
                {
                    await this.ConnectAsync();
                }

                try
                {
                    await TaskEx.Delay ( ReconnectCheckInterval, this.reconnectCancelSource.Token );
                }
                catch ( OperationCanceledException )
                {
                    // Reconnect loop has been cancelled, so return
                    return;
                }
            }
        }

        /// <summary>
        /// Stops the connector.
        /// </summary>
        public async virtual void Stop()
        {
            lock ( this )
            {
                this.IsRunning = false;

                if ( this.reconnectCancelSource != null )
                {
                    this.reconnectCancelSource.Cancel();
                }
            }

            await this.DisconnectAsync();
        }

        /// <summary>
        /// Dispose the connector.
        /// </summary>
        public override void Dispose()
        {
            base.Dispose();
            this.CleanupConnector();
        }

        /// <summary>
        /// Connect to the protocol server.
        /// </summary>
        /// <returns>A value indicating whether the connect was successful.</returns>
        protected abstract Task<bool> ConnectAsync();

        /// <summary>
        /// Disconnect from the protocol server
        /// </summary>
        /// <returns>Await-able task.</returns>
        protected abstract Task DisconnectAsync();

        /// <summary>
        /// Stops and cleans up connector resources so that a new connection can be established with this instance.
        /// </summary>
        private void CleanupConnector()
        {
            this.Stop();

            if ( this.reconnectCancelSource != null )
            {
                this.reconnectCancelSource.Cancel();
                this.reconnectCancelSource.Dispose();
                this.reconnectCancelSource = null;
            }
        }

        #endregion
    }
}
