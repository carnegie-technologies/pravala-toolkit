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

namespace Pravala.RemoteLog.Service
{
    using System.Collections.Generic;
    using Pravala.Control;
    using Pravala.Control.Fx;
    using Pravala.Control.Subscriptions;

    /// <summary>
    /// Remote link to protocol server.
    /// </summary>
    public class RemoteLink: CtrlConnectorImpl
    {
        #region Fields
        /// <summary>
        /// The service hostname.
        /// </summary>
        private const string ServiceHost = "localhost";

        /// <summary>
        /// The port the service is running on.
        /// </summary>
        private const string ServicePort = "5665";

        /// <summary>
        /// Message receivers supported by this link.
        /// </summary>
        private static readonly List<ICtrlMessageReceiver> MsgReceivers = new List<ICtrlMessageReceiver>()
        {
            new LogMessageReceiver()
        };

        /// <summary>
        /// Ctrl subscriptions supported by this link.
        /// </summary>
        private static readonly List<CtrlSubscription> CtrlSubscriptions = new List<CtrlSubscription>()
        {
            new LogSubscription()
        };
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the RemoteLink class.
        /// </summary>
        public RemoteLink():
            base ( ServiceHost, ServicePort, MsgReceivers, CtrlSubscriptions )
        {
        }

        #endregion

        #region Methods
        /// <summary>
        /// Update the log pattern to subscribe to.
        /// </summary>
        /// <remarks>
        /// The updated log pattern only applies after a (re)connect.
        /// </remarks>
        /// <param name="pattern">The log pattern to subscribe to.</param>
        public void UpdateLogPattern ( string pattern )
        {
            ( ( LogSubscription ) CtrlSubscriptions[ 0 ] ).UpdateLogPattern ( pattern );
        }

        /// <summary>
        /// Raise property changed event.
        /// </summary>
        /// <param name="propertyName">Name of the property which has changed.</param>
        protected override void NotifyPropertyChanged ( string propertyName )
        {
            // Call base notify on UI thread
            ServiceProvider.ViewModel.PostToUiThread (
                    o =>
            {
                base.NotifyPropertyChanged ( propertyName );
            },
                    this );
        }

        #endregion
    }
}
