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
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cctype>

#include "NoCopy.hpp"

#include "String.hpp"
#include "Buffer.hpp"
#include "IpAddress.hpp"
#include "MsvcSupport.hpp"

#if !defined( _MSC_VER ) \
    && !defined( PLATFORM_ANDROID ) \
    && !defined( PLATFORM_OPENWRT )
using std::strtoll;
using std::strtoull;
using std::strtof;
#endif

/// @todo This needs to be improved
#define NEW_BUF_SIZE( n )    ( ( n ) * 3 / 2 )

// Minimum buffer size to allocate
// This value has been chosen based on analysis of typical string sizes used by our application
// We always add 1 byte for NULL character at the end. We want the memory size to be nice and round,
// so we use 35 here, which results in 36 bytes allocated (at least)
#define MIN_BUF_SIZE    35

using namespace Pravala;

const String String::EmptyString;
const char String::ZeroChar ( 0 );

namespace Pravala
{
/// @brief Private data of the string.
/// To be shared between multiple String objects.
/// This class is NOT thread safe!
class StringPriv: public NoCopy
{
    public:
        /// @brief Constructor.
        /// Creates an empty StringPriv object with reference
        /// count set to 1 and memory of at least given size allocated.
        /// @param [in] initialSize Minimal size of memory allocated.
        StringPriv ( int initialSize );

        /// @brief Copy constructor.
        /// Creates an empty StringPriv object with reference
        /// count set to 1 and no memory allocated.
        /// It DOES copy the memory from the original StringPriv.
        /// It DOES remove one reference from the original StringPriv.
        /// This class is NOT thread safe!
        StringPriv ( StringPriv * other, int additionalSize );

        /// @brief Destructor
        ~StringPriv();

        /// @brief Appends memory segment onto the end of StringPriv's data.
        /// Should be only called when ref.count() = 1 (so only on a non-shared copy
        /// of StringPriv).
        /// It allocates more memory if needed. It deallocates old memory segment
        /// AFTER appending new data, so appending String to itself should be safe.
        /// Note that this function might allocate more memory than actually needed,
        /// so future appends are more efficient (and require less reallocation).
        /// To make sure that no memory is wasted, use squeeze().
        /// @param [in] data Memory to append.
        /// @param [in] size Size of that memory.
        void append ( const char * data, int size );

        /// @brief Preallocate memory for future appends.
        /// Should be only called when ref.count() = 1 (so only on a non-shared copy
        /// of StringPriv). If there is already some data in the string, it reserves
        /// total amount needed to store both existing and expected data.
        /// It deletes the old data segment (if a new one was allocated) after
        /// existing data has been copied to the new segment, so if this function
        /// is used in something like append(String & other) it will NOT handle
        /// appending string to itself properly - the old data might be deallocated here.
        /// Note that this function might allocate more memory than actually needed,
        /// so future appends are more efficient (and require less reallocation).
        /// To make sure that no memory is wasted, use squeeze().
        /// @param [in] size Size of memory to preallocate (on to of current length of this string).
        void reserve ( int size );

        /// @brief Returns a const reference to a character at the given index.
        /// Index has to be from valid range (0:length) - getting idx = length()
        /// is legal and should always return 0. If index is invalid the program crashes.
        /// @param [in] idx Index.
        /// @return Constant reference to the character.
        inline char at ( int idx ) const
        {
            assert ( idx >= 0 );
            assert ( idx <= length );
            assert ( buffer != 0 );

            if ( idx >= 0 && idx <= length )
            {
                return buffer[ idx ];
            }

            abort();
        }

        /// @brief Returns a reference to a character at the given index.
        /// Index has to be from valid range (0:length) - getting idx = length()
        /// is legal and should always return 0. If index is invalid the program crashes.
        /// @param [in] idx Index.
        /// @return Reference to the character.
        inline char & at ( int idx )
        {
            assert ( idx >= 0 );
            assert ( idx <= length );
            assert ( buffer != 0 );

            if ( idx >= 0 && idx <= length )
            {
                return buffer[ idx ];
            }

            abort();
        }

        /// @brief Reallocates memory so only the amount needed is used.
        void squeeze();

        /// @brief Pointer to allocated memory.
        char * buffer;

        /// @brief Length of the data that is actually used.
        int length;

        /// @brief Size of allocated memory that can be used by the string content.
        /// The memory actually allocated is always one byte bigger (for the NULL character at the end)
        int bufSize;

        /// @brief Reference counter for this shared data.
        SharedMemory::RefCounter ref;

    private:
        /// @brief Doesn't exist.
        StringPriv();
};
}

StringPriv::StringPriv ( int initialSize ):
    buffer ( 0 ), length ( 0 ), bufSize ( initialSize )
{
    assert ( ref.count() == 1 );
    assert ( bufSize >= 0 );

    if ( bufSize < MIN_BUF_SIZE )
        bufSize = MIN_BUF_SIZE;

    length = 0;

    buffer = static_cast<char *> ( malloc ( bufSize + 1 ) );

    assert ( buffer != 0 );

    if ( buffer != 0 )
    {
        buffer[ 0 ] = 0;
    }
}

StringPriv::StringPriv ( StringPriv * other, int additionalSize ):
    buffer ( 0 ), length ( 0 ), bufSize ( additionalSize )
{
    assert ( !other || other->ref.count() > 1 );
    assert ( !other || other->length >= 0 );
    assert ( bufSize >= 0 );

    if ( bufSize < 0 )
        bufSize = 0;

    if ( other != 0 )
        bufSize += other->length;

    if ( bufSize < MIN_BUF_SIZE )
        bufSize = MIN_BUF_SIZE;

    length = 0;

    buffer = static_cast<char *> ( malloc ( bufSize + 1 ) );

    assert ( buffer != 0 );

    if ( !buffer )
        return;

    buffer[ 0 ] = 0;

    if ( other != 0 )
    {
        append ( other->buffer, other->length );
        other->ref.unref();
    }
}

