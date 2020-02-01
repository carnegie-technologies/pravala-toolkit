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

import com.pravala.protocol.CodecException;
import com.pravala.protocol.auto.ctrl.Message;
import android.util.Log;

/**
 * The callback object of ProtocolMessageDispatcher, used to receive com.pravala.auto.ctrl messages...
 */
public abstract class ProtocolMessageHandler<MessageType extends Message>
{
    /**
     * Class name, for logging
     */
    private static final String TAG = ProtocolMessageHandler.class.getName();

    /**
     * The message object that will be deserialized over and over again
     */
    private final MessageType messageObject;

    /**
     * The message type ID
     */
    private final Integer messageType;

    /**
     * The message type ID
     */
    private final String messageName;

    /**
     * Constructor
     * @param messageObject The message object to be used; Should NOT be null!
     */
    protected ProtocolMessageHandler ( MessageType messageObject )
    {
        messageObject.setupDefines();

        this.messageObject = messageObject;
        this.messageType = messageObject.getType();
        this.messageName = messageObject.getClass().getName();
    }

    /**
     * Constructor
     * @param messageManager The message manager to subscribe to
     * @param messageObject The message object to be used; Should NOT be null!
     */
    protected ProtocolMessageHandler ( SocketMessageManager messageManager, MessageType messageObject )
    {
        this ( messageObject );

        messageManager.setHandler ( this );
    }

    /**
     * Returns the type of messages this handler is accepting
     * @return the type of messages this handler is accepting
     */
    public Integer getMessageType()
    {
        return messageType;
    }

    /**
     * Returns the type name of messages handled by this object
     * @return the type name of messages handled by this object
     */
    public String getMessageName()
    {
        return messageName;
    }

    /**
     * Called when a base message that looks like the message type this handler supports is received.
     * It will try to deserialize it, and call the 'receivedMessage' callback
     * @param baseMessage The base message to deserialize
     * @throws CodecException
     */
    public void handleMessage ( Message baseMessage ) throws CodecException
    {
        if ( baseMessage == null )
        {
            Log.e ( TAG, "handleMessage called with null baseMessage" );
            return;
        }

        if ( !baseMessage.hasType() )
        {
            Log.e ( TAG, "Failed to decode message: message has no type" );
            return;
        }

        if ( !baseMessage.getType().equals ( messageType ) )
        {
            Log.w ( TAG, "Failed to decode message: message type [" + baseMessage.getType()
                       + "] does not match this handler's type " + messageName + " [" + messageType + "]" );
            return;
        }

        // Message.generate() throws an exception on failure. It never returns null.
        @SuppressWarnings ( "unchecked" )
        final MessageType decodedMessage = ( MessageType ) messageObject.generate ( baseMessage );

        if ( decodedMessage == null )
        {
            Log.e ( TAG, "Failed to generate a message; generate() returned null; Message type: "
                       + messageType + "; Message name: " + messageName );
            return;
        }

        Log.v ( TAG, "Decoded message of type " + decodedMessage.getClass().getName()
                   + " [" + messageType + "];" );

        receivedMessage ( decodedMessage );
    }

    /**
     * Called whenever a SimpleSubscriptionResponse message is received and one of its updates
     * matches this handler's type. If any of the handlers matching any of the updates
     * inside the SimpleSubscriptionResponse message says 'true',
     * then the SimpleSubscriptionResponse is delivered first, followed by individual
     * updates to the handlers that said 'false'. The default implementation
     * returns 'false', which means that, by default, all updates in all SimpleSubscriptionResponse
     * messages are delivered independently of each other.
     * @return Whether this handler wants to see the entire SimpleSubscriptionResponse
     *          message (true), or should all internal updates be delivered independently.
     */
    public boolean deliverEntireSimpleSubscriptionResponse()
    {
        return false;
    }

    /**
     * Called when a message is received and deserialized properly
     * @param message Deserialized message
     */
    protected abstract void receivedMessage ( final MessageType message );
}
