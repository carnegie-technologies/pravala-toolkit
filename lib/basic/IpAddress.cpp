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

#include "String.hpp"
#include "IpAddress.hpp"

#ifdef SYSTEM_WINDOWS
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#else
extern "C"
{
#include <arpa/inet.h>
#include <netdb.h>
}
#endif

#include "MsvcSupport.hpp"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN    16
#endif

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED( a ) \
    ( ( ( ( const uint32_t * ) ( a ) )[ 0 ] == 0 ) \
      && ( ( ( const uint32_t * ) ( a ) )[ 1 ] == 0 ) \
      && ( ( ( const uint32_t * ) ( a ) )[ 2 ] == htonl ( 0xFFFF ) ) )
#endif

#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL( a ) \
    ( ( ( ( const uint32_t * ) ( a ) )[ 0 ] & htonl ( 0xFFC00000 ) ) == htonl ( 0xFE800000 ) )
#endif

#ifndef IN_LINKLOCAL
// IPv4 link-local addresses use 169.254.0.0/16 prefix (RFC 3927)
#define IN_LINKLOCAL( a )    ( ( ( a ) & htonl ( 0xA9FE0000 ) ) == htonl ( 0xA9FE0000 ) )
#endif

using namespace Pravala;

const IpAddress IpAddress::IpEmptyAddress;

const IpAddress IpAddress::Ipv4ZeroAddress ( "0.0.0.0" );
const IpAddress IpAddress::Ipv6ZeroAddress ( "::" );

const IpAddress IpAddress::Ipv4LocalhostAddress ( "127.0.0.1" );
const IpAddress IpAddress::Ipv6LocalhostAddress ( "::1" );

const IpAddress IpAddress::Ipv4HostNetmask ( "255.255.255.255" );
const IpAddress IpAddress::Ipv6HostNetmask ( "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF" );

/// @brief The type of the address to generate
enum SpecAddrType
{
    AddrNetmask, ///< The netmask address
    AddrNetwork, ///< The network address
    AddrBcast ///< The broadcast address
};

/// @brief Returns one of the "special" addresses for the given address and the network length
///
/// This function calculates the netmask, network address or the broadcast address.
///
/// @param [in] addr The 'base' address to use
/// @param [in] maskLen The length of the netmask
/// @param [in] addrType The type of the address to generate
/// @return The calculated, "special" address
template<typename T> IpAddress generateAddr ( const T & addr, uint8_t maskLen, SpecAddrType addrType )
{
    T retAddr;

    if ( addrType == AddrBcast )
    {
        // By default, all bits of broadcast addresses are 1!
        memset ( &retAddr, 0xFF, sizeof ( T ) );
    }
    else
    {
        assert ( addrType == AddrNetmask || addrType == AddrNetwork );

        // And all the bytes of netmask or network address are 0.
        // This is needed, because we only modify the "interesting" bytes below,
        // those that are part of the netmask!
        memset ( &retAddr, 0, sizeof ( T ) );
    }

    // We define a "full byte" as a byte of the address, that is entirely "covered" by the netmask.
    // so if the netmask has the length of 20 bits, the first 2 bytes are "full" (8+8 bits),
    // but the third one is not (only 4 bits of the third byte are covered by the netmask)
    //
    // netmask length in bits / 8 = netmask length in full bytes.
    uint8_t fullBytes = ( maskLen >> 3 );

    // The netmask is too long, or includes all the bits of the address.
    if ( fullBytes >= sizeof ( T ) )
    {
        if ( addrType == AddrNetmask )
        {
            // The netmask is all 1s:
            memset ( &retAddr, 0xFF, sizeof ( T ) );
            return retAddr;
        }

        assert ( addrType == AddrNetwork || addrType == AddrBcast );

        // The network address and the bcast address are the same as the original address.
        // (It doesn't really make sense for the bcast address, but there is not much we can do about that...)
        return addr;
    }

    uint8_t * memOut = ( uint8_t * ) &retAddr;

    // This is the bitmask within the last interesting byte.
    // The last interesting byte is the first byte after the last "full" byte
    uint8_t bMask = 0xFF << ( 8 - ( maskLen % 8 ) );

    if ( addrType == AddrNetmask )
    {
        uint8_t idx = 0;

        // All the 'full' bytes of netmask should contain 0xFF:
        for ( ; idx < fullBytes; ++idx )
        {
            memOut[ idx ] = 0xFF;
        }

        assert ( idx < sizeof ( T ) );

        // And the last (not full) byte of the netmask should be the same as the 'bMask':
        memOut[ idx ] = bMask;

        // There may be some other bytes after this, but we set them to '0' at the beginning (using memset),
        // and since they're not 'covered' by the netmask they should be 0. We don't need to touch them!
        return retAddr;
    }

    const uint8_t * memOrg = ( const uint8_t * ) &addr;

    assert ( addrType == AddrNetwork || addrType == AddrBcast );

    uint8_t idx = 0;

    // All the "full" bytes of network address and bcast address should be simply copied from the original address:
    for ( ; idx < fullBytes; ++idx )
    {
        memOut[ idx ] = memOrg[ idx ];
    }

    assert ( idx < sizeof ( T ) );

    if ( addrType == AddrNetwork )
    {
        // The last "important" byte of the network address should be equal to the
        // same byte in the original address, just masked with bMask.
        memOut[ idx ] = memOrg[ idx ] & bMask;

        // We don't need to touch any other bytes. They are set to '0' (using memset at the beginning),
        // and since this is a network address they should stay that way!

        return retAddr;
    }

    assert ( addrType == AddrBcast );

    // The last "important" byte of the broadcast address should be equal to the
    // same byte in the original address masked with bMask, with all the bytes
    // NOT set in the bMask set to 1:
    memOut[ idx ] = ( memOrg[ idx ] & bMask ) | ( ~bMask );

    // We don't need to touch any other bytes. They are set to '0xFF' (using memset at the beginning),
    // and since this is a broadcast address they should stay that way!

    return retAddr;
}

