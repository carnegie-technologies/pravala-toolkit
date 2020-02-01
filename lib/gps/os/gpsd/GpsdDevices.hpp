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

#pragma once

#include "json/Json.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief Parses a JSON-formatted DEVICES message from gpsd.
/// The structure of the message can be found at http://catb.org/gpsd/gpsd_json.html
/// This message is read from the gpsd socket, and contains information about which GPS devices gpsd is monitoring.
class GpsdDevices
{
    public:
        /// @brief Parse the DEVICES message.
        /// @param [in] devices Must be the DEVICES message, with a 'class' field named 'DEVICES'
        /// and an array named 'devices'.
        /// @return True if the object was successfully parsed; false if any field is lacking or mismatched.
        bool parse ( const Json & devices );

        /// @brief Retrieve the list of devices parsed from the message.
        /// @return Names of all devices gpsd is monitoring.
        inline const List<String> & getDevices() const
        {
            return _devices;
        }

        /// @brief Reset the object.
        void clear();

    private:
        static TextLog _log; ///< Log.

        List<String> _devices; ///< The list of parsed devices.
};
}
