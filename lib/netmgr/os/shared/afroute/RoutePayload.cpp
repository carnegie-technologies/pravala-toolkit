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

#include "RoutePayload.hpp"

#include <cassert>

extern "C"
{
#include <net/if.h>
#include <net/if_dl.h>
}

// Apple lacks the CLLADDR macro
#ifndef CLLADDR
#define CLLADDR( s )    ( ( const char * ) ( ( s )->sdl_data + ( s )->sdl_nlen ) )
#endif

/// @brief Returns a rounded up so it is evenly divisible by size
/// @param [in] a Number
/// @param [in] size Boundary to round number up to
/// @return a, rounded up to be divisible by size
template<typename T> inline T roundUp ( T a, T size )
{
    if ( size == 0 )
    {
        return a;
    }
    return ( ( ( a & ( size - 1 ) ) != 0 ) ? ( 1 + ( a | ( size - 1 ) ) ) : a );
}

using namespace Pravala;

TextLog RoutePayload::_log ( "route_payload" );

RoutePayload::RoutePayload(): _isValid ( false )
{
    memset ( &_ai, 0, sizeof ( _ai ) );
}

bool RoutePayload::setup ( int fieldsPresent, const MemHandle & payload )
{
    memset ( &_ai, 0, sizeof ( _ai ) );
    _payload.clear();
    _isValid = false;

    if ( fieldsPresent != 0 && payload.isEmpty() )
    {
        LOG ( L_ERROR, "Parsing a payload with non-zero fields present but no data; this is invalid" );
        return false;
    }

    if ( payload.size() < 1 )
    {
        assert ( fieldsPresent == 0 );
        _isValid = true;
        return true;
    }

    _payload = payload;

    size_t offset = 0;

    // We build up a new version of 'fieldsPresent' to compare against the one included in the message.
    // If the payload doesn't contain all of the fields present we can detect this by comparing the two after.
    int parsedFieldsPresent = 0;

    for ( size_t i = 0; i < FieldMax && offset < _payload.size(); ++i )
    {
        if ( ( fieldsPresent & ( 1 << i ) ) == 0 )
        {
            // it isn't present, and we already memset it to 0 above.
            assert ( !_ai.rti_info[ i ] );
            continue;
        }

        parsedFieldsPresent |= ( 1 << i );

        const struct sockaddr * s = ( const struct sockaddr * ) ( _payload.get ( offset ) );

        // Cast, because rti_info doesn't contain const ptrs and clang complains
        _ai.rti_info[ i ] = const_cast<struct sockaddr *> ( s );

        // We are getting an offset from the payload, this should never yield 0
        assert ( s != 0 );

        // Fields can return a size of 0, but they actually contain a 4 byte value
        // This is true of any possible field; not limited to Netmask or Genmask (i.e. also BRD)
        // See UNIX Network Programming (Stevens), Version 3, Volume 1, Pg. 493-494
        if ( s->sa_len == 0 )
        {
            offset += 4;
        }
        else
        {
            // The payloads are padded to the nearest 4 bytes
            // (but this padding isn't contained in sa_len),
            // so round any off-byte-boundary lengths up
            offset += roundUp ( ( int ) s->sa_len, 4 );
        }
    }

    // If we've iterated through all of the entries and their sizes total the expected payload size,
    // and the fields present equals what was present in the payload then the payload has been successfully parsed.
    if ( offset == _payload.size() && fieldsPresent == parsedFieldsPresent )
    {
        LOG ( L_DEBUG4, "Successfully parsed an AF_ROUTE payload of size " << _payload.size()
              << "; fieldsPresent is " << fieldsPresent );

        _isValid = true;
        _ai.rti_addrs = fieldsPresent;
        return true;
    }
    else
    {
        if ( offset != _payload.size() )
        {
            LOG ( L_ERROR, "Parsing AF_ROUTE payload of size " << _payload.size() << " resulted in an offset of "
                  << offset << ". This is invalid. fieldsPresent is " << fieldsPresent );
        }
        else if ( fieldsPresent != parsedFieldsPresent )
        {
            LOG ( L_ERROR, "Parsing AF_ROUTE payload of size " << _payload.size() << " resulted in a fieldsPresent of "
                  << parsedFieldsPresent << " but the payload should have had the fields " << fieldsPresent
                  << " present. This is invalid" );
        }

        memset ( &_ai, 0, sizeof ( _ai ) );
        _payload.clear();
        _isValid = false;
        return false;
    }
}

