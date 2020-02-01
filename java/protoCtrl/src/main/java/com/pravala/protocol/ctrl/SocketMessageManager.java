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

package com.pravala.protocol.ctrl;

import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;

import com.pravala.protocol.CodecException;
import com.pravala.protocol.SerializableBase;
import com.pravala.protocol.auto.ctrl.Message;
import com.pravala.protocol.auto.ctrl.SimpleSubscriptionResponse;
import com.pravala.socket.SocketUtils;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.HashMap;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Used to manage the sending and receiving of messages created using
 * the protocol generator, protoGen, through an IPC (local) socket...
 *
 * This can either connect to remote control server, or accept incoming control connections.
 */
public abstract class SocketMessageManager
{
    /**
     * Possible modes when using this class to communicate over a local socket
     */
    public enum LocalServerMode
    {
        /**
         * Connect to a remote control server socket in the abstract namespace
         */
        AbstractNamespaceClient,

        /**
         * Accept incoming control connections in the abstract namespace
         */
        AbstractNamespaceServer,

        /**
         * Connect to a remote control server socket on a socket file path
         */
        FilesystemPathClient,

        /**
         * Accept incoming control connections on a socket file path
         */
        FilesystemPathServer,
    }

    /**
     * Class name, for logging.
     */
    private static final String TAG = SocketMessageManager.class.getName();

    /**
     * Currently used link to the remote control endpoint.
     */
    private volatile Link ctrlLink = null;

    /**
     * Message handlers
     * TODO Right now we allow only a single handler to be set for each message type;
     * we may want to change it at some point!
     */
    private final HashMap<Integer, ProtocolMessageHandler> messageHandlers
        = new HashMap<Integer, ProtocolMessageHandler>();

    /**
     * Contains the state of a link to remote control endpoint.
     */
    private class Link
    {
        /**
         * Stores the server mode when a local socket is used.
         *
         * Null if we're not using a local socket.
         */
        private final LocalServerMode localServerMode;

        /**
         * The TCP socket where all messages will be read from or written to.
         *
         * When communicating via TCP socket, this will be the only socket that is non-null.
         */
        private final Socket tcpSocket;

        /**
         * The queue of messages to be written out to the socket.
         */
        private final LinkedBlockingQueue<MessageQueueItem> outboundMessageQueue
            = new LinkedBlockingQueue<MessageQueueItem>();

        /**
         * The thread dedicated to asynchronously sending messages out to the socket.
         */
        private final OutboundMessageThread outboundMessageThread;

        /**
         * The thread dedicated to processing incoming messages from the socket.  It also does initialisation...
         */
        private final InboundMessageThread inboundMessageThread;

        /**
         * The thread dedicated to timing-out local connect/accept attempt.
         */
        private final LocalConnectionTimeoutThread localConnectionTimeoutThread;

        /**
         * The local socket where all messages will be read from or written to.
         *
         * When communicating via local socket in client mode, this will be the only socket that is non-null.
         *
         * When communicating via local socket in server mode, this socket will be accepted from localServerSocket;
         * so localServerSocket and this socket will be the only sockets that are non-null.
         */
        private volatile LocalSocket localSocket = null;

        /**
         * Indicates if we're connecting or accepting a local connection.
         * If we are connecting then calls to LocalSocket.isConnected() will block;
         * instead of blindly calling isConnected() we need to check this first.
         * This is also used by the timeout thread to determine whether we are still connecting.
         */
        private volatile boolean isLocalConnecting = false;