StringPriv::~StringPriv()
{
    assert ( ref.count() == 0 );

    if ( ref.count() < 1 )
    {
        assert ( ref.count() == 0 );

        free ( buffer );
        buffer = 0;
        length = bufSize = 0;
    }
}

void StringPriv::squeeze()
{
    // This can be called even if ref.count() != 1

    if ( length < 1 )
    {
        assert ( length == 0 );

        if ( buffer != 0 )
        {
            length = bufSize = 0;
            free ( buffer );
            buffer = 0;
        }

        assert ( !buffer );
        assert ( length == 0 );
        assert ( bufSize == 0 );
    }
    else if ( length < bufSize )
    {
        bufSize = length;

        char * const newBuffer = static_cast<char *> ( realloc ( buffer, bufSize + 1 ) );

        if ( newBuffer != 0 )
        {
            buffer = newBuffer;
        }
        else
        {
            abort();
        }

        assert ( buffer != 0 );
        assert ( length > 0 );

        buffer[ length ] = 0;
    }
}

void StringPriv::reserve ( int size )
{
    assert ( ref.count() == 1 );

    if ( size < 1 ) return;

    assert ( size > 0 );
    assert ( length <= bufSize );

    if ( length + size > bufSize )
    {
        bufSize = NEW_BUF_SIZE ( bufSize );

        if ( bufSize < MIN_BUF_SIZE )
            bufSize = MIN_BUF_SIZE;

        if ( bufSize < length + size )
            bufSize = length + size;

        char * const newBuffer = static_cast<char *> ( realloc ( buffer, bufSize + 1 ) );

        if ( newBuffer != 0 )
        {
            buffer = newBuffer;
        }
        else
        {
            abort();
        }
        assert ( buffer != 0 );
        assert ( length + size <= bufSize );
        assert ( length <= bufSize );

        buffer[ length ] = 0;
    }
}

void StringPriv::append ( const char * data, int size )
{
    assert ( ref.count() == 1 );

    if ( size < 1 ) return;

    // We can't use reserve here. We don't want to delete
    // the old buffer in case one string is appended to itself!

    assert ( size > 0 );
    assert ( length <= bufSize );

    // If the [data; data+size] overlaps in any way with our buffer.
    // This happens when appending (a part of) a string to itself.

    const bool memOverlap = ( data >= buffer && data <= buffer + bufSize );

    // We don't need to check whether the 'data+size' overlaps with
    // our buffer, or whether our buffer is inside the [data; data+size]
    // range - we know that our own 'buffer' points to the beginning of the
    // chunk, and we know the size of the whole chunk - so if
    // the 'data' doesn't overlap with our buffer, but they are
    // both associated with the same memory chunk, then there is something
    // seriously wrong with the memory management!
    assert ( memOverlap || ( data + size <= buffer || data >= buffer + bufSize ) );

    if ( length + size > bufSize )
    {
        bufSize = NEW_BUF_SIZE ( bufSize );

        if ( bufSize < MIN_BUF_SIZE )
            bufSize = MIN_BUF_SIZE;

        if ( bufSize < length + size )
            bufSize = length + size;

        char * orgBuf = buffer;

        if ( memOverlap )
        {
            buffer = static_cast<char *> ( malloc ( bufSize + 1 ) );

            assert ( buffer != 0 );

            if ( !buffer )
            {
                return;
            }

            if ( length > 0 )
            {
                assert ( orgBuf != 0 );
                assert ( bufSize > 0 );

                memcpy ( buffer, orgBuf, length );
            }
        }
        else
        {
            buffer = static_cast<char *> ( realloc ( buffer, bufSize + 1 ) );
        }

        assert ( length < bufSize );
        assert ( length + size <= bufSize );

        // Here we don't need to use memmove, because
        // if we were overlapping, then buffer is, at this point,
        // a completely new buffer!
        memcpy ( buffer + length, data, size );

        length += size;

        assert ( length <= bufSize );

        buffer[ length ] = 0;

        if ( memOverlap )
        {
            assert ( orgBuf != 0 );

            // In case one string is appended to itself we have to free
            // this buffer AFTER copying the data!
            // Also, this is the reason why we can't use realloc here!
            free ( orgBuf );
        }
    }
    else
    {
        // buffer+length is our destination memory.
        // Either the entire 'data' is in front of it, or it starts after:
        assert ( ( data < buffer + length && data + size <= buffer + length )
                 || ( buffer + length + size <= data ) );

        // We just use 'memcpy'. We are appending, so in the place where we're writing TO
        // there should be no usable data at this point!
        memcpy ( buffer + length, data, size );

        length += size;

        assert ( length <= bufSize );

        buffer[ length ] = 0;
    }

    assert ( length <= bufSize );
}

String::String ( const char * data, int size ): _priv ( 0 )
{
    if ( size > 0 )
    {
        assert ( data != 0 );

        append ( data, size );

        assert ( _priv != 0 );
        assert ( _priv->ref.count() == 1 );
    }
}

String::String ( const char * str ): _priv ( 0 )
{
    if ( !str )
        return;

    append ( str );

    assert ( strlen ( str ) < 1 || _priv != 0 );
    assert ( strlen ( str ) < 1 || _priv->ref.count() == 1 );
}

String::~String()
{
    if ( _priv != 0 && _priv->ref.unref() )
    {
        assert ( _priv->ref.count() == 0 );
        delete _priv;
    }

    _priv = 0;
}

String::String ( const String & other ): _priv ( const_cast<StringPriv *> ( other._priv ) )
{
    if ( _priv != 0 )
    {
        _priv->ref.ref();
    }
}

