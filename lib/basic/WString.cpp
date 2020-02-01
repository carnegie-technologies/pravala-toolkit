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

#include <cassert>
#include <clocale>

#include "WString.hpp"
#include "MsvcSupport.hpp"

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
#include <windows.h>
}
#endif

using namespace Pravala;

const WString WString::EmptyString;
const wchar_t WString::ZeroChar ( 0 );

#ifndef SYSTEM_WINDOWS

// This sets locale to UTF-8 at the beginning
// We need this for multi byte <-> wide char conversions.
// On Windows setting locale to UTF-8 doesn't work.
// We need to use different conversion functions and tell them to use UTF-8.

class LocaleSetter
{
    public:
        LocaleSetter()
        {
            setlocale ( LC_CTYPE, "UTF-8" );
        }
};

static LocaleSetter localeSetter;
#endif

WString::~WString()
{
    clear();
}

void WString::clear()
{
    delete[] _data;
    _data = 0;
    _length = 0;
}

WString::WString ( const WString & other ): _data ( 0 ), _length ( 0 )
{
    append ( other._data, other._length );
}

WString & WString::operator= ( const WString & other )
{
    if ( &other == this )
        return *this;

    // We don't use implicit sharing, so the other string should not be using the same memory.

    delete[] _data;
    _data = 0;
    _length = 0;

    append ( other._data, other._length );
    return *this;
}

WString & WString::operator= ( const wchar_t * str )
{
    // We can't delete the old _data right away, since 'str' may depend on it!
    wchar_t * const oldData = _data;

    _data = 0;
    _length = 0;

    append ( str );

    delete[] oldData;
    return *this;
}

WString & WString::append ( const WString & str )
{
    return append ( str._data, str._length );
}

WString & WString::append ( const wchar_t * data )
{
    if ( !data )
        return *this;

    return append ( data, ( int ) wcslen ( data ) );
}

WString & WString::append ( const wchar_t * data, int size )
{
    if ( size < 1 )
        return *this;

    // We can't delete the old _data right away, since 'data' may depend on it!
    wchar_t * const oldData = _data;

    _data = new wchar_t[ _length + size + 1 ];

    if ( _length > 0 )
        memcpy ( _data, oldData, sizeof ( wchar_t ) * _length );

    memcpy ( _data + _length, data, sizeof ( wchar_t ) * size );

    _length += size;

    _data[ _length ] = 0;

    delete[] oldData;
    return *this;
}

bool WString::operator== ( const WString & other ) const
{
    if ( &other == this )
        return true;

    if ( other._length != _length )
        return false;

    if ( _length < 1 )
    {
        assert ( other._length < 1 );
        return true;
    }

    return !memcmp ( _data, other._data, sizeof ( wchar_t ) * _length );
}

bool WString::operator== ( const wchar_t * str ) const
{
    const int oLen = ( str != 0 ) ? ( ( int ) wcslen ( str ) ) : 0;

    if ( oLen != _length )
        return false;

    if ( _length < 1 )
    {
        assert ( oLen == 0 );
        return true;
    }

    assert ( str != 0 );

    return !memcmp ( _data, str, sizeof ( wchar_t ) * _length );
}

wchar_t & WString::operator[] ( int idx )
{
    assert ( idx >= 0 );
    assert ( idx <= _length );
    assert ( _data != 0 );

    return _data[ idx ];
}

wchar_t WString::operator[] ( int idx ) const
{
    assert ( idx >= 0 );
    assert ( idx <= _length );
    assert ( _data != 0 );

    return _data[ idx ];
}

