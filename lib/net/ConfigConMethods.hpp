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

#include "config/ConfigOpt.hpp"
#include "ConMethod.hpp"

namespace Pravala
{
/// @brief Config option that holds a number of ConMethods
class ConfigConMethods: public ConfigOpt
{
    public:
        /// @brief Constructor
        /// It creates an option without the default value set
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        /// @param [in] minExtraArgs The minimum number of extra ConMethod arguments
        ConfigConMethods ( uint8_t flags, const char * name, const String & helpText, size_t minExtraArgs );

        /// @brief Constructor
        /// It creates an option which will have the default set if defValue is not empty
        /// @param [in] flags The initial flags to set
        /// @param [in] name The name for this option
        /// @param [in] helpText The help text for this option
        /// @param [in] minExtraArgs The minimum number of extra ConMethod arguments
        /// @param [in] defValue Default value to set
        ConfigConMethods (
            uint8_t flags, const char * name, const String & helpText, size_t minExtraArgs,
            const char * defValue );

        /// @brief Returns the value of this option
        /// @return the value of this option
        inline const List<ConMethod> & value() const
        {
            return _value;
        }

        /// @brief Returns a map representation of the option value(s)
        /// It simply returns a map with a single entry: opt_name:value_list.
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const;

    protected:
        const size_t _minExtraArgs; ///< The minimum number of extra ConMethod arguments
        List<ConMethod> _defaultValue; ///< Default value
        List<ConMethod> _value; ///< Current value

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
