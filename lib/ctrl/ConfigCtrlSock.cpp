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

#include "basic/IpAddress.hpp"
#include "ConfigCtrlSock.hpp"

#ifdef SYSTEM_WINDOWS

#define BASE_HELP    "The value should be in 'ip_address:port_number' format.\n"

#else

#define BASE_HELP \
    "If it can be parsed as ip_address:port_number an 'internet' socket is created.\n" \
    "Otherwise a local socket is opened using 'value' as its path.\n" \
    "For local sockets, if the name starts with a '@' character,\n" \
    "a socket in the abstract namespace is created (without '@' in its name).\n" \
    "Otherwise a file-based local socket is created.\n"
#endif

using namespace Pravala;

ConfigCtrlSock::ConfigCtrlSock (
        const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const char * ownerName ):
    ConfigString ( ConfigOpt::FlagInitializeOnly, cmdLineName, cmdLineFlag, cfgName,
            String (
                    "Control socket (address) for %1 to connect to.\n"
                    BASE_HELP ).arg ( ownerName ).c_str() )
{
}

ConfigCtrlSock::ConfigCtrlSock (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const char * ownerName ):
    ConfigString ( flags | ConfigOpt::FlagInitializeOnly, cmdLineName, cmdLineFlag, cfgName,
            String (
                    "Control socket (address) for %1 to connect to.\n"
                    BASE_HELP ).arg ( ownerName ).c_str() )
{
}

ERRCODE ConfigCtrlSock::loadOption ( const String & name, const String & strValue, bool isDefault )
{
#ifdef SYSTEM_WINDOWS
    // On Windows we only support IP sockets:

    IpAddress ipAddr;
    uint16_t ipPort;

    if ( !IpAddress::convertAddrSpec ( strValue, ipAddr, ipPort ) )
        return Error::InvalidData;
#endif

    return ConfigString::loadOption ( name, strValue, isDefault );
}

ConfigCtrlListeners::ConfigCtrlListeners (
        const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const char * ownerName ):
    ConfigStringMultiSetList ( ConfigOpt::FlagInitializeOnly, cmdLineName, cmdLineFlag, cfgName,
            String ( "Socket for %1 to listen on for incoming control connections.\n"
                    BASE_HELP

#ifndef SYSTEM_WINDOWS
                    "If that file already exists, the program fails.\n"
                    "However, all the directories in the path used have to exist!\n"
#endif

                    "Can be used multiple times to create multiple sockets.\n"
            ).arg ( ownerName ).c_str() )
{
}

ConfigCtrlListeners::ConfigCtrlListeners (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const char * ownerName ):
    ConfigStringMultiSetList ( flags | ConfigOpt::FlagInitializeOnly, cmdLineName, cmdLineFlag, cfgName,
            String ( "Socket for %1 to listen on for incoming control connections.\n"
                    BASE_HELP

#ifndef SYSTEM_WINDOWS
                    "If that file already exists, the program fails.\n"
                    "However, all the directories in the path used have to exist!\n"
#endif

                    "Can be used multiple times to create multiple sockets.\n"
            ).arg ( ownerName ).c_str() )
{
}

ERRCODE ConfigCtrlListeners::loadOption ( const String & name, const String & strValue, bool isDefault )
{
#ifdef SYSTEM_WINDOWS
    // On Windows we only support IP sockets:

    IpAddress ipAddr;
    uint16_t ipPort;

    if ( !IpAddress::convertAddrSpec ( strValue, ipAddr, ipPort ) )
        return Error::InvalidData;
#endif

    return ConfigStringMultiSetList::loadOption ( name, strValue, isDefault );
}

const String ConfigCtrlListeners::getFirstValue()
{
    for ( size_t i = 0; i < value().size(); ++i )
    {
        if ( !value().at ( i ).isEmpty() )
        {
            return value().at ( i );
        }
    }

    return String::EmptyString;
}

const String ConfigCtrlListeners::getLastValue()
{
    for ( size_t i = value().size(); i > 0; --i )
    {
        if ( !value().at ( i - 1 ).isEmpty() )
        {
            return value().at ( i - 1 );
        }
    }

    return String::EmptyString;
}
