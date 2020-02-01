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
    using System.Threading.Tasks;
    using Pravala.Control;
    using Pravala.Control.Subscriptions;
    using Pravala.Protocol.Auto.Ctrl;
    using Pravala.Protocol.Auto.Log;

    /// <summary>
    /// Subscriber for log and diagnostic updates.
    /// </summary>
    public class LogSubscription: CtrlSubscription
    {
        /// <summary>
        /// The log pattern to subscribe to.
        /// </summary>
        private string pattern = string.Empty;

        /// <summary>
        /// Update the log pattern to subscribe to.
        /// </summary>
        /// <remarks>
        /// The updated log pattern only applies after a (re)connect.
        /// </remarks>
        /// <param name="pattern">The log pattern to subscribe to.</param>
        public void UpdateLogPattern ( string pattern )
        {
            this.pattern = pattern;
        }

        /// <summary>
        /// Subscribes for updates.
        /// </summary>
        /// <param name="ctrlLink">CtrlLink to send the subscribe on.</param>
        /// <returns>Task for async use.</returns>
        public override async Task SubscribeAsync ( CtrlLink ctrlLink )
        {
            // Split log pattern into pairs
            string[] logDescriptions = this.pattern.Split ( ' ' );

            // Send a subscribe message for each pair
            foreach ( string logDescription in logDescriptions )
            {
                string[] logParts = logDescription.Split ( '.' );

                if ( logParts.Length != 2 )
                {
                    continue;
                }

                string name = logParts[ 0 ];
                LogLevel level = this.LevelStringToEnum ( logParts[ 1 ] );

                if ( level != LogLevel.Invalid )
                {
                    ServiceProvider.OutputFile.WriteMessageLine ( "Subscribe for " + name + "." + level.ToString() );
                    LogSubscribe infoSubMsg = new LogSubscribe();
                    infoSubMsg.setNamePattern ( name );
                    infoSubMsg.setLevel ( level );
                    await ctrlLink.SendMessageAsync ( infoSubMsg );
                }
            }
        }

        /// <summary>
        /// Unsubscribes from updates.
        /// </summary>
        /// <param name="ctrlLink">CtrlLink to send the unsubscribe on.</param>
        /// <returns>Task for async use.</returns>
        public override async Task UnsubscribeAsync ( CtrlLink ctrlLink )
        {
            LogUnsubscribe infoSubMsg = new LogUnsubscribe();

            infoSubMsg.setNamePattern ( "*" );
            await ctrlLink.SendMessageAsync ( infoSubMsg );
        }

        /// <summary>
        /// Turns a log level string into the corresponding enumeration value.
        /// </summary>
        /// <param name="level">The level string.</param>
        /// <returns>The corresponding level enumeration.</returns>
        private LogLevel LevelStringToEnum ( string level )
        {
            level = level.ToLowerInvariant();
            level = level.Replace ( "_", string.Empty ).Replace ( "-", string.Empty ).Replace ( ".", string.Empty );

            switch ( level )
            {
                case "fatalerror":
                case "fatal":
                    return LogLevel.FatalError;

                case "error":
                    return LogLevel.Error;

                case "warning":
                case "warn":
                    return LogLevel.Warning;

                case "info":
                    return LogLevel.Info;

                case "debug":
                case "debug1":
                    return LogLevel.Debug;

                case "debug2":
                    return LogLevel.Debug2;

                case "debug3":
                    return LogLevel.Debug3;

                case "debug4":
                case "max":
                case "all":
                    return LogLevel.Debug4;
            }

            return LogLevel.Invalid;
        }
    }
}
