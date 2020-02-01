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

#include "DBusMessageWriter.hpp"
#include "DBusMessageReader.hpp"

namespace DBusC
{
extern "C" {
#include <dbus/dbus.h>
}
}

using namespace Pravala;

Pravala::DBusMessageWriter::DBusMessageWriter (
        const String & destination, const String & path, const String & iface, const String & method ):
    DBusMessage ( destination, String::EmptyString, path, iface, method ),
    _appendMode ( Basic ),
    _allowBasicAppend ( false ),
    _valid ( false )
{
    _msg = DBusC::dbus_message_new_method_call ( _dest.c_str(), _path.c_str(), _iface.c_str(), _method.c_str() );

    if ( !_msg )
    {
        LOG ( L_ERROR, "Error generating a DBus request message; Dest: "
              << _dest << "; Path: " << _path << "; Iface: " << _iface << "; Method: " << _method );

        return;
    }

    DBusC::DBusMessageIter * iter = new DBusC::DBusMessageIter();

    dbus_message_iter_init_append ( _msg, iter );

    _iters.append ( iter );

    _valid = true;
}

Pravala::DBusMessageWriter::DBusMessageWriter ( DBusMessageReader & request ):
    DBusMessage (
            request.getSender(),
            String::EmptyString,
            request.getPath(),
            request.getInterface(),
            request.getMethod() ),
    _appendMode ( Basic ),
    _allowBasicAppend ( false ),
    _valid ( false )
{
    _msg = DBusC::dbus_message_new_method_return ( request.getInternalMessage() );

    if ( !_msg )
    {
        LOG ( L_ERROR, "Error generating a DBus response message; Dest: "
              << _dest << "; Path: " << _path << "; Iface: " << _iface << "; Method: " << _method );

        return;
    }

    DBusC::DBusMessageIter * iter = new DBusC::DBusMessageIter();

    dbus_message_iter_init_append ( _msg, iter );

    _iters.append ( iter );

    _valid = true;
}

uint32_t Pravala::DBusMessageWriter::getReplySerial() const
{
    return ( _msg != 0 ) ? dbus_message_get_reply_serial ( _msg ) : 0;
}

template<typename T> bool Pravala::DBusMessageWriter::appendArg (
        int type,
        const char * typeSig,
        const char * typeDesc,
        const T & value,
        const bool asVariant )
{
    ( void ) typeDesc;

    if ( !_valid || _iters.size() < 1 || ( ( _appendMode != Basic ) && !_allowBasicAppend ) )
    {
        return false;
    }

    DBusC::DBusMessageIter variantIter;

    DBusC::DBusMessageIter * baseIter = _iters.last();
    DBusC::DBusMessageIter * iter = baseIter;

    if ( asVariant )
    {
        iter = &variantIter;

        if ( !dbus_message_iter_open_container ( baseIter, DBUS_TYPE_VARIANT, typeSig, iter ) )
        {
            return false;
        }
    }

    if ( !iter || !dbus_message_iter_append_basic ( iter, type, &value ) )
    {
        LOG ( L_ERROR, "Error appending " << typeDesc << " value " << value
              << " to iterator " << iter << " [msg: " << _msg << "]" );

        if ( asVariant )
        {
            _valid = dbus_message_iter_close_container ( baseIter, iter );
        }

        return false;
    }

    LOG ( L_DEBUG4, "Appended " << typeDesc << " value " << value
          << " to iterator " << iter << " [msg: " << _msg << "]" );

    if ( asVariant )
    {
        _valid = dbus_message_iter_close_container ( baseIter, iter );
    }

    return _valid;
}

#define SIMPLE_APPEND_DBUS_ARG_FUNC( type, dbus_type ) \
    bool Pravala::DBusMessageWriter::appendArg ( type value, bool asVariant ) { \
        return appendArg<type> ( DBUS_TYPE_ ## dbus_type, \
                                 DBUS_TYPE_ ## dbus_type ## _AS_STRING, \
                                 #dbus_type, \
                                 value, \
                                 asVariant ); \
    }