String & String::operator= ( const String & other )
{
    if ( &other == this || _priv == other._priv )
        return *this;

    // Unlink from that data. If we were the only user of it - delete it
    if ( _priv != 0 && _priv->ref.unref() )
    {
        assert ( _priv->ref.count() == 0 );
        delete _priv;
    }

    _priv = const_cast<StringPriv *> ( other._priv );

    if ( _priv != 0 )
        _priv->ref.ref();

    return *this;
}

String & String::operator= ( const char * str )
{
    if ( !_priv )
    {
        append ( str );
        return *this;
    }

    if ( !str )
    {
        clear();
        return *this;
    }

    if ( _priv->ref.count() == 1 )
    {
        _priv->length = 0;
        append ( str );
        return *this;
    }

    // To keep the existing memory around (we may be copying from it)
    StringPriv * oldPriv = _priv;

    _priv = 0;
    append ( str );

    assert ( oldPriv != 0 );

    oldPriv->ref.unref();
    oldPriv = 0;

    return *this;
}

bool String::isEmpty() const
{
    if ( !_priv )
        return true;

    return ( _priv->length <= 0 );
}

int String::length() const
{
    if ( !_priv )
        return 0;

    assert ( _priv != 0 );

    return _priv->length;
}

int String::getAllocatedSize() const
{
    if ( !_priv )
        return 0;

    assert ( _priv != 0 );

    return _priv->bufSize;
}

const char * String::c_str() const
{
    if ( !_priv || _priv->length < 1 )
    {
        return &ZeroChar;
    }

    assert ( _priv != 0 );

    return _priv->buffer;
}

int String::getRefCount() const
{
    if ( !_priv )
        return 1;

    return _priv->ref.count();
}

bool String::operator== ( const String & other ) const
{
    // Both strings are sharing the same data - they're for sure equal!
    if ( _priv == other._priv )
        return true;

    int oLen = other.length();
    int len = length();

    if ( oLen != len )
        return false;

    if ( len < 1 )
    {
        assert ( len == 0 );
        assert ( oLen == 0 );
        return true;
    }

    assert ( other._priv != 0 );
    assert ( other._priv->buffer != 0 );
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    return !memcmp ( _priv->buffer, other._priv->buffer, len );
}

bool String::operator== ( const char * str ) const
{
    if ( !str )
    {
        return isEmpty();
    }

    const int oLen = ( int ) strlen ( str );
    const int len = length();

    if ( oLen != len )
        return false;

    if ( len < 1 )
    {
        assert ( len == 0 );
        assert ( oLen == 0 );
        return true;
    }

    assert ( str != 0 );
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    return !memcmp ( _priv->buffer, str, len );
}

bool String::operator< ( const String & other ) const
{
    // Both strings are sharing the same data - they're equal!
    if ( _priv == other._priv )
        return false;

    int oLen = other.length();

    if ( oLen < 1 )
        return false;

    int len = length();

    assert ( other._priv != 0 );

    if ( len < 1 )
        return true;

    assert ( other._priv != 0 );
    assert ( other._priv->buffer != 0 );
    assert ( oLen > 0 );
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );
    assert ( len > 0 );

    if ( oLen < len )
        len = oLen;

    // len + 1 to correctly handle strings of different lengths:
    // buf[len] always constains \0 (or at least it should ;)
    return ( memcmp ( _priv->buffer, other._priv->buffer, len + 1 ) < 0 );
}

bool String::operator> ( const String & other ) const
{
    // Both strings are sharing the same data - they're equal!
    if ( _priv == other._priv )
        return false;

    int oLen = other.length();

    if ( oLen < 1 )
        return false;

    int len = length();

    assert ( other._priv != 0 );

    if ( len < 1 )
        return true;

    assert ( other._priv != 0 );
    assert ( other._priv->buffer != 0 );
    assert ( oLen > 0 );
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );
    assert ( len > 0 );

    if ( oLen < len )
        len = oLen;

    // len + 1 to correctly handle strings of different lengths:
    // buf[len] always constains \0 (or at least it should ;)
    return ( memcmp ( _priv->buffer, other._priv->buffer, len + 1 ) > 0 );
}

int String::compare (
        const char * str1, int len1,
        const char * str2, int len2,
        bool caseSensitive, int len )
{
    int sLen = len1;

    if ( len >= 0 && sLen > len ) sLen = len;

    if ( sLen > len2 ) sLen = len2;

    for ( int i = 0; i < sLen; ++i )
    {
        int a = caseSensitive ? str1[ i ] : tolower ( str1[ i ] );
        int b = caseSensitive ? str2[ i ] : tolower ( str2[ i ] );

        if ( a < b ) return -1;

        if ( a > b ) return 1;
    }

    if ( sLen == len && len >= 0 ) return 0;

    if ( len1 < len2 ) return -1;

    if ( len1 > len2 ) return 1;

    return 0;
}

char & String::operator[] ( int idx )
{
    ensureOwnCopy ( 0 );

    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    return _priv->at ( idx );
}

char String::operator[] ( int idx ) const
{
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    return _priv->at ( idx );
}

void String::reserve ( int size )
{
    if ( size > 0 )
    {
        ensureOwnCopy ( size );

        assert ( _priv != 0 );
        assert ( _priv->ref.count() == 1 );
        _priv->reserve ( size );
    }
}

void String::squeeze()
{
    if ( !_priv )
        return;

    // No reference checks, we squeeze all strings sharing the same memory.
    _priv->squeeze();
}

void String::clear()
{
    if ( length() > 0 )
    {
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        if ( _priv->ref.count() > 1 )
        {
            _priv->ref.unref();
            _priv = 0;
        }
        else
        {
            assert ( _priv->ref.count() == 1 );
            _priv->length = 0;
        }
    }
}

void String::ensureOwnCopy ( int additionalSize )
{
    if ( !_priv || _priv->ref.count() > 1 )
    {
        // This DOES remove a reference count
        // This DOES copy existing data
        _priv = new StringPriv ( _priv, additionalSize );

        assert ( _priv != 0 );
        assert ( _priv->ref.count() == 1 );
    }

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );
}

