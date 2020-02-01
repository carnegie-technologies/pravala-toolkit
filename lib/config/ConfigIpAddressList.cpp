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

#include "ConfigIpAddressList.hpp"

#include <cstdio>

using namespace Pravala;

ConfigIpAddressList::ConfigIpAddressList (
        uint8_t flags, const char * cfgName, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( flags, cfgName, helpText ), _separators ( separators )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( cfgName, defValue, true );
    }
}

ConfigIpAddressList::ConfigIpAddressList (
        const char * cmdLineName, char cmdLineFlag, const String & helpText,
        const char * separators, const char * defValue ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, defValue ), _separators ( separators )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( "", defValue, true );
    }
}

ConfigIpAddressList::ConfigIpAddressList (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText, const char * separators, const char * defValue ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, defValue ), _separators ( separators )
{
    if ( defValue != 0 )
    {
        const String sVal ( defValue );

        if ( !sVal.isEmpty() )
            loadOption ( cfgName, defValue, true );
    }
}

HashMap<String, StringList> ConfigIpAddressList::getValues() const
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

void ConfigIpAddressList::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigIpAddressList::loadOption ( const String &, const String & strValue, bool isDefault )
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

    List<IpAddress> addrList;

    for ( size_t i = 0; i < strList.size(); ++i )
    {
        const IpAddress addr ( strList.at ( i ) );

        if ( !addr.isValid() )
        {
            fprintf ( stderr, "Invalid IP address provided: %s = %s\n", optName.c_str(), strList.at ( i ).c_str() );
            return Error::InvalidAddress;
        }

        addrList.append ( addr );
    }

    _value = addrList;
    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue = _value;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

bool ConfigIpAddressList::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}

void ConfigIpAddressList::setValue ( const List<IpAddress> & val )
{
    _value = val;
    _optFlags |= FlagIsSet;
}
