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
    using Pravala.Protocol;

    /// <summary>
    /// COMPATIBILITY SHIM used for notifying when the status of control connections changes.
    /// </summary>
    public interface ICtrlStatusReceiver
    {
        /// <summary>
        /// Called whenever the control connection gets closed by the remote side.
        /// </summary>
        /// <param name="ctrlLink">The CtrlLink object that got disconnected</param>
        void CtrlLinkClosed ( CtrlLink ctrlLink );

        /// <summary>
        /// Called whenever the control link is closed due to protocol error.
        /// </summary>
        /// <param name="ctrlLink">The CtrlLink object that experienced the error</param>
        /// <param name="protoException">
        /// Exception that was generated while trying to deserialize the message.
        /// </param>
        void CtrlLinkError ( CtrlLink ctrlLink, ProtoException protoException );

        /// <summary>
        /// Called whenever the control link is closed due to some other error.
        /// </summary>
        /// <param name="ctrlLink">The CtrlLink object that experienced the error</param>
        /// <param name="exception">Exception that cased the disconnect</param>
        void CtrlLinkError ( CtrlLink ctrlLink, Exception exception );
    }
}
