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
/// @brief A synchronous API for settings Wi-Fi management options
class WifiMgrControl: public NoCopy
{
    public:
        /// @brief Constructor
        /// @param [in] ctrlInfo Information controlling which Wi-Fi radio to manage
        WifiMgrControl ( const String & ctrlInfo );

        /// @brief Destructor
        ~WifiMgrControl();

        /// @brief Retrieve the networks which the Wi-Fi supplicant is managing
        /// @param [out] networks The retrieved networks
        /// @return standard error code
        ERRCODE getConfiguredNetworks ( List<WifiMgrTypes::NetworkConfiguration> & networks );

        /// @brief Retrieve the networks available to connect to (i.e. in range now)
        /// @param [out] networks The available networks
        /// @return standard error code
        ERRCODE getAvailableNetworks ( List<WifiMgrTypes::NetworkInstance> & networks );

        /// @brief Retrieve the current Wi-Fi network status. If state != Connected, all other fields invalid
        /// @param [out] status The retrieved status
        /// @return standard error code
        ERRCODE getStatus ( WifiMgrTypes::Status & status );

        /// @brief Retrieve the current state of the Wi-Fi radio.
        /// @param [out] state The state
        /// @return standard error code
        ERRCODE getState ( WifiMgrTypes::State & state );

        /// @brief Add a Wi-Fi network to the system. If improperly formatted, it is not added
        /// @param [in] network The network to add
        /// @param [in] enable Enable the network after adding (on supported platforms)
        /// @return standard error code
        ERRCODE addNetwork ( const WifiMgrTypes::NetworkProfile & network, bool enable = true );

        /// @brief Remove a Wi-Fi network configuration from the system. If not present, nothing will happen.
        /// @param [in] ssid The SSID to remove. We can't be more precise because security type isn't available
        /// on all platforms.
        /// @return standard error code
        ERRCODE removeNetwork ( const String & ssid );

        /// @brief Enable a network with the given id (on supported platforms)
        /// @param [in] id Id of network to enable
        /// @return standard error code
        ERRCODE enableNetwork ( int id );

        /// @brief Disable a network with the given id (on supported platforms)
        /// @param [in] id Id of network to disable
        /// @return standard error code
        ERRCODE disableNetwork ( int id );

        /// @brief Disable all networks (on supported platforms)
        /// @return standard error code
        ERRCODE disableAllNetworks();

        /// @brief Request a network scan
        /// @return standard error code
        ERRCODE scan();

    private:
        static TextLog _log; ///< Log stream

        class WifiMgrControlPriv * _p; ///< A private field containing platform-specific fields.
        friend class WifiMgrControlPriv;
};
}
