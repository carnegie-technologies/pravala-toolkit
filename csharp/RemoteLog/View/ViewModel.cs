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
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Threading;
    using System.Windows;

    /// <summary>
    /// The ViewModel stores data the that the UI uses.
    /// </summary>
    public class ViewModel: DependencyObject
    {
        /// <summary>
        /// DependencyProperty as the backing store for SaveTo.
        /// </summary>
        public static readonly DependencyProperty SaveToProperty = DependencyProperty.Register (
                "SaveTo",
                typeof ( string ),
                typeof ( ViewModel ),
                new PropertyMetadata ( Environment.GetFolderPath ( Environment.SpecialFolder.Desktop ) ) );

        /// <summary>
        /// DependencyProperty as the backing store for Filters.
        /// </summary>
        public static readonly DependencyProperty FiltersProperty = DependencyProperty.Register (
                "Filters",
                typeof ( List<string> ),
                typeof ( ViewModel ),
                new PropertyMetadata ( new List<string>() ) );

        /// <summary>
        /// DependencyProperty as the backing store for ActiveFilterIndex.
        /// </summary>
        public static readonly DependencyProperty ActiveFilterIndexProperty = DependencyProperty.Register (
                "ActiveFilterIndex",
                typeof ( int ),
                typeof ( ViewModel ),
                new PropertyMetadata ( 3, new PropertyChangedCallback ( OnActiveFilterIndexChanged ) ) );

        /// <summary>
        /// DependencyProperty as the backing store for Messages.
        /// </summary>
        public static readonly DependencyProperty MessagesProperty = DependencyProperty.Register (
                "Messages",
                typeof ( ObservableCollection<BindableTextMessage> ),
                typeof ( ViewModel ),
                new PropertyMetadata ( new ObservableCollection<BindableTextMessage>() ) );

        #region Fields
        /// <summary>
        /// Amount of scrollback.
        /// </summary>
        private const int Scrollback = 5000;

        /// <summary>
        /// Collection of log patterns that the user can select from.
        /// </summary>
        /// <remarks>
        /// TODO: Add 'Custom' option and allow the user to enter their own pattern.
        /// </remarks>
        private static readonly ReadOnlyDictionary<string, string> FilterDictionary
            = new ReadOnlyDictionary<string, string> ( new Dictionary<string, string>()
        {
            { "*.Fatal", "*.fatal" },
            { "*.Warning", "*.warning" },
            { "*.Info", "*.info" },
            { "*.Debug", "*.debug" },
            { "*.Debug2", "*.debug2" },
            { "*.Debug3", "*.debug3" },
            { "*.MAX", "*.max" },
            { "Diagnostics", "diagnostics.debug2" }
        } );

        /// <summary>
        /// Synchronization context for the UI thread.
        /// </summary>
        private SynchronizationContext uiThread;

        /// <summary>
        /// The live view window, if opened.
        /// </summary>
        private LiveView liveViewWindow;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the ViewModel class.
        /// </summary>
        public ViewModel()
        {
            // Add all filters from the dictionary to the list property
            this.Filters.AddRange ( FilterDictionary.Keys );
            this.uiThread = SynchronizationContext.Current;
            ServiceProvider.RemoteLink.UpdateLogPattern ( this.GetActiveLogFilter() );
        }

        #endregion

        #region Properties
        /// <summary>
        /// Gets or sets the log file directory.
        /// </summary>
        public string SaveTo
        {
            get
            {
                return ( string ) this.GetValue ( SaveToProperty );
            }

            set
            {
                this.SetValue ( SaveToProperty, value );
            }
        }

        /// <summary>
        /// Gets the list of log filters.
        /// </summary>
        public List<string> Filters
        {
            get
            {
                return ( List<string> ) this.GetValue ( FiltersProperty );
            }

            private set
            {
                this.SetValue ( FiltersProperty, value );
            }
        }

        /// <summary>
        /// Gets or sets the active filter index.
        /// </summary>
        public int ActiveFilterIndex
        {
            get
            {
                return ( int ) this.GetValue ( ActiveFilterIndexProperty );
            }

            set
            {
                this.SetValue ( ActiveFilterIndexProperty, value );
            }
        }

        /// <summary>
        /// Gets the log messages that have been received.
        /// </summary>
        public ObservableCollection<BindableTextMessage> Messages
        {
            get
            {
                return ( ObservableCollection<BindableTextMessage> ) this.GetValue ( MessagesProperty );
            }

            private set
            {
                this.SetValue ( MessagesProperty, value );
            }
        }

        #endregion

        #region Methods
        /// <summary>
        /// Posts a callback to the UI thread.
        /// </summary>
        /// <remarks>
        /// If the UI thread does not exist (Window has not loaded/has unloaded), the task is dropped.
        /// </remarks>
        /// <param name="d">The delegate to call.</param>
        /// <param name="state">The object passed to the delegate, typically 'this'.</param>
        public void PostToUiThread ( SendOrPostCallback d, object state )
        {
            if ( this.uiThread != null )
            {
                this.uiThread.Post ( d, state );
            }
        }

        /// <summary>
        /// Get the active log filter patterns.
        /// </summary>
        /// <returns>The active log filter patterns.</returns>
        public string GetActiveLogFilter()
        {
            var activeKey = this.Filters[ this.ActiveFilterIndex ];

            return FilterDictionary[ activeKey ];
        }

        /// <summary>
        /// Show the live view window.
        /// </summary>
        public void ShowLiveView()
        {
            if ( this.liveViewWindow == null )
            {
                this.liveViewWindow = new LiveView();
                this.liveViewWindow.Closed += this.OnLiveViewWindowClosed;
            }

            this.liveViewWindow.Show();
        }

        /// <summary>
        /// Hide the live view window.
        /// </summary>
        public void HideLiveView()
        {
            if ( this.liveViewWindow != null )
            {
                this.liveViewWindow.Close();
            }
        }

        /// <summary>
        /// Add a message to the messages collection, limiting to the scrollback length.
        /// </summary>
        /// <param name="message">The message to add.</param>
        public void AddMessage ( BindableTextMessage message )
        {
            this.Messages.Add ( message );

            while ( this.Messages.Count > Scrollback )
            {
                this.Messages.RemoveAt ( 0 );
            }
        }

        /// <summary>
        /// Updates the log subscription with the current log pattern.
        /// </summary>
        /// <param name="d">Dependency object.</param>
        /// <param name="e">Event arguments</param>
        private static void OnActiveFilterIndexChanged ( DependencyObject d, DependencyPropertyChangedEventArgs e )
        {
            ServiceProvider.RemoteLink.UpdateLogPattern ( ServiceProvider.ViewModel.GetActiveLogFilter() );
        }

        /// <summary>
        /// Discard our live view window reference once it is closed.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnLiveViewWindowClosed ( object sender, EventArgs e )
        {
            this.liveViewWindow = null;
        }

        #endregion
    }
}
