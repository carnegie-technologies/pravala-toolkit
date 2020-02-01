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

#include "WSHandler.hpp"

#define LIBWEBSOCKETS_PROTO1    "dumb-increment-protocol"
#define LIBWEBSOCKETS_PROTO2    "lws-mirror-protocol"

using namespace Pravala;

TextLog WSHandler::_log ( "ws_handler" );

void WSHandler::wsRead ( WebSocketConnection * conn, MemHandle & data, bool isText )
{
    ( void ) conn;
    ( void ) data;
    ( void ) isText;

#ifndef NO_LOGGING
    if ( isText )
    {
        LOG ( L_INFO, "Read text WebSocket frame from connection: " << ( uint64_t ) conn
              << "; Data: '" << data.toString().simplified() << "'" );
    }
    else
    {
        LOG ( L_INFO, "Read binary WebSocket frame from connection: " << ( uint64_t ) conn
              << "; Data length: " << data.size() << "; Dump: " << String::hexDump ( data.get(), data.size() ) );
    }
#endif
}