        /**
         * Constructs a Link that uses a local socket
         *
         * @param socketPath The name or path to listen at or connect to.
         * @param localServerMode The mode of the local link.
         * @note Constructing a link in server mode requires that the SocketUtils native
         *       library is loaded; Constructing a link in client mode does not.
         * @param timeout Connection timeout value in milliseconds.
         *                In server mode it will time-out if there are no accepted connections during specified time.
         * @throws IOException if the local socket cannot be created
         * @throws NoSuchFieldException if the FileDescriptor.descriptor field cannot be found
         * @throws IllegalAccessException if the FileDescriptor.descriptor field cannot be assigned
         */
        public Link ( String socketPath, LocalServerMode localServerMode, int timeout )
        throws IOException, NoSuchFieldException, IllegalAccessException
        {
            this.localServerMode = localServerMode;
            this.isLocalConnecting = true;

            this.tcpSocket = null;

            final boolean isAbstract
                = ( localServerMode == LocalServerMode.AbstractNamespaceClient
                    || localServerMode == LocalServerMode.AbstractNamespaceServer );

            final LocalSocketAddress localSockAddr
                = new LocalSocketAddress (
                socketPath,
                isAbstract ? LocalSocketAddress.Namespace.ABSTRACT : LocalSocketAddress.Namespace.FILESYSTEM );

            if ( localServerMode == LocalServerMode.AbstractNamespaceClient
                 || localServerMode == LocalServerMode.FilesystemPathClient )
            {
                // Local client mode.
                // We could create a localSocket here, just like tcpSocket in the other constructor,
                // but localSocket is not final, and we prefer the InboundMessageThread to manage it for us
                // (since it already does that in local server mode anyway).

                this.inboundMessageThread
                    = new InboundMessageThread ( localSockAddr, null );
            }
            else
            {
                // Local server mode

                this.inboundMessageThread = new InboundMessageThread (
                    localSockAddr,
                    SocketUtils.createLocalServerSocket (
                        localSockAddr.getName(),
                        ( localSockAddr.getNamespace() == LocalSocketAddress.Namespace.ABSTRACT ) ) );
            }

            this.outboundMessageThread = new OutboundMessageThread();

            this.inboundMessageThread.start();

            if ( timeout > 0 )
            {
                this.localConnectionTimeoutThread = new LocalConnectionTimeoutThread ( timeout );
                this.localConnectionTimeoutThread.start();
            }
            else
            {
                this.localConnectionTimeoutThread = null;
            }
        }

        /**
         * Constructs a Link that uses a TCP socket
         *
         * @param addr IP address and port to connect to
         * @param timeout Connection timeout value in milliseconds.
         */
        public Link ( InetSocketAddress addr, int timeout )
        {
            this.localServerMode = null;
            this.localConnectionTimeoutThread = null;

            this.tcpSocket = new Socket();

            this.inboundMessageThread = new InboundMessageThread ( addr, timeout );
            this.outboundMessageThread = new OutboundMessageThread();

            this.inboundMessageThread.start();
        }

        /**
         * Stops the link.
         */
        public void stop()
        {
            if ( localConnectionTimeoutThread != null )
            {
                localConnectionTimeoutThread.interrupt();
            }

            if ( inboundMessageThread != null )
            {
                inboundMessageThread.shutdown();
            }

            if ( outboundMessageThread != null )
            {
                outboundMessageThread.shutdown();
            }

            if ( localSocket != null )
            {
                try
                {
                    // On Android just closing a socket doesn't cause the remote side to receive the event immediately.
                    // Instead it could take several seconds or minutes before the remote side gets the close event.
                    // However if we call shutdown on both input and output, then call close, the remote side will
                    // get the close immediately.

                    localSocket.shutdownInput();
                    localSocket.shutdownOutput();

                    localSocket.close();
                }
                catch ( IOException e )
                {
                }
            }

            if ( tcpSocket != null )
            {
                try
                {
                    // On Android just closing a socket doesn't cause the remote side to receive the event immediately.
                    // Instead it could take several seconds or minutes before the remote side gets the close event.
                    // However if we call shutdown on both input and output, then call close, the remote side will
                    // get the close immediately.

                    tcpSocket.shutdownInput();
                    tcpSocket.shutdownOutput();

                    tcpSocket.close();
                }
                catch ( IOException e )
                {
                }
            }

            outboundMessageQueue.clear();
        }

        /**
         * Queries whether the link is currently running and connected
         *
         * @return True if the link is running properly and it is connected, false otherwise.
         */
        public boolean isConnected()
        {
            if ( inboundMessageThread == null
                 || !inboundMessageThread.isAlive()
                 || outboundMessageThread == null
                 || !outboundMessageThread.isAlive() )
            {
                // We are not connected if we are still connecting,
                // or if one of the threads is dead.
                return false;
            }

            if ( tcpSocket != null )
            {
                // We are operating in TCP mode - depends if TCP socket is connected.
                return tcpSocket.isConnected();
            }

            // Otherwise we are in local mode.

            // To be connected, the local socket must be set.
            // In the server mode this is sufficient, because it is only set when
            // we accept a connection. However, sockets accepted this way always
            // return 'false' when calling isConnected().
            // In the client mode we actually check isConnected(), but only if isLocalConnecting is not set
            // (otherwise isConnected() on local socket blocks).

            // So to be connected we need to have a local socket, and either be a server,
            // or that socket should be connected:
            return ( localSocket != null
                     && ( localServerMode == LocalServerMode.AbstractNamespaceServer
                          || localServerMode == LocalServerMode.FilesystemPathServer
                          || ( !isLocalConnecting && localSocket.isConnected() ) ) );
        }

