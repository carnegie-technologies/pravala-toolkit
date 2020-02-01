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

#include "WpaSuppCore.hpp"

namespace Pravala
{
class WifiMgrControlPriv: public WpaSuppCore
{
    protected:
        static const String SetNetworkPrefix;
        static const String RemoveNetworkPrefix;
        static const String EnableNetworkPrefix;
        static const String DisableNetworkPrefix;

        static const String AddNetworkCmd;
        static const String ScanCmd;
        static const String ListNetworksCmd;
        static const String ScanResultsCmd;
        static const String StatusCmd;
        static const String SaveCmd;

        static const String OkResult;
        static const String FailResult;

        /// @brief Constructor
        /// @param [in] ctrlName Name of the wpa supp socket
        WifiMgrControlPriv ( const String & ctrlName );

        /// @brief Destructor
        ~WifiMgrControlPriv();

        /// @brief Executes a SET_NETWORK command; and removes the network on failure.
        /// @param [in] id The ID of the network to control
        /// @param [in] params The parameters to set on this network. Must start with a space since we prepend
        /// the SET_NETWORK <id> in front of it.
        /// @return true on success; false on failure. On false, this network is removed before returning.
        bool executeSetNetworkCommand ( int id, const String & params );

        /// @brief Executes a command with a return code of either OK or FAIL.
        /// @param [in] cmd The command to execute
        /// return true if return was OK; false if return was FAIL or anything other than OK
        bool executeBoolCommand ( const String & cmd );

        friend class WifiMgrControl;
};
}
