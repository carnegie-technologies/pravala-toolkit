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

#include "basic/IpAddress.hpp"
#include "basic/Timestamp.hpp"

#include "Json.hpp"
#include "JsonArray.hpp"

extern "C"
{
#include <jansson.h>
}

using namespace Pravala;

Json::Json(): JsonCore()
{
    _jptr = json_object();
}

bool Json::checkType() const
{
    return isObject();
}

JsonOpCode Json::get ( const char * key, int64_t & val ) const
{
    json_t * jInt = json_object_get ( _jptr, key );

    if ( !jInt )
    {
        return JsonOpCode::NotFound;
    }

    if ( !json_is_integer ( jInt ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val = json_integer_value ( jInt );

    return JsonOpCode::Success;
}

JsonOpCode Json::get ( const char * key, double & val ) const
{
    json_t * jInt = json_object_get ( _jptr, key );

    if ( !jInt )
    {
        return JsonOpCode::NotFound;
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

JsonOpCode Json::get ( const char * key, bool & val ) const
{
    json_t * jBool = json_object_get ( _jptr, key );

    if ( !jBool )
    {
        return JsonOpCode::NotFound;
    }

    if ( !json_is_boolean ( jBool ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val = json_is_true ( jBool );

    return JsonOpCode::Success;
}

JsonOpCode Json::get ( const char * key, String & val ) const
{
    json_t * jStr = json_object_get ( _jptr, key );

    if ( !jStr )
    {
        return JsonOpCode::NotFound;
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

JsonOpCode Json::get ( const char * key, MemHandle & val ) const
{
    json_t * jStr = json_object_get ( _jptr, key );

    if ( !jStr )
    {
        return JsonOpCode::NotFound;
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

JsonOpCode Json::get ( const char * key, IpAddress & val ) const
{
    json_t * jStr = json_object_get ( _jptr, key );

    if ( !jStr )
    {
        return JsonOpCode::NotFound;
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

JsonOpCode Json::get ( const char * key, Json & val ) const
{
    json_t * jObj = json_object_get ( _jptr, key );

    if ( !jObj )
    {
        return JsonOpCode::NotFound;
    }

    if ( !json_is_object ( jObj ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val.clearError();
    val.setJptr ( jObj );

    return JsonOpCode::Success;
}

JsonOpCode Json::get ( const char * key, JsonArray & val ) const
{
    json_t * jArr = json_object_get ( _jptr, key );

    if ( !jArr )
    {
        return JsonOpCode::NotFound;
    }

    if ( !json_is_array ( jArr ) )
    {
        return JsonOpCode::InvalidDataType;
    }

    val.clearError();
    val.setJptr ( jArr );

    return JsonOpCode::Success;
}

JsonOpCode Json::get ( const char * key, StringList & val ) const
{
    JsonArray a;
    JsonOpCode ret = get ( key, a );

    if ( ret != JsonOpCode::Success )
    {
        return ret;
    }

    val.clear();
    size_t len = a.size();

    String tmp;

    for ( size_t i = 0; i < len; ++i )
    {
        ret = a.get ( i, tmp );

        if ( ret != JsonOpCode::Success )
        {
            return ret;
        }

        val.append ( tmp );
    }

    return JsonOpCode::Success;
}

bool Json::put ( const char * key, bool val )
{
    return ( json_object_set_new ( _jptr, key, val ? json_true() : json_false() ) == 0 );
}

bool Json::put ( const char * key, int val )
{
    return ( json_object_set_new ( _jptr, key, json_integer ( val ) ) == 0 );
}

bool Json::put ( const char * key, uint32_t val )
{
    return ( json_object_set_new ( _jptr, key, json_integer ( val ) ) == 0 );
}

bool Json::put ( const char * key, uint64_t val )
{
    return ( json_object_set_new ( _jptr, key, json_integer ( val ) ) == 0 );
}

bool Json::put ( const char * key, int64_t val )
{
    return ( json_object_set_new ( _jptr, key, json_integer ( val ) ) == 0 );
}

bool Json::put ( const char * key, double val )
{
    return ( json_object_set_new ( _jptr, key, json_real ( val ) ) == 0 );
}

bool Json::put ( const char * key, const String & val )
{
    return ( json_object_set_new ( _jptr, key, json_stringn ( val.c_str(), val.length() ) ) == 0 );
}

bool Json::put ( const char * key, const MemHandle & val )
{
    return ( json_object_set_new ( _jptr, key, json_stringn ( val.get(), val.size() ) ) == 0 );
}

bool Json::put ( const char * key, const char * val )
{
    return ( json_object_set_new ( _jptr, key, json_string ( val ) ) == 0 );
}

bool Json::put ( const char * key, const IpAddress & val )
{
    if ( !val.isValid() )
        return false;

    return put ( key, val.toString() );
}

bool Json::put ( const char * key, const Timestamp & val )
{
    return put ( key, val.toString() );
}

bool Json::put ( const char * key, JsonCore & val )
{
    // We need to use set, not set_new since _jptr keeps a copy of the ptr as well

    return ( json_object_set ( _jptr, key, val._jptr ) == 0 );
}

void Json::clear()
{
    json_object_clear ( _jptr );
}