String & String::append ( const String & str )
{
    if ( str.length() < 1 )
        return *this;

    ensureOwnCopy ( str.length() );

    assert ( _priv != 0 );

    _priv->append ( str.c_str(), str.length() );

    return *this;
}

String & String::append ( const char * str )
{
    if ( !str )
        return *this;

    const int len = ( int ) strlen ( str );

    if ( len < 1 )
        return *this;

    ensureOwnCopy ( len );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );

    _priv->append ( str, len );

    assert ( _priv->ref.count() == 1 );

    return *this;
}

String & String::append ( const char * data, int size )
{
    if ( size < 1 )
        return *this;

    ensureOwnCopy ( size );

    assert ( _priv != 0 );

    _priv->append ( data, size );

    return *this;
}

String & String::append ( char ch )
{
    ensureOwnCopy ( 1 );

    assert ( _priv != 0 );

    _priv->append ( &ch, 1 );

    return *this;
}

String & String::arg ( const char * a )
{
    return replaceArgs ( 1, String ( a ) );
}

String & String::arg ( char * a )
{
    return replaceArgs ( 1, String ( a ) );
}

String & String::arg ( char c )
{
    return replaceArgs ( 1, String ( &c, 1 ) );
}

String & String::arg ( const String & a )
{
    return replaceArgs ( 1, a );
}

String & String::arg ( const IpAddress & a )
{
    return replaceArgs ( 1, a.toString() );
}

String & String::arg ( const SockAddr & a )
{
    return replaceArgs ( 1, a.toString() );
}

String & String::arg ( const String & a1, const String & a2 )
{
    return replaceArgs ( 2, a1, a2 );
}

String & String::arg ( const String & a1, const String & a2, const String & a3 )
{
    return replaceArgs ( 3, a1, a2, a3 );
}

String & String::arg ( const String & a1, const String & a2, const String & a3, const String & a4 )
{
    return replaceArgs ( 4, a1, a2, a3, a4 );
}

String & String::arg ( const String & a1, const String & a2, const String & a3, const String & a4, const String & a5 )
{
    return replaceArgs ( 5, a1, a2, a3, a4, a5 );
}

String & String::arg (
        const String & a1, const String & a2, const String & a3,
        const String & a4, const String & a5, const String & a6 )
{
    return replaceArgs ( 6, a1, a2, a3, a4, a5, a6 );
}

String & String::arg (
        const String & a1, const String & a2, const String & a3,
        const String & a4, const String & a5, const String & a6,
        const String & a7 )
{
    return replaceArgs ( 7, a1, a2, a3, a4, a5, a6, a7 );
}

String & String::arg (
        const String & a1, const String & a2, const String & a3,
        const String & a4, const String & a5, const String & a6,
        const String & a7, const String & a8 )
{
    return replaceArgs ( 8, a1, a2, a3, a4, a5, a6, a7, a8 );
}

String & String::arg (
        const String & a1, const String & a2, const String & a3,
        const String & a4, const String & a5, const String & a6,
        const String & a7, const String & a8, const String & a9 )
{
    return replaceArgs ( 9, a1, a2, a3, a4, a5, a6, a7, a8, a9 );
}

String & String::replaceArgs (
        int numValues,
        const String & a1, const String & a2, const String & a3,
        const String & a4, const String & a5, const String & a6,
        const String & a7, const String & a8, const String & a9 )
{
    assert ( numValues > 0 );
    assert ( numValues <= 9 );

    StringList strList;

    if ( numValues > 0 ) strList.append ( a1 );

    if ( numValues > 1 ) strList.append ( a2 );

    if ( numValues > 2 ) strList.append ( a3 );

    if ( numValues > 3 ) strList.append ( a4 );

    if ( numValues > 4 ) strList.append ( a5 );

    if ( numValues > 5 ) strList.append ( a6 );

    if ( numValues > 6 ) strList.append ( a7 );

    if ( numValues > 7 ) strList.append ( a8 );

    if ( numValues > 8 ) strList.append ( a9 );

    return replaceArgs ( strList );
}

String & String::replaceArgs ( const StringList & strList )
{
    assert ( strList.size() > 0 );
    assert ( strList.size() <= 99 );

    if ( isEmpty() || strList.isEmpty() )
        return *this;

    const int sz = length();
    const int lSize = ( int ) ( ( strList.size() <= 99 ) ? ( strList.size() ) : 99 );

    Buffer buf;

    buf.getAppendable ( sz + 1 );

    const char * c = c_str();

    if ( !c )
    {
        clear();
        return *this;
    }

    for ( int i = 0; i < sz; ++i )
    {
        if ( i + 1 >= sz
             || c[ i ] != '%'
             || c[ i + 1 ] < '1'
             || c[ i + 1 ] > '9' )
        {
            // It is one of:
            // - last character
            // - not '%'
            // - not follow by a digit between 1 and 9
            // Append, skip
            buf.appendData ( c + i, 1 );
            continue;
        }

        // We have '%N'. We can work with that!
        // c[i] is '%', so we need to skip it first.
        int numValue = c[ ++i ] - '0';

        if ( i + 1 < sz && c[ i + 1 ] >= '0' && c[ i + 1 ] <= '9' )
        {
            // Another digit! It is '%NM'.
            // Let's skip the first digit and add the second one:
            numValue = ( numValue * 10 ) + c[ ++i ] - '0';
        }

        assert ( numValue > 0 );
        assert ( numValue <= 99 );

        if ( numValue > lSize )
        {
            // We don't want to replace it yet, just decrement the argument number

            buf.append ( "%" );
            buf.append ( String::number ( numValue - lSize ) );
        }
        else
        {
            buf.append ( strList.at ( numValue - 1 ) );
        }
    }

    clear();
    append ( buf.get(), ( int ) buf.size() );

    return *this;
}

