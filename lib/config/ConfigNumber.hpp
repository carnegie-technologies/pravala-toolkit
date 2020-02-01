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

namespace Pravala
{
/// @brief Config option that holds a single numeric value
/// @tparam T The type of the value
template<typename T> class ConfigNumber: public ConfigOpt
{
    public:
        /// @brief Constructor.
        /// It registers this option only as a config file parameter.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cfgName The name (or name filter) for this option.
        /// @param [in] helpText The help text for this option.
        ConfigNumber ( uint8_t flags, const char * cfgName, const String & helpText ):
            ConfigOpt ( flags, cfgName, helpText ), _defaultValue ( 0 ), _value ( 0 )
        {
        }

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        ConfigNumber ( const char * cmdLineName, char cmdLineFlag, const String & helpText ):
            ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" ),
            _defaultValue ( 0 ), _value ( 0 )
        {
        }

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        ConfigNumber (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText ):
            ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, "" ),
            _defaultValue ( 0 ), _value ( 0 )
        {
        }

        /// @brief Constructor.
        /// It registers this option only as a config file parameter.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cfgName The name (or name filter) for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] defValue Default value to set.
        ConfigNumber ( uint8_t flags, const char * cfgName, const String & helpText, T defValue ):
            ConfigOpt ( flags, cfgName, helpText ), _defaultValue ( defValue ), _value ( defValue )
        {
            _optFlags |= FlagIsSet;
            _optFlags |= FlagIsDefaultSet;
        }

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @param [in] defValue Default value to set.
        ConfigNumber ( const char * cmdLineName, char cmdLineFlag, const String & helpText, T defValue ):
            ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, String::number ( defValue ) ),
            _defaultValue ( defValue ), _value ( defValue )
        {
            _optFlags |= FlagIsSet;
            _optFlags |= FlagIsDefaultSet;
        }

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] defValue Default value to set.
        ConfigNumber (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText, T defValue ):
            ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, String::number ( defValue ) ),
            _defaultValue ( defValue ), _value ( defValue )
        {
            _optFlags |= FlagIsSet;
            _optFlags |= FlagIsDefaultSet;
        }

        /// @brief Returns the value of this option
        /// @return the value of this option
        inline T value() const
        {
            return _value;
        }

        /// @brief Cast operator
        /// @return the value of this option
        inline operator T() const
        {
            return _value;
        }

        /// @brief Returns default value of this option
        /// @return default value of this option
        inline T defaultValue() const
        {
            return _defaultValue;
        }

        /// @brief Returns a map representation of the option value(s)
        /// It simply returns a map with a single entry: opt_name:value_list.
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const
        {
            HashMap<String, StringList> ret;

            if ( isSet() )
            {
                ret[ optName ].append ( String::number ( _value ) );
            }
            else
            {
                ret[ optName ].append ( "" );
            }

            return ret;
        }

        /// @brief Sets the new value.
        /// It will also set the 'is set' flag.
        /// @param [in] val Value to set
        /// @return Standard error code
        virtual ERRCODE setValue ( T val )
        {
            _value = val;
            _optFlags |= FlagIsSet;
            return Error::Success;
        }

    protected:
        T _defaultValue; ///< Default value
        T _value; ///< Current value

        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] value The value of the config option
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String &, const String & strValue, bool isDefault )
        {
            if ( !strValue.toNumber ( _value ) )
            {
                return Error::InvalidData;
            }

            _optFlags |= FlagIsSet;

            if ( isDefault )
            {
                _defaultValue = _value;
                _optFlags |= FlagIsDefaultSet;
            }

            return Error::Success;
        }

        /// @brief Called just before the configuration is (re)loaded
        /// It restores the config option to its default value
        virtual void restoreDefaults()
        {
            _value = _defaultValue;
        }
};

/// @brief Config option that holds a single numeric value with range limitations
/// @tparam T The type of the value
template<typename T> class ConfigLimitedNumber: public ConfigNumber<T>
{
    public:
        const T minValue; ///< Min legal value
        const T maxValue; ///< Max legal value

        /// @brief Constructor.
        /// It registers this option only as a config file parameter.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cfgName The name (or name filter) for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        ConfigLimitedNumber (
            uint8_t flags, const char * cfgName, const String & helpText,
            T minVal, T maxVal ):
            ConfigNumber<T> ( flags, cfgName, String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ) ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        ConfigLimitedNumber (
            const char * cmdLineName, char cmdLineFlag, const String & helpText,
            T minVal, T maxVal ):
            ConfigNumber<T> (
                    cmdLineName, cmdLineFlag, String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ) ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        ConfigLimitedNumber (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText,
            T minVal, T maxVal ):
            ConfigNumber<T> (
                    flags,
                    cmdLineName,
                    cmdLineFlag,
                    cfgName,
                    String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ) ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Constructor.
        /// It registers this option only as a config file parameter.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cfgName The name (or name filter) for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        /// @param [in] defValue Default value to set.
        ConfigLimitedNumber (
            uint8_t flags, const char * cfgName, const String & helpText,
            T minVal, T maxVal, T defValue ):
            ConfigNumber<T> (
                    flags, cfgName, String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ), defValue ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        /// @param [in] defValue Default value to set.
        ConfigLimitedNumber (
            const char * cmdLineName, char cmdLineFlag, const String & helpText,
            T minVal, T maxVal, T defValue ):
            ConfigNumber<T> (
                    cmdLineName,
                    cmdLineFlag,
                    String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ),
                    defValue ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] minVal Min legal value.
        /// @param [in] maxVal Max legal value.
        /// @param [in] defValue Default value to set.
        ConfigLimitedNumber (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText,
            T minVal, T maxVal, T defValue ):
            ConfigNumber<T> (
                    flags,
                    cmdLineName,
                    cmdLineFlag,
                    cfgName,
                    String ( "%1 [%2-%3]" ).arg ( helpText ).arg ( minVal ).arg ( maxVal ),
                    defValue ),
            minValue ( minVal ), maxValue ( maxVal )
        {
        }

        /// @brief Returns true if the given value is within limits
        /// @param [in] val Value to check
        /// @return True if the given value is within allowed range
        inline bool withinLimits ( T val ) const
        {
            return ( val >= minValue && val <= maxValue );
        }

        /// @brief Sets the new value
        /// @param [in] val Value to set
        /// @return Standard error code
        virtual ERRCODE setValue ( T val )
        {
            if ( !withinLimits ( val ) )
                return Error::FieldValueOutOfRange;

            return ConfigNumber<T>::setValue ( val );
        }

    protected:
        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String & /*name*/, const String & strValue, bool isDefault )
        {
            T tmpVal;

            if ( !strValue.toNumber ( tmpVal ) )
            {
                return Error::InvalidData;
            }

            if ( !withinLimits ( tmpVal ) )
                return Error::FieldValueOutOfRange;

            ConfigNumber<T>::_value = tmpVal;
            this->_optFlags |= ConfigOpt::FlagIsSet;

            if ( isDefault )
            {
                ConfigNumber<T>::_defaultValue = tmpVal;
                this->_optFlags |= ConfigOpt::FlagIsDefaultSet;
            }

            return Error::Success;
        }
};
}
