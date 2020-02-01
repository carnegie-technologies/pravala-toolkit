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

namespace Pravala.Control
{
    using System;
    using System.Threading.Tasks;
    using Pravala.Protocol;

    /// <summary>
    /// Base class for control readers, allowing use across .NET and WinRT.
    /// </summary>
    public abstract class CtrlReader: DeserializerStream, IDisposable
    {
        /// <summary>
        /// Dispose resources created by CtrlReader.
        /// </summary>
        [ System.Diagnostics.CodeAnalysis.SuppressMessage ( "Microsoft.Design", "CA1063:ImplementIDisposableCorrectly",
                  Justification = "Abstract class" ) ]
        public abstract void Dispose();

        /// <summary>
        /// Load data from the input stream.
        /// </summary>
        /// <param name="count">The number of bytes to read.</param>
        /// <returns>The number of bytes that were read.</returns>
        public abstract Task<uint> LoadAsync ( uint count );
    }
}
