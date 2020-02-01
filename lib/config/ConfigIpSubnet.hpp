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
/// @brief Config option that holds a single IP subnet configuration (subnet address + netmask length)
class ConfigIpSubnet: public ConfigOpt
{
    public:
        /// @brief Constructor
        /// It creates an option without the default value set
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        ConfigIpSubnet ( uint8_t flags, const char * name, const String & helpText );

        /// @brief Constructor
        /// It creates an option which will have the default set
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        /// @param [in] defAddress Default subnet address to set
        /// @param [in] defPrefixLength Default prefix length to set
        ConfigIpSubnet (
            uint8_t flags, const char * name, const String & helpText,
            const IpAddress & defAddress, uint8_t defPrefixLength );

        /// @brief Returns the address of the subnet
        /// @return the address of the subnet
        inline const IpAddress & address() const
        {
            return _value.addr;
        }

        /// @brief Returns the address of the subnet
        /// @return the address of the subnet
        inline uint8_t prefixLength() const
        {
            return _value.prefixLength;
        }

        /// @brief Checks whether this option is set to non-empty value.
        /// @return True if this option is set to non-empty value.
        virtual bool isNonEmpty();

        /// @brief Returns a map representation of the option value(s)
        /// It simply returns a map with a single entry: opt_name:value_list.
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const;

    protected:
        /// @brief Subnet configuration
        struct SubnetData
        {
            IpAddress addr; ///< Address of the subnet
            uint8_t prefixLength; ///< Prefix length

            /// @brief Default constructor
            SubnetData(): prefixLength ( 0 )
            {
            }
        };

        SubnetData _defaultValue; ///< Default value
        SubnetData _value; ///< Current value

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
