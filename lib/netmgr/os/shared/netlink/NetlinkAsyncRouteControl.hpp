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

#include "event/EventManager.hpp"
#include "NetlinkRouteMonitor.hpp"

namespace Pravala
{
/// @brief An object for performing asynchronous Netlink "route" control operations.
/// This class should only be used to modify the state of the system.
/// To actually read the state of the system, NetlinkRouteMonitor should be used.
class NetlinkAsyncRouteControl: public NetlinkRouteMonitor
{
    public:
        /// @brief Creates a new Socket object with a Netlink socket fd set with parameters
        /// @param [in] owner Owner of this Netlink monitor
        NetlinkAsyncRouteControl ( Owner & owner );

        /// @brief Asynchronously add a route.
        /// @param [in] dst The destination address
        /// @param [in] mask The netmask (number of bits) associated with the destination address
        /// @param [in] gw The gateway to route this network over. If not set, the route will be added over the
        /// interface specified via ifaceId.
        /// @param [in] ifaceId The ID of the network interface this route should be added on.
        /// If specified and gw is specified, both will be used (i.e. multiple ifaces on same subnet).
        /// If specified and gw not specified, the route will be added over this interface.
        /// If neither gw nor ifaceId is specified, this will return an error.
        /// @param [in] metric The associated metric.
        /// @return Sequence number used for the request.
        uint32_t addRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        /// @brief Asynchronously remove a route.
        /// @param [in] dst The destination address
        /// @param [in] mask The netmask (number of bits) associated with the destination address
        /// @param [in] gw The gateway to route this network over. If not set, the route will be removed from the
        /// interface specified via ifaceId.
        /// @param [in] ifaceId The ID of the network interface this route should be removed from.
        /// If specified and gw is specified, both will be used (i.e. multiple ifaces on same subnet).
        /// If specified and gw not specified, the route will be removed from this interface.
        /// If neither gw nor ifaceId is specified, this will return an error.
        /// @param [in] metric The associated metric.
        /// @param [in] tableId The ID of the routing table to use. If < 0, the default table will be used.
        /// @return Sequence number used for the request.
        uint32_t removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );
};
}
