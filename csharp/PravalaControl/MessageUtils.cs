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

    /// <summary>
    /// Contains helper methods for working with control messages.
    /// </summary>
    public class MessageUtils
    {
        /// <summary>
        /// Converts the number of seconds since epoch to DataTime object.
        /// </summary>
        /// <param name="unixTime">The number of seconds since epoch.</param>
        /// <returns>The equivalent DateTime object.</returns>
        public static DateTime FromUnixTime ( ulong unixTime )
        {
            var epoch = new DateTime ( 1970, 1, 1, 0, 0, 0, DateTimeKind.Local );

            return epoch.AddSeconds ( unixTime );
        }
    }
}
