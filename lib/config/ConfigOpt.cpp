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

#include "ConfigOpt.hpp"

using namespace Pravala;

List< ConfigOpt *> & ConfigOpt::cfgOptions()
{
    static List<ConfigOpt *> global;

    return global;
}

List< ConfigOpt *> & ConfigOpt::cmdLineOptions()
{
    static List<ConfigOpt *> global;

    return global;
}

ConfigOpt::ConfigOpt ( uint8_t flags, const char * cfgName, const String & helpText ):
    optName ( cfgName ), optHelpText ( helpText ), optCmdLineFlag ( 0 ),
    _optFlags ( flags & ( FlagBuiltInOnly | FlagInitializeOnly | FlagIsCommon ) )
{
    assert ( !optName.isEmpty() );

    if ( !optName.isEmpty() )
    {
        cfgOptions().append ( this );
    }
}

ConfigOpt::ConfigOpt (
        uint8_t flags, const char * cmdLineName, char cmdLineFlag,
        const char * cfgName, const String & helpText, const String & defValue ):
    optName ( cfgName ), optHelpText ( helpText ),
    optCmdLineFlag ( cmdLineFlag ), optCmdLineName ( cmdLineName ), optCmdLineDefault ( defValue ),
    _optFlags ( flags & ( FlagBuiltInOnly | FlagInitializeOnly | FlagIsCommon | FlagReqNonEmpty ) )
{
    assert ( !optName.isEmpty() || !optCmdLineName.isEmpty() || optCmdLineFlag != 0 );

    if ( !optCmdLineName.isEmpty() || optCmdLineFlag != 0 )
    {
        cmdLineOptions().append ( this );
    }

    if ( !optName.isEmpty() )
    {
        cfgOptions().append ( this );
    }
}

ConfigOpt::~ConfigOpt()
{
    cfgOptions().removeValue ( this );
    cmdLineOptions().removeValue ( this );
}

bool ConfigOpt::isNonEmpty()
{
    return isSet();
}

void ConfigOpt::configUpdated()
{
    // Nothing by default
}