bool String::matches ( const char * pattern, const char * str )
{
    assert ( str != 0 );
    assert ( pattern != 0 );

    if ( !str || !pattern )
        return false;

    int idx = -1;

    do
    {
        ++idx;

        // Both are empty - match

        if ( !pattern[ idx ] && !str[ idx ] ) return true;

        assert ( str[ idx ] != 0 || pattern[ idx ] != 0 );

        // If there is nothing left in the pattern,
        // (but since we got here we know there is something in str)
        // there is no match.
        if ( !pattern[ idx ] ) return false;

        // If there is nothing else in the str, but there is something in the
        // pattern, we have a match if all that is left in the pattern
        // is some number of '*' signs. If there is anything else - there is no match.
        if ( !str[ idx ] )
        {
            while ( pattern[ idx ] == '*' )
            {
                ++idx;
            }

            if ( !pattern[ idx ] ) return true;

            return false;
        }

        if ( pattern[ idx ] == '*' )
        {
            int pidx = idx;

            // In case someone gave multiple '*' next to each other
            // in the pattern, we want to skip to the last one

            while ( pattern[ pidx + 1 ] == '*' )
            {
                ++pidx;
            }

            // We want to try to interpret '*' as 0, 1, 2, 3, etc. characters
            // and see if we can find a match.

            do
            {
                // Next character in 'str' and in the pattern are the same
                // - try to run matches() for that sub-pattern and sub-string
                if ( str[ idx ] == pattern[ pidx + 1 ]
                     && matches ( pattern + pidx + 1, str + idx ) ) return true;
            }
            while ( str[ idx++ ] != 0 ); // Post-increment!

            return false;
        }

        assert ( pattern[ idx ] != '*' );
    }
    while ( pattern[ idx ] == '?' || pattern[ idx ] == str[ idx ] );

    return false;
}

String String::substr ( int pos, int n ) const
{
    String ret;
    int len = length();

    if ( n == 0 || len < 1 || pos >= len )
        return ret;

    len -= pos;

    if ( n > 0 && n < len )
        len = n;

    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );
    assert ( _priv->bufSize >= pos + len );

    ret.append ( _priv->buffer + pos, len );

    return ret;
}

String String::toLower() const
{
    String ret;

    ret.reserve ( length() );

    int len = length();

    for ( int i = 0; i < len; ++i )
    {
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        ret.append ( tolower ( _priv->buffer[ i ] ) );
    }

    return ret;
}

String String::toUpper() const
{
    String ret;

    ret.reserve ( length() );

    int len = length();

    for ( int i = 0; i < len; ++i )
    {
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        ret.append ( toupper ( _priv->buffer[ i ] ) );
    }

    return ret;
}

String String::capitalize() const
{
    String ret ( *this );

    if ( length() > 0 )
    {
        ret[ 0 ] = toupper ( operator[] ( 0 ) );
    }

    return ret;
}

String String::trimmed() const
{
    const int len = length();

    if ( len < 1 ) return String();

    int beg = 0;
    int end = len - 1;

    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    while ( beg < len && isSpaceOrNull ( _priv->buffer[ beg ] ) )
    {
        ++beg;
    }

    // We don't test index 0 here. if it is whitespace,
    // 'beg' would detect it anyway.
    while ( end > 0 && isSpaceOrNull ( _priv->buffer[ end ] ) )
    {
        --end;
    }

    if ( end >= beg ) return substr ( beg, end - beg + 1 );

    return String();
}

String String::reversed() const
{
    const int len = length();

    String ret;

    if ( len < 1 ) return ret;

    ret.reserve ( len );

    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    for ( int i = len; i > 0; --i )
    {
        ret.append ( _priv->buffer[ i - 1 ] );
    }

    return ret;
}

String String::simplified() const
{
    int len = length();

    String ret;

    if ( len < 1 )
        return ret;

    ret.reserve ( len );

    bool appended = false;
    bool hasSpace = false;

    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    for ( int i = 0; i < len; ++i )
    {
        if ( isSpaceOrNull ( _priv->buffer[ i ] ) )
        {
            hasSpace = true;
        }
        else
        {
            if ( hasSpace && appended )
            {
                ret.append ( ' ' );
            }

            ret.append ( _priv->buffer[ i ] );

            appended = true;
            hasSpace = false;
        }
    }

    return ret;
}

String String::removeChars ( const char * toRemove ) const
{
    const int rmLen = ( toRemove != 0 ) ? strlen ( toRemove ) : 0;
    const int len = length();

    if ( rmLen < 1 || len < 1 )
    {
        return *this;
    }

    String ret;

    for ( int i = 0; i < len; ++i )
    {
        char c = operator[] ( i );

        for ( int j = 0; j < rmLen; ++j )
        {
            if ( c == toRemove[ j ] )
            {
                c = 0;
                break;
            }
        }

        if ( c != 0 )
        {
            ret.append ( c );
        }
    }

    return ret;
}

String String::replace ( const char * what, int wLen, const char * withWhat, int wwLen ) const
{
    if ( wLen < 1 )
        return *this;

    String ret;

    int len = length();

    if ( len < 1 )
        return ret;

    int endPos = 0;
    int pos = -1;

    while ( ( pos = find ( what, endPos ) ) >= 0 )
    {
        assert ( pos >= 0 );
        assert ( pos < len );
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        ret.append ( _priv->buffer + endPos, pos - endPos );
        ret.append ( withWhat, wwLen );

        endPos = pos + wLen;
    }

    assert ( endPos <= len );
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    ret.append ( _priv->buffer + endPos, len - endPos );

    return ret;
}

String String::replace ( const char * what, const char * withWhat ) const
{
    const int wLen = ( int ) strlen ( what );

    if ( wLen < 1 )
        return *this;

    return replace ( what, wLen, withWhat, ( int ) strlen ( withWhat ) );
}