IpAddress::IpAddress(): _version ( EmptyAddress )
{
}

IpAddress::IpAddress ( const struct sockaddr_storage & sockAddr ): _version ( EmptyAddress )
{
    switch ( ( ( const struct sockaddr * ) &sockAddr )->sa_family )
    {
        case AF_INET:
            _version = V4Address;
            _address = toSockaddrInPtr ( &sockAddr )->sin_addr;
            break;

        case AF_INET6:
            _version = V6Address;
            _address = toSockaddrIn6Ptr ( &sockAddr )->sin6_addr;
            break;

        default:
            break;
    }
}

IpAddress::IpAddress ( const SockAddr & sockAddr ): _version ( EmptyAddress )
{
    switch ( sockAddr.sa.sa_family )
    {
        case AF_INET:
            _version = V4Address;
            _address = sockAddr.sa_in.sin_addr;
            break;

        case AF_INET6:
            _version = V6Address;
            _address = sockAddr.sa_in6.sin6_addr;
            break;

        default:
            break;
    }
}

IpAddress::IpAddress ( const in_addr & v4Address ):
    _version ( V4Address ), _address ( v4Address )
{
}

IpAddress::IpAddress ( const in6_addr & v6Address ):
    _version ( V6Address ), _address ( v6Address )
{
}

IpAddress::IpAddress ( const IpAddress & other ):
    _version ( other._version ), _address ( other._address )
{
}

IpAddress::IpAddress ( const char * strAddress ): _version ( EmptyAddress )
{
    operator= ( strAddress );
}

IpAddress::IpAddress ( const String & strAddress ): _version ( EmptyAddress )
{
    operator= ( strAddress );
}

void IpAddress::clear()
{
    _version = EmptyAddress;
}

IpAddress & IpAddress::operator= ( const IpAddress & other )
{
    if ( &other != this )
    {
        _version = other._version;
        _address = other._address;
    }

    return *this;
}

/// @brief Converts IPv4 address from text to binary form
/// @param [in] str The string representation of the address
/// @param [out] addr The binary representation of the address. It has to have a room for four uint8_t entries.
/// @return True if the address was converted successfully; False otherwise.
///         In case of a failure the content of addr is undefined - some or none entries may have been written.
static bool inetPton4 ( const String & str, uint8_t * addr )
{
    if ( str.length() < 7 )
    {
        return false;
    }

    const StringList list ( str.split ( ".", true ) );

    if ( list.size() != 4 )
    {
        return false;
    }

    for ( size_t i = 0; i < 4; ++i )
    {
        if ( !list.at ( i ).toNumber ( addr[ i ] ) )
        {
            return false;
        }
    }

    return true;
}

