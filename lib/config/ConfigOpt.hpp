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

#include "basic/String.hpp"
#include "error/Error.hpp"

namespace Pravala
{
/// @brief A base class for all config options
class ConfigOpt
{
    public:
        /// @brief Marks options that can only be set using built-in config.
        /// This flag CAN be passed to ConfigOpt constructor.
        static const uint8_t FlagBuiltInOnly = ( 1 << 0 );

        /// @brief Marks options that can only be set using built-in config or during
        /// initial config initialization using config file.
        /// Reloading the config will not affect the option.
        /// This flag CAN be passed to ConfigOpt constructor.
        static const uint8_t FlagInitializeOnly = ( 1 << 1 );

        /// @brief Is this a 'common' option
        /// This is used to mark what config file options are commonly changed
        /// and should be present in the 'common' example config (e.g. IP addresses, file paths).
        static const uint8_t FlagIsCommon = ( 1 << 2 );

        /// @brief Whether this option is a simple command-line switch that doesn't take arguments.
        /// This flag is ignored if this option is configured without command line flag and name.
        static const uint8_t FlagIsSimpleSwitch = ( 1 << 3 );

        /// @brief When set this option needs to be initialized to non-empty value.
        /// Only works with command-line options.
        static const uint8_t FlagReqNonEmpty = ( 1 << 4 );

        /// @brief Whether the option is currently set
        /// This flag is IGNORED by the constructor (filtered out).
        static const uint8_t FlagIsSet = ( 1 << 5 );

        /// @brief Whether the option's default value is set
        /// This flag is IGNORED by the constructor (filtered out).
        static const uint8_t FlagIsDefaultSet = ( 1 << 6 );

        const String optName; ///< The name of the option (or the filter)
        const String optHelpText; ///< The help text for this option

        const char optCmdLineFlag; ///< The flag of this option when used from command line, or 0
        const String optCmdLineName; ///< The name of the option when used from command line, or empty

        const String optCmdLineDefault; ///< The default value (for command line help)

        /// @brief Returns true if this option has been set using; False otherwise
        /// @return True if this option has been set; False otherwise
        inline bool isSet() const
        {
            return ( _optFlags & FlagIsSet ) != 0;
        }

        /// @brief Checks whether this option is set to non-empty value.
        /// The default implementation only checks if it is set.
        /// @return True if this option is set to non-empty value.
        virtual bool isNonEmpty();

        /// @brief Destructor
        virtual ~ConfigOpt();

    protected:
        uint8_t _optFlags; ///< The flags of this option

        /// @brief Constructor.
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name (or name filter) for this option
        /// @param [in] helpText The help text for this option
        ConfigOpt ( uint8_t flags, const char * name, const String & helpText );

        /// @brief Constructor.
        /// It registers this object as a command line option if cmdLineName is not empty or cmdLineFlag is not 0.
        /// It registers this object as a config file option if cfgName is not empty.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The config name (or name filter) for this option.
        ///                   If it is empty this option will NOT be registered as a config-file option.
        /// @param [in] helpText The help text for this option
        /// @param [in] defValue Default value. Its content will be described in help message.
        ConfigOpt (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText, const String & defValue );

        /// @brief Called just before the configuration is (re)loaded
        /// It should restore the config option to its default value
        virtual void restoreDefaults() = 0;

        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault ) = 0;

        /// @brief Called in all options every time the configuration is updated.
        /// The default implementation does nothing.
        virtual void configUpdated();

        /// @brief Returns a map representation of the option value(s)
        /// Each entry in the map is in the form of "option_name:value_list".
        /// In case of simple options it will only contain a single entry hash map entry with a list of size 1.
        /// If this is a multi-set option, that list may contain several elements.
        /// However, complex options (that use a filter instead of the name) can return the entire list
        /// of configured name:value_list pairs.
        /// @note Each class that inherits ConfigOpt should provide a function that returns
        /// the specific type, and code using those options should use that function.
        /// getValues() is only (rarely) used by ConfigMgr for printing the values stored in specific options
        /// using printSampleConfig() and printOptions().
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const = 0;

        /// @brief Returns the list of all registered config options
        /// @return the list of all registered config options
        static List<ConfigOpt *> & cfgOptions();

        /// @brief Returns the list of all registered command line options
        /// @return the list of all registered command line options
        static List<ConfigOpt *> & cmdLineOptions();

        friend class ConfigCore;
};
}
