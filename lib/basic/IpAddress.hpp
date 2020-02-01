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

#include "HashMap.hpp"
#include "SimpleArray.hpp"
#include "SockAddr.hpp"

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
}
#else
extern "C"
{
#include <netinet/in.h>
#include <sys/socket.h>
}
#endif

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

#ifndef s6_addr16
#define s6_addr16    u.Word
#endif

// Sadly, Windows doesn't provide anything that can be used for 32-bit operations...

#else

// For Apple + BSDs which include union values but not the defines...
#ifndef s6_addr16
#define s6_addr16    __u6_addr.__u6_addr16
#endif

#ifndef s6_addr32
#define s6_addr32    __u6_addr.__u6_addr32
#endif
#endif

namespace Pravala
{
class String;

/// @brief An IP (v4 or v6) address
class IpAddress
{
    public:
        /// @brief An empty IP address.
        const static IpAddress IpEmptyAddress;

        /// @brief The 0 address in IPv4.
        const static IpAddress Ipv4ZeroAddress;

        /// @brief The 0 address in IPv6.
        const static IpAddress Ipv6ZeroAddress;

        /// @brief The /32 netmask.
        const static IpAddress Ipv4HostNetmask;

        /// @brief The /128 netmask.
        const static IpAddress Ipv6HostNetmask;

        /// @brief The localhost (127.0.0.1) address in IPv4.
        const static IpAddress Ipv4LocalhostAddress;

        /// @brief The localhost (::1) address in IPv6.
        const static IpAddress Ipv6LocalhostAddress;

        /// @brief Enum describing possible types of IP addresses.
        enum AddressType
        {
            EmptyAddress = 0, ///< The IpAddress is empty and does not specify an IP
            V4Address = 4,    ///< The IpAddress contains an IPv4 address
            V6Address = 6     ///< The IpAddress contains an IPv6 address
        };

        /// @brief Default constructor, creates an empty IP address.
        IpAddress();

        /// @brief Creates an instance of the IpAddress from a sockaddr_storage
        /// @param [in] sockAddr Reference to sockaddr_storage
        IpAddress ( const struct sockaddr_storage & sockAddr );

        /// @brief Creates an instance of the IpAddress from a SockAddr
        /// @param [in] sockAddr Reference to SockAddr
        IpAddress ( const SockAddr & sockAddr );

        /// @brief Creates an instance of the IpAddress class with an IPv4 address
        /// @param [in] v4Address An IPv4 address
        IpAddress ( const in_addr & v4Address );

        /// @brief Creates an instance of the IpAddress class with an IPv6 address
        /// @param [in] v6Address An IPv6 address
        IpAddress ( const in6_addr & v6Address );

        /// @brief Creates an instance of the IpAddress as a copy of existing one
        /// @param [in] other Existing IpAddress
        IpAddress ( const IpAddress & other );

        /// @brief Creates an instance of the IpAddress class from a string
        /// @param [in] strAddress String containing the address
        IpAddress ( const String & strAddress );

        /// @brief Creates an instance of the IpAddress class from a string
        /// @param [in] strAddress String containing the address
        IpAddress ( const char * strAddress );

        /// @brief Assigns new value to IpAddress
        /// @param [in] other The existing IpAddress to create from
        IpAddress & operator= ( const IpAddress & other );

        /// @brief Assigns new value to IpAddress
        /// @param [in] strAddress The existing IpAddress to create from
        IpAddress & operator= ( const String & strAddress );

        /// @brief Assigns new value to IpAddress
        /// @param [in] strAddress The existing IpAddress to create from
        IpAddress & operator= ( const char * strAddress );

        /// @brief Assigns new value to IpAddress
        /// @param [in] v4Address The in_addr struct to create an IpAddress from
        IpAddress & operator= ( const in_addr & v4Address );

        /// @brief Assigns new value to IpAddress
        /// @param [in] v6Address The in6_addr struct to create an IpAddress from
        IpAddress & operator= ( const in6_addr & v6Address );

        /// @brief Sets up the IP address based on the memory provided
        /// The memory has to point to in_addr data of correct size (4 bytes)
        /// @param [in] memory The memory to use
        void setupV4Memory ( const char * memory );

