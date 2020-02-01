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

#include "basic/HashMap.hpp"
#include "SimpleHttpServer.hpp"

using namespace Pravala;

SimpleHttpServer::SimpleHttpServer():
    HttpServer ( ( HttpServer::Owner & ) *this )
{
}

void SimpleHttpServer::removeContent ( const String & url )
{
    _data.remove ( sanitizeUrl ( url ) );
}

void SimpleHttpServer::setContent ( const String & url, int respCode )
{
    UrlData & data = _data[ sanitizeUrl ( url ) ];

    data.data.clear();
    data.contentType.clear();
    data.respCode = respCode;
}

void SimpleHttpServer::setContent ( const String & url, const String & contentType, const char * content )
{
    const size_t len = ( content != 0 ) ? strlen ( content ) : 0;

    if ( len < 1 )
    {
        setContent ( url, StatusOK );
        return;
    }

    MemHandle mh ( len );

    memcpy ( mh.getWritable(), content, len );

    setContent ( url, contentType, mh );
}

void SimpleHttpServer::setContent ( const String & url, const String & contentType, const String & content )
{
    if ( content.length() < 1 )
    {
        setContent ( url, StatusOK );
        return;
    }

    MemHandle mh ( content.length() );

    memcpy ( mh.getWritable(), content.c_str(), content.length() );

    setContent ( url, contentType, mh );
}

void SimpleHttpServer::setContent ( const String & url, const String & contentType, const MemHandle & content )
{
    UrlData & data = _data[ sanitizeUrl ( url ) ];

    data.data = content;
    data.contentType = contentType;
    data.respCode = StatusOK;
}

int SimpleHttpServer::httpHandleGetRequest (
        HttpServer * server, const SockAddr & remoteAddr, HttpParser & request,
        HashMap<String, String> & respHeaders, String & respContentType, MemHandle & respPayload )
{
    ( void ) server;
    ( void ) remoteAddr;
    ( void ) respHeaders;

    assert ( server == this );

    UrlData data;

    if ( !_data.find ( request.getUrl(), data ) )
    {
        LOG ( L_DEBUG, remoteAddr << ": Requested URL '" << request.getUrl()
              << "' doesn't exist; Responding with " << StatusNotFound << " code" );

        return StatusNotFound;
    }

    respContentType = data.contentType;
    respPayload = data.data;

    LOG ( L_DEBUG, remoteAddr << ": Requested URL '" << request.getUrl()
          << "' exists; Responding with " << data.respCode << " code" );

    return data.respCode;
}

String Pravala::SimpleHttpServer::sanitizeUrl ( const String & url )
{
    if ( url.isEmpty() )
        return "/";

    const String tmp = url.removeChars ( " \t\v\f\r\n" );

    return ( tmp.startsWith ( "/" ) ) ? tmp : String ( "/" ).append ( tmp );
}
