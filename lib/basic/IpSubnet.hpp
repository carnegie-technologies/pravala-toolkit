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

#include "IpAddress.hpp"

namespace Pravala
{
/// @brief An IP subnet (IpAddress + prefix length).
class IpSubnet
{
    public:
        /// @brief Default constructor, creates an empty subnet (empty IP address, prefix length 0).
        IpSubnet();

        /// @brief Constructor.
        /// Prefix length is set to 32 (IPv4), 128 (IPv6) or 0 (invalid address)
        /// @param [in] addr The IP address to use.
        IpSubnet ( const IpAddress & addr );

        /// @brief Constructor.
        /// @param [in] addr The IP address to use.
        /// @param [in] prefixLength Prefix length to use. It will be limited by the address type used
        ///                           (max 32 for IPv4, max 128 for IPv6 and 0 for invalid addresses).
        IpSubnet ( const IpAddress & addr, uint8_t prefixLength );

        /// @brief Copy constructor.
        /// @param [in] other Existing IpSubnet to copy.
        IpSubnet ( const IpSubnet & other );

        /// @brief Assigns new value to IpSubnet
        /// @param [in] other The existing IpSubnet to create from
        IpSubnet & operator= ( const IpSubnet & other );

        /// @brief Assigns new value to IpSubnet
        /// @param [in] addr The IpAddress to use. Prefix length is set to 32 (IPv4), 128 (IPv6) or 0 (invalid address).
        IpSubnet & operator= ( const IpAddress & addr );

        /// @brief Constructs a subnet based on a string.
        /// If invalid string is used, this subnet will contain an invalid address and 0 prefix.
        /// @param [in] str The string to use. Should be in "IP_address/prefix_length" format.
        explicit IpSubnet ( const String & str );

        /// @brief Compares two IpSubnets to determine equality.
        /// @param [in] other The second IP subnet to compare
        /// @return A value indicating whether the two IP subnets are equal
        inline bool operator== ( const IpSubnet & other ) const
        {
            return ( _address == other._address && _prefixLength == other._prefixLength );
        }

        /// @brief Compares two IpSubnets to determine inequality.
        /// @param [in] other The second IP subnet to compare
        /// @return A value indicating whether the two IP subnets are inequal
        inline bool operator!= ( const IpSubnet & other ) const
        {
            return !( *this == other );
        }

        /// @brief Configures this IpSubnet based on the string passed.
        /// @param [in] str The string to use. Should be in "IP_address/prefix_length" format.
        /// @return True if the string was in the right format;
        ///         False if there was an error (this object is NOT modified on error).
        bool setFromString ( const String & str );

        /// @brief Gets the IP subnet as a human-friendly string
        /// @param [in] includeIPv6Brackets If true, it will include brackets around IPv6 addresses.
        /// @return The IP subnet as a human-friendly string.
        String toString ( bool includeIPv6Brackets = false ) const;

        /// @brief Returns the type of this subnet
        /// @return the type of this subnet
        inline IpAddress::AddressType getAddrType() const
        {
            return _address.getAddrType();
        }

        /// @brief Clears the data.
        inline void clear()
        {
            _address.clear();
            _prefixLength = 0;
        }

        /// @brief Returns true if this subnet is valid (has a valid IP address)
        /// @return True if this subnet is valid (has a valid IP address); False otherwise.
        inline bool isValid() const
        {
            return _address.isValid();
        }

        /// @brief Exposes the underlying IP address of this subnetwork.
        /// @return Underlying IP address of this subnetwork.
        inline const IpAddress & getAddress() const
        {
            return _address;
        }

        /// @brief Exposes the underlying prefix length of this subnetwork.
        /// @return Underlying prefix length of this subnetwork.
        inline uint8_t getPrefixLength() const
        {
            return _prefixLength;
        }

        /// @brief Checks if this subnetwork contains given IP address.
        /// @param [in] addr The address to check.
        /// @return True if the IP address given is part of this IP subnetwork; False otherwise.
        inline bool contains ( const IpAddress & addr ) const
        {
            return _address.isEqual ( addr, _prefixLength );
        }

    protected:
        IpAddress _address; ///< The address of this subnetwork.
        uint8_t _prefixLength; ///< The prefix length of this subnetwork.
};

/// @brief Hash function needed for using IpAddress objects as HashMap and HashSet keys.
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const IpSubnet & key );

/// @brief Generates a string with the list of IpSubnets
/// @param [in] ipSubnetList The list of the IpSubnets objects to describe
/// @return The description of all the elements in the list.
String toString ( const List<IpSubnet> & ipSubnetList );
}