        /**
         * Sends a message through the socket with an ancillary file descriptor.
         *
         * @param message The message to send through.
         * @param fd The file descriptor to send through.
         */
        public void send ( SerializableBase message, FileDescriptor fd )
        {
            MessageQueueItem item = new MessageQueueItem ( message, fd );
            outboundMessageQueue.add ( item );
        }

        /**
         * Encapsulates additional information about messages while in the queue.
         */
        private class MessageQueueItem
        {
            /**
             * The protocol message to send over the socket.
             */
            public final SerializableBase message;

            /**
             * An optional file descriptor to send over the socket.
             */
            public final FileDescriptor fd;

            /**
             * Simple constructor to ease the creation of this object.
             */
            public MessageQueueItem ( SerializableBase msg, FileDescriptor f )
            {
                message = msg;
                fd = f;
            }
        }

        /**
         * A thread for processing messages going out through the socket.
         * <p/>
         * This thread is only started after the socket is connected to the host...
         */
        private class OutboundMessageThread extends Thread
        {
            /*
             * Indicates whether the thread is in the process of shutting down.
             * Used to solve a race condition that can potentially send a false socket error...
             */
            private volatile boolean shutdown = false;

            /**
             * Constructor
             */
            OutboundMessageThread()
            {
                super ( "SMM_Outbound" );
            }

            /**
             * Process outgoing messages...
             */
            @Override
            public void run()
            {
                if ( localSocket == null && tcpSocket == null )
                {
                    return;
                }

                try
                {
                    while ( !shutdown )
                    {
                        MessageQueueItem item = outboundMessageQueue.take();

                        try
                        {
                            if ( tcpSocket != null )
                            {
                                if ( item.fd != null )
                                {
                                    Log.e ( TAG, "No local socket, not sending message with an FD" );
                                }

                                item.message.serializeWithLength ( tcpSocket.getOutputStream() );
                            }
                            else if ( item.fd == null )
                            {
                                // No FDs, so we can send this message in parts,
                                // the other side will reassemble it.

                                item.message.serializeWithLength ( localSocket.getOutputStream() );
                            }
                            else
                            {
                                FileDescriptor[] fds = new FileDescriptor[] {
                                    item.fd
                                };
                                localSocket.setFileDescriptorsForSend ( fds );

                                Log.d ( TAG, "Sending a message with an FD" );

                                // We need to serialise this message to a buffer first.
                                // Because we include some FDs in it, we need to write the whole
                                // thing using a single write!

                                ByteArrayOutputStream buf = new ByteArrayOutputStream();
                                item.message.serializeWithLength ( buf );

                                OutputStream outStream = localSocket.getOutputStream();

                                buf.writeTo ( outStream );

                                outStream.flush();

                                // We need to clear the FDs.
                                // Otherwise they would be sent with every future message!
                                localSocket.setFileDescriptorsForSend ( null );
                            }

                            Log.v ( TAG, "Message sent: " + item.message.getClass().getSimpleName() );
                        }
                        catch ( CodecException e )
                        {
                            Log.e ( TAG, "CodecException while sending '"
                                       + item.message.getClass().getSimpleName() + "' message: " + e );
                        }
                    }
                }
                catch ( InterruptedException e )
                {
                    // Safely do nothing to terminate the thread...
                }
                catch ( IOException e )
                {
                    if ( !shutdown )
                    {
                        Log.d ( TAG, "Error sending message!" );

                        onSocketError ( Link.this );
                    }
                }
            }

            /**
             * Gracefully stops the thread from running.
             */
            public void shutdown()
            {
                shutdown = true;

                interrupt();
            }
        }

        /**
         * A thread for timing-out local socket connection attempt.
         */
        private class LocalConnectionTimeoutThread extends Thread
        {
            /**
             * Connection timeout value in milliseconds.
             */
            private final int timeout;

            /**
             * Creates the thread that will process incoming messages
             *
             * @param timeout Connection timeout value in milliseconds.
             */
            LocalConnectionTimeoutThread ( int timeout )
            {
                super ( "SMM_LocalConTimeout" );

                this.timeout = timeout;
            }

