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

#include "config/ConfigString.hpp"
#include "config/ConfigStringMultiSetList.hpp"

namespace Pravala
{
/// @brief A convenience class for creating options to configure a single control socket in client mode.
class ConfigCtrlSock: public ConfigString
{
    public:
        /// @brief Constructor.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option in the config file.
        /// @param [in] ownerName The name of the object that will be handling does connections.
        ///                        This is used in the help text. For example: "Foo Bar Daemon"
        ConfigCtrlSock (
            const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const char * ownerName );

        /// @brief Constructor.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option in the config file.
        /// @param [in] ownerName The name of the object that will be handling does connections.
        ///                        This is used in the help text. For example: "Foo Bar Daemon"
        ConfigCtrlSock (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const char * ownerName );

    protected:
        virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault );
};

/// @brief A convenience class for creating options to configure control listeners.
class ConfigCtrlListeners: public ConfigStringMultiSetList
{
    public:
        /// @brief Constructor.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option in the config file.
        /// @param [in] ownerName The name of the object that will be handling does connections.
        ///                        This is used in the help text. For example: "Foo Bar Daemon"
        ConfigCtrlListeners (
            const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const char * ownerName );

        /// @brief Constructor.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option in the config file.
        /// @param [in] ownerName The name of the object that will be handling does connections.
        ///                        This is used in the help text. For example: "Foo Bar Daemon"
        ConfigCtrlListeners (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const char * ownerName );

        /// @brief Returns first non-empty control listener's address.
        /// @return First non-empty control listener's address, or an empty string if there are no values
        ///          or all of them are empty.
        const String getFirstValue();

        /// @brief Returns last non-empty control listener's address.
        /// @return Last non-empty control listener's address, or an empty string if there are no values
        ///          or all of them are empty.
        const String getLastValue();

    protected:
        virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault );
};
}
