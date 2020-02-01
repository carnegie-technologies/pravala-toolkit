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
    using System.Windows.Data;

    /// <summary>
    /// Converter for the text on the stop/start button.
    /// </summary>
    public class StartStopValueConverter: IValueConverter
    {
        /// <summary>
        /// Modifies the source data before passing it to the target for display in the UI.
        /// </summary>
        /// <param name="value">The source data being passed to the target.</param>
        /// <param name="targetType">The type of the target property.</param>
        /// <param name="parameter">An optional parameter to be used in the converter logic.</param>
        /// <param name="language">The language of the conversion.</param>
        /// <returns>The value to be passed to the target dependency property.</returns>
        public object Convert ( object value, Type targetType, object parameter, CultureInfo language )
        {
            if ( value.GetType() == typeof ( bool ) )
            {
                bool val = ( bool ) value;

                return val ? "Stop" : "Start";
            }

            return "-";
        }

        /// <summary>
        /// Modifies the target data before passing it to the source object.
        /// This method is called only in TwoWay bindings.
        /// </summary>
        /// <param name="value">The source data being passed to the target.</param>
        /// <param name="targetType">The type of the target property.</param>
        /// <param name="parameter">An optional parameter to be used in the converter logic.</param>
        /// <param name="language">The language of the conversion.</param>
        /// <returns>The value to be passed to the source object.</returns>
        public object ConvertBack ( object value, Type targetType, object parameter, CultureInfo language )
        {
            throw new System.NotImplementedException();
        }
    }
}
