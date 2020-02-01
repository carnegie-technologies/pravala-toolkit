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

#include "basic/NoCopy.hpp"
#include "basic/String.hpp"

#include "GpsTypes.hpp"

namespace Pravala
{
/// @brief Monitor the GPS of the system for location changes.
class GpsMonitor: public NoCopy
{
    public:
        /// @brief Callbacks used to signal to the owner when things have changed.
        class Receiver
        {
            protected:
                /// @brief The data associated with the GPS device is available.
                /// @param [in] location The location as measured by the GPS device.
                /// @param [in] vector The vector as measured by the GPS device.
                virtual void gpsUpdate ( const GpsCoordinate & location, const GpsVector & vector );

                /// @brief The devices watching for GPS data has changed.
                /// @param [in] devices The list of devices being used.
                virtual void gpsDevicesChanged ( const List<String> & devices );

                /// @brief Virtual destructor. Required for Android builds.
                virtual ~Receiver()
                {
                }

                friend class GpsMonitor;
                friend class GpsMonitorPriv;
        };

        /// @brief Constructor.
        /// @param [in] receiver The object to notify about changes in GPS data.
        GpsMonitor ( Receiver & receiver );

        /// @brief Destructor.
        ~GpsMonitor();

        /// @brief Start monitoring for GPS changes.
        /// On some platforms this will create a socket to the GPS watching daemon.
        /// @return true on successful starting; false on error.
        bool start();

        /// @brief Stop monitoring for GPS changes.
        void stop();

    private:
        Receiver & _receiver; ///< The object to notify about changes.

        class GpsMonitorPriv * _p; ///< Implementation-specific required fields

        friend class GpsMonitorPriv;
};
}
