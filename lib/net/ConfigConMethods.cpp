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

#include "ConfigConMethods.hpp"

#include <cstdio>

using namespace Pravala;

ConfigConMethods::ConfigConMethods ( uint8_t flags, const char * name, const String & helpText, size_t minExtraArgs ):
    ConfigOpt ( flags, name, helpText ), _minExtraArgs ( minExtraArgs )
{
}

ConfigConMethods::ConfigConMethods (
        uint8_t flags, const char * name, const String & helpText, size_t minExtraArgs,
        const char * defValue ):
    ConfigOpt ( flags, name, helpText ), _minExtraArgs ( minExtraArgs )
{
    const String sVal ( defValue );

    if ( sVal.isEmpty() )
        return;

    loadOption ( name, defValue, true );
}

HashMap<String, StringList> ConfigConMethods::getValues() const
{
    HashMap<String, StringList> ret;

    String desc;

    for ( size_t i = 0; i < _value.size(); ++i )
    {
        StringList vals;

        const ConMethod & m = _value.at ( i );

        vals.append ( m.getAddress().toString() );
        vals.append ( String::number ( m.getPort() ) );

        for ( size_t j = 0; j < m.getArgs().size(); ++j )
        {
            vals.append ( m.getArgs().at ( j ) );
        }

        if ( !desc.isEmpty() )
        {
            desc.append ( " " );
        }

        if ( m.useTcp() && !m.useUdp() )
        {
            desc.append ( "TCP:" );
        }
        else if ( !m.useTcp() && m.useUdp() )
        {
            desc.append ( "UDP:" );
        }

        desc.append ( String::join ( vals, "/" ) );
    }

    ret[ optName ].append ( desc );

    return ret;
}

void ConfigConMethods::restoreDefaults()
{
    _value = _defaultValue;
}

ERRCODE ConfigConMethods::loadOption ( const String &, const String & strValue, bool isDefault )
{
    const StringList strList = strValue.split ( " \t,;" );
    List<ConMethod> methods;

    for ( size_t i = 0; i < strList.size(); ++i )
    {
        ConMethod method ( strList[ i ] );

        // there must be exactly one argument -- the "interface name"
        if ( !method.isValid() || method.getArgs().size() < _minExtraArgs )
        {
            fprintf ( stderr, "Not enough extra ConMethod arguments (%lu required): %s\n",
                      ( unsigned long ) _minExtraArgs, strList[ i ].c_str() );
            return Error::InvalidData;
        }

        methods.append ( method );
    }

    _value = methods;

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
