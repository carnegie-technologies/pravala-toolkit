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

#include "ConfigIpSubnet.hpp"

#include <cstdio>

using namespace Pravala;

ConfigIpSubnet::ConfigIpSubnet ( uint8_t flags, const char * name, const String & helpText ):
    ConfigOpt ( flags, name, helpText )
{
}

ConfigIpSubnet::ConfigIpSubnet (
        uint8_t flags, const char * name, const String & helpText,
        const IpAddress & defAddress, uint8_t defPrefixLength ):
    ConfigOpt ( flags, name, helpText )
{
    _defaultValue.addr = defAddress.getNetworkAddress ( defPrefixLength );

    if ( _defaultValue.addr.isIPv4() )
    {
        _defaultValue.prefixLength = ( defPrefixLength < 32 ) ? defPrefixLength : 32;
    }
    else if ( _defaultValue.addr.isIPv6() )
    {
        _defaultValue.prefixLength = ( defPrefixLength < 128 ) ? defPrefixLength : 128;
    }

    _value = _defaultValue;

    if ( _defaultValue.addr.isValid() )
    {
        _optFlags |= FlagIsSet;
        _optFlags |= FlagIsDefaultSet;
    }
}

ERRCODE ConfigIpSubnet::loadOption ( const String &, const String & strValue, bool isDefault )
{
    if ( strValue.isEmpty() )
    {
        _value.addr.clear();
        _value.prefixLength = 0;

        _optFlags &= ~FlagIsSet;

        if ( isDefault )
        {
            _defaultValue = _value;
            _optFlags &= ~FlagIsDefaultSet;
        }

        return Error::Success;
    }

    const StringList vals = strValue.split ( "/", true );

    if ( vals.size() != 2 || vals.at ( 0 ).isEmpty() || vals.at ( 1 ).isEmpty() )
    {
        fprintf ( stderr, "Invalid subnet specified: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidData;
    }

    const IpAddress addr ( vals.at ( 0 ) );

    if ( !addr.isValid() || ( !addr.isIPv4() && !addr.isIPv6() ) )
    {
        fprintf ( stderr, "Invalid subnet address provided: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidAddress;
    }

    bool ok = false;
    const uint8_t prefLen = vals.at ( 1 ).toUInt8 ( &ok );

    if ( !ok || ( addr.isIPv4() && prefLen > 32 ) || ( addr.isIPv6() && prefLen > 128 ) )
    {
        fprintf ( stderr, "Invalid subnet prefix length provided: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidData;
    }

    if ( addr != addr.getNetworkAddress ( prefLen ) )
    {
        fprintf ( stderr, "Invalid subnet address given prefix length used: %s = %s\n",
                  optName.c_str(), strValue.c_str() );
        return Error::InvalidData;
    }

    _value.addr = addr;
    _value.prefixLength = prefLen;

    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue = _value;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

void ConfigIpSubnet::restoreDefaults()
{
    _value = _defaultValue;
}

HashMap<String, StringList> ConfigIpSubnet::getValues() const
{
    HashMap<String, StringList> ret;

    if ( _value.addr.isValid() && _value.prefixLength > 0 )
    {
        String desc ( _value.addr.toString() );
        desc.append ( "/" );
        desc.append ( String::number ( _value.prefixLength ) );

        ret[ optName ].append ( desc );
    }

    return ret;
}

bool ConfigIpSubnet::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && _value.addr.isValid() );
}
