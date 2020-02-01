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
    using Pravala.Control;
    using Pravala.RemoteLog.View;

    /// <summary>
    /// Used to receive control messages with LogMessage type.
    /// </summary>
    internal class LogMessageReceiver: CtrlMessageReceiver<Pravala.Protocol.Auto.Ctrl.LogMessage>
    {
        #region Methods
        /// <summary>
        /// Called when a control message LogMessage is received.
        /// </summary>
        /// <param name="ctrlLink">The link that received this message.</param>
        /// <param name="msg">The message itself.</param>
        public override void ReceiveMessage ( CtrlLink ctrlLink, Protocol.Auto.Ctrl.LogMessage msg )
        {
            // Don't care about error code; log messages are just updates with no code set
            if ( !msg.hasLogMessage() )
            {
                return;
            }

            if ( msg.getLogMessage().getType() == Pravala.Protocol.Auto.Log.LogType.TextMessage )
            {
                Pravala.Protocol.Auto.Log.TextMessage textMsg = new Pravala.Protocol.Auto.Log.TextMessage();

                if ( textMsg.DeserializeFromBaseMessage ( msg.getLogMessage() ) )
                {
                    BindableTextMessage bindableMessage = new BindableTextMessage ( textMsg );
                    ServiceProvider.OutputFile.Write ( bindableMessage.ToString() );

                    ServiceProvider.ViewModel.PostToUiThread (
                            o =>
                    {
                        ServiceProvider.ViewModel.AddMessage ( bindableMessage );
                    },
                            this );
                }
            }
        }

        #endregion
    }
}
