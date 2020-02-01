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

#include "ConfigSwitch.hpp"

using namespace Pravala;

ConfigSwitch::ConfigSwitch ( const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigOpt ( 0, cmdLineName, cmdLineFlag, "", helpText, "" ),
    _value ( 0 )
{
    _optFlags = FlagIsSimpleSwitch;
}

ERRCODE ConfigSwitch::loadOption ( const String &, const String & strValue, bool isDefault )
{
    ( void ) isDefault;
    ( void ) strValue;

    assert ( !isDefault );
    assert ( strValue.isEmpty() );

    ++_value;
    _optFlags |= FlagIsSet;

    return Error::Success;
}

void ConfigSwitch::restoreDefaults()
{
    _value = 0;
}

HashMap<String, StringList> ConfigSwitch::getValues() const
{
    return HashMap<String, StringList>();
}