IpAddress & IpAddress::operator= ( const String & strAddress )
{
    if ( inetPton4 ( strAddress, ( uint8_t * ) &_address.v4 ) )
    {
        _version = V4Address;
        return *this;
    }

    _version = EmptyAddress;

    StringList strList;

    if ( strAddress.length() > 2 && strAddress[ 0 ] == '[' && strAddress[ strAddress.length() - 1 ] == ']' )
    {
        strList = strAddress.substr ( 1, strAddress.length() - 2 ).split ( ":", true );
    }
    else
    {
        strList = strAddress.split ( ":", true );
    }

    // Shortest possible list: 3 elements (::)
    // Longest possible: 9 elements
    // Normally it would be up to 8, but this is also legal: "::2:3:4:5:6:7:8"
    // In that case "::" represents only a single, 0, entry, but results in two list entries!
    if ( strList.size() < 3 || strList.size() > 9 )
    {
        return *this;
    }

    // Let's check if the last part looks like IPv4.
    // It is required to be able to parse IPv6 addresses in which the last four bytes are described
    // with an IPv4 address. This is usually used for IPv6-mapped-IPv4 addresses (::ffff:11.22.33.44),
    // but any IPv6 address can have its last four bytes represented using an IPv4 address.
    // So first we need to detect that, and convert it back to "normal" IPv6 string format:

    uint8_t ipv4[ 4 ];

    if ( inetPton4 ( strList.last(), ipv4 ) )
    {
        // By converting the last part of IPv6 address from IPv4 format to normal IPv6 representation,
        // we remove a single, last entry of IPv6 (after the last ':'), and instead of it
        // we append two entries in IPv6 format. This increases the list length by 1.
        // So if the list already has the longest length then something is wrong!

        if ( strList.size() >= 9 )
        {
            return *this;
        }

        // It looks like an IPv4! Remove it, and use two IPv6-like entries in its place:
        strList.removeLast();

        for ( int i = 0; i <= 2; i += 2 )
        {
            static const char * hexVals = "0123456789abcdef";

            const char tmp[ 4 ] =
            {
                hexVals[ ipv4[ i ] / 16 ],
                hexVals[ ipv4[ i ] % 16 ],
                hexVals[ ipv4[ i + 1 ] / 16 ],
                hexVals[ ipv4[ i + 1 ] % 16 ]
            };

            strList.append ( String ( tmp, 4 ) );
        }
    }

    assert ( strList.size() >= 3 );

    // starts with ':'
    if ( strList.at ( 0 ).isEmpty() )
    {
        // ":foo" - this is illegal
        if ( !strList.at ( 1 ).isEmpty() )
        {
            return *this;
        }

        // "::foo" - this is legal. We want to represent the whole '::' as a single empty entry
        // (which up to this point would be illegal if at the beginning). But it's easier to leave
        // just a single empty entry instead of two, and then treat any empty entry as a 0 padding.

        strList.removeFirst();
    }

    // ends with ':'
    if ( strList.at ( strList.size() - 1 ).isEmpty() )
    {
        // "foo:" - this is illegal
        if ( !strList.at ( strList.size() - 2 ).isEmpty() )
        {
            return *this;
        }

        // "foo::" - this is legal. We want to represent the whole '::' as a single empty entry
        // (which up to this point would be illegal if at the end). But it's easier to leave
        // just a single empty entry instead of two, and then treat any empty entry as a 0 padding.

        strList.removeLast();
    }

    // Previously we made sure that the list size is at most 9 entries. This was to handle "::foo" and "foo::"
    // cases where "::" represents only a single 0 entry. They were only possible at the beginning or the end.
    // However, now we would have gotten rid of that extra ":" (the two if blocks above this comment).
    // So if at this point we have more than 8 entries in the list, something is wrong!

    if ( strList.size() > 8 )
    {
        return *this;
    }

    // At this point we have a single empty entry where zeroes should be.
    // Number of zeroes to add = 8 - strList.size()

    List<uint16_t> addrList; // In host order

    bool foundEmpty = false;

    for ( size_t i = 0; i < strList.size(); ++i )
    {
        if ( strList.at ( i ).isEmpty() )
        {
            if ( foundEmpty )
            {
                // We found second empty (compressed) entry.
                // This is illegal!

                return *this;
            }

            foundEmpty = true;

            // We have to "fill" each empty entry with zeroes until the list has 8 entries.
            // We need to add at least one zero (hence j >= 0).

            for ( int j = ( int ) ( 8 - strList.size() ); j >= 0; --j )
            {
                addrList.append ( 0 );
            }
        }
        else
        {
            uint16_t val;

            // Not uint16_t!
            if ( !strList.at ( i ).toNumber ( val, 16 ) )
            {
                return *this;
            }

            addrList.append ( val );
        }
    }

    // At this point the list should have exactly 8 entries. If it doesn't then something is clearly wrong!

    if ( addrList.size() != 8 )
    {
        return *this;
    }

    for ( size_t i = 0; i < 8; ++i )
    {
        ( ( uint16_t * ) &_address.v6 )[ i ] = htons ( addrList.at ( i ) );
    }

    _version = V6Address;
    return *this;
}

IpAddress & IpAddress::operator= ( const char * strAddress )
{
    return operator= ( String ( strAddress ) );
}

IpAddress & IpAddress::operator= ( const in_addr & v4Address )
{
    setupV4Memory ( ( const char * ) &v4Address );

    return *this;
}

IpAddress & IpAddress::operator= ( const in6_addr & v6Address )
{
    setupV6Memory ( ( const char * ) &v6Address );

    return *this;
}

void IpAddress::setupV4Memory ( const char * memory )
{
    memcpy ( &_address.v4, memory, sizeof ( _address.v4 ) );
    _version = V4Address;
}

void IpAddress::setupV6Memory ( const char * memory )
{
    memcpy ( &_address.v6, memory, sizeof ( _address.v6 ) );
    _version = V6Address;
}

