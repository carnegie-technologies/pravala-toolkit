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

#include "ConfigStringMap.hpp"

using namespace Pravala;

ConfigStringMap::ConfigStringMap ( uint8_t flags, const char * name, const String & helpText ):
    ConfigOpt ( flags, name, helpText )
{
}

HashMap<String, StringList> ConfigStringMap::getValues() const
{
    HashMap<String, StringList> ret;

    for ( HashMap<String, String>::Iterator it ( _value ); it.isValid(); it.next() )
    {
        ret[ it.key() ].append ( it.value() );
    }

    return ret;
}

void ConfigStringMap::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigStringMap::loadOption ( const String & name, const String & strValue, bool isDefault )
{
    _value.insert ( name, strValue );
    _optFlags |= FlagIsSet;

    if ( isDefault )
    {
        _defaultValue.insert ( name, strValue );
        _optFlags |= FlagIsDefaultSet;
    }

    return Error::Success;
}

bool ConfigStringMap::isNonEmpty()
{
    return ( ConfigOpt::isNonEmpty() && !_value.isEmpty() );
}
