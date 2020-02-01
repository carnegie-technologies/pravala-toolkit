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

namespace Pravala.Control.Rt.Compat
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Windows.Networking;

    /// <summary>
    /// COMPATIBILITY SHIM control client that creates a control link with the remote control server.
    /// </summary>
    public class CtrlClient: CtrlConnectorImpl
    {
        /// <summary>
        /// Status receiver object.
        /// </summary>
        private ICtrlStatusReceiver statusReceiver;

        /// <summary>
        /// Initializes a new instance of the CtrlClient class.
        /// </summary>
        /// <param name="hostName">Hostname of the server to connect to.</param>
        /// <param name="port">Port to connect to on the server.</param>
        /// <param name="msgReceivers">Message receivers to dispatch responses from the server.</param>
        /// <param name="statusReceiver">Status receiver for events on this link.</param>
        /// <remarks>
        /// Compat shim does not support control subscriptions.
        /// </remarks>
        public CtrlClient (
                HostName hostName,
                string port,
                List<ICtrlMessageReceiver> msgReceivers,
                ICtrlStatusReceiver statusReceiver ):
            base ( hostName.ToString(), port, msgReceivers, null )
        {
            this.statusReceiver = statusReceiver;
        }

        /// <summary>
        /// Connect client to remote control server.
        /// </summary>
        /// <returns>Await-able task.</returns>
        public async Task ConnectClientAsync()
        {
            await this.ConnectAsync();
        }

        /// <summary>
        /// Start is not available in compat shim; use CtrlConnectorImpl.
        /// </summary>
        public override void Start()
        {
            throw new NotSupportedException ( "Start/Stop not available in compat shim; use CtrlConnectorImpl." );
        }

        /// <summary>
        /// Start is not available in compat shim; use CtrlConnectorImpl.
        /// </summary>
        public override void Stop()
        {
            throw new NotSupportedException ( "Start/Stop not available in compat shim; use CtrlConnectorImpl." );
        }

        /// <summary>
        /// Called when the link is closed.
        /// </summary>
        /// <remarks>
        /// Called both on natural closes, and error closes.
        /// </remarks>
        protected override void OnCtrlLinkClosed()
        {
            base.OnCtrlLinkClosed();

            if ( this.statusReceiver != null )
            {
                this.statusReceiver.CtrlLinkClosed ( this );
            }
        }

        /// <summary>
        /// Called when the link encounters a protocol error.
        /// </summary>
        /// <param name="e">Exception details.</param>
        protected override void OnCtrlLinkError ( Protocol.ProtoException e )
        {
            base.OnCtrlLinkError ( e );

            if ( this.statusReceiver != null )
            {
                this.statusReceiver.CtrlLinkError ( this, e );
            }
        }

        /// <summary>
        /// Called when the link encounters a generic error.
        /// </summary>
        /// <param name="e">Exception details</param>
        protected override void OnCtrlLinkError ( Exception e )
        {
            base.OnCtrlLinkError ( e );

            if ( this.statusReceiver != null )
            {
                this.statusReceiver.CtrlLinkError ( this, e );
            }
        }
    }
}
