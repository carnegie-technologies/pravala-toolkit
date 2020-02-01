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

#include "ConfigOpt.hpp"
#include "basic/IpAddress.hpp"

namespace Pravala
{
/// @brief Config option that holds a single IP address and port number pair
class ConfigAddrSpec: public ConfigOpt
{
    public:
        /// @brief Constructor
        /// It creates an option without the default value set
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        ConfigAddrSpec ( uint8_t flags, const char * name, const String & helpText );

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        ConfigAddrSpec ( const char * cmdLineName, char cmdLineFlag, const String & helpText );

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        ConfigAddrSpec (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText );

        /// @brief Constructor
        /// It creates an option which will have the default set
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        /// @param [in] defAddr Default address to set.
        /// @param [in] defPort Default port number to set.
        ConfigAddrSpec (
            uint8_t flags, const char * name, const String & helpText,
            const IpAddress & defAddr, uint16_t defPort );

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @param [in] defAddr Default address to set.
        /// @param [in] defPort Default port number to set.
        ConfigAddrSpec (
            const char * cmdLineName, char cmdLineFlag, const String & helpText,
            const IpAddress & defAddr, uint16_t defPort );

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] defAddr Default address to set.
        /// @param [in] defPort Default port number to set.
        ConfigAddrSpec (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText,
            const IpAddress & defAddr, uint16_t defPort );

        /// @brief Returns the value of this option
        /// @return the value of this option
        inline const SockAddr & value() const
        {
            return _addr;
        }

        /// @brief Returns the address value stored
        /// @return the value of this option
        inline IpAddress address() const
        {
            return _addr.getAddr();
        }

        /// @brief Returns the port value stored
        /// @return the value of this option
        inline uint16_t port() const
        {
            return _addr.getPort();
        }

        /// @brief Checks whether this option is set to non-empty value.
        /// @return True if this option is set to non-empty value.
        virtual bool isNonEmpty();

        /// @brief Returns a map representation of the option value(s)
        /// It simply returns a map with a single entry: opt_name:value_list.
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const;

    protected:
        SockAddr _defaultAddr; ///< Default address
        SockAddr _addr; ///< Current address

        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option.
        ///                      Note that using an empty string will cause the option to become 'unset'
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault );

        /// @brief Called just before the configuration is (re)loaded
        /// It restores the config option to its default value
        virtual void restoreDefaults();
};
}
