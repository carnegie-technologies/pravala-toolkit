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

#include "NetlinkSyncSocket.hpp"

namespace Pravala
{
/// @brief An object for performing SYNCHRONOUS Netlink "route" operations
class NetlinkRouteControl: public NetlinkSyncSocket
{
    public:
        /// @brief Creates a new NetlinkRouteControl object.
        NetlinkRouteControl();

        /// @brief Gets all links on the system
        /// @param [out] links The links read from the operating system
        /// @return Standard error code
        ERRCODE getLinks ( List<NetlinkTypes::Link> & links );

        /// @brief Gets all addresses on the system
        /// @param [out] addresses The addresses read from the operating system
        /// @param [in] addrType The IP type of the addresses to retrieve (defaults to both IPv4 and IPv6).
        /// Specifying V4Address will only return IPv4 addresses; likewise V6Address will only return IPv6 addresses.
        /// @return Standard error code
        ERRCODE getAddresses (
            List<NetlinkTypes::Address> & addresses,
            IpAddress::AddressType addrType = IpAddress::EmptyAddress );

        /// @brief Gets specified routes
        /// @param [out] routes The routes read from the operating system
        /// @param [in] rtTable Routing table number (see /etc/iproute2/rt_tables) on Linux
        /// @return Standard error code
        ERRCODE getRoutes ( List<NetlinkTypes::Route> & routes, uint8_t rtTable = RT_TABLE_MAIN );

        /// @brief Add an address
        /// @param [in] ifaceId The ID of the network interface this address will be added on
        /// @param [in] addr The IP address to add
        /// @param [in] mask The netmask (in number of bits) associated with the address
        /// @return Standard error code
        ERRCODE addIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask );

        /// @brief Remove an address
        /// @param [in] ifaceId The ID of the network interface this address will be removed from
        /// @param [in] addr The IP address to remove
        /// @param [in] mask The netmask (in number of bits) associated with the address
        /// @return Standard error code
        ERRCODE removeIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask );

        /// @brief Add a route
        /// @param [in] dst The destination address
        /// @param [in] mask The netmask (in number of bits) associated with the destination address
        /// @param [in] gw The gateway to route this network over. If not set, the route will be added over the
        /// interface specified via ifaceId.
        /// @param [in] ifaceId The ID of the network interface this route should be added on.
        /// If specified and gw is specified, both will be used (i.e. multiple ifaces on same subnet).
        /// If specified and gw not specified, the route will be added over this interface.
        /// If neither gw nor ifaceId is specified, this will return an error.
        /// @param [in] metric The metric of this route.
        /// @return Standard error code
        ERRCODE addRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        /// @brief Remove a route
        /// @param [in] dst The destination address
        /// @param [in] mask The netmask (in number of bits) associated with the destination address
        /// @param [in] gw The gateway to route this network over. If not set, the route will be removed from the
        /// interface specified via ifaceId.
        /// @param [in] ifaceId The ID of the network interface this route should be removed from.
        /// If specified and gw is specified, both will be used (i.e. multiple ifaces on same subnet).
        /// If specified and gw not specified, the route will be remove from this interface.
        /// If neither gw nor ifaceId is specified, this will return an error.
        /// @param [in] metric The metric of this route.
        /// @return Standard error code
        ERRCODE removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        /// @brief Set the interface MTU
        /// @param [in] ifaceId The ID of the network interface the MTU will be set on
        /// @param [in] mtu The MTU value
        /// @return Standard error code
        ERRCODE setIfaceMtu ( int ifaceId, int mtu );

        /// @brief Set the interface up or down
        /// @param [in] ifaceId The ID of the network interface to set up or down
        /// @param [in] isUp If True, bring up; if false, take down
        /// @return Standard error code
        ERRCODE setIfaceState ( int ifaceId, bool isUp );
};
}
