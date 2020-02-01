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

#include "DBusMessageReader.hpp"

namespace DBusC
{
extern "C" {
#include <dbus/dbus.h>
}
}

using namespace Pravala;

Pravala::DBusMessageReader::DBusMessageReader ( DBusC::DBusMessage * msg ):
    DBusMessage (
            dbus_message_get_destination ( msg ),
            dbus_message_get_sender ( msg ),
            dbus_message_get_path ( msg ),
            dbus_message_get_interface ( msg ),
            dbus_message_get_member ( msg ) )
{
    _msg = msg;

    if ( !_msg )
    {
        LOG ( L_ERROR, "Invalid DBus message passed" );
        return;
    }

    dbus_message_ref ( _msg );

    DBusC::DBusMessageIter * iter = new DBusC::DBusMessageIter();

    dbus_message_iter_init ( _msg, iter );

    _iters.append ( iter );
}

void Pravala::DBusMessageReader::restartArgReadIterator()
{
    if ( _iters.size() < 1 )
    {
        return;
    }

    // Don't delete the first iterator, since we would just have to allocate a new one anyway.
    while ( _iters.size() > 1 )
    {
        delete _iters.last();
        _iters.removeLast();
    }

    assert ( _iters.size() == 1 );

    dbus_message_iter_init ( _msg, _iters.last() );
}

bool Pravala::DBusMessageReader::hasNextArg()
{
    if ( _iters.size() < 1 )
    {
        return false;
    }

    return ( dbus_message_iter_get_arg_type ( _iters.last() ) != DBUS_TYPE_INVALID );
}

template<typename T> bool Pravala::DBusMessageReader::getNextArg ( int type, const char * typeDesc, T & value )
{
    ( void ) typeDesc;

    if ( _iters.size() < 1 )
    {
        return false;
    }

    int itType = DBUS_TYPE_INVALID;

    DBusC::DBusMessageIter variantIter;

    DBusC::DBusMessageIter * baseIter = _iters.last();
    DBusC::DBusMessageIter * iter = baseIter;

    itType = dbus_message_iter_get_arg_type ( baseIter );

    if ( itType == DBUS_TYPE_VARIANT )
    {
        iter = &variantIter;

        dbus_message_iter_recurse ( baseIter, iter );
    }

    if ( !iter || ( itType = dbus_message_iter_get_arg_type ( iter ) ) != type )
    {
        LOG ( L_ERROR, "Error reading next argument using iterator " << iter
              << "; Expected type: " << type << " (" << typeDesc << "); Actual type: "
              << itType << "; Message: " << *this );

        return false;
    }

    dbus_message_iter_get_basic ( iter, &value );

    dbus_message_iter_next ( baseIter );

    LOG ( L_DEBUG4, "Read " << typeDesc << " value " << value
          << " from iterator " << iter << " [msg: " << _msg << "]" );

    return true;
}

#define SIMPLE_GET_NEXT_DBUS_ARG_FUNC( type, dbus_type ) \
    bool Pravala::DBusMessageReader::getNextArg ( type & value ) { \
        return getNextArg<type> ( DBUS_TYPE_ ## dbus_type, #dbus_type, value ); \
    }

SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( char, BYTE )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( int8_t, BYTE )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( uint8_t, BYTE )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( int16_t, INT16 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( uint16_t, UINT16 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( int32_t, INT32 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( uint32_t, UINT32 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( int64_t, INT64 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( uint64_t, UINT64 )
SIMPLE_GET_NEXT_DBUS_ARG_FUNC ( double, DOUBLE )

bool Pravala::DBusMessageReader::getNextArg ( bool & value )
{
    // DBus represents booleans with 32 bits. Possible values are 0 or 1.
    uint32_t dbusBoolean;

    if ( !getNextArg<uint32_t> ( DBUS_TYPE_BOOLEAN, "BOOLEAN", dbusBoolean ) || dbusBoolean > 1 )
    {
        return false;
    }

    value = ( dbusBoolean != 0 );

    return true;
}

bool Pravala::DBusMessageReader::getNextArg ( String & value )
{
    const char * str = 0;

    if ( getNextArg<const char *> ( DBUS_TYPE_STRING, "STRING", str ) )
    {
        value = str;
        return true;
    }

    return false;
}

bool Pravala::DBusMessageReader::getNextArgObjectPath ( String & value )
{
    const char * str = 0;

    if ( getNextArg<const char *> ( DBUS_TYPE_OBJECT_PATH, "OBJECT_PATH", str ) )
    {
        value = str;
        return true;
    }

    return false;
}

bool Pravala::DBusMessageReader::openContainer()
{
    if ( _iters.size() < 1 )
    {
        return false;
    }

    const int valType = dbus_message_iter_get_arg_type ( _iters.last() );

    switch ( valType )
    {
        case DBUS_TYPE_VARIANT:
        case DBUS_TYPE_ARRAY:
        case DBUS_TYPE_DICT_ENTRY:
        case DBUS_TYPE_STRUCT:
            LOG ( L_DEBUG4, "Opening container of type (" << valType << ") [msg: " << _msg << "]" );
            break;

        default:
            LOG ( L_ERROR, "Cannot open non-container type (" << valType << ") [msg: " << _msg << "]" );
            return false;
    }

    DBusC::DBusMessageIter * newIter = new DBusC::DBusMessageIter();

    // The current last iterator will be left at the container position. It will be advanced
    // when this container is closed.
    dbus_message_iter_recurse ( _iters.last(), newIter );

    _iters.append ( newIter );

    return true;
}

bool Pravala::DBusMessageReader::closeContainer()
{
    // Don't close the first iterator - it's the message, not a container.
    if ( _iters.size() < 2 )
    {
        LOG ( L_ERROR, "Cannot close container for message with no open containers [msg: " << _msg << "]" );

        return false;
    }

    LOG ( L_DEBUG4, "Closing container [msg: " << _msg << "]" );

    delete _iters.last();
    _iters.removeLast();

    return dbus_message_iter_next ( _iters.last() );
}
