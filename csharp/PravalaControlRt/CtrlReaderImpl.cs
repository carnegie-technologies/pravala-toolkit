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

namespace Pravala.Control.Rt
{
    using System;
    using System.Threading.Tasks;
    using Windows.Storage.Streams;

    /// <summary>
    /// WinRT implementation of CtrlReader.
    /// </summary>
    internal class CtrlReaderImpl: CtrlReader
    {
        #region Fields
        /// <summary>
        /// The data reader used for reading from the input stream.
        /// </summary>
        private readonly DataReader dataReader;
        #endregion

        #region Constructor
        /// <summary>
        /// Initializes a new instance of the CtrlReaderImpl class.
        /// </summary>
        /// <param name="dataReader">The data reader to use.</param>
        internal CtrlReaderImpl ( DataReader dataReader )
        {
            this.dataReader = dataReader;
        }

        #endregion

        #region Properties
        /// <summary>
        /// Returns the number of bytes available in the stream.
        /// </summary>
        public override uint AvailableBytes
        {
            get
            {
                return this.dataReader.UnconsumedBufferLength;
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Reads a single byte from the stream.
        /// </summary>
        /// <returns>The byte read.</returns>
        public override byte ReadByte()
        {
            return this.dataReader.ReadByte();
        }

        /// <summary>
        /// Reads multiple bytes from the stream.
        /// </summary>
        /// <param name="output">The buffer to fill with the data from the stream.</param>
        public override void ReadBytes ( byte[] output )
        {
            this.dataReader.ReadBytes ( output );
        }

        /// <summary>
        /// Loads data from the input stream.
        /// </summary>
        /// <param name="count">The number of bytes to read.</param>
        /// <returns>The asynchronous load data request.</returns>
        public override async Task<uint> LoadAsync ( uint count )
        {
            uint bytes = await this.dataReader.LoadAsync ( count );

            return bytes;
        }

        /// <summary>
        /// Dispose resources used by CtrlReader.
        /// </summary>
        public override void Dispose()
        {
            // Not used - CtrlReader doesn't create anything dispose-able.
        }

        #endregion
    }
}