const in_addr & IpAddress::getV4() const
{
    assert ( isIPv4() );

    return _address.v4;
}

const in6_addr & IpAddress::getV6() const
{
    assert ( isIPv6() );

    return _address.v6;
}

bool IpAddress::isLinkLocal() const
{
    return ( ( isIPv4() && IN_LINKLOCAL ( _address.v4.s_addr ) )
             || ( isIPv6() && IN6_IS_ADDR_LINKLOCAL ( &_address.v6 ) ) );
}

bool IpAddress::isIPv6MappedIPv4() const
{
    return ( isIPv6() && IN6_IS_ADDR_V4MAPPED ( &_address.v6 ) );
}

bool IpAddress::isZero() const
{
    // IP address as an array of uint32 elements: 1 for IPv4, 4 for IPv6.
    const uint32_t * addrData = ( const uint32_t * ) &_address;

    if ( _version == V4Address )
    {
        // IPv4, so it's zero if all of it (4 bytes) is 0:
        return ( addrData[ 0 ] == 0 );
    }

    if ( _version == V6Address && addrData[ 0 ] == 0 && addrData[ 1 ] == 0 && addrData[ 3 ] == 0 )
    {
        // IPv6, and bytes 0-3, 4-7 and 12-15 are zeroes, and either:
        // 8-11 all zeroes - then it's simply an IPv6 zero address.
        // 8-11 is 0000ffff - then it's an IPv6-Mapped IPv4 zero address.
        // In both cases we want to return 'true'.

        return ( addrData[ 2 ] == 0 || addrData[ 2 ] == htonl ( 0xFFFF ) );
    }

    return false;
}

SockAddr IpAddress::getSockAddr ( uint16_t portNumber ) const
{
    SockAddr sockAddr;

    if ( sockAddr.setAddr ( *this ) )
    {
        sockAddr.setPort ( portNumber );
    }

    return sockAddr;
}

bool IpAddress::operator== ( const IpAddress & other ) const
{
    if ( isIPv4() && other.isIPv4() )
    {
        return ( _address.v4.s_addr == other._address.v4.s_addr );
    }
    else if ( isIPv6() && other.isIPv6() )
    {
        return ( memcmp ( &_address.v6, &other._address.v6, sizeof ( _address.v6 ) ) == 0 );
    }
    else if ( _version == EmptyAddress && other._version == EmptyAddress )
    {
        return true;
    }

    return false;
}

bool IpAddress::isEqual ( const IpAddress & addr, const IpAddress & netmask ) const
{
    if ( !isValid() || !addr.isValid() || !netmask.isValid()
         || getAddrType() != addr.getAddrType()
         || getAddrType() != netmask.getAddrType() )
    {
        return false;
    }

    if ( addr.isIPv4() )
    {
        return isEqual ( addr.getV4(), netmask.getV4() );
    }
    else
    {
        return isEqual ( addr.getV6(), netmask.getV6() );
    }
}

bool IpAddress::isEqual ( const IpAddress & addr, uint8_t netmaskLength ) const
{
    if ( !isValid() || !addr.isValid() || getAddrType() != addr.getAddrType() )
    {
        return false;
    }

    // All addresses are the same when we're using an empty netmask!
    if ( netmaskLength < 1 )
    {
        return true;
    }

    uint8_t memLen;
    const uint8_t * memA;
    const uint8_t * memB;

    if ( isIPv4() )
    {
        memLen = sizeof ( _address.v4 );
        memA = ( const uint8_t * ) &( _address.v4 );
        memB = ( const uint8_t * ) &( addr._address.v4 );
    }
    else
    {
        assert ( isIPv6() );

        memLen = sizeof ( _address.v6 );
        memA = ( const uint8_t * ) &( _address.v6 );
        memB = ( const uint8_t * ) &( addr._address.v6 );
    }

    // We define a "full byte" as a byte of the address, that is entirely "covered" by the netmask.
    // so if the netmask has the length of 20 bits, the first 2 bytes are "full" (8+8 bits),
    // but the third one is not (only 4 bits of the third byte are covered by the netmask)
    //
    // netmask length in bits / 8 = netmask length in full bytes:
    uint8_t fullBytes = ( netmaskLength >> 3 );

    // The netmask is too long, or includes all the bits of the address.
    // We can simply use memcmp:
    if ( fullBytes >= memLen )
    {
        return ( memcmp ( memA, memB, memLen ) == 0 );
    }

    for ( uint8_t i = 0; i < fullBytes; ++i )
    {
        if ( memA[ i ] != memB[ i ] )
        {
            return false;
        }
    }

    uint8_t bMask = 0xFF << ( 8 - ( netmaskLength % 8 ) );

    assert ( fullBytes < memLen );

    return ( ( memA[ fullBytes ] & bMask ) == ( memB[ fullBytes ] & bMask ) );
}

