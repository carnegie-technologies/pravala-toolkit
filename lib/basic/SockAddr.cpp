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

#include "Math.hpp"
#include "SockAddr.hpp"
#include "IpAddress.hpp"

using namespace Pravala;

const SockAddr Pravala::Ipv4ZeroSockAddress ( IpAddress ( "0.0.0.0" ), 0 );
const SockAddr Pravala::Ipv6ZeroSockAddress ( IpAddress ( "::" ), 0 );
const SockAddr Pravala::EmptySockAddress;

SockAddr::SockAddr ( const IpAddress & addr, uint16_t port )
{
    memset ( this, 0, sizeof ( SockAddr ) );

    if ( addr.isIPv4() )
    {
        sa.sa_family = AF_INET;
        sa_in.sin_addr = addr.getV4();
        sa_in.sin_port = htons ( port );

// BSD systems define sin_len (and sin6_len) as a field; POSIX (including Linux) doesn't.
// Historical reason; neither defines its existence or lack of existence since each thinks they are canonical.
// (POSIX doesn't include sin_len, but doesn't prevent other implementations from extending sockaddr_in to include it).
#ifdef SYSTEM_UNIX
        sa_in.sin_len = sizeof ( sa_in );
#endif
    }
    else if ( addr.isIPv6() )
    {
        sa.sa_family = AF_INET6;
        sa_in6.sin6_addr = addr.getV6();
        sa_in6.sin6_port = htons ( port );

// See above.
#ifdef SYSTEM_UNIX
        sa_in6.sin6_len = sizeof ( sa_in6 );
#endif
    }
}

SockAddr::SockAddr ( const struct sockaddr_in & sAddr )
{
    memset ( this, 0, sizeof ( SockAddr ) );

    sa_in = sAddr;
}

SockAddr::SockAddr ( const struct sockaddr_in6 & sAddr )
{
    memset ( this, 0, sizeof ( SockAddr ) );

    sa_in6 = sAddr;
}

SockAddr::SockAddr ( const struct sockaddr * sAddr, size_t sAddrLen )
{
    memset ( this, 0, sizeof ( SockAddr ) );

    if ( !sAddr )
    {
        return;
    }

    if ( sAddrLen > 0 )
    {
        memcpy ( this, sAddr, min ( sizeof ( *this ), sAddrLen ) );
        return;
    }

    // No length passed, let's assume it's correct for the type (if recognized).

    if ( sAddr->sa_family == AF_INET )
    {
        memcpy ( this, sAddr, sizeof ( struct sockaddr_in ) );
    }
    else if ( sAddr->sa_family == AF_INET6 )
    {
        memcpy ( this, sAddr, sizeof ( struct sockaddr_in6 ) );
    }
}

IpAddress SockAddr::getAddr() const
{
    return IpAddress ( *this );
}

bool SockAddr::setAddr ( const IpAddress & addr )
{
    if ( addr.isIPv4() )
    {
        return setAddr ( AF_INET, &addr.getV4(), sizeof ( sa_in.sin_addr ) );
    }

    if ( addr.isIPv6() )
    {
        return setAddr ( AF_INET6, &addr.getV6(), sizeof ( sa_in6.sin6_addr ) );
    }

    return false;
}

bool SockAddr::setAddr ( unsigned short family, const void * addr, size_t addrLen )
{
    if ( !addr )
    {
        return false;
    }

    const uint16_t orgPort
        = ( sa.sa_family == AF_INET ) ? sa_in.sin_port : ( ( sa.sa_family == AF_INET6 ) ? sa_in6.sin6_port : 0 );

    if ( family == AF_INET && addrLen >= sizeof ( sa_in.sin_addr ) )
    {
        clear();

        sa.sa_family = AF_INET;
        sa_in.sin_port = orgPort;

        memcpy ( &sa_in.sin_addr, addr, sizeof ( sa_in.sin_addr ) );

// See above.
#ifdef SYSTEM_UNIX
        sa_in.sin_len = sizeof ( sa_in );
#endif

        return true;
    }

    if ( family == AF_INET6 && addrLen >= sizeof ( sa_in6.sin6_addr ) )
    {
        clear();

        sa.sa_family = AF_INET6;
        sa_in6.sin6_port = orgPort;

        memcpy ( &sa_in6.sin6_addr, addr, sizeof ( sa_in6.sin6_addr ) );

// See above.
#ifdef SYSTEM_UNIX
        sa_in6.sin6_len = sizeof ( sa_in6 );
#endif

        return true;
    }

    return false;
}