String String::replace ( const char * what, const String & withWhat ) const
{
    const int wLen = ( int ) strlen ( what );

    if ( wLen < 1 )
        return *this;

    return replace ( what, wLen, withWhat.c_str(), withWhat.length() );
}

bool String::startsWith ( const String & str, bool caseSensitive ) const
{
    if ( str.length() > length() )
        return false;

    for ( int i = 0; i < str.length(); ++i )
    {
        if ( caseSensitive )
        {
            if ( tolower ( operator[] ( i ) ) != tolower ( str[ i ] ) )
                return false;
        }
        else
        {
            if ( operator[] ( i ) != str[ i ] )
                return false;
        }
    }

    return true;
}

bool String::endsWith ( const String & str, bool caseSensitive ) const
{
    if ( str.length() > length() )
        return false;

    int otherIdx = 0;
    int myIdx = length() - str.length();

    assert ( myIdx >= 0 );

    for ( ; otherIdx < str.length(); ++otherIdx, ++myIdx )
    {
        if ( caseSensitive )
        {
            if ( tolower ( operator[] ( myIdx ) ) != tolower ( str[ otherIdx ] ) )
                return false;
        }
        else
        {
            if ( operator[] ( myIdx ) != str[ otherIdx ] )
                return false;
        }
    }

    return true;
}

int String::findFirstOf ( const String & str, int pos ) const
{
    int sLen = str.length();
    int len = length();

    if ( sLen < 1 || len < 1 || pos >= len || pos < 0 )
        return -1;

    int i;

    for ( ; pos < len; ++pos )
    {
        // Otherwise length() == 0
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        /// @note This could probably be faster :)

        for ( i = 0; i < sLen; ++i )
        {
            // Otherwise str.length() == 0
            assert ( str._priv != 0 );
            assert ( str._priv->buffer != 0 );

            if ( _priv->buffer[ pos ] == str._priv->buffer[ i ] )
                return pos;
        }
    }

    return -1;
}

int String::findLastOf ( const String & str, int pos ) const
{
    int sLen = str.length();
    int len = length();

    if ( pos < 0 )
        pos = len - 1;

    if ( sLen < 1 || len < 1 || pos >= len || pos < 0 )
        return -1;

    int i;

    for ( ; pos >= 0; --pos )
    {
        // Otherwise length() == 0
        assert ( _priv != 0 );
        assert ( _priv->buffer != 0 );

        /// @note This could probably be faster :)

        for ( i = 0; i < sLen; ++i )
        {
            // Otherwise str.length() == 0
            assert ( str._priv != 0 );
            assert ( str._priv->buffer != 0 );

            if ( _priv->buffer[ pos ] == str._priv->buffer[ i ] )
                return pos;
        }
    }

    return -1;
}

int String::find ( char c, int pos ) const
{
    int len = length();

    if ( len < 1 || pos >= len || pos < 0 )
        return -1;

    // Length would be 0
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );

    for ( ; pos < len; ++pos )
    {
        if ( _priv->buffer[ pos ] == c )
            return pos;
    }

    return -1;
}

int String::find ( const char * str, int sLen, int pos ) const
{
    int len = length();

    if ( len < 1 || pos >= len || pos < 0 )
        return -1;

    if ( sLen < 1 )
        return 0;

    if ( pos + sLen > len )
        return -1;

    // Length would be 0
    assert ( _priv != 0 );
    assert ( _priv->buffer != 0 );
    assert ( pos + sLen <= len );

    // There is no point in checking positions that are closer to the end
    // of this string than sLen.
    len -= ( sLen - 1 );

    int idx;

    for ( ; pos < len; ++pos )
    {
        for ( idx = 0; idx < sLen; ++idx )
        {
            assert ( idx < length() );

            if ( str[ idx ] != _priv->buffer[ pos + idx ] )
                break;
        }

        if ( idx == sLen )
            return pos;
    }

    return -1;
}

String String::join ( const StringList & strList, const String & separator )
{
    String ret;
    const int size = ( int ) strList.size();
    const int sSize = separator.length();
    int reqSize = 0;

    assert ( sSize >= 0 );
    assert ( size >= 0 );

    for ( int i = 0; i < size; ++i )
    {
        if ( i > 0 )
        {
            reqSize += sSize;
        }

        reqSize += strList[ i ].length();
    }

    ret.reserve ( reqSize );

    for ( int i = 0; i < size; ++i )
    {
        if ( i > 0 )
        {
            ret.append ( separator );
        }

        ret.append ( strList[ i ] );
    }

    return ret;
}

bool String::toBool ( bool * ok ) const
{
    if ( !compare ( "1", false )
         || !compare ( "on", false )
         || !compare ( "yes", false )
         || !compare ( "true", false ) )
    {
        if ( ok != 0 )
            *ok = true;

        return true;
    }
    else if ( !compare ( "0", false )
              || !compare ( "off", false )
              || !compare ( "no", false )
              || !compare ( "false", false ) )
    {
        if ( ok != 0 )
            *ok = true;

        return false;
    }

    if ( ok != 0 )
        *ok = false;

    return false;
}