String WString::toString ( bool * isOk ) const
{
    // We set it to false when there is an error
    if ( isOk != 0 )
        *isOk = true;

    if ( !_data || _length < 1 || _data[ 0 ] == 0 )
        return String::EmptyString;

#ifdef SYSTEM_WINDOWS
    // Returns the number of bytes needed, including null

    const int cSize = WideCharToMultiByte ( CP_UTF8, 0, _data, -1, 0, 0, NULL, NULL );

    if ( cSize < 2 )
    {
        // cSize = 0 means "error",
        // cSize = 1 means "empty string" (it includes null)
        //          this should not happen since we check data[0] at the beginning, but it doesn't hurt
        // In both cases there is nothing to do, but we need to set isOk to false if cSize is not 1!

        if ( isOk != 0 && cSize != 1 )
            *isOk = false;

        return String::EmptyString;
    }

    char * const cData = new char[ cSize ];
    const int cSizeCheck = WideCharToMultiByte ( CP_UTF8, 0, _data, -1, cData, cSize, NULL, NULL );

    ( void ) cSizeCheck;

    assert ( cSizeCheck == cSize );
    assert ( cData[ cSize - 1 ] == 0 );

    String ret;

    ret.append ( cData, cSize - 1 );

    delete[] cData;
    return ret;
#else
    mbstate_t mbsState;
    memset ( &mbsState, 0, sizeof ( mbsState ) );

    const wchar_t * ptr = _data;
    const size_t cSize = wcsrtombs ( 0, &ptr, 0, &mbsState );

    // There was an error:
    if ( cSize == ( size_t ) ( -1 ) || !mbsinit ( &mbsState ) )
    {
        if ( isOk != 0 )
            *isOk = false;

        return String::EmptyString;
    }

    char * const cData = new char[ cSize + 1 ];

    memset ( &mbsState, 0, sizeof ( mbsState ) );
    ptr = _data;

    // cSize + 1 to include the null character
    const size_t cSizeCheck = wcsrtombs ( cData, &ptr, cSize + 1, &mbsState );

    ( void ) cSizeCheck;

    assert ( !ptr );
    assert ( cSizeCheck == cSize );
    assert ( cData[ cSize ] == 0 );

    String ret;

    ret.append ( cData, ( int ) cSize );

    delete[] cData;
    return ret;
#endif
}

// This is String's method, but it makes sense to have wchar conversions in the same place.
WString String::toWString ( bool * isOk ) const
{
// We set it to false when there is an error
    if ( isOk != 0 )
        *isOk = true;

    if ( length() < 1 )
        return WString::EmptyString;

    const char * ptr = c_str();

#ifdef SYSTEM_WINDOWS
    // Returns the number of wchars needed, including null

    const int wSize = MultiByteToWideChar ( CP_UTF8, 0, ptr, -1, 0, 0 );

    if ( wSize < 2 )
    {
        if ( wSize < 1 && isOk != 0 )
            *isOk = false;

        return WString::EmptyString;
    }

    wchar_t * const wData = new wchar_t[ wSize ];

    const int wSizeCheck = MultiByteToWideChar ( CP_UTF8, 0, ptr, -1, wData, wSize );

    ( void ) wSizeCheck;

    assert ( wSizeCheck == wSize );
    assert ( wData[ wSize - 1 ] == 0 );

    WString ret;

    ret.append ( wData, wSize - 1 );

    delete[] wData;
    return ret;
#else
    mbstate_t mbsState;
    memset ( &mbsState, 0, sizeof ( mbsState ) );

    // wSize doesn't include null
    const size_t wSize = mbsrtowcs ( 0, &ptr, 0, &mbsState );

    // There was an error:
    if ( wSize == ( size_t ) ( -1 ) || !mbsinit ( &mbsState ) )
    {
        if ( isOk != 0 )
            *isOk = false;

        return WString::EmptyString;
    }

    wchar_t * const wData = new wchar_t[ wSize + 1 ];

    memset ( &mbsState, 0, sizeof ( mbsState ) );
    ptr = c_str();

    // wSize + 1 to include the null character
    const size_t wSizeCheck = mbsrtowcs ( wData, &ptr, wSize + 1, &mbsState );

    ( void ) wSizeCheck;

    assert ( !ptr );
    assert ( mbsinit ( &mbsState ) );
    assert ( wSizeCheck == wSize );
    assert ( wData[ wSize ] == 0 );

    WString ret;

    ret.append ( wData, ( int ) wSize );

    delete[] wData;
    return ret;
#endif
}

size_t Pravala::getHash ( const WString & key )
{
    unsigned int hash = 5381;
    const wchar_t * str = key.c_wstr();
    const wchar_t * c;

    // Adapted from getHash(String)
    // http://www.cse.yorku.ca/~oz/hash.html

    while ( *( c = ( str++ ) ) != 0 )
    {
        for ( size_t i = 0; i < sizeof ( wchar_t ); ++i )
        {
            hash = ( ( hash << 5 ) + hash ) + ( ( const char * ) c )[ i ]; /* hash * 33 + c[i] */
        }
    }

    return hash;
}
