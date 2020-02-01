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

#include "ConfigAddrSpecList.hpp"

#include <cstdio>

using namespace Pravala;

ConfigAddrSpecList::ConfigAddrSpecList (
        uint8_t flags, const char * cfgName, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( flags, cfgName, helpText ),
    _separators ( String ( separators ).removeChars ( ":" ) )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( cfgName, defValue, true );
    }
}

ConfigAddrSpecList::ConfigAddrSpecList (
        const char * cmdLineName, char cmdLineFlag, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue ),
    _separators ( String ( separators ).removeChars ( ":" ) )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( "", defValue, true );
    }
}

ConfigAddrSpecList::ConfigAddrSpecList (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText, const char * separators, const char * defValue ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue ),
    _separators ( String ( separators ).removeChars ( ":" ) )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( cfgName, defValue, true );
    }
}

HashMap<String, StringList> ConfigAddrSpecList::getValues() const
{
    String desc;
    char s[ 2 ];

    s[ 0 ] = ' ';
    s[ 1 ] = 0;

    if ( _separators.length() > 0 )
        s[ 0 ] = _separators[ 0 ];

    for ( size_t i = 0; i < _value.size(); ++i )
    {
        if ( !desc.isEmpty() )
        {
            desc.append ( s );
        }

        desc.append ( _value.at ( i ).toString() );
    }

    HashMap<String, StringList> ret;

    ret[ optName ].append ( desc );

    return ret;
}

void ConfigAddrSpecList::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigAddrSpecList::loadOption ( const String &, const String & strValue, bool isDefault )
{
    const StringList strList ( strValue.split ( _separators ) );

    if ( strList.isEmpty() )
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

    List<SockAddr> valList;

    for ( size_t i = 0; i < strList.size(); ++i )
    {
        const String s = strList.at ( i );

        SockAddr a;

        if ( !SockAddr::convertAddrSpec ( s, a ) )
        {
            fprintf ( stderr, "Invalid address specification provided: %s = %s\n", optName.c_str(), s.c_str() );
            return Error::InvalidData;
        }

        valList.append ( a );
    }

    _value = valList;
    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue = _value;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

bool ConfigAddrSpecList::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