bool SockAddr::hasZeroIpAddr() const
{
    switch ( sa.sa_family )
    {
        case AF_INET:
            // IPv4 is 'zero' when s_addr is 0:
            return ( sa_in.sin_addr.s_addr == 0 );
            break;

        case AF_INET6:
            {
                // IPv6 address as an array of 4 uint32 elements.
                const uint32_t * addrData = ( const uint32_t * ) &sa_in6.sin6_addr;

                // IPv6 is 'zero' if bytes 0-3, 4-7 and 12-15 are zeroes, and either:
                // 8-11 all zeroes - then it's simply an IPv6 zero address.
                // 8-11 is 0000ffff - then it's an IPv6-Mapped IPv4 zero address.
                // In both cases we want to return 'true'.

                return ( addrData[ 0 ] == 0
                         && addrData[ 1 ] == 0
                         && addrData[ 3 ] == 0
                         && ( addrData[ 2 ] == 0 || addrData[ 2 ] == htonl ( 0xFFFF ) ) );
            }
            break;
    }

    return false;
}

bool SockAddr::isEquivalent ( const SockAddr & other ) const
{
    if ( *this == other )
    {
        return true;
    }

    if ( sa.sa_family == AF_INET6 && other.sa.sa_family == AF_INET )
    {
        if ( sa_in6.sin6_port != other.sa_in.sin_port )
        {
            return false;
        }

        IpAddress v4 ( *this );

        if ( !v4.convertToV4() )
        {
            // this isn't a V6 mapped V4 address, so return false
            return false;
        }

        return v4.getV4().s_addr == other.sa_in.sin_addr.s_addr;
    }

    if ( sa.sa_family == AF_INET && other.sa.sa_family == AF_INET6 )
    {
        if ( sa_in.sin_port != other.sa_in6.sin6_port )
        {
            return false;
        }

        IpAddress v4other ( other );

        if ( !v4other.convertToV4() )
        {
            // this isn't a V6 mapped V4 address, so return false
            return false;
        }

        return sa_in.sin_addr.s_addr == v4other.getV4().s_addr;
    }

    return false;
}

bool SockAddr::convertToV4MappedV6()
{
    if ( isIPv4() )
    {
        const uint16_t port = sa_in.sin_port;
        IpAddress addr ( sa_in.sin_addr );

        if ( addr.convertToV4MappedV6() && addr.isIPv6() )
        {
            sa.sa_family = AF_INET6;
            sa_in6.sin6_addr = addr.getV6();
            sa_in6.sin6_port = port;

// See above.
#ifdef SYSTEM_UNIX
            sa_in6.sin6_len = sizeof ( sa_in6 );
#endif

            return true;
        }
    }

    return false;
}

bool SockAddr::convertToV4()
{
    if ( isIPv6() )
    {
        const uint16_t port = sa_in6.sin6_port;
        IpAddress addr ( sa_in6.sin6_addr );

        if ( addr.convertToV4() && addr.isIPv4() )
        {
            sa.sa_family = AF_INET;
            sa_in.sin_addr = addr.getV4();
            sa_in.sin_port = port;

// See above.
#ifdef SYSTEM_UNIX
            sa_in.sin_len = sizeof ( sa_in );
#endif

            return true;
        }
    }

    return false;
}

String SockAddr::toString() const
{
    String ret;

    if ( isIPv4() )
    {
        // 15 - max length of IPv4 address
        // 1  - for the ":"
        // 5  - for the port
        ret.reserve ( 15 + 1 + 5 );
        ret.append ( getAddr().toString() );
        ret.append ( ":" ).append ( String::number ( ntohs ( sa_in.sin_port ) ) );
    }
    else if ( isIPv6() )
    {
        // Some OSes don't have the constant INET6_ADDRSTRLEN, so we'll just use its value here: 46
        // 46 - max length of IPv6 address
        // 2  - for the brackets
        // 1  - for the ":"
        // 5  - for the port
        ret.reserve ( 46 + 2 + 1 + 5 );
        ret.append ( getAddr().toString ( true ) );
        ret.append ( ":" ).append ( String::number ( ntohs ( sa_in6.sin6_port ) ) );
    }
    else
    {
        ret = "Unknown";
    }

    return ret;
}

size_t Pravala::getHash ( const SockAddr & key )
{
    if ( key.isIPv4() )
    {
        return ( ( ( unsigned int ) key.sa_in.sin_addr.s_addr )
                 ^ ( ( unsigned int ) key.sa_in.sin_port ) );
    }
    else if ( key.isIPv6() )
    {
        const uint32_t * v6 = ( const uint32_t * ) ( &key.sa_in6.sin6_addr );

        return v6[ 0 ] ^ v6[ 1 ] ^ v6[ 2 ] ^ v6[ 3 ] ^ ( ( unsigned int ) key.sa_in6.sin6_port );
    }

    return 0;
}

bool SockAddr::convertAddrSpec ( const String & addrSpec, SockAddr & addr )
{
    IpAddress a;
    uint16_t p = 0;

    return ( IpAddress::convertAddrSpec ( addrSpec, a, p )
             && a.isValid()
             && p > 0
             && ( addr = SockAddr ( a, p ) ).hasIpAddr()
             && addr.hasPort() );
}
