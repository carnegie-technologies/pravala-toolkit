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

extern "C"
{
#include <jansson.h>
}

#include "JsonCore.hpp"

using namespace Pravala;

struct Pravala::JsonError: public json_error_t
{};

JsonCore::JsonCore(): _jptr ( 0 ), _jerr ( 0 )
{
}

bool JsonCore::operator== ( const JsonCore & other ) const
{
    return ( json_equal ( _jptr, other._jptr ) == 1 );
}

void JsonCore::clearError()
{
    if ( _jerr != 0 )
    {
        delete _jerr;
        _jerr = 0;
    }
}

void JsonCore::setJptr ( json_t * jptr )
{
    if ( _jptr == jptr )
        return;

    json_t * tmp = _jptr;
    _jptr = jptr;

    if ( _jptr != 0 )
        json_incref ( _jptr );

    if ( tmp != 0 )
        json_decref ( tmp );
}

JsonCore::~JsonCore()
{
    clearError();

    if ( _jptr != 0 )
    {
        json_decref ( _jptr );
        _jptr = 0;
    }
}

bool JsonCore::isArray() const
{
    return json_is_array ( _jptr );
}

bool JsonCore::isObject() const
{
    return json_is_object ( _jptr );
}

bool JsonCore::decode ( const char * data, size_t len )
{
    if ( !data || len < 1 )
        return false;

    if ( _jptr != 0 )
    {
        json_decref ( _jptr );
        _jptr = 0;
    }

    JsonError jErr;

    // This returns a reference-counted JSON object. To free, we must call json_decref on root prior to returning.
    _jptr = json_loadb ( data, len, 0, &jErr );

    if ( !_jptr )
    {
        if ( !_jerr )
        {
            _jerr = new JsonError;
        }

        assert ( _jerr != 0 );

        memcpy ( _jerr, &jErr, sizeof ( jErr ) );

        return false;
    }

    return checkType();
}

bool JsonCore::decode ( const char * payload )
{
    size_t len;

    if ( !payload || ( len = strlen ( payload ) ) < 1 )
        return false;

    return decode ( payload, len );
}

bool JsonCore::decode ( const String & payload )
{
    return decode ( payload.c_str(), payload.length() );
}

bool JsonCore::decode ( const MemHandle & buf )
{
    return decode ( buf.get(), buf.size() );
}

bool JsonCore::encode ( String & payload, bool resetPayload ) const
{
    if ( resetPayload )
        payload.clear();

    if ( !_jptr )
        return false;

    char * encoded = json_dumps ( _jptr, JSON_COMPACT );

    if ( encoded != 0 )
    {
        payload.append ( encoded );
        free ( encoded );
        return true;
    }

    return false;
}

/// @brief A callback for JSON to store data in a buffer.
/// @param [in] buffer Memory containing a chunk of output
/// @param [in] size The length of the memory
/// @param [in] data Pointer to a Buffer object to store the data in.
/// @return 0 on success; -1 on failure.
static int jsonDumpCb ( const char * buffer, size_t size, void * data )
{
    Buffer * buf = reinterpret_cast<Buffer *> ( data );

    if ( !buf )
        return -1;

    buf->appendData ( buffer, size );
    return 0;
}

bool JsonCore::encode ( Buffer & buf, bool resetBuffer ) const
{
    if ( resetBuffer )
        buf.clear();

    if ( !_jptr )
        return false;

    return ( json_dump_callback ( _jptr, &jsonDumpCb, &buf, JSON_COMPACT ) == 0 );
}

bool JsonCore::encode ( MemHandle & data ) const
{
    if ( !_jptr )
        return false;

    Buffer buf;

    if ( json_dump_callback ( _jptr, &jsonDumpCb, &buf, JSON_COMPACT ) != 0 )
        return false;

    data = buf.getHandle();
    buf.clear();

    return true;
}

String JsonCore::getLastErrorMessage() const
{
    if ( !_jerr )
    {
        return String::EmptyString;
    }

    return String ( "Error parsing provided payload: " )
           .append ( _jerr->text ).append ( " at line " ).append ( _jerr->line );
}
