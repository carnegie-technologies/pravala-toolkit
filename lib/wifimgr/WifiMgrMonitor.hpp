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
#include "log/TextLog.hpp"

#include "WifiMgrTypes.hpp"

namespace Pravala
{
/// @brief User-exposed API for Wi-Fi management functions
///
/// Currently supported are:
/// - getting Wi-Fi scan results
/// - getting Wi-Fi state changes
///
/// Currently not supported are:
/// - setting Wi-Fi fields; as they are all blocking
///
/// Typical usage:
/// wmMon = WifiMgrMonitor( ... )
/// wmMon->getXXXX ( ... )
/// wait for callback to wifiMgrUpdate
///
class WifiMgrMonitor: public NoCopy
{
    public:
        /// @brief The class to be inherited/implemented by all owners of a WifiMgrMonitor
        class Owner
        {
            protected:
                /// @brief Called when we are able to query the system for Wi-Fi scan results without blocking
                /// @param [in] monitor Calling WifiMgrMonitor object
                virtual void wifiScanResultReady ( WifiMgrMonitor * monitor ) = 0;

                /// @brief Called when we receive a Wi-Fi state change message
                /// @param [in] monitor Calling WifiMgrMonitor object
                /// @param [in] state New Wi-Fi state
                virtual void wifiStateChanged ( WifiMgrMonitor * monitor, WifiMgrTypes::State state ) = 0;

                /// @brief Destructor - needed for Android
                virtual ~Owner()
                {
                }

                friend class WifiMgrMonitor;
                friend class WifiMgrMonitorPriv;
        };

        /// @brief Creates a new NetMgrMonitor object with a set of relevant notifications
        /// @param [in] owner Owner of this NetMgrMonitor socket
        /// @param [in] ctrlInfo Control information for configuring the monitor (i.e. name of wpa_supp socket)
        WifiMgrMonitor ( Owner & owner, const String & ctrlInfo );

        /// @brief Destructor
        ~WifiMgrMonitor();

    private:
        Owner & _owner; ///< The owner of the WifiMgrMonitor.

        static TextLog _log; ///< Log stream

        class WifiMgrMonitorPriv * _p; ///< Implementation-specific required fields
        friend class WifiMgrMonitorPriv;
};
}
