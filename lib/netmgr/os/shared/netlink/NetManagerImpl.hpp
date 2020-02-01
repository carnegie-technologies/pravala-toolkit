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

#include "basic/Thread.hpp"

#include "../../../NetManagerBase.hpp"
#include "NetlinkRouteControl.hpp"
#include "NetlinkAsyncRouteControl.hpp"
#include "NetlinkRouteMonitor.hpp"

namespace Pravala
{
/// @brief Netlink implementation of the Network Manager.
class NetManagerImpl: public NetManagerBase, public NetlinkRouteMonitor::Owner
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
        virtual void addRouteAsync (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        virtual ERRCODE removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );
        virtual void removeRouteAsync (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        virtual ERRCODE getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface );
        virtual ERRCODE getUncachedIface ( const String & ifaceName, NetManagerTypes::Interface & iface );

        virtual ERRCODE readIfaceUsage ( const String & ifaceName, uint64_t & rxBytes, uint64_t & txBytes );

    protected:
        virtual void netlinkRcvRouteResults (
            NetlinkRouteMonitor * monitor,
            uint32_t seqNum, const struct nlmsgerr * netlinkError,
            NetlinkRoute::RouteResults & routeResults );
        virtual void netlinkRouteReqFailed ( NetlinkRouteMonitor * monitor, uint32_t reqSeqNum, ERRCODE errorCode );
        virtual void netlinkRouteMonitorFailed ( NetlinkRouteMonitor * monitor );

    private:
        static THREAD_LOCAL NetManagerImpl * _instance; ///< This thread's instance of NetManagerImpl.

        NetlinkRouteControl _routeCtrl; ///< Route control object.
        NetlinkAsyncRouteControl _asyncRouteCtrl; ///< Asynchronous route control object.
        NetlinkRouteMonitor _routeMon; ///< Route monitor object.

        bool _fullUpdate; ///< Set to true whenever we are performing a full update (ifaces, addresses and routes)

        /// @brief The sequence number of an asynchronous link list request.
        /// It is only set (to a non-zero value) if we are waiting for a result of a full link update).
        uint32_t _linkListReq;

        /// @brief The sequence number of an asynchronous address list request.
        /// It is only set (to a non-zero value) if we are waiting for a result of a full address update).
        uint32_t _addrListReq;

        /// @brief The sequence number of an asynchronous route list request.
        /// It is only set (to a non-zero value) if we are waiting for a result of a full route update).
        uint32_t _routeListReq;

        /// @brief List of links read during multi-part asynchronous full state update.
        List<NetlinkTypes::Link> _pendingLinks;

        /// @brief List of addresses read during multi-part asynchronous full state update.
        List<NetlinkTypes::Address> _pendingAddresses;

        /// @brief Constructor.
        NetManagerImpl();

        /// @brief Destructor.
        virtual ~NetManagerImpl();

        /// @brief Clears 'pending' state.
        /// It clears all _pending* structures, _fullUpdate flag, as well as all pending request IDs.
        void clearPending();

        /// @brief Starts asynchronous, full state update.
        void startStateRefresh();

        /// @brief Sets the entire state.
        /// It takes data in the format returned by Netlink.
        /// @param [in] links The list of link updates.
        /// @param [in] addresses The list of address updates.
        /// @param [in] routes The list of route updates.
        void setNetlinkLinks (
            const List<NetlinkTypes::Link> & links,
            const List<NetlinkTypes::Address> & addresses,
            const List<NetlinkTypes::Route> & routes );

        /// @brief A helper function that handles individual RouteMonitor requests that failed.
        /// @param [in] seqNum The sequence number of the failed request.
        void routeMonitorReqError ( uint32_t seqNum );
};
}
