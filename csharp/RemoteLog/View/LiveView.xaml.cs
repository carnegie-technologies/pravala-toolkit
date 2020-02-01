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
    using System.Collections.Specialized;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Media;
    using Pravala.RemoteLog.Service;

    /// <summary>
    /// Interaction logic for LiveView window.
    /// </summary>
    public partial class LiveView: Window
    {
        #region Fields
        /// <summary>
        /// Stores the messages scroll viewer so that we can scroll it to the end.
        /// </summary>
        private ScrollViewer messagesScrollViewer;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the LiveView class.
        /// </summary>
        public LiveView()
        {
            this.InitializeComponent();

            this.Loaded += this.OnLiveViewLoaded;
            this.Unloaded += this.OnLiveViewUnloaded;

            string title = "Remote Log: " + ServiceProvider.RemoteLink.HostName + ":"
                           + ServiceProvider.RemoteLink.Port;
            this.Title = title;
            this.TitleTextBlock.Text = title;
        }

        #endregion

        #region Methods
        /// <summary>
        /// Listens for changes to messages so that we can keep the viewer scrolled to the bottom.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnLiveViewLoaded ( object sender, RoutedEventArgs e )
        {
            if ( VisualTreeHelper.GetChildrenCount ( this.MessagesListBox ) > 0 )
            {
                Border border = ( Border ) VisualTreeHelper.GetChild ( this.MessagesListBox, 0 );
                this.messagesScrollViewer = ( ScrollViewer ) VisualTreeHelper.GetChild ( border, 0 );
            }

            ServiceProvider.ViewModel.Messages.CollectionChanged += this.OnMessagesCollectionChanged;
        }

        /// <summary>
        /// Stops listening for message changes.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnLiveViewUnloaded ( object sender, RoutedEventArgs e )
        {
            ServiceProvider.ViewModel.Messages.CollectionChanged -= this.OnMessagesCollectionChanged;
        }

        /// <summary>
        /// Scrolls to the bottom of the scroll viewer when a new message is added.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnMessagesCollectionChanged ( object sender, NotifyCollectionChangedEventArgs e )
        {
            if ( this.messagesScrollViewer != null )
            {
                this.messagesScrollViewer.ScrollToBottom();
            }
        }

        /// <summary>
        /// Stops the logger.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnStopButtonClick ( object sender, RoutedEventArgs e )
        {
            if ( ServiceProvider.OutputFile.IsOpen )
            {
                // File open; close the connection (if present), then close the file
                if ( ServiceProvider.RemoteLink.IsRunning )
                {
                    ServiceProvider.RemoteLink.Stop();
                }

                string filename = ServiceProvider.OutputFile.CloseFile();
                this.Close();
                ServiceProvider.ViewModel.Messages.Clear();
                System.Windows.MessageBox.Show ( "Wrote log to: " + filename, "Logging complete" );
            }
        }

        #endregion
    }
}