            /**
             * Timeouts local connection attempt.
             */
            @Override
            public void run()
            {
                try
                {
                    Thread.sleep ( timeout );

                    synchronized ( Link.this )
                    {
                        if ( isLocalConnecting )
                        {
                            isLocalConnecting = false;

                            Log.e ( TAG, "Local connect/accept timed out (after " + timeout + " ms)" );

                            onSocketError ( Link.this );
                        }
                    }
                }
                catch ( InterruptedException e )
                {
                    Thread.currentThread().interrupt();
                }
            }
        }

        /**
         * A thread for processing messages coming in from the socket.
         * <p/>
         * The functionality of this thread is overloaded to also connect to the socket.
         * It's done here to save us from having to create yet another thread...
         */
        private class InboundMessageThread extends Thread
        {
            /**
             * The path of the socket with which to connect (if we're using a local socket for I/O).
             *
             * Null if we're not using a local socket.
             */
            private final LocalSocketAddress localAddress;

            /**
             * The local server to accept a connection on.
             */
            private LocalServerSocket localServer = null;

            /**
             * Stores the TCP socket address (if we're using a TCP socket for I/O)
             * <p/>
             * Null if we're not using a TCP socket.
             */
            private final InetSocketAddress tcpSocketAddress;

            /**
             * Connection timeout value in milliseconds.
             */
            private final int timeout;

            /*
             * Indicates whether the thread is in the process of shutting down.
             * Used to solve a race condition that can potentially send a false socket error...
             */
            private volatile boolean shutdown = false;

            /**
             * Creates the thread that will process incoming messages
             *
             * @param localAddress The address of the local socket to use.
             * @param localServer The local server to accept a connection on.
             *                    If null, this thread will try to connect to given address instead of waiting
             *                    for incoming connection.
             */
            InboundMessageThread ( LocalSocketAddress localAddress, LocalServerSocket localServer )
            {
                super ( "SMM_Inbound" );

                this.timeout = 0;
                this.localServer = localServer;
                this.localAddress = localAddress;
                this.tcpSocketAddress = null;
            }

            /**
             * Creates the thread that will process incoming messages
             *
             * @param addr IP address and port to connect to
             * @param timeout Connection timeout value in milliseconds.
             */
            InboundMessageThread ( InetSocketAddress addr, int timeout )
            {
                super ( "SMM_Inbound" );

                this.timeout = timeout;
                this.localAddress = null;
                this.tcpSocketAddress = addr;
            }

