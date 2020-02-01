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

namespace Pravala.RemoteLog
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.IO;
    using Pravala.RemoteLog.Service;

    /// <summary>
    /// Manages logging to file.
    /// </summary>
    [ System.Diagnostics.CodeAnalysis.SuppressMessage (
              "Microsoft.Design",
              "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable",
              Justification = "File handle is disposed in CloseFile" ) ]
    public class OutputFile
    {
        #region Fields
        /// <summary>
        /// InternalMessageMarkup for internally-generated messages.
        /// </summary>
        private const string InternalMessageMarkup = "==========";

        /// <summary>
        /// StreamWriter for writing to file.
        /// </summary>
        private StreamWriter file;

        /// <summary>
        /// Path of the file we are outputting to.
        /// </summary>
        private string path;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the OutputFile class.
        /// </summary>
        public OutputFile()
        {
            ServiceProvider.RemoteLink.PropertyChanged += this.OnConnectionManagerPropertyChanged;
        }

        #endregion

        #region Properties
        /// <summary>
        /// Gets a value indicating whether the log file has been opened.
        /// </summary>
        public bool IsOpen
        {
            get
            {
                return this.file != null;
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Opens the log file.
        /// </summary>
        /// <returns>Whether the log file was opened.</returns>
        public bool OpenFile()
        {
            if ( this.file != null )
            {
                return false;
            }

            // File name
            string fileName = string.Format ( "RemoteLog_{0:yyyy-MM-ddThh-mm-ss}.txt", DateTime.Now );

            try
            {
                this.path = Path.Combine ( ServiceProvider.ViewModel.SaveTo, fileName );
                this.file = new StreamWriter ( this.path );
            }
            catch ( IOException )
            {
                this.path = null;
                this.file = null;
                return false;
            }

            this.WriteMessageLine ( "Log started " + DateTime.UtcNow.ToString ( "s", CultureInfo.InvariantCulture ) );

            return true;
        }

        /// <summary>
        /// Closes the log file.
        /// </summary>
        /// <returns>The path of the file that was closed.</returns>
        public string CloseFile()
        {
            string wroteTo = this.path;

            this.WriteMessageLine ( "Log ended " + DateTime.UtcNow.ToString ( "s", CultureInfo.InvariantCulture ) );

            if ( this.file == null )
            {
                return null;
            }

            this.file.Close();
            this.file.Dispose();
            this.file = null;
            this.path = null;

            return wroteTo;
        }

        /// <summary>
        /// Writes to the log file.
        /// </summary>
        /// <param name="text">The text to write.</param>
        public void Write ( string text )
        {
            if ( this.file != null )
            {
                this.file.Write ( text );
            }
        }

        /// <summary>
        /// Writes a line to the log file.
        /// </summary>
        /// <param name="text">The text to write.</param>
        public void WriteLine ( string text )
        {
            if ( this.file != null )
            {
                this.file.WriteLine ( text );
            }
        }

        /// <summary>
        /// Writes an internal message line to the log file.
        /// </summary>
        /// <param name="text">The text to write.</param>
        public void WriteMessageLine ( string text )
        {
            this.WriteLine ( InternalMessageMarkup + " " + text + " " + InternalMessageMarkup );
        }

        /// <summary>
        /// Writes connection state to the log file.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnConnectionManagerPropertyChanged ( object sender, PropertyChangedEventArgs e )
        {
            if ( e.PropertyName == "IsRunning" || e.PropertyName == "IsConnected" )
            {
                if ( this.IsOpen )
                {
                    var cm = ServiceProvider.RemoteLink;

                    if ( cm.IsRunning && !cm.IsConnected )
                    {
                        this.WriteMessageLine ( "Connecting to " + ServiceProvider.RemoteLink.HostName + ":"
                                + cm.Port );
                    }
                    else if ( cm.IsRunning && cm.IsConnected )
                    {
                        this.WriteMessageLine ( "Connected to " + ServiceProvider.RemoteLink.HostName + ":" + cm.Port );
                    }
                    else
                    {
                        this.WriteMessageLine ( "Disconnected" );
                    }
                }
            }
        }

        #endregion
    }
}