bool IpAddress::isEqual ( const in_addr & addr, const in_addr & netmask ) const
{
    if ( !isIPv4() )
    {
        return false;
    }

    in_addr myAddr;
    in_addr otherAddr;

    memcpy ( &myAddr, &_address.v4, sizeof ( myAddr ) );

    myAddr.s_addr &= netmask.s_addr;

    memcpy ( &otherAddr, &addr, sizeof ( otherAddr ) );

    otherAddr.s_addr &= netmask.s_addr;

    return ( myAddr.s_addr == otherAddr.s_addr );
}

bool IpAddress::isEqual ( const in6_addr & addr, const in6_addr & netmask ) const
{
    if ( !isIPv6() )
    {
        return false;
    }

    in6_addr myAddr;
    in6_addr otherAddr;

    memcpy ( &myAddr, &_address.v6, sizeof ( myAddr ) );

    myAddr.s6_addr16[ 0 ] &= netmask.s6_addr16[ 0 ];
    myAddr.s6_addr16[ 1 ] &= netmask.s6_addr16[ 1 ];
    myAddr.s6_addr16[ 2 ] &= netmask.s6_addr16[ 2 ];
    myAddr.s6_addr16[ 3 ] &= netmask.s6_addr16[ 3 ];
    myAddr.s6_addr16[ 4 ] &= netmask.s6_addr16[ 4 ];
    myAddr.s6_addr16[ 5 ] &= netmask.s6_addr16[ 5 ];
    myAddr.s6_addr16[ 6 ] &= netmask.s6_addr16[ 6 ];
    myAddr.s6_addr16[ 7 ] &= netmask.s6_addr16[ 7 ];

    memcpy ( &otherAddr, &addr, sizeof ( otherAddr ) );

    otherAddr.s6_addr16[ 0 ] &= netmask.s6_addr16[ 0 ];
    otherAddr.s6_addr16[ 1 ] &= netmask.s6_addr16[ 1 ];
    otherAddr.s6_addr16[ 2 ] &= netmask.s6_addr16[ 2 ];
    otherAddr.s6_addr16[ 3 ] &= netmask.s6_addr16[ 3 ];
    otherAddr.s6_addr16[ 4 ] &= netmask.s6_addr16[ 4 ];
    otherAddr.s6_addr16[ 5 ] &= netmask.s6_addr16[ 5 ];
    otherAddr.s6_addr16[ 6 ] &= netmask.s6_addr16[ 6 ];
    otherAddr.s6_addr16[ 7 ] &= netmask.s6_addr16[ 7 ];

    return ( memcmp ( &myAddr, &otherAddr, sizeof ( myAddr ) ) == 0 );
}

bool IpAddress::operator< ( const IpAddress & other ) const
{
    if ( _version == EmptyAddress || _version != other._version )
    {
        return false;
    }

    assert ( _version == other._version );
    assert ( ( isIPv4() && other.isIPv4() ) || ( isIPv6() && other.isIPv6() ) );

    if ( isIPv4() )
    {
        assert ( other.isIPv4() );

        return ( memcmp ( &getV4(), &other.getV4(), sizeof ( in_addr ) ) < 0 );
    }

    if ( isIPv6() )
    {
        assert ( other.isIPv6() );

        return ( memcmp ( &getV6(), &other.getV6(), sizeof ( in6_addr ) ) < 0 );
    }

    assert ( false );

    return false;
}

bool IpAddress::operator> ( const IpAddress & other ) const
{
    if ( _version == EmptyAddress || _version != other._version )
    {
        return false;
    }

    assert ( _version == other._version );
    assert ( ( isIPv4() && other.isIPv4() ) || ( isIPv6() && other.isIPv6() ) );

    if ( isIPv4() )
    {
        assert ( other.isIPv4() );

        return ( memcmp ( &getV4(), &other.getV4(), sizeof ( in_addr ) ) > 0 );
    }

    if ( isIPv6() )
    {
        assert ( other.isIPv6() );

        return ( memcmp ( &getV6(), &other.getV6(), sizeof ( in6_addr ) ) > 0 );
    }

    assert ( false );

    return false;
}

void IpAddress::incrementBy ( uint8_t val )
{
    if ( isIPv4() )
    {
        uint32_t tmp = ntohl ( _address.v4.s_addr ) + val;

        _address.v4.s_addr = htonl ( tmp );
    }
    else if ( isIPv6() )
    {
        uint8_t oldB15 = _address.v6.s6_addr[ 15 ];

        _address.v6.s6_addr[ 15 ] += val;

        if ( oldB15 > _address.v6.s6_addr[ 15 ] )
        {
            // there's a carry, so we need to propagate it

            for ( int i = 14; i >= 0; --i )
            {
                ++_address.v6.s6_addr[ i ];

                // if the value of the current byte isn't 0, then the carry that
                // was just added didn't result in another carry, so we're done!
                if ( _address.v6.s6_addr[ i ] != 0 )
                {
                    break;
                }

                // otherwise we have another carry to propagate up
            }
        }
    }
}

