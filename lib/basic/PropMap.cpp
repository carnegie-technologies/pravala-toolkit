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

#include "MemHandle.hpp"
#include "Buffer.hpp"
#include "PropMap.hpp"

using namespace Pravala;

bool PropMap::parseProperty (
        const String & entry, String & propName, String & propValue,
        const String & propSeparators, const String & commentChars )
{
    String tEntry = entry.trimmed();

    propName.clear();
    propValue.clear();

    // ignore empty list, list with whitespace only and comments
    if ( tEntry.isEmpty() || commentChars.find ( tEntry[ 0 ] ) >= 0 )
        return true;

    // find one of the delimiters
    int pos = tEntry.findFirstOf ( propSeparators );

    // no delimiter is found
    if ( pos < 0 )
        return false;

    propName = tEntry.substr ( 0, pos ).trimmed().toLower();
    propValue = tEntry.substr ( pos + 1 ).trimmed();

    return true;
}

size_t PropMap::appendLines ( const StringList & list, const String & propSeparators, const String & commentChars )
{
    String propName;
    String propValue;

    for ( size_t i = 0; i < list.size(); ++i )
    {
        if ( !parseProperty ( list[ i ], propName, propValue, propSeparators, commentChars ) )
            return ( i + 1 );

        if ( propName.length() > 0 )
            HashMap<String, String>::insert ( propName, propValue );
    }

    return 0;
}

size_t PropMap::appendData (
        const MemHandle & fromBuffer, const String & propSeparators,
        const String & commentChars, const String & listSeparators )
{
    return appendLines ( fromBuffer.toStringList ( listSeparators, true ),
                         propSeparators, commentChars );
}

ssize_t PropMap::appendFile (
        const String & filePath,
        const String & propSeparators,
        const String & commentChars,
        const String & listSeparators )
{
    bool isOk = false;
    MemHandle mh ( filePath, &isOk );

    if ( !isOk )
        return -1;

    return appendData ( mh, propSeparators, commentChars, listSeparators );
}

MemHandle PropMap::getBuffer ( const String & entrySeparator, char propSeparator )
{
    Buffer ret;

    writeToBuffer ( ret, entrySeparator, propSeparator );

    return ret;
}

void PropMap::writeToBuffer ( Buffer & toBuffer, const String & entrySeparator, char propSeparator )
{
    for ( Iterator it ( *this ); it.isValid(); it.next() )
    {
        toBuffer.append ( it.key() );
        toBuffer.appendData ( &propSeparator, 1 );
        toBuffer.append ( it.value() );
        toBuffer.append ( entrySeparator );
    }
}
