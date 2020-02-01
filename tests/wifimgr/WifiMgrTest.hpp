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

#include "wifimgr/WifiMgrMonitor.hpp"

namespace Pravala
{
/// @brief Used for testing WifiMgr by listening for operating system Wi-Fi network changes
/// on the system, then monitoring it all for changes.
class WifiMgrTest: public WifiMgrMonitor::Owner
{
    public:
        /// @brief Constructor
        /// @param [in] wpaSockName wpa_supplicant socket name
        WifiMgrTest ( const String & wpaSockName );

        /// @brief Destructor
        ~WifiMgrTest();

    protected:
        /// @brief Called when we receive a message informing that scan results are ready
        /// @param [in] monitor Calling WifiMgrMonitor object
        /// @param [in] results The list of scan results received
        virtual void wifiScanResultReady ( WifiMgrMonitor * monitor );

        /// @brief Called when we receive a Wi-Fi state change message
        /// @param [in] monitor Calling WifiMgrMonitor object
        /// @param [in] state New Wi-Fi state
        virtual void wifiStateChanged ( WifiMgrMonitor * monitor, WifiMgrTypes::State state );

    private:
        /// @brief WifiMgrMonitor used by this object
        WifiMgrMonitor _wmMonitor;
};
}