SIMPLE_APPEND_DBUS_ARG_FUNC ( char, BYTE )
SIMPLE_APPEND_DBUS_ARG_FUNC ( int8_t, BYTE )
SIMPLE_APPEND_DBUS_ARG_FUNC ( uint8_t, BYTE )
SIMPLE_APPEND_DBUS_ARG_FUNC ( int16_t, INT16 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( uint16_t, UINT16 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( int32_t, INT32 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( uint32_t, UINT32 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( int64_t, INT64 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( uint64_t, UINT64 )
SIMPLE_APPEND_DBUS_ARG_FUNC ( double, DOUBLE )

// While appending, we can use "simple" append functions for strings (unlike in "get next" case).
SIMPLE_APPEND_DBUS_ARG_FUNC ( const char *, STRING )

bool DBusMessageWriter::appendArg ( bool value, bool asVariant )
{
    // DBus represents booleans with 32 bits. Possible values are 0 or 1.
    return appendArg<uint32_t> ( DBUS_TYPE_BOOLEAN,
                                 DBUS_TYPE_BOOLEAN_AS_STRING,
                                 "BOOLEAN",
                                 value ? 1 : 0,
                                 asVariant );
}

bool DBusMessageWriter::appendObjectPath ( const String & value, bool asVariant )
{
    return appendArg<const char *> ( DBUS_TYPE_OBJECT_PATH,
                                     DBUS_TYPE_OBJECT_PATH_AS_STRING,
                                     "OBJECT_PATH",
                                     value.c_str(),
                                     asVariant );
}

bool DBusMessageWriter::openDictEntry ( const String & key )
{
    // We need at least two iterators - one for the message, one for the dictionary itself.
    if ( !_valid || _appendMode != DictEntry || _iters.size() < 2 )
    {
        return false;
    }

    DBusC::DBusMessageIter * const dictIter = new DBusC::DBusMessageIter();

    if ( !dbus_message_iter_open_container ( _iters.last(), DBUS_TYPE_DICT_ENTRY, 0, dictIter ) )
    {
        delete dictIter;

        return false;
    }

    _iters.append ( dictIter );

    _allowBasicAppend = true;

    appendArg ( key );

    _allowBasicAppend = false;

    return true;
}

bool DBusMessageWriter::closeDictEntry()
{
    // The first iterator must be for the message, then one for the dictionary and one for the dictionary entry
    if ( !_valid || _appendMode != DictEntry || _iters.size() < 3 )
    {
        return false;
    }

    DBusC::DBusMessageIter * const dictIter = _iters.last();
    _iters.removeLast();

    _valid = dbus_message_iter_close_container ( _iters.last(), dictIter );

    delete dictIter;

    return _valid;
}

bool DBusMessageWriter::openDictionary()
{
    if ( !_valid || _appendMode != Basic || _iters.size() < 1 )
    {
        return false;
    }

    DBusC::DBusMessageIter * newIter = new DBusC::DBusMessageIter();

    // A dictionary entry will always contain a string and variant
    if ( !dbus_message_iter_open_container ( _iters.last(), DBUS_TYPE_ARRAY, "{sv}", newIter ) )
    {
        delete newIter;

        return false;
    }

    _iters.append ( newIter );

    _appendMode = DictEntry;

    return true;
}

bool DBusMessageWriter::closeDictionary()
{
    // The first iterator must be for the message, and then at least one more for the dictionary itself.
    if ( !_valid || _appendMode != DictEntry || _iters.size() < 2 )
    {
        return false;
    }

    DBusC::DBusMessageIter * dictIter = _iters.last();

    _iters.removeLast();

    _valid = dbus_message_iter_close_container ( _iters.last(), dictIter );

    delete dictIter;

    _appendMode = Basic;

    return _valid;
}
