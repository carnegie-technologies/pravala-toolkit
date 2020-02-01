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
    using Pravala.RemoteLog.Service;
    using Pravala.RemoteLog.View;

    /// <summary>
    /// The ServiceProvider provides access to UI services.
    /// </summary>
    public class ServiceProvider
    {
        #region Fields
        /// <summary>
        /// The current instance of the ServiceProvider.
        /// </summary>
        private static ServiceProvider instance;

        /// <summary>
        /// Remote link for communicating with protocol server.
        /// </summary>
        private RemoteLink remoteLink;

        /// <summary>
        /// View model for saving UI state.
        /// </summary>
        private ViewModel viewModel;

        /// <summary>
        /// The file we are outputting to.
        /// </summary>
        private OutputFile outputFile;
        #endregion

        #region Properties
        /// <summary>
        /// Gets the current instance of the ServiceProvider.
        /// </summary>
        public static ServiceProvider Instance
        {
            get
            {
                if ( instance == null )
                {
                    instance = new ServiceProvider();
                }

                return instance;
            }
        }

        /// <summary>
        /// Gets the remote link.
        /// </summary>
        public static RemoteLink RemoteLink
        {
            get
            {
                if ( Instance.remoteLink == null )
                {
                    Instance.remoteLink = new RemoteLink();
                }

                return Instance.remoteLink;
            }
        }

        /// <summary>
        /// Gets the view model.
        /// </summary>
        public static ViewModel ViewModel
        {
            get
            {
                if ( Instance.viewModel == null )
                {
                    Instance.viewModel = new ViewModel();
                }

                return Instance.viewModel;
            }
        }

        /// <summary>
        /// Gets the output file.
        /// </summary>
        /// <remarks>
        /// Must be opened before writing.
        /// </remarks>
        public static OutputFile OutputFile
        {
            get
            {
                if ( Instance.outputFile == null )
                {
                    Instance.outputFile = new OutputFile();
                }

                return Instance.outputFile;
            }
        }
        #endregion
    }
}