/// @brief Attempt to convert a netmask in IP address form, or part of a netmask in IP address form
/// to a network prefix length. This function is used internally by toPrefix()
/// @param [in] addr A netmask or part of a netmask to be converted (in IP address form)
/// The bits of this value should be in host order.
/// @param [in] len Length of addr (in bits) in use for the address
/// @note This is useful when converting part of an address and the entire 32-bit range isn't used.
/// @return -1 if addr wasn't a netmask, or network prefix length otherwise
int toPrefixInt ( uint32_t addr, uint8_t len )
{
    int count = 0;
    bool foundZero = false;

    // A mask with a single 1 bit at the MSB
    int mask = 1 << ( len - 1 );

    for ( int i = 0; i < len; ++i )
    {
        if ( ( addr & mask ) > 0 )
        {
            if ( foundZero )
            {
                // already got a zero, but now we have a 1...
                // so this isn't a valid netmask
                return -1;
            }

            ++count;
        }
        else
        {
            foundZero = true;
        }

        // shift everything left 1
        addr <<= 1;
    }

    return count;
}

int IpAddress::toPrefix() const
{
    if ( isIPv4() )
    {
        return toPrefixInt ( ( uint32_t ) ntohl ( _address.v4.s_addr ), 32 );
    }

    if ( isIPv6() )
    {
        int count = 0;
        bool hasZero = false;

        for ( int i = 0; i < 16; ++i )
        {
            int pre = toPrefixInt ( ( uint32_t ) _address.v6.s6_addr[ i ], 8 );

            count += pre;

            if ( pre < 0 )
            {
                return -1;
            }

            if ( hasZero && pre > 0 )
            {
                return -1;
            }

            if ( pre < 8 )
            {
                hasZero = true;
            }
        }

        return count;
    }

    return -1;
}

IpAddress IpAddress::getNetmaskAddress ( uint8_t netmaskLen ) const
{
    if ( _version == V4Address )
    {
        return generateAddr ( _address.v4, netmaskLen, AddrNetmask );
    }

    if ( _version == V6Address )
    {
        return generateAddr ( _address.v6, netmaskLen, AddrNetmask );
    }

    return IpEmptyAddress;
}

IpAddress IpAddress::getNetworkAddress ( uint8_t netmaskLen ) const
{
    if ( _version == V4Address )
    {
        return generateAddr ( _address.v4, netmaskLen, AddrNetwork );
    }

    if ( _version == V6Address )
    {
        return generateAddr ( _address.v6, netmaskLen, AddrNetwork );
    }

    return IpEmptyAddress;
}

IpAddress IpAddress::getBcastAddress ( uint8_t netmaskLen ) const
{
    if ( _version == V4Address )
    {
        return generateAddr ( _address.v4, netmaskLen, AddrBcast );
    }

    if ( _version == V6Address )
    {
        return generateAddr ( _address.v6, netmaskLen, AddrBcast );
    }

    return IpEmptyAddress;
}

bool IpAddress::convertToV4MappedV6()
{
    if ( _version != V4Address )
    {
        return false;
    }

    _version = V6Address;

    // The v4 mapped address is:
    // 10 bytes with zeroes, followed by 2 bytes with 0xFF 0xFF, followed by 4 bytes of the IPv4 address.
    // NOTE: We perform operations on 2 bytes at a time (Windows doesn't have anything like s6_addr32):

    _address.v6.s6_addr16[ 5 ] = 0xFFFF;
    _address.v6.s6_addr16[ 6 ] = _address.v6.s6_addr16[ 0 ];
    _address.v6.s6_addr16[ 7 ] = _address.v6.s6_addr16[ 1 ];

    _address.v6.s6_addr16[ 0 ] = 0;
    _address.v6.s6_addr16[ 1 ] = 0;
    _address.v6.s6_addr16[ 2 ] = 0;
    _address.v6.s6_addr16[ 3 ] = 0;
    _address.v6.s6_addr16[ 4 ] = 0;

    return true;
}

bool IpAddress::convertToV4()
{
    if ( !isIPv6MappedIPv4() )
        return false;

    _version = V4Address;

    // copy the last 4 bytes of the V6 address to the start (i.e. the v4 address)
    _address.v6.s6_addr16[ 0 ] = _address.v6.s6_addr16[ 6 ];
    _address.v6.s6_addr16[ 1 ] = _address.v6.s6_addr16[ 7 ];

    // set everything after that to 0:
    _address.v6.s6_addr16[ 2 ] = 0;
    _address.v6.s6_addr16[ 3 ] = 0;
    _address.v6.s6_addr16[ 4 ] = 0;
    _address.v6.s6_addr16[ 5 ] = 0;
    _address.v6.s6_addr16[ 6 ] = 0;
    _address.v6.s6_addr16[ 7 ] = 0;

    return true;
}

