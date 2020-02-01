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

namespace Pravala.RemoteLog.View
{
    using System;
    using System.Globalization;
    using System.Text;
    using Pravala.Control;
    using Pravala.Protocol.Auto.Log;

    /// <summary>
    /// Bindable version of TextMessage.
    /// </summary>
    public class BindableTextMessage: TextMessage
    {
        /// <summary>
        /// Initializes a new instance of the BindableTextMessage class.
        /// </summary>
        /// <param name="textMsg">The TextMessage to extend.</param>
        public BindableTextMessage ( TextMessage textMsg )
        {
            // Output the same way as TextLogOutput::formatMessage(), but with better date
            if ( textMsg.hasTime() )
            {
                this.setTime ( textMsg.getTime() );
            }

            if ( textMsg.hasName() )
            {
                this.setName ( textMsg.getName() );
            }

            if ( textMsg.hasLevel() )
            {
                this.setLevel ( textMsg.getLevel() );
            }

            if ( textMsg.hasFuncName() )
            {
                this.setFuncName ( textMsg.getFuncName() );
            }

            if ( textMsg.hasContent() )
            {
                this.setContent ( textMsg.getContent() );
            }
        }

        /// <summary>
        /// Gets the message time.
        /// </summary>
        public string Time
        {
            get
            {
                return this.GetIsoDateString();
            }
        }

        /// <summary>
        /// Gets the message name.
        /// </summary>
        public string Name
        {
            get
            {
                return this.hasName() ? this.getName() : null;
            }
        }

        /// <summary>
        /// Gets the message level.
        /// </summary>
        public LogLevel Level
        {
            get
            {
                return this.hasLevel() ? this.getLevel() : LogLevel.Invalid;
            }
        }

        /// <summary>
        /// Gets the message function name.
        /// </summary>
        public string FuncName
        {
            get
            {
                return this.hasFuncName() ? this.getFuncName() : null;
            }
        }

        /// <summary>
        /// Gets the message content.
        /// </summary>
        public string Content
        {
            get
            {
                return this.hasContent() ? this.getContent() : null;
            }
        }

        /// <summary>
        /// Gets the string representation of this class.
        /// </summary>
        /// <remarks>Outputs the same way as TextLogOutput::formatMessage(), but with better date</remarks>
        /// <returns>String representation of this class.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            if ( this.hasTime() )
            {
                sb.Append ( this.GetIsoDateString() ).Append ( "|" );
            }

            if ( this.hasName() )
            {
                sb.Append ( this.getName() ).Append ( "|" );
            }

            if ( this.hasLevel() )
            {
                sb.Append ( this.getLevel() ).Append ( "|" );
            }

            if ( this.hasFuncName() )
            {
                sb.Append ( this.getFuncName() ).Append ( "|" );
            }

            if ( this.hasContent() )
            {
                sb.AppendLine ( this.getContent() );
            }

            return sb.ToString();
        }

        /// <summary>
        /// Gets the message date/time as an ISO 8601 formatted string.
        /// </summary>
        /// <returns>The message date/time as an ISO 8601 formatted string.</returns>
        private string GetIsoDateString()
        {
            if ( this.hasTime() )
            {
                DateTime time = MessageUtils.FromUnixTime ( this.getTime() );
                return time.ToString ( "s", CultureInfo.InvariantCulture );
            }

            return null;
        }
    }
}
