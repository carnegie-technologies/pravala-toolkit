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

#pragma once

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
#include <ws2tcpip.h>
}
#else
extern "C"
{
// QNX doesn't define ssize_t in sys/socket but requires it be set, so include unistd first
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
}
#endif

#include "String.hpp"

namespace Pravala
{
class IpAddress;

/// @brief Structure for holding both sa_in and sa_in6 addresses
union SockAddr
{
    struct sockaddr sa; ///< Generic socket address
    struct sockaddr_in sa_in; ///< Internet socket address
    struct sockaddr_in6 sa_in6; ///< Internet socket address version 6

    /// @brief Default constructor.
    SockAddr()
    {
        clear();
    }

    /// @brief Constructor.
    /// @param [in] addr IP address to use.
    /// @param [in] port Port number to use.
    SockAddr ( const IpAddress &addr, uint16_t port );

    /// @brief Constructor.
    /// @param [in] sAddr The IPv4 sockaddr to use.
    SockAddr ( const struct sockaddr_in & sAddr );

    /// @brief Constructor.
    /// @param [in] sAddr The IPv6 sockaddr to use.
    SockAddr ( const struct sockaddr_in6 & sAddr );

    /// @brief Constructor.
    /// @param [in] sAddr A pointer to a sockaddr structure.
    /// @param [in] sAddrLen The size of the memory pointed to by sAddr.
    ///                      If the size is not sufficient for the address type specified in sa_family field,
    ///                      then the SockAddr created will be invalid.
    ///                      It is also possible to pass 0 size, in which case the size check will be skipped,
    ///                      and we will assume that the size is sufficient.
    ///                      This should only be used if the caller is certain that the size is correct!
    SockAddr ( const struct sockaddr * sAddr, size_t sAddrLen );

    /// @brief Used for determining if the address is "zero" (like 0.0.0.0)
    /// It does not check the port number.
    /// @note This also returns true for IPv6-Mapped IPv4 address (::ffff:0.0.0.0).
    /// @return True if address contains just zeros (but only if it is a valid IPv4 or IPv6 address).
    bool hasZeroIpAddr() const;

    /// @brief Gets a value indicating whether this is an IPv4 address
    /// @returns A value indicating whether this is an IPv4 address
    inline bool isIPv4() const
    {
        return ( sa.sa_family == AF_INET );
    }

    /// @brief Gets a value indication whether this is an IPv6 address
    /// @returns A value indication whether this is an IPv6 address
    inline bool isIPv6() const
    {
        return ( sa.sa_family == AF_INET6 );
    }

    /// @brief Checks whether this address is an IPv4 address mapped to IPv6.
    /// @return True if this is an IPv4 address mapped to IPv6.
    inline bool isIPv6MappedIPv4() const
    {
        return ( isIPv6() && IN6_IS_ADDR_V4MAPPED ( &sa_in6.sin6_addr ) );
    }

    /// @brief Gets the IP address in this SockAddr
    /// @return The IP address in this SockAddr.
    IpAddress getAddr() const;

    /// @brief Gets the port in this SockAddr
    /// @return The port in this SockAddr, or 0 if unknown family.
    inline uint16_t getPort() const
    {
        switch ( sa.sa_family )
        {
            case AF_INET:
                return ntohs ( sa_in.sin_port );
                break;

            case AF_INET6:
                return ntohs ( sa_in6.sin6_port );
                break;

            default:
                return 0;
        }
    }

    /// @brief Sets the address in this SockAddr.
    /// @note This maintains existing port information.
    /// @param [in] addr The address to set.
    /// @return True if address was successfully set; False otherwise.
    bool setAddr ( const IpAddress &addr );

    /// @brief Sets the address in this SockAddr.
    /// @note This maintains existing port information.
    /// @param [in] family Address family to use (AF_INET or AF_INET6).
    /// @param [in] addr Pointer to the address to set. This is not the entire sockaddr_in* structure,
    ///                  just the 4 byte (for AF_INET) or 16 byte (for AF_INET6) address part.
    /// @param [in] addrLen The length of the memory pointed to by addr.
    ///                     If the size is too small for selected family this function will fail.
    ///                     The size, however, may be longer than needed.
    /// @return True if address was successfully set; False otherwise.
    bool setAddr ( unsigned short family, const void * addr, size_t addrLen );

    /// @brief Sets the port in this SockAddr
    /// @note IP address (family) must be set first!
    /// @return True if port was successfully set, false otherwise
    inline bool setPort ( uint16_t port )
    {
        switch ( sa.sa_family )
        {
            case AF_INET:
                sa_in.sin_port = htons ( port );
                break;

            case AF_INET6:
                sa_in6.sin6_port = htons ( port );
                break;

            default:
                return false;
        }

        return true;
    }