        /// @brief Sets up the IP address based on the memory provided
        /// The memory has to point to in6_addr data of correct size (16 bytes)
        /// @param [in] memory The memory to use
        void setupV6Memory ( const char * memory );

        /// @brief Compares two IpAddresses to determine equality. Returns true when both addresses are empty.
        /// Returns false if the versions do not match.
        /// @param [in] other The second IP address to compare
        /// @return A value indicating whether the two IP addresses are equal
        bool operator== ( const IpAddress & other ) const;

        /// @brief Compares two IpAddresses to determine inequality. Returns false when both addresses are empty.
        /// @param [in] other The second IP address to compare
        /// @return A value indicating whether the two IP addresses are inequal
        inline bool operator!= ( const IpAddress & other ) const
        {
            return !( *this == other );
        }

        /// @brief Compares two IpAddresses to determine less-then relation.
        /// Returns false when both addresses are empty and if the versions do not match.
        /// @param [in] other The second IP address to compare
        /// @return A value indicating whether the second IP address is greater than the first one.
        bool operator< ( const IpAddress & other ) const;

        /// @brief Compares two IpAddresses to determine greater-then relation.
        /// Returns false when both addresses are empty.
        /// @param [in] other The second IP address to compare
        /// @return A value indicating whether the second IP address is greater than the first one.
        bool operator> ( const IpAddress & other ) const;

        /// @brief Increments the IP address to the next IP address val times
        /// @param [in] val Number of times to increment this IP address by
        /// @note The address may loop around if the storage type loops around
        void incrementBy ( uint8_t val );

        /// @brief Convert the current IP address to a V4-mapped V6 address.
        /// @note The current IP address must be a V4 address
        /// @return True if conversion was successful
        bool convertToV4MappedV6();

        /// @brief Convert the current IP address to a V4 address.
        /// @note The current IP address must be a V4-mapped V6 address
        /// @return True if conversion was successful
        bool convertToV4();

        /// @brief Gets a value indicating whether this address is valid
        /// @returns A value indicating whether this address is valid
        inline bool isValid() const
        {
            return ( _version == V4Address || _version == V6Address );
        }

        /// @brief Used for determining if the address is "zero" (like 0.0.0.0)
        /// @note This also returns true for IPv6-Mapped IPv4 address (::ffff:0.0.0.0).
        /// @return True if the address contains just zeros.
        bool isZero() const;

        /// @brief Checks whether this is an IPv4 address
        /// @returns True if this is an IPv4 address
        inline bool isIPv4() const
        {
            return ( _version == V4Address );
        }

        /// @brief Checks whether this is an IPv6 address
        /// @returns True if this is an IPv6 address
        inline bool isIPv6() const
        {
            return ( _version == V6Address );
        }

        /// @brief Checks whether this is an IPv4 or an IPv6 link-local address.
        /// For IPv4 it checks if the address uses 169.254.0.0/16 prefix.
        /// For IPv6 it checks if the address uses fe80::/64 prefix.
        /// @returns True if this is an IPv4 link-local address
        bool isLinkLocal() const;

        /// @brief Checks whether this address is an IPv4 address mapped to IPv6.
        /// @return True if this is an IPv4 address mapped to IPv6.
        bool isIPv6MappedIPv4() const;

        /// @brief Returns the IP address as uint32 using local endianness
        /// @warning If this is an IPv6 address, the returned value will represent only the last 4 bytes of it.
        /// @return The IP address (or part of it) as uint32
        inline uint32_t toUInt32() const
        {
            if ( isIPv4() )
            {
                return ntohl ( _address.v4.s_addr );
            }
            else if ( isIPv6() )
            {
                return ntohl ( ( ( const uint32_t * ) &( _address.v6 ) )[ 3 ] );
            }

            return 0;
        }

        /// @brief Gets the IPv4 address
        /// @returns The IPv4 address
        const in_addr & getV4() const;

        /// @brief Gets the IPv6 address
        /// @returns The IPv6 address
        const in6_addr & getV6() const;

        /// @brief Returns the type of this address
        /// @return the type of this address
        inline AddressType getAddrType() const
        {
            return _version;
        }