            /**
             * Process incoming messages...
             */
            @Override
            public void run()
            {
                // Asynchronously set up the message manager!

                if ( tcpSocket != null )
                {
                    try
                    {
                        Log.d ( TAG, "Trying to connect to '" + tcpSocketAddress + "'" );

                        tcpSocket.connect ( tcpSocketAddress, timeout );
                    }
                    catch ( IOException e )
                    {
                        if ( !shutdown )
                        {
                            Log.e ( TAG, "Could not connect to '" + tcpSocketAddress.toString() + "': " + e );

                            onSocketError ( Link.this );
                        }

                        return;
                    }
                    finally
                    {
                        isLocalConnecting = false;
                    }
                }
                else if ( localServer != null )
                {
                    try
                    {
                        Log.d ( TAG, "Trying to accept socket on '" + localAddress.getName()
                                   + "' in namespace: " + localAddress.getNamespace() );

                        localSocket = localServer.accept();
                    }
                    catch ( IOException e )
                    {
                        if ( !shutdown )
                        {
                            Log.e ( TAG,
                                       "Could not accept socket on '" + localAddress.getName()
                                       + "' in namespace: " + localAddress.getNamespace() + ": " + e );

                            onSocketError ( Link.this );
                        }

                        return;
                    }
                    finally
                    {
                        synchronized ( Link.this )
                        {
                            if ( isLocalConnecting )
                            {
                                isLocalConnecting = false;
                            }
                            else
                            {
                                // This means that we timed-out in the meantime.
                                // Nothing else to do (the time-out thread would have already called onSocketError)
                                return;
                            }
                        }

                        try
                        {
                            localServer.close();
                        }
                        catch ( Exception e )
                        {
                        }
                    }
                }
                else
                {
                    try
                    {
                        Log.d ( TAG, "Trying to connect to '" + localAddress.getName()
                                   + "' in namespace: " + localAddress.getNamespace() );

                        localSocket = new LocalSocket();
                        localSocket.connect ( localAddress );
                    }
                    catch ( IOException e )
                    {
                        if ( !shutdown )
                        {
                            Log.e ( TAG, "Could not connect to '" + localAddress.getName()
                                       + "' in namespace: " + localAddress.getNamespace() + ": " + e );

                            onSocketError ( Link.this );
                        }

                        return;
                    }
                    finally
                    {
                        synchronized ( Link.this )
                        {
                            if ( isLocalConnecting )
                            {
                                isLocalConnecting = false;
                            }
                            else
                            {
                                // This means that we timed-out in the meantime.
                                // Nothing else to do (the time-out thread would have already called onSocketError)
                                return;
                            }
                        }
                    }
                }

                try
                {
                    // It is possible that this can fail if we are shutting down at the same time.
                    // In which case, just return (it will shut down on its own).
                    outboundMessageThread.start();
                }
                catch ( Exception e )
                {
                    return;
                }

                Log.d ( TAG, "InboundMessageThread socket connected" );

                socketConnected();

                // Dedicate the rest of this thread for processing incoming messages...

                try
                {
                    Message base = new Message();

                    while ( !shutdown
                            && localSocket != null
                            && base.deserializeWithLength ( localSocket.getInputStream() ) > 0 )
                    {
                        dispatch ( base );

                        base = new Message();
                    }

                    while ( !shutdown
                            && tcpSocket != null
                            && base.deserializeWithLength ( tcpSocket.getInputStream() ) > 0 )
                    {
                        dispatch ( base );

                        base = new Message();
                    }

                    Log.e ( TAG, "InboundMessageThread has reached the end of its stream!" );

                    if ( !shutdown )
                    {
                        onSocketError ( Link.this );
                    }
                }
                catch ( IOException e )
                {
                    if ( !shutdown )
                    {
                        Log.e ( TAG, "IOException thrown in InboundMessageThread: " + e );

                        onSocketError ( Link.this );
                    }
                }
                catch ( CodecException e )
                {
                    Log.e ( TAG, "CodecException thrown in InboundMessageThread: " + e );
                }
            }

            /**
             * Gracefully stops the thread from running.
             */
            public void shutdown()
            {
                shutdown = true;

                interrupt();
            }
        }
    }

    /**
     * Internal method for dealing with socket errors.
     * @param errorLink The link which generated an error.
     */
    private void onSocketError ( Link errorLink )
    {
        synchronized ( this )
        {
            if ( errorLink != ctrlLink )
                return;

            // stop() will synchronize as well, but it doesn't hurt us.
            stop();
        }

        socketDisconnected();
    }

    /**
     * Sets a protocol message handler for specific message type
     * At the moment there can be only one message handler for each message type
     * @param msgHandler The message handler to set
     * @throws IllegalArgumentException
     */
    public void setHandler ( ProtocolMessageHandler msgHandler )
    {
        if ( msgHandler == null )
        {
            return;
        }

        final Integer msgType = msgHandler.getMessageType();

        synchronized ( messageHandlers )
        {
            if ( messageHandlers.containsKey ( msgType ) )
                throw new IllegalArgumentException ( "Message type '" + msgType + "' already has a handler" );

            messageHandlers.put ( msgType, msgHandler );
        }
    }

    /**
     * Removes a protocol message handler
     * @param msgHandler Message handler to remove
     */
    public void removeHandler ( ProtocolMessageHandler msgHandler )
    {
        synchronized ( messageHandlers )
        {
            messageHandlers.remove ( msgHandler.getMessageType() );
        }
    }

    /**
     * Decodes the message to its particular message and dispatches it to callbacks...
     *
     * @param baseMessage The general message that buffers the serialised message data.
     */
    private void dispatch ( Message baseMessage )
    {
        final Integer msgType = baseMessage.getType();

        if ( msgType == null )
        {
            return;
        }

        if ( msgType.equals ( SimpleSubscriptionResponse.DEF_TYPE ) )
        {
            Log.v ( TAG, "Received a SimpleSubscriptionResponse; Trying to deserialize" );

            try
            {
                dispatchSimpleSubResponse ( simpleSubResponseMsg.generate ( baseMessage ) );
            }
            catch ( CodecException e )
            {
                Log.e ( TAG, "Protocol.CodecException while handling a "
                           + "SimpleSubscriptionResponse message; Error: " + e );
            }
            return;
        }
        else
        {
            dispatchDirectly ( baseMessage, false );
        }
    }

