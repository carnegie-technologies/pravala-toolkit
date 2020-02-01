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

#include "ConfigIpAddress.hpp"

#include <cstdio>

using namespace Pravala;

ConfigIpAddress::ConfigIpAddress ( uint8_t flags, const char * name, const String & helpText ):
    ConfigOpt ( flags, name, helpText )
{
}

ConfigIpAddress::ConfigIpAddress ( const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" )
{
}

ConfigIpAddress::ConfigIpAddress (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, "" )
{
}

ConfigIpAddress::ConfigIpAddress (
        uint8_t flags, const char * name, const String & helpText, const IpAddress & defValue ):
    ConfigOpt ( flags, name, helpText ),
    _defaultValue ( defValue ), _value ( _defaultValue )
{
    if ( _defaultValue.isValid() )
    {
        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ConfigIpAddress::ConfigIpAddress (
        const char * cmdLineName, char cmdLineFlag, const String & helpText,
        const IpAddress & defValue ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue.toString() ),
    _defaultValue ( defValue ), _value ( _defaultValue )
{
    if ( _defaultValue.isValid() )
    {
        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ConfigIpAddress::ConfigIpAddress (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText, const IpAddress & defValue ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue.toString() ),
    _defaultValue ( defValue ), _value ( _defaultValue )
{
    if ( _defaultValue.isValid() )
    {
        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ERRCODE ConfigIpAddress::loadOption ( const String &, const String & strValue, bool isDefault )
{
    if ( strValue.isEmpty() )
    {
        _value.clear();
        _optFlags &= ~FlagIsSet;

        if ( isDefault )
        {
            _defaultValue.clear();
            _optFlags &= ~FlagIsDefaultSet;
        }

        return Error::Success;
    }

    const IpAddress addr ( strValue );

    if ( !addr.isValid() )
    {
        fprintf ( stderr, "Invalid IP address provided: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidAddress;
    }

    _value = addr;
    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue = _value;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

void ConfigIpAddress::restoreDefaults()
{
    _value = _defaultValue;
}

HashMap<String, StringList> ConfigIpAddress::getValues() const
{
    HashMap<String, StringList> ret;

    if ( _value.isValid() )
    {
        ret[ optName ].append ( _value.toString() );
    }

    return ret;
}

bool ConfigIpAddress::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && _value.isValid() );
}