        /// @brief Returns this IP address converted to SockAddr using given port number
        /// @param [in] portNumber Port number to put in SockAddr
        /// @return IpAddress and port number in SockAddr form
        SockAddr getSockAddr ( uint16_t portNumber ) const;

        /// @brief Compares IpAddress to a given address, taking into account provided netmask.
        /// @param [in] addr The second IP address to compare
        /// @param [in] netmask Netmask to use while comparing
        /// @return True if addresses are equal (in netmask part)
        /// @note This also returns false if addr and netmask are not the same type of address
        bool isEqual ( const IpAddress & addr, const IpAddress & netmask ) const;

        /// @brief Compares IpAddress to a given address, taking into account provided netmask (given by its length).
        /// @param [in] addr The second IP address to compare
        /// @param [in] netmaskLength Netmask length (in bits) to use while comparing
        /// @return True if addresses are equal (in netmask part)
        bool isEqual ( const IpAddress & addr, uint8_t netmaskLength ) const;

        /// @brief Compares IpAddress to a given IPv4 address, taking into account provided netmask.
        /// @param [in] addr The second IP address to compare
        /// @param [in] netmask Netmask to use while comparing
        /// @return True if addresses are equal (in netmask part)
        bool isEqual ( const in_addr & addr, const in_addr & netmask ) const;

        /// @brief Compares IpAddress to a given IPv6 address, taking into account provided netmask.
        /// @param [in] addr The second IP address to compare
        /// @param [in] netmask Netmask to use while comparing
        /// @return True if addresses are equal (in netmask part)
        bool isEqual ( const in6_addr & addr, const in6_addr & netmask ) const;

        /// @brief Given the netmask length it calculates the broadcast address (based on this address)
        /// @param [in] netmaskLen The length (in bits) of the netmask to use
        /// @return The calculated broadcast address for this IP address
        IpAddress getBcastAddress ( uint8_t netmaskLen ) const;

        /// @brief Given the netmask length it calculates the netmask address (based on this address)
        /// @param [in] netmaskLen The length (in bits) of the netmask to use
        /// @return The calculated netmask address for this IP address
        IpAddress getNetmaskAddress ( uint8_t netmaskLen ) const;

        /// @brief Given the netmask length it calculates the network address (based on this address)
        /// @param [in] netmaskLen The length (in bits) of the netmask to use
        /// @return The calculated network address for this IP address
        IpAddress getNetworkAddress ( uint8_t netmaskLen ) const;

        /// @brief Returns the IP address as a network prefix, or -1 if it is not a network prefix
        /// @note This should really only be used on addresses that are netmasks (though it can also
        /// be used to test if an IP address could be a valid netmask.
        /// @return Network prefix, or -1 if the address is not a network prefix
        int toPrefix() const;

        /// @brief Gets the IP address as a human-friendly string
        /// @param [in] includeIPv6Brackets If true, it will include brackets around IPv6 addresses.
        /// @return The IP address as a human-friendly string.
        String toString ( bool includeIPv6Brackets = false ) const;

        /// @brief Converts the given IPv4 address to a human-friendly string
        /// @param [in] address IPv4 address to convert to a string.
        /// @return The IPv4 address as a human-friendly string.
        static String toString ( const in_addr & address );

        /// @brief Converts the given IPv6 address to a human-friendly string
        /// @param [in] address IPv6 address to convert to a string.
        /// @param [in] includeBrackets If true, it will include brackets around the address.
        /// @return The IPv6 address as a human-friendly string.
        static String toString ( const in6_addr & address, bool includeBrackets = false );

        /// @brief Clears (invalidates) this IpAddress
        void clear();

        /// @brief Resolves given DNS hostname.
        /// @param [in] hostname The hostname to resolve.
        ///                      If it already is an IP address in string form, it is simply converted.
        /// @param [out] errDesc If used and there is an error, its description will be stored here.
        ///                      It is NOT set if there were no errors, even if no addresses are returned.
        /// @return The list of addresses resolved from the given hostname.
        static List<IpAddress> dnsResolve ( const String & hostname, String * errDesc = 0 );

