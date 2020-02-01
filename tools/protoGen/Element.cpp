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

#include <cstdio>

#include "Element.hpp"
#include "Symbol.hpp"
#include "Error.hpp"
#include "ProtoSpec.hpp"

using namespace Pravala;

Element::Element ( Symbol * contSymbol ):
    code ( 0 ),
    rule ( RuleUnknown ),
    access ( AccessUnknown ),
    isUnique ( false ),
    typeSymbol ( 0 ),
    containerSymbol ( contSymbol ),
    definedTarget ( 0 ),
    aliasTargetBitLength ( 0 ),
    aliasTarget ( 0 ),
    presenceIndex ( -1 ),
    iAliasRangeFrom ( 0 ),
    iAliasPayloadRangeFrom ( 0 ),
    iAliasRangeTo ( 0 )
{
}

bool Element::isAlias() const
{
    return ( typeName == ProtocolSpec::KW_ALIAS || typeName == ProtocolSpec::KW_SALIAS );
}

bool Element::isSalias() const
{
    return ( typeName == ProtocolSpec::KW_SALIAS );
}

String Element::checkAssValue ( String assValue )
{
    int bitLength = 0;
    bool canBeNegative = false;

    if ( typeSymbol != 0 )
    {
        if ( typeSymbol->isEnum() )
        {
            String cName;
            String cExtName;

            Symbol::parseEnumCode ( assValue, cName, cExtName );

            bool found = false;

            for ( HashMap<String, Element *>::Iterator it ( typeSymbol->getElements() ); it.isValid(); it.next() )
            {
                if ( it.value()->name == cName )
                {
                    found = true;
                    break;
                }
            }

            if ( !found )
                throw Error ( "Enum element '", name, "' cannot be assigned '", assValue, "' value, ",
                              "it has to be the name (and NOT the code) of one of enum's elements" );

            // We want to return simplified code instead of the original one (which might be using an extended name):
            return cName;
        }
        else if ( typeSymbol->isBasic() )
        {
            if ( typeSymbol->getName() == "float" )
            {
                bool ok = false;
                assValue.toFloat ( &ok );

                if ( !ok )
                    throw Error ( "Could not assign '", assValue, "' to data type '",
                                  typeSymbol->getName(), "'" );
            }
            else if ( typeSymbol->getName() == "double" )
            {
                bool ok = false;
                assValue.toDouble ( &ok );

                if ( !ok )
                    throw Error ( "Could not assign '", assValue, "' to data type '",
                                  typeSymbol->getName(), "'" );
            }
            else if ( typeSymbol->isInteger() && typeSymbol->getBitLength() > 0 )
            {
                bitLength = typeSymbol->getBitLength();
                canBeNegative = typeSymbol->canBeNegative();
            }
        }
        else
        {
            throw Error ( "Values can only be assigned to aliases, enumerators and basic types" );
        }
    }
    else if ( aliasTarget != 0 && getAliasPayloadBitLength() > 0 )
    {
        bitLength = getAliasPayloadBitLength();
        canBeNegative = false;
    }

    if ( bitLength > 0 )
    {
        if ( bitLength == 1 )
        {
            assert ( !canBeNegative );

            if ( assValue.compare ( "true", false ) == 0 || assValue == "1" )
                return "true";

            if ( assValue.compare ( "false", false ) == 0 || assValue == "0" )
                return "false";

            throw Error ( "Could not assign value '", assValue,
                          "' to this element. Only 0, 1, true and false values are allowed" );
        }

        if ( bitLength > 64 )
            throw Error ( "Data types longer than 64 bits are not supported" );

        assert ( bitLength > 1 );

        // We want to test if the value used can fit
        // in the number of bits that we have available.

        bool ok = false;

        if ( canBeNegative )
        {
            int64_t val = assValue.toInt64 ( &ok );

            if ( ok )
            {
                // We "lose" one bit for the sign bit
                bitLength -= 1;

                assert ( bitLength > 0 );

                // We want to create the biggest possible, positive valute that can fit
                // in the number of bits we have (minus one for the sign).
                // We do this on the biggest supported, unsigned type - uint64_t

                uint64_t maxVal = 0;

                for ( int i = 0; i < bitLength; ++i )
                {
                    maxVal <<= 1;
                    maxVal |= 1;
                }

                if ( val < 0 )
                {
                    // The min possible value will be ( ( -1 * maxVal ) - 1 ):
                    int64_t minVal = ( int64_t ) maxVal;
                    minVal *= -1;
                    minVal -= 1;

                    if ( val < minVal )
                    {
                        throw Error ( "Could not assign value '", assValue,
                                      "' to this element - the storage type is too small" );
                    }
                }
                else
                {
                    if ( ( uint64_t ) val > maxVal )
                    {
                        throw Error ( "Could not assign value '", assValue,
                                      "' to this element - the storage type is too small" );
                    }
                }
            }
            else
            {
                throw Error ( "Could not assign value '", assValue,
                              "' to this element. Only integer values are accepted by this data type" );
            }
        }
        else
        {
            uint64_t val = assValue.toUInt64 ( &ok );

            if ( ok )
            {
                // We want to create the biggest possible, positive valute that can fit
                // in the number of bits we have. We do this on the biggest
                // supported, unsigned type - uint64_t

                uint64_t maxVal = 0;

                for ( int i = 0; i < bitLength; ++i )
                {
                    maxVal <<= 1;
                    maxVal |= 1;
                }

                if ( val > maxVal )
                {
                    throw Error ( "Could not assign value '", assValue,
                                  "' to this element - the storage type is too small" );
                }
            }
            else
            {
                throw Error ( "Could not assign value '", assValue,
                              "' to this element. Only positive integer values are accepted by this data type" );
            }
        }
    }

    return assValue;
}

String Element::getCamelCaseName ( const String & prefix, bool usePascalCase ) const
{
    String tmp ( prefix );

    // It doesn't hurt if we get a prefix which ends with '_' already
    tmp.append ( "_" );
    tmp.append ( name );

    StringList strList = tmp.split ( "_" );

    for ( size_t i = ( usePascalCase ? 0 : 1 ); i < strList.size(); ++i )
    {
        strList[ i ] = strList[ i ].capitalize();
    }

    return String::join ( strList, "" );
}

bool Element::isEnumDefault() const
{
    return ( options.contains ( ProtocolSpec::KW_DEFAULT ) );
}