String IpAddress::toString ( bool includeIPv6Brackets ) const
{
    if ( isIPv4() )
    {
        return toString ( _address.v4 );
    }

    if ( isIPv6() )
    {
        return toString ( _address.v6, includeIPv6Brackets );
    }

    return "Unknown Address";
}

String IpAddress::toString ( const in_addr & address )
{
    static const char maxAddr[] = "255.255.255.255";
    char buf[ sizeof ( maxAddr ) ]; // sizeof() includes \0

    const uint8_t * a = ( const uint8_t * ) &address;

    const int sRet = snprintf ( buf, sizeof ( maxAddr ), "%u.%u.%u.%u", a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ] );

    if ( sRet < 7 || ( size_t ) sRet >= sizeof ( maxAddr ) )
    {
        return String::EmptyString;
    }

    return String ( buf, sRet );
}

String IpAddress::toString ( const in6_addr & address, bool includeBrackets )
{
    // Max IPv6:                         FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF
    // Max IPv6-mapped IPV4:             0000:0000:0000:0000:0000:FFFF:255.255.255.255
    // Max compressed IPv6-mapped IPV4:  ::FFFF:255.255.255.255

    static const char maxAddr[] = "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    char buf[ sizeof ( maxAddr ) ]; // sizeof() includes \0

    const uint8_t * a = ( const uint8_t * ) &address;

    // Let's count zeroes at the beginning
    int zeroCount = 0;

    for ( int i = 0; i < 16; ++i )
    {
        if ( a[ i ] != 0U )
        {
            break;
        }

        ++zeroCount;
    }

    // 10 zeroes, followed by FF FF - we have a mapped IPv4!
    // ::ffff:1.2.3.4 or even ::ffff:0.0.0.0
    const bool mappedV4F = ( zeroCount == 10 && a[ 10 ] == 0xFFU && a[ 11 ] == 0xFFU );

    // 10 zeroes, followed by OO OO (instead of FF FF) and at most one additional zero.
    // So either 12 or 13 zeroes.
    // This works for:
    // ::1.2.3.4 or ::0.2.3.4 (but also ::1.0.0.0)
    // However, ::0.0.3.4 ::0.0.0.4 and ::0.0.0.0 are all treated as IPv6
    // This is weird, but that's what Linux does.
    const bool mappedV4Z = ( zeroCount == 12 || zeroCount == 13 );

    if ( mappedV4F || mappedV4Z )
    {
        const int sRet = snprintf ( buf, sizeof ( maxAddr ),
                                    ( mappedV4F ? "::ffff:%u.%u.%u.%u" : "::%u.%u.%u.%u" ),
                                    a[ 12 ], a[ 13 ], a[ 14 ], a[ 15 ] );

        if ( sRet < ( mappedV4F ? 14 : 9 ) || ( size_t ) sRet >= sizeof ( maxAddr ) )
        {
            return String::EmptyString;
        }

        String ret;

        if ( includeBrackets )
        {
            ret.reserve ( sRet + 2 );

            ret.append ( "[" );
            ret.append ( buf, sRet );
            ret.append ( "]" );
        }
        else
        {
            ret.append ( buf, sRet );
        }

        return ret;
    }

    const uint16_t * s = ( const uint16_t * ) &address;

    struct
    {
        int beg;
        int len;
    } current, best;

    current.beg = 0;
    current.len = 0;

    best = current;

    // Let's find the longest group of zeros
    for ( int i = 0; i < 8; ++i )
    {
        // We have a zero!
        if ( s[ i ] == 0 )
        {
            // We are not inside the group already, let's mark a new one
            if ( current.len == 0 )
            {
                current.beg = i;
            }

            ++current.len;

            // Update the best one if necessary
            if ( current.len > best.len )
            {
                best = current;
            }
        }
        else
        {
            // We are outside of a group of zeros
            current.len = 0;
        }
    }

    char * ptr = buf;
    size_t bufLen = sizeof ( maxAddr );

    for ( int i = 0; i < 8; ++i )
    {
        // Not enough space, for some reason
        if ( bufLen < 2 )
        {
            return String::EmptyString;
        }

        if ( best.len > 0 )
        {
            if ( i == best.beg )
            {
                assert ( s[ i ] == 0 );

                *ptr = ':';
                ++ptr;
                --bufLen;

                continue;
            }
            else if ( i >= best.beg && i < best.beg + best.len )
            {
                assert ( s[ i ] == 0 );

                continue;
            }
        }

        const int sRet = snprintf ( ptr, bufLen, "%s%x", ( i > 0 ) ? ":" : "", ntohs ( s[ i ] ) );

        if ( sRet < 1 || ( size_t ) sRet >= bufLen )
        {
            return String::EmptyString;
        }

        ptr += sRet;
        bufLen -= sRet;
    }

    // Our group is at the end, we need to add one more ':'
    if ( best.beg + best.len == 8 )
    {
        if ( bufLen < 2 )
        {
            return String::EmptyString;
        }

        *ptr = ':';
        ++ptr;
        --bufLen;
    }

    if ( bufLen < 1 )
    {
        return String::EmptyString;
    }

    *ptr = 0;

    String ret;

    if ( includeBrackets )
    {
        ret.append ( "[" );
        ret.append ( buf );
        ret.append ( "]" );
    }
    else
    {
        ret.append ( buf );
    }

    return ret;
}

