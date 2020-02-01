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

#include "ConfigStringMultiSetList.hpp"

using namespace Pravala;

ConfigStringMultiSetList::ConfigStringMultiSetList ( uint8_t flags, const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cfgName, helpText )
{
}

ConfigStringMultiSetList::ConfigStringMultiSetList (
        const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" )
{
}

ConfigStringMultiSetList::ConfigStringMultiSetList (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, "" )
{
}

HashMap<String, StringList> ConfigStringMultiSetList::getValues() const
{
    HashMap<String, StringList> ret;

    if ( _value.isEmpty() )
    {
        // This is a multi-set list which is empty.
        // We want it to show up in the option list, so we need a single empty value.

        ret[ optName ].append ( "" );
    }
    else
    {
        // Here we are setting the entire list!
        ret.insert ( optName, _value );
    }

    return ret;
}

void ConfigStringMultiSetList::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigStringMultiSetList::loadOption ( const String &, const String & strValue, bool isDefault )
{
    _value.append ( strValue );

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

bool ConfigStringMultiSetList::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
