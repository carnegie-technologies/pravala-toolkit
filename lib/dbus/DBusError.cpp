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

#include "DBusError.hpp"
#include "internal/DBusErrorWrapper.hpp"

using namespace Pravala;

void Pravala::DBusError::set ( const DBusErrorWrapper & err )
{
    _name.clear();
    _message.clear();

    if ( err.name != 0 )
        _name = err.name;

    if ( err.message != 0 )
        _message = err.message;
}

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const DBusError & value )
{
    Buffer & buf = textMessage.getInternalBuf();

    if ( value.isSet() )
    {
        buf.append ( value.getName() );
        buf.append ( ": " );
        buf.append ( value.getMessage() );
    }
    else
    {
        buf.append ( "NO-ERROR" );
    }

    return textMessage;
}