    /**
     * We use this for deserializing SimpleSubscriptionResponse messages.
     */
    private SimpleSubscriptionResponse simpleSubResponseMsg = new SimpleSubscriptionResponse();

    /**
     * Dispatches a message. Doesn't care whether it is a SimpleSubscriptionResponse.
     * @param baseMessage The message to dispatch.
     * @param onlyIndependentUpdates If set to true, this message will be delivered only
     *                               if handler's deliverEntireSimpleSubscriptionResponse()
     *                               method returns false.
     */
    private void dispatchDirectly ( Message baseMessage, boolean onlyIndependentUpdates )
    {
        final Integer msgType = baseMessage.getType();

        Log.v ( TAG, "Attempting to decode the message into a derived type; Type: " + msgType );

        synchronized ( messageHandlers )
        {
            final ProtocolMessageHandler msgHandler = messageHandlers.get ( msgType );

            if ( msgHandler == null )
            {
                Log.w ( TAG, "There is no handler set for message type " + msgType
                           + "; Ignoring" );
                return;
            }

            if ( onlyIndependentUpdates && msgHandler.deliverEntireSimpleSubscriptionResponse() )
            {
                Log.d ( TAG, "Message handler for type " + msgType
                           + " wants entire SimpleSubscriptionResponse message; "
                           + "Skipping individual update delivery" );
                return;
            }

            try
            {
                msgHandler.handleMessage ( baseMessage );
            }
            catch ( CodecException e )
            {
                Log.e ( TAG, "Protocol.CodecException while handling a message type "
                           + msgType + "; Error: " + e );
            }
        }
    }

    /**
     * Delivery modes for SimpleSubscriptionResponse messages.
     */
    private enum SubRespDeliveryMode
    {
        /**
         * Nothing will be delivered
         */
        DontDeliver,

        /**
         * The entire SimpleSubscriptionResponse will be delivered.
         * Internal updates will not be delivered.
         */
        DeliverDirectly,

        /**
         * SimpleSubscriptionResponse will not be delivered directly.
         * Each of the updates inside SimpleSubscriptionResponse message
         * will be delivered independently.
         */
        DeliverIndependently,

        /**
         * SimpleSubscriptionResponse will be delivered directly.
         * After that, each of the internal updates will be delivered
         * independently (unless its handler wants the entire SimpleSubscriptionResponse,
         * in case this particular update will not be delivered).
         */
        DeliverBoth
    }

    /**
     * Dispatches SimpleSubscriptionResponse messages.
     * @param msg The SimpleSubscriptionResponse message to dispatch.
     */
    private void dispatchSimpleSubResponse ( SimpleSubscriptionResponse msg )
    {
        if ( msg == null )
        {
            return;
        }

        SubRespDeliveryMode dMode = getSubRespDeliveryMode ( msg );

        if ( dMode == SubRespDeliveryMode.DontDeliver )
        {
            Log.d ( TAG, "Delivery mode: Don't deliver; Ignoring the message" );
            return;
        }

        if ( dMode == SubRespDeliveryMode.DeliverDirectly
             || dMode == SubRespDeliveryMode.DeliverBoth )
        {
            Log.d ( TAG, "Delivery mode: " + dMode + "; Delivering sub response directly" );
            dispatchDirectly ( msg, false );
        }

        if ( dMode == SubRespDeliveryMode.DeliverIndependently
             || dMode == SubRespDeliveryMode.DeliverBoth )
        {
            for ( Message updateMsg : msg.getUpdates() )
            {
                dispatchDirectly ( updateMsg, true );
            }
        }
    }