int8_t String::toInt8 ( bool * ok, int base ) const
{
    const long ret = toLong ( ok, base );

    if ( ret <= 0x7F && ret >= -0x80 )
        return ( int8_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( int8_t ) ( ( ret > 0 ) ? 0x7F : -0x80 );
}

uint8_t String::toUInt8 ( bool * ok, int base ) const
{
    const unsigned long ret = toULong ( ok, base );

    if ( ret <= 0xFF )
        return ( uint8_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( uint8_t ) 0xFF;
}

int16_t String::toInt16 ( bool * ok, int base ) const
{
    const long ret = toLong ( ok, base );

    if ( ret <= 0x7FFF && ret >= -0x8000 )
        return ( int16_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( int16_t ) ( ( ret > 0 ) ? 0x7FFF : -0x8000 );
}

uint16_t String::toUInt16 ( bool * ok, int base ) const
{
    const unsigned long ret = toULong ( ok, base );

    if ( ret <= 0xFFFF )
        return ( uint16_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( uint16_t ) 0xFFFF;
}

// Clang in some Android NDK versions has issues with the range checks...
#if defined( __clang_major__ ) && ( __clang_major__ == 6 )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-constant-compare"
#endif

int32_t String::toInt32 ( bool * ok, int base ) const
{
    const long ret = toLong ( ok, base );

    if ( ret <= 0x7FFFFFFFL && ret >= ( -0x7FFFFFFFL - 1 ) )
        return ( int32_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( int32_t ) ( ( ret > 0 ) ? 0x7FFFFFFFL : ( -0x7FFFFFFFL - 1 ) );
}

uint32_t String::toUInt32 ( bool * ok, int base ) const
{
    const unsigned long ret = toULong ( ok, base );

    if ( ret <= 0xFFFFFFFFUL )
        return ( uint32_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( uint32_t ) 0xFFFFFFFFUL;
}

int64_t String::toInt64 ( bool * ok, int base ) const
{
    const long long ret = toLongLong ( ok, base );

    if ( ret <= 0x7FFFFFFFFFFFFFFFLL )
        return ( int64_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( int64_t ) 0x7FFFFFFFFFFFFFFFLL;
}

uint64_t String::toUInt64 ( bool * ok, int base ) const
{
    const unsigned long long ret = toULongLong ( ok, base );

    if ( ret <= 0xFFFFFFFFFFFFFFFFULL )
        return ( uint64_t ) ret;

    if ( ok != 0 )
        *ok = false;

    return ( uint64_t ) 0xFFFFFFFFFFFFFFFFULL;
}

#if defined( __clang_major__ ) && ( __clang_major__ == 6 )
#pragma GCC diagnostic pop
#endif

long String::toLong ( bool * ok, int base ) const
{
    long ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtol ( c_str(), &end_ptr, base );

    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length ) )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

unsigned long String::toULong ( bool * ok, int base ) const
{
    unsigned long ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtoul ( c_str(), &end_ptr, base );

    // We check if there are any signs of errors.
    // Because strtoul() interprets negative values as (very large) positive values,
    // we use strtol() to make sure it was not negative.
    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length )
         || strtol ( c_str(), 0, base ) < 0 )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

long long String::toLongLong ( bool * ok, int base ) const
{
    long long ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtoll ( c_str(), &end_ptr, base );

    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length ) )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

unsigned long long String::toULongLong ( bool * ok, int base ) const
{
    unsigned long long ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtoull ( c_str(), &end_ptr, base );

    // We check if there are any signs of errors.
    // Because strtoul() interprets negative values as (very large) positive values,
    // we use strtol() to make sure it was not negative.
    // Because we don't actually care about the exact value, we don't need to use strtoll().
    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length )
         || strtol ( c_str(), 0, base ) < 0 )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

float String::toFloat ( bool * ok ) const
{
    float ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtof ( c_str(), &end_ptr );

    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length ) )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

double String::toDouble ( bool * ok ) const
{
    double ret;
    char * end_ptr;

    if ( length() < 1 )
    {
        if ( ok != 0 )
            *ok = false;

        return 0;
    }

    assert ( _priv != 0 );

    errno = 0;

    ret = strtod ( c_str(), &end_ptr );

    if ( errno != 0
         || end_ptr == c_str()
         || end_ptr != ( _priv->buffer + _priv->length ) )
    {
        if ( ok != 0 )
            *ok = false;
    }
    else if ( ok != 0 )
    {
        *ok = true;
    }

    return ret;
}

// long double String::toLongDouble ( bool * ok ) const
// {
//     long double ret;
//     char *end_ptr;
//
//     if ( length() < 1 )
//     {
//         if ( ok != 0 )
//             *ok = false;
//
//         return 0;
//     }
//
//     assert (_priv != 0);
//
//     errno = 0;
//
//     ret = strtold ( c_str(), &end_ptr );
//
//     if ( errno != 0
//             || end_ptr == c_str()
//             || end_ptr != ( _priv->buffer + _priv->length ) )
//     {
//         if ( ok != 0 )
//             *ok = false;
//     }
//
//     return ret;
// }

size_t Pravala::getHash ( const String & key )
{
    unsigned int hash = 5381;
    const char * str = key.c_str();
    int c;

    // http://www.cse.yorku.ca/~oz/hash.html

    while ( ( c = *( str++ ) ) != 0 )
    {
        hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */
    }

    return hash;
}

#define CONV_BUF_SIZE           32
#define CONV_FORMAT_BUF_SIZE    20

template<typename T> inline static String doConv (
        T num, const char * lenModif, char format, int width,
        bool zeroFill, int prec )
{
    char convFmtBuf[ CONV_FORMAT_BUF_SIZE + 1 ];

    if ( width > 0 && prec > 0 )
    {
        if ( width >= CONV_BUF_SIZE )
            width = CONV_BUF_SIZE - 1;

        if ( prec >= CONV_BUF_SIZE )
            prec = CONV_BUF_SIZE - 1;

        if ( width + prec >= CONV_BUF_SIZE )
        {
            width = ( CONV_BUF_SIZE - 1 ) / 2;
            prec = ( CONV_BUF_SIZE - 1 ) / 2;
        }

        snprintf ( convFmtBuf, CONV_FORMAT_BUF_SIZE, "%%%s%d.%d%s%c",
                   zeroFill ? "0" : "", width, prec, lenModif, format );
    }
    else if ( width > 0 )
    {
        if ( width >= CONV_BUF_SIZE )
            width = CONV_BUF_SIZE - 1;

        snprintf ( convFmtBuf, CONV_FORMAT_BUF_SIZE, "%%%s%d%s%c",
                   zeroFill ? "0" : "", width, lenModif, format );
    }
    else if ( prec > 0 )
    {
        if ( prec >= CONV_BUF_SIZE )
            prec = CONV_BUF_SIZE - 1;

        snprintf ( convFmtBuf, CONV_FORMAT_BUF_SIZE, "%%%s.%d%s%c",
                   zeroFill ? "0" : "", prec, lenModif, format );
    }
    else
    {
        snprintf ( convFmtBuf, CONV_FORMAT_BUF_SIZE, "%%%s%s%c",
                   zeroFill ? "0" : "", lenModif, format );
    }

    convFmtBuf[ CONV_FORMAT_BUF_SIZE ] = 0;

    char convBuf[ CONV_BUF_SIZE + 1 ];

    snprintf ( convBuf, CONV_BUF_SIZE, convFmtBuf, num );
    convBuf[ CONV_BUF_SIZE ] = 0;

    return String ( convBuf );
}

inline static char intFormat ( String::IntStyle style, char defFormat )
{
    switch ( style )
    {
        case String::Int_Dec:
            // We return 'defFormat', because it depends on the type (unlike octal and hex styles).
            return defFormat;
            break;

        case String::Int_Oct:
            return 'o';
            break;

        case String::Int_Hex:
            return 'x';
            break;

        case String::Int_HEX:
            return 'X';
            break;
    }

    return defFormat;
}

inline static char doubleFormat ( String::DoubleStyle style )
{
    switch ( style )
    {
        case String::Double_f:
            return 'f';
            break;

        case String::Double_F:
            return 'F';
            break;

        case String::Double_e:
            return 'e';
            break;

        case String::Double_E:
            return 'E';
            break;

        case String::Double_g:
            return 'g';
            break;

        case String::Double_G:
            return 'G';
            break;

        case String::Double_a:
            return 'a';
            break;

        case String::Double_A:
            return 'A';
            break;
    }

    return 'f';
}

String String::number (
        char num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( ( int ) num, "", intFormat ( style, 'd' ), width, zeroFill, 0 );
}

String String::number (
        short num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( ( int ) num, "", intFormat ( style, 'd' ), width, zeroFill, 0 );
}

String String::number (
        int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "", intFormat ( style, 'd' ), width, zeroFill, 0 );
}

String String::number (
        long int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "l", intFormat ( style, 'd' ), width, zeroFill, 0 );
}

