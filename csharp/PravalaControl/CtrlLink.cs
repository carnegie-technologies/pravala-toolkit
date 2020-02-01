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

namespace Pravala.Control
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Pravala.Control.Subscriptions;
    using Pravala.Protocol.Auto.Ctrl;

    /// <summary>
    /// CtrlLink reads/writes bytes from an underlying stream, and also manages message receivers/subscribers.
    /// </summary>
    public abstract class CtrlLink: INotifyPropertyChanged, IDisposable
    {
        #region Fields
        /// <summary>
        /// Cancellation source for reads.
        /// </summary>
        private CancellationTokenSource readCancellationSource;

        /// <summary>
        /// Message receivers for dispatching received messages.
        /// </summary>
        /// <remarks>Key is the message type, value is the list of receivers.</remarks>
        private Dictionary<uint, List<ICtrlMessageReceiver>> msgReceivers
            = new Dictionary<uint, List<ICtrlMessageReceiver>>();

        /// <summary>
        /// List of possible subscriptions for the link.
        /// </summary>
        /// <remarks>Key is the subscription type, value is the subscription data.</remarks>
        private Dictionary<Type, CtrlSubscription> ctrlSubscriptions
            = new Dictionary<Type, CtrlSubscription>();

        /// <summary>
        /// Indicates whether the control link is currently connected.
        /// </summary>
        private bool isConnected;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the CtrlLink class.
        /// </summary>
        /// <param name="msgReceivers">Message receivers to dispatch responses from the server.</param>
        /// <param name="ctrlSubscriptions">List of supported subscriptions on this link.</param>
        public CtrlLink ( List<ICtrlMessageReceiver> msgReceivers, List<CtrlSubscription> ctrlSubscriptions )
        {
            if ( msgReceivers != null )
            {
                foreach ( var receiver in msgReceivers )
                {
                    this.AddMessageReceiver ( receiver );
                }
            }

            if ( ctrlSubscriptions != null )
            {
                foreach ( var subscription in ctrlSubscriptions )
                {
                    this.AddCtrlSubscription ( subscription );
                }
            }
        }

        #endregion

        #region Events
        /// <summary>
        /// PropertyChanged event.
        /// </summary>
        public event PropertyChangedEventHandler PropertyChanged;
        #endregion

        #region Properties
        /// <summary>
        /// Gets a value indicating whether this link is currently connected.
        /// </summary>
        public bool IsConnected
        {
            get
            {
                return this.isConnected;
            }

            private set
            {
                this.isConnected = value;
                this.NotifyPropertyChanged ( "IsConnected" );
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Disposes the read cancellation source.
        /// </summary>
        public virtual void Dispose()
        {
            this.CleanReadCancellationSource();
        }

        /// <summary>
        /// Sends given message.
        /// </summary>
        /// <param name="msg">Message to be sent</param>
        /// <returns>
        /// <c>true</c>, if the message has been serialized and sent properly,
        /// <c>false</c>, if the socket is unavailable.
        /// </returns>
        /// <exception cref="System.ArgumentNullException">If the message passed is null.</exception>
        /// <exception cref="Protocol.ProtoException">If the message cannot be serialized properly.</exception>
        public virtual async Task<bool> SendMessageAsync ( Message msg )
        {
            if ( msg == null )
            {
                throw new System.ArgumentNullException ( "Message cannot be null" );
            }

            byte[] buf = msg.SerializeMessageWithLength();

#if DEBUG
            // We do this after 'serialize' so that the type is set!
            Debug.WriteLine ( "Writing message with type " + msg.getType() );
#endif

            return await this.SendBytesAsync ( buf );
        }

        /// <summary>
        /// Helper function to set the Subscribe flag and send the message over the link.
        /// </summary>
        /// <param name="message">The subscription request to send.</param>
        /// <returns>
        /// <c>true</c>, if the message has been serialized and sent properly,
        /// <c>false</c>, if the socket is unavailable.
        /// </returns>
        public async virtual Task<bool> SendSubscribeAsync ( SubscriptionRequest message )
        {
            message.setSubRequestType ( SubscriptionRequest.ReqType.Subscribe );
            return await this.SendMessageAsync ( message );
        }

        /// <summary>
        /// Helper function to set the Unsubscribe flag and send the message over the link.
        /// </summary>
        /// <param name="message">The subscription request to send.</param>
        /// <returns>
        /// <c>true</c>, if the message has been serialized and sent properly,
        /// <c>false</c>, if the socket is unavailable.
        /// </returns>
        public async virtual Task<bool> SendUnsubscribeAsync ( SubscriptionRequest message )
        {
            message.setSubRequestType ( SubscriptionRequest.ReqType.Unsubscribe );
            return await this.SendMessageAsync ( message );
        }

        /// <summary>
        /// Subscribe for updates using a particular subscriber class.
        /// </summary>
        /// <param name="t">The subscriber class type.</param>
        /// <returns>A task for async operations.</returns>
        public async Task SubscribeTo ( Type t )
        {
            if ( !this.ctrlSubscriptions.ContainsKey ( t ) )
            {
                return;
            }

            bool sendSubscription;

            // Lock when accessing ctrl subscriptions
            lock ( this.ctrlSubscriptions[ t ] )
            {
                sendSubscription = this.IsConnected
                                   && this.ctrlSubscriptions[ t ].ReferenceCount == 0
                                   && !this.ctrlSubscriptions[ t ].IsSubscribed;

                if ( sendSubscription )
                {
                    this.ctrlSubscriptions[ t ].IsSubscribed = true;
                }

                this.ctrlSubscriptions[ t ].ReferenceCount += 1;
            }

            // If we're connected, send immediately
            // (otherwise, this will happen on connect)
            if ( sendSubscription )
            {
                await this.ctrlSubscriptions[ t ].SubscribeAsync ( this );
            }
        }

        /// <summary>
        /// Unsubscribe from updates using a particular subscriber class.
        /// </summary>
        /// <param name="t">The subscriber class type.</param>
        /// <returns>A task for async operations.</returns>
        public async Task UnsubscribeFrom ( Type t )
        {
            if ( !this.ctrlSubscriptions.ContainsKey ( t ) )
            {
                return;
            }

            bool sendUnsubscribe;

            // Lock when accessing ctrl subscription
            lock ( this.ctrlSubscriptions[ t ] )
            {
                if ( this.ctrlSubscriptions[ t ].ReferenceCount > 0 )
                {
                    this.ctrlSubscriptions[ t ].ReferenceCount -= 1;
                }

                sendUnsubscribe = this.IsConnected
                                  && this.ctrlSubscriptions[ t ].ReferenceCount == 0
                                  && this.ctrlSubscriptions[ t ].IsSubscribed;

                if ( this.ctrlSubscriptions[ t ].ReferenceCount == 0 )
                {
                    this.ctrlSubscriptions[ t ].IsSubscribed = false;
                }
            }

            if ( sendUnsubscribe )
            {
                await this.ctrlSubscriptions[ t ].UnsubscribeAsync ( this );
            }
        }

        /// <summary>
        /// Sends provided bytes.
        /// </summary>
        /// <param name="buf">Serialized message bytes to send.</param>
        /// <returns>Whether the send was successful.</returns>
        protected abstract Task<bool> SendBytesAsync ( byte[] buf );

        /// <summary>
        /// Called when the link connects; sends active subscriptions.
        /// </summary>
        /// <remarks>
        /// Default implementation is a no-op.
        /// </remarks>
        protected virtual void OnCtrlLinkConnected()
        {
        }

        /// <summary>
        /// Called when the link is closed.
        /// </summary>
        /// <remarks>
        /// Called both on natural closes, and error closes.
        /// Default implementation is a no-op.
        /// </remarks>
        protected virtual void OnCtrlLinkClosed()
        {
        }

        /// <summary>
        /// Called when the link encounters a protocol error.
        /// </summary>
        /// <remarks>
        /// Default implementation is a no-op; can be used to recover or reconnect on error.
        /// </remarks>
        /// <param name="e">Exception details.</param>
        protected virtual void OnCtrlLinkError ( Protocol.ProtoException e )
        {
        }

        /// <summary>
        /// Called when the link encounters a generic error.
        /// </summary>
        /// <remarks>
        /// Default implementation is a no-op; can be used to recover or reconnect on error.
        /// </remarks>
        /// <param name="e">Exception details</param>
        protected virtual void OnCtrlLinkError ( Exception e )
        {
        }

        /// <summary>
        /// Starts the link control reader; should be called by link implementation after the link is connected.
        /// </summary>
        /// <param name="ctrlReader">CtrlReader instance used to read from the link.</param>
        /// <returns>A task that can be awaited on.</returns>
        protected async Task StartLinkAsync ( CtrlReader ctrlReader )
        {
            // Check to make sure the link has been cleaned up, if it was used previously
            if ( this.readCancellationSource != null )
            {
                throw new InvalidOperationException ( "Invalid state; CleanupLink() did not run before link re-use" );
            }

            this.readCancellationSource = new CancellationTokenSource();

            // ConfigureAwait so that it will not run on same thread
            await this.CreateReadingTask ( ctrlReader, this.readCancellationSource.Token ).ConfigureAwait ( false );

            this.IsConnected = true;
            this.OnCtrlLinkConnected();
            await this.SendActiveSubscriptions();
        }

        /// <summary>
        /// Called whenever a control message is received.
        /// </summary>
        /// <param name="msg">Received control message (as a base message).</param>
        protected void CtrlMessageReceived ( Message msg )
        {
            if ( msg == null )
            {
                Debug.WriteLine ( "Not dispatching a null message" );
                return;
            }

            if ( !msg.hasType() )
            {
                Debug.WriteLine ( "Missing message type in the message for dispatching; Ignoring" );
                return;
            }

            uint msgType = msg.getType();

#if DEBUG
            Debug.WriteLine ( "Received message with type: " + msg.getType() );
#endif

            // Simple subscription responses may have a body containing multiple update messages.
            // Parse them and deliver independently if there is no handler for the complete
            // simple subscription response registered.
            if ( msgType == SimpleSubscriptionResponse.defType )
            {
                if ( !this.msgReceivers.ContainsKey ( SimpleSubscriptionResponse.defType ) )
                {
#if DEBUG
                    Debug.WriteLine ( "No SimpleSubscriptionResponse receiver, splitting updates" );
#endif
                    var splitMsg = new SimpleSubscriptionResponse();
                    splitMsg.DeserializeFromBaseMessage ( msg );

                    foreach ( var update in splitMsg.getUpdates() )
                    {
#if DEBUG
                        Debug.WriteLine ( "Dispatching update with type: " + update.getType() );
#endif
                        this.CtrlMessageReceived ( update );
                    }

                    return;
                }
            }

            if ( !this.msgReceivers.ContainsKey ( msgType ) )
            {
                Debug.WriteLine ( "There are no receivers subscribed to message type " + msgType + "; Ignoring" );
                return;
            }

            foreach ( ICtrlMessageReceiver rcvr in this.msgReceivers[ msgType ] )
            {
                if ( rcvr == null )
                {
                    continue;
                }

                try
                {
                    rcvr.ReceiveBaseMessage ( this, msg );
                }
                catch ( Protocol.ProtoException e )
                {
                    Debug.WriteLine ( "Protocol exception while delivering message with type "
                            + msgType + ":" + e );
                }
                catch ( Exception e )
                {
                    Debug.WriteLine ( "Unknown exception while delivering message with type " + msgType
                            + ":" + e );
                }
            }
        }

        /// <summary>
        /// Raise property changed event.
        /// </summary>
        /// <param name="propertyName">Name of the property which has changed.</param>
        protected virtual void NotifyPropertyChanged ( string propertyName )
        {
            if ( this.PropertyChanged != null )
            {
                this.PropertyChanged ( this, new PropertyChangedEventArgs ( propertyName ) );
            }
        }

        /// <summary>
        /// Adds given message receiver.
        /// </summary>
        /// <remarks>
        /// Multiple message receivers per message type are supported.
        /// Collection does not need to be locked as message receivers are created only in the constructor.
        /// </remarks>
        /// <param name="receiver">The receiver to add.</param>
        private void AddMessageReceiver ( ICtrlMessageReceiver receiver )
        {
            if ( receiver == null )
            {
                Debug.WriteLine ( "Received null message receiver; Ignoring" );
                return;
            }

            uint msgType = receiver.GetMessageType();

            if ( msgType == 0 )
            {
                Debug.WriteLine ( "Received subscription request with message type 0; Ignoring" );
                return;
            }

            if ( !this.msgReceivers.ContainsKey ( msgType ) )
            {
                this.msgReceivers.Add ( msgType, new List<ICtrlMessageReceiver>() );
            }

            if ( this.msgReceivers[ msgType ].Contains ( receiver ) )
            {
                return;
            }

            this.msgReceivers[ msgType ].Add ( receiver );
        }

        /// <summary>
        /// Adds the given ctrl subscription.
        /// </summary>
        /// <remarks>
        /// If multiple ctrl subscriptions of the same type are passed, only the first is added.
        /// Collection does not need to be locked as the subscriptions are created only in the constructor.
        /// </remarks>
        /// <param name="subscription">The subscription to add.</param>
        private void AddCtrlSubscription ( CtrlSubscription subscription )
        {
            if ( subscription == null )
            {
                Debug.WriteLine ( "Received null subscription; Ignoring" );
                return;
            }

            Type subType = subscription.GetType();

            if ( !this.ctrlSubscriptions.ContainsKey ( subType ) )
            {
                this.ctrlSubscriptions.Add ( subType, subscription );
            }
        }

        /// <summary>
        /// Resets link state after it is closed and notifies listeners of close.
        /// </summary>
        private void LinkClosed()
        {
            this.IsConnected = false;
            this.CleanupLink();
            this.OnCtrlLinkClosed();
        }

        /// <summary>
        /// Cleans up the read cancellation source.
        /// </summary>
        private void CleanReadCancellationSource()
        {
            if ( this.readCancellationSource != null )
            {
                this.readCancellationSource.Cancel();
                this.readCancellationSource.Dispose();
                this.readCancellationSource = null;
            }
        }

        /// <summary>
        /// Clean up the link before disposal or re-use.
        /// </summary>
        private void CleanupLink()
        {
            this.CleanReadCancellationSource();
            this.CleanActiveSubscriptions();
            this.DisconnectMessageReceivers();
        }

        /// <summary>
        /// Registers all active subscribers on connect.
        /// </summary>
        /// <returns>A task for async operations.</returns>
        private async Task SendActiveSubscriptions()
        {
            foreach ( var subscription in this.ctrlSubscriptions.ToList() )
            {
                bool sendSubscription;

                // Lock before accessing ctrl subscription
                lock ( subscription.Value )
                {
                    sendSubscription = subscription.Value.ReferenceCount > 0 && !subscription.Value.IsSubscribed;

                    if ( sendSubscription )
                    {
                        subscription.Value.IsSubscribed = true;
                    }
                }

                if ( sendSubscription )
                {
                    await subscription.Value.SubscribeAsync ( this );
                }
            }
        }

        /// <summary>
        /// Mark all active subscriptions as unsubscribed
        /// </summary>
        private void CleanActiveSubscriptions()
        {
            foreach ( var subscription in this.ctrlSubscriptions.ToList() )
            {
                // Lock accessing ctrl subscription
                lock ( subscription.Value )
                {
                    subscription.Value.IsSubscribed = false;
                }
            }
        }

        /// <summary>
        /// Notify message receivers that the link is connected.
        /// </summary>
        private void ConnectMessageReceivers()
        {
            foreach ( var messageReceivers in this.msgReceivers.ToList() )
            {
                foreach ( var messageReceiver in messageReceivers.Value )
                {
                    messageReceiver.OnConnect();
                }
            }
        }

        /// <summary>
        /// Notify message receivers that the link is disconnected.
        /// </summary>
        private void DisconnectMessageReceivers()
        {
            foreach ( var messageReceivers in this.msgReceivers.ToList() )
            {
                foreach ( var messageReceiver in messageReceivers.Value )
                {
                    messageReceiver.OnDisconnect();
                }
            }
        }

        /// <summary>
        /// Creates a reading task.
        /// </summary>
        /// <param name="ctrlReader">CtrlReader instance used to read from the link.</param>
        /// <param name="cts">Cancellation token for the read class.</param>
        /// <returns>Task that can be awaited on.</returns>
        private Task CreateReadingTask ( CtrlReader ctrlReader, CancellationToken cts )
        {
            return Task.Factory.StartNew (
                    async() =>
            {
                try
                {
                    Message msg = null;

                    // We need to read at least one byte:
                    uint readBytes = 1;

                    while ( !cts.IsCancellationRequested )
                    {
                        if ( msg == null )
                        {
                            msg = new Message();
                        }

                        uint byteCount = await ctrlReader.LoadAsync ( readBytes );

                        // Unless we figure out below how many bytes we need, let's set it to the default,
                        // safe value of one byte. It may have been changed to something bigger
                        // if we were reading the rest of a larger message.
                        readBytes = 1;

#if DEBUG2
                        Debug.WriteLine ( "Read " + byteCount + " bytes" );
#endif

                        if ( byteCount == 0 )
                        {
                            Debug.WriteLine ( "Control link closed in reader." );
                            this.LinkClosed();
                            return;
                        }

                        int reqBytes = 0;

                        try
                        {
                            reqBytes = msg.DeserializeBaseMessage ( ctrlReader );
                        }
                        catch ( Protocol.ProtoException e )
                        {
                            Debug.WriteLine ( "Protocol exception while deserializing base message: " + e );
                            this.OnCtrlLinkError ( e );
                            this.LinkClosed();
                            return;
                        }

                        if ( reqBytes < 1 )
                        {
#if DEBUG2
                            Debug.WriteLine ( "Received complete message - delivering" );
#endif

                            CtrlMessageReceived ( msg );
                            msg = null;
                        }
                        else
                        {
#if DEBUG2
                            Debug.WriteLine ( "DeserializeBase needs " + reqBytes + " bytes more." );
#endif
                            // Now we know exactly how many bytes we need.
                            // We may need more than that, but this is the "at least this many bytes" value,
                            // so it is safe to read this much. If it is not enough,
                            // we will have a better idea the next time we read something.
                            readBytes = ( uint ) reqBytes;
                        }
                    }
                }
                catch ( Exception e )
                {
                    Debug.WriteLine ( "Unknown exception inside read loop: " + e );
                    this.OnCtrlLinkError ( e );
                    this.LinkClosed();
                    return;
                }
            },
                    cts );
        }

        #endregion
    }
}
