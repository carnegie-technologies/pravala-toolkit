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

#include "ConfigStringList.hpp"

using namespace Pravala;

ConfigStringList::ConfigStringList (
        uint8_t flags, const char * cfgName, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( flags, cfgName, helpText ),
    _separators ( separators )
{
    if ( defValue != 0 )
    {
        loadOption ( String::EmptyString, defValue, true );
    }
}

ConfigStringList::ConfigStringList (
        const char * cmdLineName, char cmdLineFlag, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue ),
    _separators ( separators )
{
    if ( defValue != 0 )
    {
        loadOption ( String::EmptyString, defValue, true );
    }
}

ConfigStringList::ConfigStringList (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue ),
    _separators ( separators )
{
    if ( defValue != 0 )
    {
        loadOption ( String::EmptyString, defValue, true );
    }
}

HashMap<String, StringList> ConfigStringList::getValues() const
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

void ConfigStringList::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigStringList::loadOption ( const String &, const String & strValue, bool isDefault )
{
    _value = strValue.split ( _separators );

    if ( isDefault )
    {
        _defaultValue = _value;
    }

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

bool ConfigStringList::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