    /**
     * Function that inspects the SimpleSubscriptionResponse and decides how it should
     * be delivered.
     * @param msg The SimpleSubscriptionResponse message to inspect.
     * @return One of the SubRespDeliveryMode modes.
     */
    private SubRespDeliveryMode getSubRespDeliveryMode ( SimpleSubscriptionResponse msg )
    {
        if ( msg == null || !msg.hasUpdates() )
        {
            return SubRespDeliveryMode.DontDeliver;
        }

        boolean deliverDirectly = false;
        boolean deliverIndependently = false;

        synchronized ( messageHandlers )
        {
            for ( com.pravala.protocol.auto.ctrl.Message updateMsg : msg.getUpdates() )
            {
                final Integer updateType = updateMsg.getType();
                final ProtocolMessageHandler msgHandler = messageHandlers.get ( updateType );

                if ( msgHandler == null )
                {
                    Log.w ( TAG, "There is no handler set for internal update type "
                               + updateType + "; Ignoring" );
                }
                else if ( msgHandler.deliverEntireSimpleSubscriptionResponse() )
                {
                    Log.d ( TAG, "Handler for update type " + updateType
                               + " says we need to deliver the entire SimpleSubscriptionResponse" );

                    deliverDirectly = true;
                }
                else
                {
                    deliverIndependently = true;
                }
            }
        }

        if ( deliverDirectly )
        {
            return deliverIndependently
                   ? SubRespDeliveryMode.DeliverBoth
                   : SubRespDeliveryMode.DeliverDirectly;
        }
        else if ( deliverIndependently )
        {
            return SubRespDeliveryMode.DeliverIndependently;
        }

        return SubRespDeliveryMode.DontDeliver;
    }

    /**
     * Start SocketMessageManager to connect and process messages over a local socket
     *
     * @param socketPath The path to connect a socket to.
     * @param localServerMode The mode of the local server socket
     * @return True on success; False if the manager is already running or cannot be started.
     */
    public boolean start ( String socketPath, LocalServerMode localServerMode )
    {
        return start ( socketPath, localServerMode, 0 );
    }

    /**
     * Start SocketMessageManager to connect and process messages over a local socket
     *
     * @param socketPath The path to connect a socket to.
     * @param localServerMode The mode of the local server socket.
     * @param timeout Connection timeout value in milliseconds.
     *                In server mode it will time-out if there are no accepted connections during specified time.
     * @return True on success; False if the manager is already running or cannot be started.
     */
    public boolean start ( String socketPath, LocalServerMode localServerMode, int timeout )
    {
        synchronized ( this )
        {
            if ( ctrlLink != null )
                return false;

            try
            {
                ctrlLink = new Link ( socketPath, localServerMode, timeout );
            }
            catch ( IOException e )
            {
                Log.e ( TAG, "Could not generate a new local control link on '"
                           + socketPath + "' in " + localServerMode + " mode: " + e );
                return false;
            }
            catch ( Exception e )
            {
                e.printStackTrace();
                return false;
            }
        }

        return true;
    }

    /**
     * Start SocketMessageManager to connect and process messages over a TCP socket
     *
     * @param addr IP address and port to connect to
     * @return True on success; False if the manager is already running.
     */
    public boolean start ( InetSocketAddress addr )
    {
        return start ( addr, 0 );
    }

    /**
     * Start SocketMessageManager to connect and process messages over a TCP socket
     *
     * @param addr IP address and port to connect to
     * @param timeout Connection timeout value in milliseconds.
     * @return True on success; False if the manager is already running.
     */
    public boolean start ( InetSocketAddress addr, int timeout )
    {
        synchronized ( this )
        {
            if ( ctrlLink != null )
                return false;

            ctrlLink = new Link ( addr, timeout );
        }

        return true;
    }

    /**
     * Stops the processing of messages.
     */
    public void stop()
    {
        Link curLink = null;

        synchronized ( this )
        {
            curLink = ctrlLink;
            ctrlLink = null;
        }

        if ( curLink != null )
        {
            curLink.stop();
        }
    }

    /**
     * Sends a message through the socket.
     *
     * @param message The message to send through.
     */
    public void send ( SerializableBase message )
    {
        send ( message, null );
    }

    /**
     * Sends a message through the socket with an ancillary file descriptor.
     *
     * @param message The message to send through.
     * @param fd The file descriptor to send through.
     */
    public void send ( SerializableBase message, FileDescriptor fd )
    {
        final Link curLink = this.ctrlLink;

        if ( curLink != null )
        {
            curLink.send ( message, fd );
        }
    }

    /**
     * Queries whether the message manager is currently running and connected
     *
     * @return True if the manager's running properly and the socket is connected, false otherwise.
     */
    public boolean isConnected()
    {
        final Link curLink = this.ctrlLink;

        return ( curLink != null && curLink.isConnected() );
    }

    /**
     * Called when the socket is connected.
     */
    abstract protected void socketConnected();

    /**
     * Called whenever the socket is disconnected (ie, due to a fatal error).
     * The receiver should handle reconnecting the socket if necessary.
     */
    abstract protected void socketDisconnected();
}
