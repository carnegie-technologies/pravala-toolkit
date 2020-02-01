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

#include "ConfigIpAddressMultiSetList.hpp"

#include <cstdio>

using namespace Pravala;

ConfigIpAddressMultiSetList::ConfigIpAddressMultiSetList (
        uint8_t flags, const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cfgName, helpText )
{
}

ConfigIpAddressMultiSetList::ConfigIpAddressMultiSetList (
        const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" )
{
}

ConfigIpAddressMultiSetList::ConfigIpAddressMultiSetList (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, "" )
{
}

HashMap<String, StringList> ConfigIpAddressMultiSetList::getValues() const
{
    HashMap<String, StringList> ret;
    StringList strList;

    for ( size_t i = 0; i < _value.size(); ++i )
    {
        strList.append ( _value.at ( i ).toString() );
    }

    if ( strList.isEmpty() )
    {
        // This is a multi-set list which is empty.
        // We want it to show up in the option list, so we need a single empty value.

        strList.append ( "" );
    }

    // Here we are setting the entire list!
    ret.insert ( optName, strList );

    return ret;
}

void ConfigIpAddressMultiSetList::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigIpAddressMultiSetList::loadOption ( const String &, const String & strValue, bool isDefault )
{
    const IpAddress addr ( strValue );

    if ( !addr.isValid() )
    {
        fprintf ( stderr, "Invalid IP address provided: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidAddress;
    }

    _value.append ( addr );

    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue = _value;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

bool ConfigIpAddressMultiSetList::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
