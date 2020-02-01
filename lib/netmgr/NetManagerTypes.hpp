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

#include "basic/HashSet.hpp"
#include "basic/IpAddress.hpp"
#include "basic/String.hpp"

namespace Pravala
{
class NetManagerBase;

namespace NetManagerTypes
{
/// @brief Information about an interface IP address.
struct Address
{
    IpAddress localAddress; ///< Local address
    IpAddress broadcastAddress; ///< Broadcast address

    int ifaceId; ///< Interface ID

    uint8_t prefixLen; ///< Address prefix length

    /// @brief Default constructor.
    Address();

    /// @brief Compares two Address to determine equality.
    /// @param [in] other The second Address to compare
    /// @return A value indicating whether the two Address objects are equal
    bool operator== ( const Address & other ) const;
};

/// @brief Information about a system route.
struct Route
{
    IpAddress src; ///< Source address
    IpAddress dst; ///< Destination address
    IpAddress gw; ///< Gateway address

    int ifaceIdIn; ///< Interface ID (input)
    int ifaceIdOut; ///< Interface ID (output)

    int metric; ///< Route metric

    uint8_t dstPrefixLen; ///< Destination address prefix length
    uint8_t srcPrefixLen; ///< Source address prefix length
    uint8_t table; ///< Table
    uint8_t routingProtocol; ///< The 'routing protocol'; On Linux it's kernel/boot/static/redirect/unknown

    /// @brief Compares two Routes to determine equality.
    /// @param [in] other The second Route to compare
    /// @return A value indicating whether the two Route objects are equal
    bool operator== ( const Route & other ) const;

    /// @brief Default constructor.
    Route();

    /// @brief Checks if this Route object describes a default route.
    /// It returns true if this object's route prefix has length 0, OR if it contains an IPv6
    /// address "2000::" with prefix length 3. That route was used as a default one by older Linux kernels.
    /// @return True if this Route object describes a default route; False otherwise.
    inline bool isDefaultRoute() const
    {
        static const IpAddress ipv6AltDefaultGateway ( "2000::" );

        return ( dstPrefixLen == 0 || ( dstPrefixLen == 3 && dst == ipv6AltDefaultGateway ) );
    }

    /// @brief checks if this Route object describes a host route.
    /// @return True if this route is a host route (has full prefix length, 32 bits for IPv4, 128 bits for IPv6).
    inline bool isHostRoute() const
    {
        return ( dstPrefixLen == ( dst.isIPv6() ? 128 : 32 ) );
    }

    /// @brief Returns a description of the route for logging.
    /// @return A description of the route for logging.
    inline String toString() const
    {
        return String ( "%1/%2 [iface %3 gw %4]" ).arg ( dst ).arg ( dstPrefixLen ).arg ( ifaceIdOut ).arg ( gw );
    }
};

/// @brief Information about an interface and its configuration on the system.
struct Interface
{
    static const uint32_t FlagIsUp = 0x01; ///< Flag determining whether this interface is UP
    static const uint32_t FlagIsRunning = 0x02; ///< Flag determining whether this interface is running.
    static const uint32_t FlagIsLoopback = 0x04; ///< Flag determining whether this interface is a loopback interface.
    static const uint32_t FlagIsPtp = 0x08; ///< Flag determining whether this interface is a Point-to-Point interface.

    /// @brief Flag determining whether this interface is active (UP and running)
    static const uint32_t FlagIsActive = ( FlagIsUp | FlagIsRunning );

    uint8_t hwAddr[ 16 ]; ///< hardware address
    uint8_t hwBroadcastAddr[ 16 ]; ///< hardware broadcast address

    String name;  ///< name of the interface

    int type;   ///< device type, see net/if_arp.h ARPHRD_*
    int id;  ///< unique ID associated with this interface
    int realId;   ///< ID of the real interface if this interface is virtual and real interface is fixed+known

    int hwAddrLen;  ///< length of hardware address
    int hwBroadcastAddrLen;  ///< length of hardware broadcast address

    uint32_t mtu;   ///< maximum transmission unit of the interface
    uint32_t flags;  ///< interface flags

    /// @brief Checks whether this interface is UP.
    /// @brief True if this interface is UP; False otherwise.
    inline bool isUp() const
    {
        return ( ( flags & FlagIsUp ) == FlagIsUp );
    }

    /// @brief Checks whether this interface is running.
    /// @brief True if this interface is running; False otherwise.
    inline bool isRunning() const
    {
        return ( ( flags & FlagIsRunning ) == FlagIsRunning );
    }

    /// @brief Checks whether this interface is active (UP and running).
    /// @brief True if this interface is active (UP and running); False otherwise.
    inline bool isActive() const
    {
        return ( ( flags & FlagIsActive ) == FlagIsActive );
    }

    /// @brief Checks whether this interface is a loopback interface.
    /// @brief True if this interface is a loopback interface; False otherwise.
    inline bool isLoopback() const
    {
        return ( ( flags & FlagIsLoopback ) == FlagIsLoopback );
    }

    /// @brief Checks whether this interface is a Point-to-Point interface.
    /// @brief True if this interface is a Point-to-Point interface; False otherwise.
    inline bool isPtp() const
    {
        return ( ( flags & FlagIsPtp ) == FlagIsPtp );
    }

    /// @brief Constructor.
    Interface();
};

/// @brief An object representing an interface.
class InterfaceObject
{
    public:
        /// @brief Returns the system ID of this interface.
        /// @return the system ID of this interface.
        inline int getId() const
        {
            return _data.id;
        }

        /// @brief Returns the data of this interface.
        /// @return the data of this interface.
        inline const Interface & getData() const
        {
            return _data;
        }

        /// @brief Returns the set of addresses assigned to this interface.
        /// @return the set of addresses assigned to this interface.
        inline const HashSet<Address> & getAddresses() const
        {
            return _addrs;
        }

        /// @brief Returns the set of routes associated with this interface.
        /// (routes that have either ifIndexIn or ifIndexOut the same as _index of this interface).
        /// @return the set of routes associated with this interface.
        inline const HashSet<Route> & getRoutes() const
        {
            return _routes;
        }

        /// @brief Checks whether this interface is active (UP and running).
        /// @brief True if this interface is active (UP and running); False otherwise.
        inline bool isActive() const
        {
            return _data.isActive();
        }

    private:
        Interface _data; ///< This interface's data

        HashSet<Address> _addrs; ///< The addresses assigned to this interface

        /// @brief Routes associated with this interface.
        /// (routes that have either ifIndexIn or ifIndexOut the same as _index of this interface)
        HashSet<Route> _routes;

        /// @brief Updates this interface's underlying data.
        /// @param [in] data The new data to use. It MUST has the same ID!
        void updateData ( const Interface & data )
        {
            assert ( _data.id == data.id );

            _data = data;
        }

        /// @brief Constructor.
        /// @param [in] data Interface data to use.
        InterfaceObject ( const Interface & data ): _data ( data )
        {
        }

        friend class Pravala::NetManagerBase;
};

/// @brief Hash function needed for storing Address objects in HashMap and HashSet containers.
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const NetManagerTypes::Address & key );

/// @brief Hash function needed for storing Route objects in HashMap and HashSet containers.
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const NetManagerTypes::Route & key );
}
}