        /// @brief A helper function for converting a string that contains 'ip_address:port_number'
        ///        into IpAddress and port values
        /// @param [in] addrSpec String version of address:port
        /// @param [out] addr address part of addrSpec; Not modified if the conversion fails
        /// @param [out] port port part of addrSpec; Not modified if the conversion fails
        /// @return True if the conversion was successful and both address and port number are valid; False otherwise
        static bool convertAddrSpec ( const String & addrSpec, IpAddress & addr, uint16_t & port );

        /// @brief Converts pointers * to struct sockaddr_in *
        /// Should be implemented in separate module - otherwise gcc tries to optimize
        /// this and complains about breaking strict-aliasing.
        /// @param [in] ptr Pointer to struct sockaddr
        /// @return Pointer to struct sockaddr_in
        static struct sockaddr_in * toSockaddrInPtr ( void * ptr );

        /// @brief Converts pointers * to struct sockaddr_in *
        /// Should be implemented in separate module - otherwise gcc tries to optimize
        /// this and complains about breaking strict-aliasing.
        /// @param [in] ptr Pointer to struct sockaddr
        /// @return Pointer to struct sockaddr_in
        static const struct sockaddr_in * toSockaddrInPtr ( const void * ptr );

        /// @brief Converts pointers to struct sockaddr_in6 *
        /// Should be implemented in separate module - otherwise gcc tries to optimize
        /// this and complains about breaking strict-aliasing.
        /// @param [in] ptr Pointer to struct sockaddr
        /// @return Pointer to struct sockaddr_in6
        static struct sockaddr_in6 * toSockaddrIn6Ptr ( void * ptr );

        /// @brief Converts pointers * to struct sockaddr_in *
        /// Should be implemented in separate module - otherwise gcc tries to optimize
        /// this and complains about breaking strict-aliasing.
        /// @param [in] ptr Pointer to struct sockaddr
        /// @return Pointer to struct sockaddr_in
        static const struct sockaddr_in6 * toSockaddrIn6Ptr ( const void * ptr );

        /// @brief Sets a given byte in the storage type
        /// @param [in,out] storage The variable to modify
        /// @param [in] byteNum The number of the byte to modify. Has to be >=0 and < sizeof(storage)
        /// @param [in] value The value to set
        template<typename T> static inline void setByte ( T & storage, uint8_t byteNum, uint8_t value )
        {
            assert ( byteNum < sizeof ( storage ) );

            ( ( uint8_t * ) &storage )[ byteNum ] = value;
        }

        /// @brief Gets a given byte in the storage type
        /// @param [in,out] storage The variable to read from
        /// @param [in] byteNum The number of the byte to read. Has to be >=0 and < sizeof(storage)
        /// @return The value of the given byte
        template<typename T> static inline uint8_t getByte ( const T & storage, uint8_t byteNum )
        {
            assert ( byteNum < sizeof ( storage ) );

            return ( ( const uint8_t * ) &storage )[ byteNum ];
        }

    protected:
        /// @brief The IP version of this address
        AddressType _version;

        /// @brief A union containing an IPv4 or IPv6 address
        union Address
        {
            in_addr v4;
            in6_addr v6;

            Address()
            {
                memset ( this, 0, sizeof ( Address ) );
            }

            Address ( const in_addr &addrV4 )
            {
                memset ( this, 0, sizeof ( Address ) );
                v4 = addrV4;
            }

            Address ( const in6_addr &addrV6 )
            {
                memset ( this, 0, sizeof ( Address ) );
                v6 = addrV6;
            }
        };

        /// @brief The IP address stored by this class
        Address _address;
};

/// @brief Hash function needed for using IpAddress objects as HashMap and HashSet keys.
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const IpAddress & key );

/// @brief Generates a string with the list of IpAddresses
/// @param [in] ipAddrList The list of the IpAddress objects to describe
/// @return The description of all the elements in the list.
String toString ( const List<IpAddress> & ipAddrList );

/// @brief Generates a string with the list of IpAddresses
/// @param [in] ipAddrList The list of the IpAddress objects to describe
/// @return The description of all the elements in the list.
String toString ( const SimpleArray<IpAddress> & ipAddrList );
}