String String::number (
        long long int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "ll", intFormat ( style, 'd' ), width, zeroFill, 0 );
}

String String::number (
        unsigned char num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( ( unsigned int ) num, "", intFormat ( style, 'u' ), width, zeroFill, 0 );
}

String String::number (
        unsigned short int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( ( unsigned int ) num, "", intFormat ( style, 'u' ), width, zeroFill, 0 );
}

String String::number (
        unsigned int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "", intFormat ( style, 'u' ), width, zeroFill, 0 );
}

String String::number (
        unsigned long int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "l", intFormat ( style, 'u' ), width, zeroFill, 0 );
}

String String::number (
        unsigned long long int num, String::IntStyle style,
        int width, bool zeroFill )
{
    return doConv ( num, "ll", intFormat ( style, 'u' ), width, zeroFill, 0 );
}

String String::number (
        double num, String::DoubleStyle style,
        int width, bool zeroFill, int precision )
{
    return doConv ( num, "", doubleFormat ( style ), width, zeroFill, precision );
}

String String::number (
        long double num, String::DoubleStyle style,
        int width, bool zeroFill, int precision )
{
    return doConv ( num, "L", doubleFormat ( style ), width, zeroFill, precision );
}

String String::number ( bool num )
{
    if ( num )
        return "true";

    return "false";
}

void String::hexDesc ( char buf[ 2 ], uint8_t num, bool useLowercase )
{
    uint8_t v = ( num >> 4 ) & 0xF;
    buf[ 0 ] = ( v < 10 ) ? ( v + '0' ) : ( v - 10 + ( useLowercase ? 'a' : 'A' ) );

    v = num & 0xF;
    buf[ 1 ] = ( v < 10 ) ? ( v + '0' ) : ( v - 10 + ( useLowercase ? 'a' : 'A' ) );
}

String String::hexDump (
        const char * data, size_t dataSize, bool include0x,
        const String & separator, bool useLowercase )
{
    String ret;

    // We have dataSize bytes, each byte will, at most, use:
    // 2 bytes with '0x' + 2 bytes for the actual value + separator.length()
    // Plus the \0' at the end. +2 just in case

    ret.reserve ( ( int ) ( dataSize * ( 2 + 2 + separator.length() ) + 2 ) );

    for ( size_t idx = 0; idx < dataSize; ++idx )
    {
        if ( !ret.isEmpty() )
            ret.append ( separator );

        if ( include0x )
            ret.append ( "0x" );

        char buf[ 2 ];
        hexDesc ( buf, data[ idx ] & 0xFF, useLowercase );

        ret.append ( buf, 2 );
    }

    return ret;
}

StringList String::toStringList ( const char * data, size_t size, const String & separators, bool keepEmpty )
{
    assert ( data != 0 || size < 1 );

    StringList ret;
    size_t beg = 0;
    bool found;

    do
    {
        found = false;

        for ( size_t i = beg; !found && i < size; ++i )
        {
            for ( int j = 0; !found && j < separators.length(); ++j )
            {
                if ( data[ i ] == separators[ j ] )
                {
                    if ( keepEmpty || i > beg )
                    {
                        ret.append ( String ( data + beg, ( int ) ( i - beg ) ) );
                    }

                    beg = i + 1;
                    found = true;
                }
            }
        }

        if ( keepEmpty && beg == size )
        {
            // We want that 'extra' empty element. Either the last byte in the memory
            // was the separator, or the memory was empty.
            ret.append ( String() );
        }
    }
    while ( found && beg < size );

    if ( beg < size )
    {
        ret.append ( String ( data + beg, ( int ) ( size - beg ) ) );
    }

    return ret;
}
