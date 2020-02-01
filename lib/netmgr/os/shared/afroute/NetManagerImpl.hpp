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

#include "../../../NetManagerBase.hpp"
#include "AfRouteControl.hpp"
#include "AfRouteMonitor.hpp"

namespace Pravala
{
/// @brief AF_ROUTE implementation of the Network Manager.
class NetManagerImpl: public NetManagerBase, public AfRouteMonitor::Owner
{
    public:
        /// @brief Returns global instance of the NetManagerImpl
        static NetManagerImpl & get();

        virtual ERRCODE addIfaceAddress ( int ifaceId, const IpAddress & address );
        virtual ERRCODE removeIfaceAddress ( int ifaceId, const IpAddress & address );

        virtual ERRCODE setIfaceMtu ( int ifaceId, int mtu );
        virtual ERRCODE setIfaceState ( int ifaceId, bool isUp );

        virtual ERRCODE addRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );
        virtual ERRCODE removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        virtual ERRCODE getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface );
        virtual ERRCODE getUncachedIface ( const String & ifaceName, NetManagerTypes::Interface & iface );

        virtual ERRCODE readIfaceUsage ( const String & ifaceName, uint64_t & rxBytes, uint64_t & txBytes );

    protected:
        virtual void afRouteMonUpdate (
            AfRouteMonitor * monitor,
            List<AfRouteTypes::Link> & links,
            List<AfRouteTypes::Address> & addrs,
            List<AfRouteTypes::Route> & routes );

    private:
        static THREAD_LOCAL NetManagerImpl * _instance; ///< This thread's instance of NetManagerImpl.

        AfRouteControl _routeCtrl; ///< Control object.
        AfRouteMonitor _routeMon; ///< Monitor object.

        /// @brief Constructor.
        NetManagerImpl();

        /// @brief Destructor.
        virtual ~NetManagerImpl();

        /// @brief Performs a synchronous route state update.
        /// @return Standard error code.
        ERRCODE refreshRoutes();

        /// @brief Performs a full, synchronous, state refresh.
        /// @return Standard error code.
        ERRCODE refreshState();

        /// @brief Sets the entire state.
        /// It takes data in the format returned by AF_ROUTE.
        /// @param [in] links The list of link updates.
        /// @param [in] addresses The list of address updates.
        /// @param [in] routes The list of route updates.
        void setAfRouteLinks (
            const List<AfRouteTypes::Link > & links,
            const List<AfRouteTypes::Address > & addresses,
            const List<AfRouteTypes::Route > & routes );
};
}
