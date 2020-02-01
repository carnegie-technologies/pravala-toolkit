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

#include "basic/Timestamp.hpp"
#include "basic/IpAddress.hpp"

#include "JsonArray.hpp"
#include "Json.hpp"

using namespace Pravala;

JsonArray::JsonArray(): JsonCore()
{
    _jptr = json_array();
}

size_t JsonArray::size() const
{
    return json_array_size ( _jptr );
}

bool JsonArray::checkType() const
{
    return isArray();
}

JsonOpCode JsonArray::get ( const size_t idx, int64_t & val ) const
{
    json_t * jInt = json_array_get ( _jptr, idx );

    if ( !jInt )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_integer ( jInt ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val = json_integer_value ( jInt );

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, double & val ) const
{
    json_t * jInt = json_array_get ( _jptr, idx );

    if ( !jInt )
    {
        return JsonOpCode::InvalidIndex;
    }

    // If the JSON value is an integer, it can be stored as a double - but we have to read it differently.
    if ( json_is_integer ( jInt ) )
    {
        val = ( double ) json_integer_value ( jInt );

        return JsonOpCode::Success;
    }

    if ( !json_is_real ( jInt ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val = json_real_value ( jInt );

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, bool & val ) const
{
    json_t * jBool = json_array_get ( _jptr, idx );

    if ( !jBool )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_boolean ( jBool ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val = json_is_true ( jBool );

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, String & val ) const
{
    json_t * jStr = json_array_get ( _jptr, idx );

    if ( !jStr )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_string ( jStr ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    const char * str = json_string_value ( jStr );

    if ( !str )
    {
        assert ( false ); // since we verified it is a string above, this shouldn't trigger

        return JsonOpCode::InvalidData;
    }

    val = str;

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, MemHandle & val ) const
{
    json_t * jStr = json_array_get ( _jptr, idx );

    if ( !jStr )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_string ( jStr ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    const char * str = json_string_value ( jStr );

    if ( !str )
    {
        assert ( false ); // since we verified it is a string above, this shouldn't trigger

        return JsonOpCode::InvalidData;
    }

    const size_t len = strlen ( str );

    if ( len < 1 )
    {
        val.clear();
        return JsonOpCode::Success;
    }

    MemHandle mh ( len );

    if ( !mh.getWritable() || mh.size() != len )
        return JsonOpCode::Unknown;

    memcpy ( mh.getWritable(), str, len );

    val = mh;
    mh.clear();

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, IpAddress & val ) const
{
    json_t * jStr = json_array_get ( _jptr, idx );

    if ( !jStr )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_string ( jStr ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    const char * str = json_string_value ( jStr );

    if ( !str )
    {
        assert ( false ); // since we verified it is a string above, this shouldn't trigger

        return JsonOpCode::InvalidData;
    }

    const IpAddress addr ( str );

    if ( !addr.isValid() )
    {
        return JsonOpCode::InvalidData;
    }

    val = addr;
    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, Json & val ) const
{
    json_t * jObj = json_array_get ( _jptr, idx );

    if ( !jObj )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_object ( jObj ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val.clearError();
    val.setJptr ( jObj );

    return JsonOpCode::Success;
}

JsonOpCode JsonArray::get ( const size_t idx, JsonArray & val ) const
{
    json_t * jObj = json_array_get ( _jptr, idx );

    if ( !jObj )
    {
        return JsonOpCode::InvalidIndex;
    }

    if ( !json_is_array ( jObj ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val.clearError();
    val.setJptr ( jObj );

    return JsonOpCode::Success;
}

bool JsonArray::put ( const size_t idx, bool val )
{
    return ( json_array_set_new ( _jptr, idx, val ? json_true() : json_false() ) == 0 );
}

bool JsonArray::put ( const size_t idx, int val )
{
    return ( json_array_set_new ( _jptr, idx, json_integer ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, uint32_t val )
{
    return ( json_array_set_new ( _jptr, idx, json_integer ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, int64_t val )
{
    return ( json_array_set_new ( _jptr, idx, json_integer ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, uint64_t val )
{
    return ( json_array_set_new ( _jptr, idx, json_integer ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, double val )
{
    return ( json_array_set_new ( _jptr, idx, json_real ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, const String & val )
{
    return ( json_array_set_new ( _jptr, idx, json_stringn ( val.c_str(), val.length() ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, const MemHandle & val )
{
    return ( json_array_set_new ( _jptr, idx, json_stringn ( val.get(), val.size() ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, const char * val )
{
    return ( json_array_set_new ( _jptr, idx, json_string ( val ) ) == 0 );
}

bool JsonArray::put ( const size_t idx, const IpAddress & val )
{
    if ( !val.isValid() )
        return false;

    return put ( idx, val.toString() );
}

bool JsonArray::put ( const size_t idx, const Timestamp & val )
{
    return put ( idx, val.toString() );
}

bool JsonArray::put ( const size_t idx, JsonCore & val )
{
    // We need to use set, not set_new since _jptr keeps a copy of the ptr as well

    return ( json_array_set ( _jptr, idx, val._jptr ) == 0 );
}

bool JsonArray::append ( bool val )
{
    return ( json_array_append_new ( _jptr, val ? json_true() : json_false() ) == 0 );
}

bool JsonArray::append ( int val )
{
    return ( json_array_append_new ( _jptr, json_integer ( val ) ) == 0 );
}

bool JsonArray::append ( uint32_t val )
{
    return ( json_array_append_new ( _jptr, json_integer ( val ) ) == 0 );
}

bool JsonArray::append ( int64_t val )
{
    return ( json_array_append_new ( _jptr, json_integer ( val ) ) == 0 );
}

bool JsonArray::append ( uint64_t val )
{
    return ( json_array_append_new ( _jptr, json_integer ( val ) ) == 0 );
}

bool JsonArray::append ( double val )
{
    return ( json_array_append_new ( _jptr, json_real ( val ) ) == 0 );
}

bool JsonArray::append ( const String & val )
{
    return ( json_array_append_new ( _jptr, json_stringn ( val.c_str(), val.length() ) ) == 0 );
}

bool JsonArray::append ( const MemHandle & val )
{
    return ( json_array_append_new ( _jptr, json_stringn ( val.get(), val.size() ) ) == 0 );
}

bool JsonArray::append ( const char * val )
{
    return ( json_array_append_new ( _jptr, json_string ( val ) ) == 0 );
}

bool JsonArray::append ( const IpAddress & val )
{
    if ( !val.isValid() )
        return false;

    return append ( val.toString() );
}

bool JsonArray::append ( const Json & val )
{
    // We need to use set, not set_new since _jptr keeps a copy of the ptr as well

    return ( json_array_append ( _jptr, val._jptr ) == 0 );
}

bool JsonArray::append ( const JsonArray & val )
{
    // We need to use set, not set_new since _jptr keeps a copy of the ptr as well

    return ( json_array_append ( _jptr, val._jptr ) == 0 );
}

bool JsonArray::appendContents ( const JsonArray & val )
{
    return ( json_array_extend ( _jptr, val._jptr ) == 0 );
}

void JsonArray::clear()
{
    json_array_clear ( _jptr );
}