String Pravala::toString ( const List<IpAddress> & ipAddrList )
{
    String ret ( "[" );

    for ( size_t i = 0; i < ipAddrList.size(); ++i )
    {
        if ( i > 0 )
        {
            ret.append ( ", " );
        }

        ret.append ( ipAddrList[ i ].toString() );
    }

    ret.append ( "]" );

    return ret;
}

String Pravala::toString ( const SimpleArray<IpAddress> & ipAddrList )
{
    String ret ( "[" );

    for ( size_t i = 0; i < ipAddrList.size(); ++i )
    {
        if ( i > 0 )
        {
            ret.append ( ", " );
        }

        ret.append ( ipAddrList[ i ].toString() );
    }

    ret.append ( "]" );

    return ret;
}

struct sockaddr_in * IpAddress::toSockaddrInPtr ( void * ptr )
{
    // To make sure the alignment is correct!
    assert ( ( ( size_t ) ptr ) % 4U == 0 );

    return reinterpret_cast<struct sockaddr_in *> ( ptr );
}

const struct sockaddr_in * IpAddress::toSockaddrInPtr ( const void * ptr )
{
    // To make sure the alignment is correct!
    assert ( ( ( size_t ) ptr ) % 4U == 0 );

    return reinterpret_cast<const struct sockaddr_in *> ( ptr );
}

struct sockaddr_in6 * IpAddress::toSockaddrIn6Ptr ( void * ptr )
{
    // To make sure the alignment is correct!
    assert ( ( ( size_t ) ptr ) % 4U == 0 );

    return reinterpret_cast<struct sockaddr_in6 *> ( ptr );
}

const struct sockaddr_in6 * IpAddress::toSockaddrIn6Ptr ( const void * ptr )
{
    // To make sure the alignment is correct!
    assert ( ( ( size_t ) ptr ) % 4U == 0 );

    return reinterpret_cast<const struct sockaddr_in6 *> ( ptr );
}

size_t Pravala::getHash ( const IpAddress & key )
{
    if ( key.isIPv4() )
    {
        return ( unsigned int ) key.getV4().s_addr;
    }
    else if ( key.isIPv6() )
    {
        const uint32_t * v6 = ( const uint32_t * ) ( &( key.getV6() ) );

        return v6[ 0 ] ^ v6[ 1 ] ^ v6[ 2 ] ^ v6[ 3 ];
    }

    return 0;
}

bool IpAddress::convertAddrSpec ( const String & addrSpec, IpAddress & addr, uint16_t & port )
{
    const int idx = addrSpec.findLastOf ( ":" );

    if ( idx > 0 && idx < addrSpec.length() - 1 )
    {
        IpAddress ipAddr ( addrSpec.substr ( 0, idx ) );
        bool ok = false;
        uint16_t ipPort = addrSpec.substr ( idx + 1 ).toUInt16 ( &ok );

        if ( ok && ipAddr.isValid() && ipPort > 0 )
        {
            addr = ipAddr;
            port = ipPort;
            return true;
        }
    }

    return false;
}

List<IpAddress> IpAddress::dnsResolve ( const String & hostname, String * errDesc )
{
    List<IpAddress> results;

    struct addrinfo hints;
    memset ( &hints, 0, sizeof ( hints ) );

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // This doesn't matter

    struct addrinfo * res = 0;

    const int ret = getaddrinfo ( hostname.c_str(), 0, &hints, &res );

    if ( ret != 0 )
    {
        if ( errDesc != 0 )
        {
            *errDesc = String ( "getaddrinfo failed: %1 [%2]" ).arg ( gai_strerror ( ret ) ).arg ( ret );
        }

        return results;
    }

    for ( const struct addrinfo * node = res; node != 0; node = node->ai_next )
    {
        if ( !node->ai_addr )
            continue;

        if ( ( unsigned ) node->ai_addrlen >= sizeof ( struct sockaddr_in )
             && node->ai_addr->sa_family == AF_INET )
        {
            results.append ( toSockaddrInPtr ( node->ai_addr )->sin_addr );
        }
        else if ( ( unsigned ) node->ai_addrlen >= sizeof ( struct sockaddr_in6 )
                  && node->ai_addr->sa_family == AF_INET6 )
        {
            results.append ( toSockaddrIn6Ptr ( node->ai_addr )->sin6_addr );
        }
    }

    if ( res != 0 )
    {
        freeaddrinfo ( res );
    }

    return results;
}
