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
/// @brief Config option that holds a list of numeric values
/// @tparam T The type of the value
template<typename T> class ConfigNumberList: public ConfigOpt
{
    public:
        /// @brief Constructor.
        /// It registers this option only as a config file parameter.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cfgName The name (or name filter) for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigNumberList (
            uint8_t flags, const char * cfgName, const String & helpText,
            const char * separators, const char * defValue = 0 ):
            ConfigOpt ( flags, cfgName, helpText ),
            _separators ( separators )
        {
            if ( defValue != 0 )
            {
                loadOption ( String::EmptyString, defValue, true );
            }
        }

        /// @brief Constructor.
        /// It registers this option only as a command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigNumberList (
            const char * cmdLineName, char cmdLineFlag, const String & helpText,
            const char * separators, const char * defValue = 0 ):
            ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue ),
            _separators ( separators )
        {
            if ( defValue != 0 )
            {
                loadOption ( String::EmptyString, defValue, true );
            }
        }

        /// @brief Constructor.
        /// It registers this option BOTH as a config file and command line parameter.
        /// At least one of cmdLineFlag or cmdLineName has to be valid.
        /// @param [in] flags The initial flags to set.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] cfgName The name for this option.
        /// @param [in] helpText The help text for this option.
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigNumberList (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText,
            const char * separators, const char * defValue = 0 ):
            ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue ),
            _separators ( separators )
        {
            if ( defValue != 0 )
            {
                loadOption ( String::EmptyString, defValue, true );
            }
        }

        /// @brief Returns the value of this option
        /// @return the value of this option
        inline const List<T> & value() const
        {
            return _value;
        }

        /// @brief Returns default value of this option
        /// @return default value of this option
        inline const List<T> & defaultValue() const
        {
            return _defaultValue;
        }

        /// @brief Checks whether the list is empty
        /// @return True if the list is empty, false otherwise
        inline bool isEmpty() const
        {
            return _value.isEmpty();
        }

        virtual bool isNonEmpty()
        {
            return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
        }

        virtual HashMap<String, StringList> getValues() const
        {
            HashMap<String, StringList> ret;

            char s[ 2 ];

            s[ 0 ] = ' ';
            s[ 1 ] = 0;

            if ( _separators.length() > 0 )
                s[ 0 ] = _separators[ 0 ];

            ret[ optName ].append ( String::join ( _value, s ) );

            return ret;
        }

    protected:
        const String _separators; ///< List of separators to use
        List<T> _defaultValue; ///< Default value
        List<T> _value; ///< Current value

        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option.
        /// Note that an empty value will unset this option.
        /// If there are invalid values in the list, this option will remain unchanged and an error will be returned.
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String &, const String & strValue, bool isDefault )
        {
            const StringList list = strValue.split ( _separators );
            List<T> tmpList;

            for ( size_t i = 0; i < list.size(); ++i )
            {
                T tmp;
                if ( !list.at ( i ).toNumber ( tmp ) )
                {
                    return Error::InvalidData;
                }

                tmpList.append ( tmp );
            }

            _value = tmpList;

            if ( !_value.isEmpty() )
            {
                _optFlags |= FlagIsSet;
                isDefault && ( _optFlags |= FlagIsDefaultSet );
            }
            else
            {
                _optFlags &= ~FlagIsSet;
                isDefault && ( _optFlags &= ~FlagIsDefaultSet );
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

/// @brief Config option that holds a list of numeric values with range limitations
/// @tparam T The type of the value
template<typename T> class ConfigLimitedNumberList: public ConfigNumberList<T>
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
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigLimitedNumberList (
            uint8_t flags, const char * cfgName, const String & helpText, T minVal, T maxVal,
            const char * separators, const char * defValue = 0 ):
            ConfigNumberList<T> ( flags, cfgName, helpText, separators, defValue ),
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
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigLimitedNumberList (
            const char * cmdLineName, char cmdLineFlag, const String & helpText,
            T minVal, T maxVal,
            const char * separators, const char * defValue = 0 ):
            ConfigNumberList<T> ( 0, cmdLineName, cmdLineFlag, "", helpText, separators, defValue ),
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
        /// @param [in] separators List of separators to use.
        /// @param [in] defValue Default value to set if not 0.
        ConfigLimitedNumberList (
            uint8_t flags, const char * cmdLineName, char cmdLineFlag,
            const char * cfgName, const String & helpText,
            T minVal, T maxVal,
            const char * separators, const char * defValue = 0 ):
            ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, separators, defValue ),
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

    protected:
        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option.
        /// Note that an empty value will unset this option.
        /// If there are invalid values in the list, this option will remain unchanged and an error will be returned.
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String &, const String & strValue, bool isDefault )
        {
            const StringList list = strValue.split ( ConfigNumberList<T>::_separators );
            List<T> tmpList;

            for ( size_t i = 0; i < list.size(); ++i )
            {
                T tmp;
                if ( !list.at ( i ).toNumber ( tmp ) )
                {
                    return Error::InvalidData;
                }

                if ( !withinLimits ( tmp ) )
                {
                    return Error::FieldValueOutOfRange;
                }

                tmpList.append ( tmp );
            }

            ConfigNumberList<T>::_value = tmpList;

            if ( !ConfigNumberList<T>::_value.isEmpty() )
            {
                ConfigNumberList<T>::_optFlags |= ConfigOpt::FlagIsSet;
                isDefault && ( ConfigNumberList<T>::_optFlags |= ConfigOpt::FlagIsDefaultSet );
            }
            else
            {
                ConfigNumberList<T>::_optFlags &= ~ConfigOpt::FlagIsSet;
                isDefault && ( ConfigNumberList<T>::_optFlags &= ~ConfigOpt::FlagIsDefaultSet );
            }

            return Error::Success;
        }
};
}
