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
    using System.ComponentModel;
    using System.Windows;
    using System.Windows.Forms;
    using System.Windows.Media;
    using Pravala.RemoteLog.Service;

    /// <summary>
    /// Interaction logic for MainWindow.
    /// </summary>
    public partial class MainWindow: Window
    {
        #region Fields
        /// <summary>
        /// DependencyProperty as the backing store for StatusText.
        /// </summary>
        public static readonly DependencyProperty StatusTextProperty = DependencyProperty.Register (
                "StatusText",
                typeof ( string ),
                typeof ( MainWindow ),
                new PropertyMetadata ( "Ready" ) );

        /// <summary>
        /// DependencyProperty as the backing store for StatusTextColor.
        /// </summary>
        public static readonly DependencyProperty StatusTextColorProperty = DependencyProperty.Register (
                "StatusTextColor",
                typeof ( Brush ),
                typeof ( MainWindow ),
                new PropertyMetadata ( null ) );
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the MainWindow class.
        /// </summary>
        public MainWindow()
        {
            this.InitializeComponent();

            this.Loaded += this.OnLoaded;
            this.Unloaded += this.OnUnloaded;
            this.Closing += this.OnMainWindowClosing;

            this.SetStatus ( "Ready", Colors.Gray );
        }

        #endregion

        #region Properties
        /// <summary>
        /// Gets or sets the status text.
        /// </summary>
        public string StatusText
        {
            get
            {
                return ( string ) this.GetValue ( StatusTextProperty );
            }

            set
            {
                this.SetValue ( StatusTextProperty, value );
            }
        }

        /// <summary>
        /// Gets or sets the color of the status text.
        /// </summary>
        public Brush StatusTextColor
        {
            get
            {
                return ( Brush ) this.GetValue ( StatusTextColorProperty );
            }

            set
            {
                this.SetValue ( StatusTextColorProperty, value );
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Closes open files and hides the live view window when closing.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnMainWindowClosing ( object sender, System.ComponentModel.CancelEventArgs e )
        {
            ServiceProvider.ViewModel.HideLiveView();
        }

        /// <summary>
        /// Subscribes for log messages.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private async void OnLoaded ( object sender, RoutedEventArgs e )
        {
            await ServiceProvider.RemoteLink.SubscribeTo ( typeof ( LogSubscription ) );

            ServiceProvider.RemoteLink.PropertyChanged += this.OnConnectionManagerPropertyChanged;
        }

        /// <summary>
        /// Stops subscribing for log messages.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private async void OnUnloaded ( object sender, RoutedEventArgs e )
        {
            await ServiceProvider.RemoteLink.UnsubscribeFrom ( typeof ( LogSubscription ) );

            ServiceProvider.RemoteLink.PropertyChanged -= this.OnConnectionManagerPropertyChanged;
        }

        /// <summary>
        /// Updates the status text when the connection status changes.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnConnectionManagerPropertyChanged ( object sender, PropertyChangedEventArgs e )
        {
            if ( e.PropertyName == "IsConnected" || e.PropertyName == "IsRunning" )
            {
                if ( ServiceProvider.RemoteLink.IsRunning )
                {
                    if ( ServiceProvider.RemoteLink.IsConnected )
                    {
                        this.SetStatus ( "Running", Colors.Green );
                    }
                    else
                    {
                        this.SetStatus ( "Connecting", Colors.Gray );
                    }
                }
                else
                {
                    this.SetStatus ( "Ready", Colors.Gray );
                }
            }
        }

        /// <summary>
        /// Opens the log file directory selector.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        private void OnSaveToButtonClick ( object sender, RoutedEventArgs e )
        {
            FolderBrowserDialog fd = new FolderBrowserDialog();

            fd.SelectedPath = ServiceProvider.ViewModel.SaveTo;

            var result = fd.ShowDialog();

            if ( result == System.Windows.Forms.DialogResult.OK )
            {
                ServiceProvider.ViewModel.SaveTo = fd.SelectedPath;
            }
        }

        /// <summary>
        /// Starts or stops logging.
        /// </summary>
        /// <param name="sender">Event source.</param>
        /// <param name="e">Event arguments.</param>
        /// <remarks>File is always opened before connection.</remarks>
        private void OnStartButtonClick ( object sender, RoutedEventArgs e )
        {
            if ( ServiceProvider.OutputFile.IsOpen )
            {
                // File open; close the connection (if present), then close the file
                if ( ServiceProvider.RemoteLink.IsRunning )
                {
                    ServiceProvider.RemoteLink.Stop();
                }

                ServiceProvider.ViewModel.HideLiveView();
                ServiceProvider.ViewModel.Messages.Clear();
                string filename = ServiceProvider.OutputFile.CloseFile();
                System.Windows.MessageBox.Show ( "Wrote log to: " + filename, "Logging complete" );
            }
            else
            {
                // File is not open; open and connect if successful
                if ( ServiceProvider.OutputFile.OpenFile() )
                {
                    ServiceProvider.RemoteLink.Start();
                    this.SetStatus ( "Connecting", Colors.Gray );

                    // Show live view, if requested
                    if ( this.LiveViewCheckBox.IsChecked == true )
                    {
                        ServiceProvider.ViewModel.ShowLiveView();
                    }
                }
                else
                {
                    this.SetStatus ( "Failed to open file", Colors.DarkRed );
                }
            }
        }

        /// <summary>
        /// Sets the status text.
        /// </summary>
        /// <param name="status">The status text.</param>
        /// <param name="color">The color to use.</param>
        private void SetStatus ( string status, Color color )
        {
            this.StatusText = status;
            this.StatusTextColor = new SolidColorBrush ( color );
        }

        #endregion
    }
}