bool RoutePayload::getAddress ( Field field, IpAddress & addr )
{
    // If the payload is invalid or the field isn't in the structure, we can't retrieve an address.
    if ( !contains ( field ) )
        return false;

    const struct sockaddr * s = _ai.rti_info[ field ];

    if ( !s )
        return false;

    if ( s->sa_family == AF_INET && s->sa_len == sizeof ( struct sockaddr_in ) )
    {
        const struct sockaddr_in * sin = ( const struct sockaddr_in * ) s;
        addr.setupV4Memory ( ( const char * ) &( sin->sin_addr ) );
        return true;
    }
    else if ( s->sa_family == AF_INET6 && s->sa_len == sizeof ( struct sockaddr_in6 ) )
    {
        const struct sockaddr_in6 * sin6 = ( const struct sockaddr_in6 * ) s;
        addr.setupV6Memory ( ( const char * ) &( sin6->sin6_addr ) );
        return true;
    }
    // This means an empty IPv4 address
    else if ( s->sa_len == 0 )
    {
        addr = IpAddress::Ipv4ZeroAddress;
        return true;
    }

    LOG ( L_ERROR, "Attempting to retrieve an IP address from field " << field
          << " but this field lacks a valid IP address; family: " << s->sa_family << "; sa_len: " << s->sa_len );

    return false;
}

bool RoutePayload::getNetmask ( Field field, IpAddress & addr )
{
    if ( !contains ( field ) )
        return false;

    const struct sockaddr * s = _ai.rti_info[ field ];

    if ( !s )
        return false;

    if ( ( s->sa_family == AF_INET && s->sa_len == sizeof ( struct sockaddr_in ) )
         || ( s->sa_family == AF_INET6 && s->sa_len == sizeof ( struct sockaddr_in6 ) )
         || s->sa_len == 0 )
    {
        return getAddress ( field, addr );
    }

    // If it's not a valid IP address (len and type match), then parse it out ourselves.
    // See UNIX Network Programming (Stevens) Volume 1, Edition 3, pg. 495 for reference.

    // The routing socket sometimes encodes the netmask field with only some of the sin_addr bytes
    // set (unset bytes are interpreted to be 0). For example, if the length is 5, only the first byte
    // of sin_addr is valid, and the remaining 3 bytes are interpreted as being set to 0.
    // We index to sa_data at offset 2 to skip the port (2 bytes) from the sockaddr_in.
    //
    // We subtract 4 from sa_len since 2 bytes are represented by the sin_family, and 2 bytes are represented
    // by the port we skipped.
    if ( s->sa_len >= 5 && s->sa_len <= 8 )
    {
        struct in_addr v4addr;
        memset ( &v4addr, 0, sizeof ( v4addr ) );
        memcpy ( &v4addr, ( s->sa_data + 2 ), s->sa_len - 4 );

        addr = IpAddress ( v4addr );
        return true;
    }
    // The same principle applies for IPv6 netmasks as was described above.
    // This is not documented in Stevens, but was observed to work on OS X and QNX.
    else if ( s->sa_len >= 12 && s->sa_len <= 16 )
    {
        struct in6_addr v6addr;
        memset ( &v6addr, 0, sizeof ( v6addr ) );
        memcpy ( &v6addr, ( s->sa_data + 2 ), s->sa_len - 4 );

        addr = IpAddress ( v6addr );
        return true;
    }

    LOG ( L_ERROR, "Attempting to parse an invalid netmask with sa_len of " << s->sa_len );
    return false;
}

bool RoutePayload::getName ( Field field, String & str )
{
    if ( !contains ( field ) )
        return false;

    const struct sockaddr * s = _ai.rti_info[ field ];

    if ( !s || s->sa_family != AF_LINK )
        return false;

    const struct sockaddr_dl * sdl = ( const struct sockaddr_dl * ) s;

    if ( sdl->sdl_nlen > sdl->sdl_len )
    {
        LOG ( L_ERROR, "Link name is specified as larger than the entire field, this is wrong" );
        return false;
    }

    str = String ( sdl->sdl_data, sdl->sdl_nlen );
    return true;
}

bool RoutePayload::getLLAddr ( Field field, String & lladdr )
{
    if ( !contains ( field ) )
        return false;

    const struct sockaddr * s = _ai.rti_info[ field ];

    if ( !s || s->sa_family != AF_LINK )
        return false;

    const struct sockaddr_dl * sdl = ( const struct sockaddr_dl * ) s;

    if ( ( sdl->sdl_nlen + sdl->sdl_alen ) > sdl->sdl_len )
    {
        LOG ( L_ERROR, "Link name + link layer addr length is larger than the entire field, this is wrong" );
        return false;
    }

    lladdr = String ( CLLADDR ( sdl ), sdl->sdl_alen );
    return true;
}
