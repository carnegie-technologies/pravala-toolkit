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

#include "ConfigString.hpp"

using namespace Pravala;

ConfigString::ConfigString ( uint8_t flags, const char * cfgName, const String & helpText, const char * defValue ):
    ConfigOpt ( flags, cfgName, helpText )
{
    if ( defValue != 0 )
    {
        _value = defValue;

        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ConfigString::ConfigString (
        const char * cmdLineName, char cmdLineFlag, const String & helpText, const char * defValue ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue )
{
    if ( defValue != 0 )
    {
        _value = defValue;

        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ConfigString::ConfigString (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText, const char * defValue ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue )
{
    if ( defValue != 0 )
    {
        _value = defValue;

        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ERRCODE ConfigString::loadOption ( const String &, const String & strValue, bool isDefault )
{
    _value = strValue;

    if ( _value.length() > 0 )
    {
        _optFlags |= FlagIsSet;
    }
    else
    {
        _optFlags &= ~FlagIsSet;
    }

    if ( isDefault )
    {
        _defaultValue = _value;

        if ( _value.length() > 0 )
        {
            _optFlags |= FlagIsDefaultSet;
        }
        else
        {
            _optFlags &= ~FlagIsDefaultSet;
        }
    }

    return Error::Success;
}

void ConfigString::restoreDefaults()
{
    _value = _defaultValue;
}

HashMap<String, StringList> ConfigString::getValues() const
{
    HashMap<String, StringList> ret;

    ret[ optName ].append ( _value );

    return ret;
}

bool ConfigString::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
