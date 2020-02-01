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

#include "DBusMessage.hpp"

namespace DBusC
{
extern "C" {
#include <dbus/dbus.h>
}
}

using namespace Pravala;

TextLog Pravala::DBusMessage::_log ( "dbus_message" );

Pravala::DBusMessage::DBusMessage (
        const String & destination,
        const String & sender,
        const String & path,
        const String & iface,
        const String & method ):
    _dest ( destination ),
    _sender ( sender ),
    _path ( path ),
    _iface ( iface ),
    _method ( method )
{
}

Pravala::DBusMessage::~DBusMessage()
{
    for ( size_t i = 0; i < _iters.size(); ++i )
    {
        delete _iters[ i ];
    }

    _iters.clear();

    if ( _msg != 0 )
    {
        dbus_message_unref ( _msg );
    }
}

uint32_t Pravala::DBusMessage::getSerial() const
{
    return ( _msg != 0 ) ? dbus_message_get_serial ( _msg ) : 0;
}

Pravala::DBusMessage::Type Pravala::DBusMessage::getType() const
{
    if ( _msg != 0 )
    {
        switch ( dbus_message_get_type ( _msg ) )
        {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                return TypeMethodCall;
                break;

            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                return TypeMethodReturn;
                break;

            case DBUS_MESSAGE_TYPE_ERROR:
                return TypeError;
                break;

            case DBUS_MESSAGE_TYPE_SIGNAL:
                return TypeSignal;
                break;

            case DBUS_MESSAGE_TYPE_INVALID:
                break;
        }
    }

    return TypeInvalid;
}

const char * Pravala::DBusMessage::getTypeName ( Pravala::DBusMessage::Type type )
{
    switch ( type )
    {
        case TypeMethodCall:
            return "Method-Call";
            break;

        case TypeMethodReturn:
            return "Method-Return";
            break;

        case TypeError:
            return "Error";
            break;

        case TypeSignal:
            return "Signal";
            break;

        case TypeInvalid:
            break;
    }

    return "Invalid";
}

void Pravala::DBusMessage::describe ( Buffer & buf ) const
{
    buf.append ( String ( "D-BusMessage; Type: %1; Dest: %2; Path: %3; Iface: %4; Method: %5" )
                 .arg ( getTypeName(), _dest, _path, _iface, _method ) );

    if ( _msg != 0 )
    {
        buf.append ( String ( "; Serial: %1" ).arg ( getSerial() ) );
    }
}

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const Pravala::DBusMessage & value )
{
    value.describe ( textMessage.getInternalBuf() );

    return textMessage;
}

#define CASE_ARG_TYPE_DUMP_BASIC( iter, buf, type, dbus_type ) \
    case DBUS_TYPE_ ## dbus_type: \
        { \
            type val; \
            dbus_message_iter_get_basic ( iter, &val ); \
            buf.append ( String ( " %1: '%2'" ).arg ( #dbus_type ).arg ( val ) ); \
        } \
        break

#define CASE_ARG_TYPE_OPEN_CONTAINER( curIter, iterList, dbus_type ) \
    case DBUS_TYPE_ ## dbus_type: \
        { \
            buf.append ( String ( " %1 {" ).arg ( #dbus_type ) ); \
            DBusC::DBusMessageIter * newIter = new DBusC::DBusMessageIter(); \
            dbus_message_iter_recurse ( curIter, newIter ); \
            iterList.append ( newIter ); \
        } \
        break

String Pravala::DBusMessage::dumpArgs() const
{
    List<DBusC::DBusMessageIter *> iters;

    DBusC::DBusMessageIter * firstIter = new DBusC::DBusMessageIter();

    dbus_message_iter_init ( _msg, firstIter );

    iters.append ( firstIter );

    int valType = DBUS_TYPE_INVALID;

    Buffer buf;
    buf.append ( "Argument dump" );

    while ( !iters.isEmpty() )
    {
        DBusC::DBusMessageIter * const iter = iters.last();

        valType = dbus_message_iter_get_arg_type ( iter );

        if ( valType == DBUS_TYPE_INVALID )
        {
            delete iters.last();

            iters.removeLast();

            // For everything but the last iterator (which is the whole message), we want to
            // append the closing brace for the container.
            if ( !iters.isEmpty() )
            {
                buf.append ( " }" );
            }

            continue;
        }

        switch ( valType )
        {
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, bool, BOOLEAN );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, char, BYTE );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, int16_t, INT16 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, uint16_t, UINT16 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, int32_t, INT32 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, uint32_t, UINT32 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, int64_t, INT64 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, uint64_t, UINT64 );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, double, DOUBLE );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, const char *, STRING );
            CASE_ARG_TYPE_DUMP_BASIC ( iter, buf, const char *, OBJECT_PATH );

            CASE_ARG_TYPE_OPEN_CONTAINER ( iter, iters, VARIANT );
            CASE_ARG_TYPE_OPEN_CONTAINER ( iter, iters, ARRAY );
            CASE_ARG_TYPE_OPEN_CONTAINER ( iter, iters, DICT_ENTRY );
            CASE_ARG_TYPE_OPEN_CONTAINER ( iter, iters, STRUCT );

            default:
                buf.append ( String ( "; Unsupported argument type (%1)" ).arg ( valType ) );
                break;
        }

        // If we just opened a container, 'iter' is still the base container. We are done with the current
        // argument (either we read the basic type, or opened the container), so we want to read the next
        // argument the next time we use this iterator.
        dbus_message_iter_next ( iter );
    }

    return buf.toString();
}

bool Pravala::DBusMessage::isLegalMemberName ( const String & name )
{
    // Must only contain the ASCII characters "[A-Z][a-z][0-9]_" and may not begin with a digit.
    // Must not contain the '.' (period) character.
    // Must not exceed the maximum name length.
    // Must be at least 1 byte in length.

    if ( name.length() < 1 || name.length() > DBUS_MAXIMUM_NAME_LENGTH )
        return false;

    for ( int i = 0; i < name.length(); ++i )
    {
        const char c = name[ i ];

        if ( !( c == '_'
                || ( 'A' <= c && c <= 'Z' )
                || ( 'a' <= c && c <= 'z' )
                || ( i > 0 && '0' <= c && c <= '9' ) ) )
        {
            return false;
        }
    }

    return true;
}

bool Pravala::DBusMessage::isLegalInterfaceName ( const String & name )
{
    // Interface names are composed of 1 or more elements separated by a period ('.') character.
    // All elements must contain at least one character.
    // Each element must only contain the ASCII characters "[A-Z][a-z][0-9]_" and must not begin with a digit.
    // Interface names must contain at least one '.' (period) character (and thus at least two elements).
    // Interface names must not begin with a '.' (period) character.
    // Interface names must not exceed the maximum name length.

    if ( name.length() < 1 || name.length() > DBUS_MAXIMUM_NAME_LENGTH )
        return false;

    StringList list = name.split ( ".", true );

    if ( list.size() < 2 )
        return false;

    for ( size_t i = 0; i < list.size(); ++i )
    {
        if ( !isLegalMemberName ( list.at ( i ) ) )
            return false;
    }

    return true;
}
