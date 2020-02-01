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

#include "ConfigAddrSpec.hpp"

#include <cstdio>

using namespace Pravala;

ConfigAddrSpec::ConfigAddrSpec ( uint8_t flags, const char * name, const String & helpText ):
    ConfigOpt ( flags, name, helpText )
{
}

ConfigAddrSpec::ConfigAddrSpec ( const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" )
{
}

ConfigAddrSpec::ConfigAddrSpec (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText, "" )
{
}

ConfigAddrSpec::ConfigAddrSpec (
        uint8_t flags, const char * name, const String & helpText,
        const IpAddress & defAddr, uint16_t defPort ):
    ConfigOpt ( flags, name, helpText ),
    _defaultAddr ( defAddr, defPort ), _addr ( _defaultAddr )
{
    _optFlags |= FlagIsSet;
    _optFlags |= FlagIsDefaultSet;
}

ConfigAddrSpec::ConfigAddrSpec (
        const char * cmdLineName, char cmdLineFlag, const String & helpText,
        const IpAddress & defAddr, uint16_t defPort ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText,
            String ( "%1:%2" ).arg ( defAddr.toString(), String::number ( defPort ) ) ),
    _defaultAddr ( defAddr, defPort ), _addr ( _defaultAddr )
{
    _optFlags |= FlagIsSet;
    _optFlags |= FlagIsDefaultSet;
}

ConfigAddrSpec::ConfigAddrSpec (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText,
        const IpAddress & defAddr, uint16_t defPort ):
    ConfigOpt ( flags, cmdLineName, cmdLineFlag, cfgName, helpText,
            String ( "%1:%2" ).arg ( defAddr.toString(), String::number ( defPort ) ) ),
    _defaultAddr ( defAddr, defPort ), _addr ( _defaultAddr )
{
    _optFlags |= FlagIsSet;
    _optFlags |= FlagIsDefaultSet;
}

ERRCODE ConfigAddrSpec::loadOption ( const String &, const String & strValue, bool isDefault )
{
    if ( strValue.isEmpty() )
    {
        _addr.clear();

        _optFlags &= ~FlagIsSet;

        if ( isDefault )
        {
            _defaultAddr.clear();

            _optFlags &= ~FlagIsDefaultSet;
        }

        return Error::Success;
    }

    if ( !SockAddr::convertAddrSpec ( strValue, _addr ) )
    {
        fprintf ( stderr, "Invalid address specification provided: %s = %s\n", optName.c_str(), strValue.c_str() );
        return Error::InvalidData;
    }

    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultAddr = _addr;
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

void ConfigAddrSpec::restoreDefaults()
{
    _addr = _defaultAddr;
}

HashMap<String, StringList> ConfigAddrSpec::getValues() const
{
    HashMap<String, StringList> ret;

    ret[ optName ].append ( _addr.toString() );

    return ret;
}

bool ConfigAddrSpec::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && _addr.hasIpAddr() && _addr.hasPort() );
}
