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

#include <cstring>

#include "WebSocketStatelessHandler.hpp"

using namespace Pravala;

WebSocketStatelessHandler::~WebSocketStatelessHandler()
{
    for ( HashSet<WebSocketConnection *>::Iterator it ( _allSocks ); it.isValid(); it.next() )
    {
        it.value()->unrefOwner ( this );
    }

    _allSocks.clear();
}

void WebSocketStatelessHandler::addConnection ( WebSocketListener *, WebSocketConnection * conn )
{
    assert ( conn != 0 );

    const size_t oldSize = _allSocks.size();

    _allSocks.insert ( conn );

    if ( oldSize != _allSocks.size() )
    {
        conn->refOwner ( this );
    }
}

void WebSocketStatelessHandler::wsClosed ( WebSocketConnection * sock )
{
    assert ( sock != 0 );
    assert ( _allSocks.contains ( sock ) );

    sock->unrefOwner ( this );
    _allSocks.remove ( sock );
}

void WebSocketStatelessHandler::broadcast ( const MemHandle & data, bool isText )
{
    for ( HashSet<WebSocketConnection *>::MutableIterator it ( _allSocks ); it.isValid(); )
    {
        assert ( it.value() != 0 );

        if ( !it.value()->send ( data, isText ) )
        {
            it.value()->unrefOwner ( this );
            it.remove();
        }
        else
        {
            it.next();
        }
    }
}

void WebSocketStatelessHandler::broadcast ( const char * data, size_t len, bool isText )
{
    for ( HashSet<WebSocketConnection *>::MutableIterator it ( _allSocks ); it.isValid(); )
    {
        assert ( it.value() != 0 );

        if ( !it.value()->send ( data, len, isText ) )
        {
            it.value()->unrefOwner ( this );
            it.remove();
        }
        else
        {
            it.next();
        }
    }
}

void WebSocketStatelessHandler::broadcast ( const char * data )
{
    broadcast ( data, strlen ( data ), true );
}
