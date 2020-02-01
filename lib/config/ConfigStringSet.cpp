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

#include "ConfigStringSet.hpp"

using namespace Pravala;

ConfigStringSet::ConfigStringSet (
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

ConfigStringSet::ConfigStringSet (
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

ConfigStringSet::ConfigStringSet (
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

HashMap<String, StringList> ConfigStringSet::getValues() const
{
    String sep;

    if ( _separators.length() > 0 )
    {
        sep = " ";
    }
    else
    {
        sep.append ( _separators[ 0 ] );
    }

    String vals;

    for ( HashSet<String>::Iterator it ( _value ); it.isValid(); it.next() )
    {
        if ( !vals.isEmpty() )
            vals.append ( sep );

        vals.append ( it.value() );
    }

    HashMap<String, StringList> ret;

    ret[ optName ].append ( vals );

    return ret;
}

void ConfigStringSet::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigStringSet::loadOption ( const String &, const String & strValue, bool isDefault )
{
    StringList vals = strValue.split ( _separators );

    _value.clear();

    for ( size_t i = 0; i < vals.size(); ++i )
    {
        _value.insert ( vals.at ( i ) );
    }

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

bool ConfigStringSet::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
