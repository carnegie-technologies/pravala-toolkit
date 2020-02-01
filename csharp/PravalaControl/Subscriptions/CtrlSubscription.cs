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

namespace Pravala.Control.Subscriptions
{
    using System.Threading.Tasks;

    /// <summary>
    /// Wrapper class for reference counting subscriptions.
    /// </summary>
    /// <remarks>
    /// This can be inherited by things that want to do something (e.g. send subscription messages or otherwise) when
    /// the link connects as well as be automatically re-called should the link disconnect and then reconnect.
    /// </remarks>
    public abstract class CtrlSubscription
    {
        /// <summary>
        /// Gets or sets the reference count.
        /// </summary>
        public int ReferenceCount
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a value indicating whether this subscription has been subscribed to the service.
        /// </summary>
        /// <remarks>
        /// This should be set by anything sending subscribe/unsubscribe messages to prevent double sends.
        /// </remarks>
        public bool IsSubscribed
        {
            get;
            set;
        }

        /// <summary>
        /// Subscribes for updates.
        /// </summary>
        /// <param name="ctrlLink">CtrlLink to send the subscribe on.</param>
        /// <returns>Task for async use.</returns>
        public abstract Task SubscribeAsync ( CtrlLink ctrlLink );

        /// <summary>
        /// Unsubscribes from updates.
        /// </summary>
        /// <param name="ctrlLink">CtrlLink to send the unsubscribe on.</param>
        /// <returns>Task for async use.</returns>
        public abstract Task UnsubscribeAsync ( CtrlLink ctrlLink );
    }
}
