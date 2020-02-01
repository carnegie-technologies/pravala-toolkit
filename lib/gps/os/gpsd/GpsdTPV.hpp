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

#include "log/TextLog.hpp"
#include "json/Json.hpp"

#include "../../GpsTypes.hpp"

namespace Pravala
{
/// @brief Parses a JSON-formatted TPV (time-position-velocity) message from gpsd.
/// Format of the message can be found at http://catb.org/gpsd/gpsd_json.html.
/// This message is read from the gpsd socket, and contains information about the current location of the device.
class GpsdTPV
{
    public:
        /// @brief Constructor
        GpsdTPV();

        /// @brief Parses the TPV message
        /// @param [in] tpv The JSON message to decode. It must include a field, "class", set to "TPV".
        /// @return True on a successful parse; false otherwise.
        bool parse ( const Json & tpv );

        /// @brief Returns true if this TPV shows that there is a GPS lock
        /// @return True if this TPV shows that there is a GPS lock
        inline bool hasGpsLock() const
        {
            return _hasGpsLock;
        }

        /// @brief Retrieve the location data as parsed from the TPV message.
        /// @return Location object.
        inline const GpsCoordinate & getLocation() const
        {
            return _location;
        }

        /// @brief Retrieve the vector data as parsed from the TPV message.
        /// @return Vector object.
        inline const GpsVector & getVector() const
        {
            return _vector;
        }

        /// @brief Retrieve the name of the device the TPV message came from.
        /// @return Device name
        inline const String & getDeviceName() const
        {
            return _deviceName;
        }

        /// @brief Resets all of the fields.
        void clear();

    private:
        static TextLog _log; ///< Log

        bool _hasGpsLock; ///< True if this object shows that there is a GPS lock

        String _deviceName; ///< The name of the device found in the TPV message.

        GpsCoordinate _location; ///< The coordinate location in the TPV message.

        GpsVector _vector; ///< The set of vector fields in the TPV message.
};
}
