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

namespace Pravala.Control
{
    /// <summary>
    /// Receiver of control messages of a specific type.
    /// </summary>
    /// <typeparam name="T">The type of the message to receive.</typeparam>
    public abstract class CtrlMessageReceiver<T> : ICtrlMessageReceiver where T: Protocol.Auto.Ctrl.Message, new ( )
    {
        /// <summary>
        /// Called whenever a base message is received.
        /// </summary>
        /// <param name="ctrlLink">The link over which the message was received.</param>
        /// <param name="baseMsg">The message received.</param>
        public void ReceiveBaseMessage ( CtrlLink ctrlLink, Protocol.Auto.Ctrl.Message baseMsg )
        {
            T msg = new T();

            msg.DeserializeFromBaseMessage ( baseMsg );
            this.ReceiveMessage ( ctrlLink, msg );
        }

        /// <summary>
        /// Returns the type of the message supported by this receiver.
        /// </summary>
        /// <returns>The type of the message supported by this receiver.</returns>
        public uint GetMessageType()
        {
            T msg = new T();

            msg.SetupMessageDefines();
            return msg.getType();
        }

        /// <summary>
        /// Called whenever a message is received.
        /// </summary>
        /// <param name="ctrlLink">The link over which the message was received.</param>
        /// <param name="msg">The message received.</param>
        public abstract void ReceiveMessage ( CtrlLink ctrlLink, T msg );

        /// <summary>
        /// Called whenever the CtrlLink hosting this message receiver connects.
        /// </summary>
        /// <remarks>
        /// Default implementation is a no-op.
        /// </remarks>
        public virtual void OnConnect()
        {
        }

        /// <summary>
        /// Called whenever the CtrlLink hosting this message receiver disconnects.
        /// </summary>
        /// <remarks>
        /// Can be used (for instance) to clear state when not connected.
        /// Default implementation is a no-op.
        /// </remarks>
        public virtual void OnDisconnect()
        {
        }
    }
}