    /// @brief Gets the size of the struct in use in the union
    /// @returns sizeof the proper struct
    inline socklen_t getSocklen() const
    {
        if ( isIPv4() )
        {
            return sizeof ( sockaddr_in );
        }
        else if ( isIPv6() )
        {
            return sizeof ( sockaddr_in6 );
        }
        else
        {
            return sizeof ( SockAddr );
        }
    }

    /// @brief Clears this union
    inline void clear()
    {
        memset ( this, 0, sizeof ( SockAddr ) );
    }

    /// @brief == operator
    /// @param [in] other SockAddr to compare this one too
    /// @return true if the SockAddrs are the same, false otherwise
    inline bool operator== ( const SockAddr & other ) const
    {
        // We only check our own family.
        // In the switch we only compare individual fields, instead of doing a memcmp,
        // because we don't care about irrelevant fields that could have different values, such as:
        //  - the sin_zero field at the end of a v4 address, which we don't care about
        //  - the sin_len/sin6_len field on BSD, which is not relevant to equality

        switch ( sa.sa_family )
        {
            case AF_INET:
                return ( other.sa_in.sin_family == AF_INET
                         && other.sa_in.sin_addr.s_addr == sa_in.sin_addr.s_addr
                         && other.sa_in.sin_port == sa_in.sin_port );

            case AF_INET6:
                return ( other.sa_in6.sin6_family == AF_INET6
                         && memcmp ( &other.sa_in6.sin6_addr, &sa_in6.sin6_addr, sizeof ( sa_in6.sin6_addr ) ) == 0
                         && other.sa_in6.sin6_port == sa_in6.sin6_port );
        }

        // If the family type is unknown, we have no choice but to compare memory directly.
        return memcmp ( this, &other, sizeof ( *this ) ) == 0;
    }

    /// @brief != operator
    /// @param [in] other SockAddr to compare this one too
    /// @return true if the SockAddrs are different, false otherwise
    inline bool operator!= ( const SockAddr & other ) const
    {
        return !( *this == other );
    }

    /// @brief Returns true if this SockAddr has an IPv4 or IPv6 address
    /// The IP address must be defined (either IPv4 or IPv6), but can be zero, i.e. a socket
    /// bound to the "any" address.
    /// @return True if this SockAddr represents a valid IP address, false otherwise
    inline bool hasIpAddr() const
    {
        return isIPv4() || isIPv6();
    }

    /// @brief Returns true if this SockAddr's getPort() returns a non-zero value
    /// @return True if this SockAddr's getPort() returns a non-zero value, false otherwise
    inline bool hasPort() const
    {
        return getPort() != 0;
    }

    /// @brief Convert the current address to a V4-mapped V6 address.
    /// It keeps current port number.
    /// @note The current address must be a V4 address
    /// @return True if conversion was successful
    bool convertToV4MappedV6();

    /// @brief Convert the current address to a V4 address.
    /// It keeps current port number.
    /// @note The current address must be a V4-mapped V6 address
    /// @return True if conversion was successful
    bool convertToV4();

    /// @brief Returns true if the other SockAddr means the same as this one
    /// In this case, it means that either "==" is true, or
    /// these two SockAddrs can both be represented by the same IPv4 SockAddr
    /// (e.g. one of them is a V6 mapped V4).
    /// @param [in] other SockAddr to compare this one too
    /// @return true if matches, false otherwise
    bool isEquivalent ( const SockAddr &other ) const;

    /// @brief Returns the String representation of this SockAddr
    /// @return The String representation of this SockAddr
    String toString() const;

    /// @brief A helper function for converting a string that contains 'ip_address:port_number' into a SockAddr value.
    /// @param [in] addrSpec String version of address:port
    /// @param [out] addr The address; Not modified if the conversion fails
    /// @return True if the conversion was successful; False otherwise
    static bool convertAddrSpec ( const String &addrSpec, SockAddr & addr );
};

/// @brief The 0 address and port in IPv4.
extern const SockAddr Ipv4ZeroSockAddress;

/// @brief The 0 address and port in IPv6.
extern const SockAddr Ipv6ZeroSockAddress;

/// @brief An empty SockAddr object.
extern const SockAddr EmptySockAddress;

/// @brief Hash function needed for using SockAddr objects as HashMap and HashSet keys.
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const SockAddr & key );
}
