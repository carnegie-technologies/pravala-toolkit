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

#include "sys/File.hpp"

#include "NetManagerImpl.hpp"

// Netlink-specific tweaks are marked with "NETLINK-HACK" comments

using namespace Pravala;

THREAD_LOCAL NetManagerImpl * NetManagerImpl::_instance ( 0 );

NetManager & NetManager::get()
{
    return NetManagerImpl::get();
}

NetManagerImpl & NetManagerImpl::get()
{
    if ( !_instance )
    {
        _instance = new NetManagerImpl();

        assert ( _instance != 0 );
    }

    return *_instance;
}

NetManagerImpl::NetManagerImpl():
    _asyncRouteCtrl ( *this ),
    _routeMon ( *this, RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE ),
    _fullUpdate ( false ),
    _linkListReq ( 0 ),
    _addrListReq ( 0 ),
    _routeListReq ( 0 )
{
    startStateRefresh();
}

NetManagerImpl::~NetManagerImpl()
{
    // To run all remaining tasks.
    runTasks();

    if ( _instance == this )
    {
        _instance = 0;
    }
}

ERRCODE NetManagerImpl::addIfaceAddress ( int ifaceId, const IpAddress & address )
{
    return _routeCtrl.addIfaceAddress ( ifaceId, address, address.isIPv6() ? 128 : 32 );
}

ERRCODE NetManagerImpl::removeIfaceAddress ( int ifaceId, const IpAddress & address )
{
    return _routeCtrl.removeIfaceAddress ( ifaceId, address, address.isIPv6() ? 128 : 32 );
}

ERRCODE NetManagerImpl::setIfaceState ( int ifaceId, bool isUp )
{
    return _routeCtrl.setIfaceState ( ifaceId, isUp );
}

ERRCODE NetManagerImpl::setIfaceMtu ( int ifaceId, int mtu )
{
    return _routeCtrl.setIfaceMtu ( ifaceId, mtu );
}

ERRCODE NetManagerImpl::addRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    return _routeCtrl.addRoute ( dst, mask, gw, ifaceId, metric, tableId );
}

void NetManagerImpl::addRouteAsync (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    _asyncRouteCtrl.addRoute ( dst, mask, gw, ifaceId, metric, tableId );
}

ERRCODE NetManagerImpl::removeRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    return _routeCtrl.removeRoute ( dst, mask, gw, ifaceId, metric, tableId );
}

void NetManagerImpl::removeRouteAsync (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    _asyncRouteCtrl.removeRoute ( dst, mask, gw, ifaceId, metric, tableId );
}

void NetManagerImpl::setNetlinkLinks (
        const List< NetlinkTypes::Link > & links,
        const List< NetlinkTypes::Address > & addrs,
        const List< NetlinkTypes::Route > & routes )
{
    // This map will contain ALL interfaces that are present (active or not); Key is the iface ID;
    HashMap<int, NetManagerTypes::Interface> ifaces;

    for ( size_t i = 0; i < links.size(); ++i )
    {
        const NetlinkTypes::Link & link = links.at ( i );

        if ( link.id == 0 )
            continue;

        if ( link.act == PosixNetMgrTypes::ActionAdd )
        {
            ifaces.insert ( link.id, link );
        }
        else
        {
            ifaces.remove ( link.id );
        }
    }

    HashSet<NetManagerTypes::Address> addrSet;

    for ( size_t i = 0; i < addrs.size(); ++i )
    {
        const NetlinkTypes::Address & addr = addrs.at ( i );

        if ( !ifaces.contains ( addr.ifaceId ) )
            continue;

        // Address entry doesn't have a valid local address, skip it.
        // This is normal when Netlink sends us other data in a NEWADDR message.
        if ( !addr.localAddress.isValid() )
            continue;

        if ( addr.act == PosixNetMgrTypes::ActionAdd )
        {
            addrSet.insert ( addr );
        }
        else
        {
            addrSet.remove ( addr );
        }
    }

    HashSet<NetManagerTypes::Route> routeSet;

    for ( size_t i = 0; i < routes.size(); ++i )
    {
        const NetlinkTypes::Route & route = routes.at ( i );

        // On Linux (and Android) we want to ignore routes from 'LOCAL' routing table.
        if ( route.table == 255 )
        {
            LOG ( L_DEBUG4, "Ignoring a route info for " << route.dst << "/" << route.dstPrefixLen
                  << " address from the LOCAL routing table" );
            continue;
        }

        if ( route.act == PosixNetMgrTypes::ActionAdd )
        {
            LOG ( L_DEBUG3, "Adding a route info for " << route.dst << "/" << route.dstPrefixLen );

            routeSet.insert ( route );
        }
        else
        {
            LOG ( L_DEBUG3, "Removing a route info for " << route.dst << "/" << route.dstPrefixLen );

            routeSet.remove ( route );
        }
    }

    setIfaces ( ifaces, addrSet, routeSet );
}

void NetManagerImpl::clearPending()
{
    _fullUpdate = false;
    _linkListReq = 0;
    _addrListReq = 0;
    _routeListReq = 0;

    _pendingLinks.clear();
    _pendingAddresses.clear();
}

ERRCODE NetManagerImpl::getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface )
{
    List<NetlinkTypes::Link> links;

    ERRCODE eCode = _routeCtrl.getLinks ( links );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    for ( size_t i = 0; i < links.size(); ++i )
    {
        if ( links.at ( i ).id == ifaceId )
        {
            iface = links.at ( i );

            return Error::Success;
        }
    }

    return Error::NotFound;
}

ERRCODE NetManagerImpl::getUncachedIface ( const String & ifaceName, NetManagerTypes::Interface & iface )
{
    List<NetlinkTypes::Link> links;

    ERRCODE eCode = _routeCtrl.getLinks ( links );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    for ( size_t i = 0; i < links.size(); ++i )
    {
        if ( links.at ( i ).name == ifaceName )
        {
            iface = links.at ( i );

            return Error::Success;
        }
    }

    return Error::NotFound;
}

ERRCODE NetManagerImpl::readIfaceUsage ( const String & ifaceName, uint64_t & rxBytes, uint64_t & txBytes )
{
    MemHandle fContents;

    String fName = String ( "/sys/class/net/%1/statistics/tx_bytes" ).arg ( ifaceName );

    ERRCODE eCode = File::read ( fName, fContents );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Unable to get statistics from " << fName );
        return eCode;
    }
    else
    {
        LOG ( L_DEBUG2, "Read TX statistics from " << fName );
    }

    const String strTxBytes = fContents.toString();

    fName = String ( "/sys/class/net/%1/statistics/rx_bytes" ).arg ( ifaceName );

    eCode = File::read ( fName, fContents );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Unable to get statistics from " << fName );
        return eCode;
    }
    else
    {
        LOG ( L_DEBUG2, "Read RX statistics from " << fName );
    }

    const String strRxBytes = fContents.toString();

    bool isOk = true;

    txBytes = strTxBytes.trimmed().toUInt64 ( &isOk );

    if ( !isOk )
    {
        LOG ( L_ERROR, "Unable to parse TX bytes string '" << strTxBytes << "' as uint64" );
        return Error::InvalidData;
    }

    rxBytes = strRxBytes.trimmed().toUInt64 ( &isOk );

    if ( !isOk )
    {
        LOG ( L_ERROR, "Unable to parse RX bytes string '" << strRxBytes << "' as uint64" );
        return Error::InvalidData;
    }

    return Error::Success;
}

void NetManagerImpl::netlinkRouteMonitorFailed ( NetlinkRouteMonitor * monitor )
{
    if ( monitor != &_routeMon )
        return;

    LOG ( L_WARN, "NetlinkRouteMonitor failed; Starting a full state refresh" );

    startStateRefresh();
}

void NetManagerImpl::netlinkRouteReqFailed ( NetlinkRouteMonitor * monitor, uint32_t reqSeqNum, ERRCODE errorCode )
{
    if ( monitor == &_asyncRouteCtrl )
    {
        LOG_ERR ( L_ERROR, errorCode, "An asynchronous 'set' request failed; SeqNum: " << reqSeqNum );
        return;
    }

    if ( monitor == &_routeMon )
    {
        routeMonitorReqError ( reqSeqNum );
    }
}

void NetManagerImpl::routeMonitorReqError ( uint32_t seqNum )
{
    if ( seqNum == 0 )
        return;

    if ( seqNum == _linkListReq )
    {
        if ( _fullUpdate )
        {
            LOG ( L_WARN, "Link list request failed (SeqNum: " << seqNum << "); Restarting the state refresh" );

            startStateRefresh();
        }
        else
        {
            LOG ( L_WARN, "Link list request failed (SeqNum: " << seqNum << "); Restarting the link list update" );

            _pendingLinks.clear();
            _linkListReq = _routeMon.getLinks();
        }

        return;
    }

    if ( seqNum == _addrListReq )
    {
        if ( _fullUpdate )
        {
            LOG ( L_WARN, "Address list request failed (SeqNum: " << seqNum << "); Restarting the state refresh" );

            startStateRefresh();
        }
        else
        {
            LOG ( L_WARN, "Address list request failed (SeqNum: " << seqNum
                  << "); Restarting the address list update" );

            _pendingAddresses.clear();
            _addrListReq = _routeMon.getAddresses();
        }

        return;
    }

    if ( seqNum == _routeListReq )
    {
        if ( _fullUpdate )
        {
            LOG ( L_WARN, "Route list request failed (SeqNum: " << seqNum << "); Restarting the state refresh" );

            startStateRefresh();
        }
        else
        {
            LOG ( L_WARN, "Route list request failed (SeqNum: " << seqNum << "); Restarting the route list update" );

            _routeListReq = _routeMon.getRoutes();
        }

        return;
    }
}

void NetManagerImpl::startStateRefresh()
{
    // If there are some operations pending, let's unset them:
    clearPending();

    _routeMon.clearRequestQueue();

    _fullUpdate = true;

    if ( ( _linkListReq = _routeMon.getLinks() ) == 0 )
    {
        LOG ( L_FATAL_ERROR, "Error requesting for a full list of links" );

        clearPending();
        return;
    }

    if ( ( _addrListReq = _routeMon.getAddresses() ) == 0 )
    {
        LOG ( L_FATAL_ERROR, "Error requesting for a full list of addresses" );

        clearPending();
        return;
    }

    if ( ( _routeListReq = _routeMon.getRoutes() ) == 0 )
    {
        LOG ( L_FATAL_ERROR, "Error requesting for a full list of routes" );

        clearPending();
        return;
    }

    LOG ( L_DEBUG, "Started full, asynchronous, state update; Link list request: " << _linkListReq
          << "; Address list request: " << _addrListReq << "; Route list request: " << _routeListReq );
}

void NetManagerImpl::netlinkRcvRouteResults (
        NetlinkRouteMonitor * monitor, uint32_t seqNum,
        const struct nlmsgerr * netlinkError, NetlinkRoute::RouteResults & routeResults )
{
    assert ( monitor == &_routeMon || monitor == &_asyncRouteCtrl );

    LOG ( L_DEBUG2, "Received RouteResults from "
          << ( ( monitor == &_routeMon ) ? "Route Monitor" : "Route Async Control" )
          << "; SeqNum: " << seqNum
          << "; Has error: " << ( netlinkError != 0 )
          << "; Error code: " << ( ( netlinkError != 0 ) ? ( netlinkError->error ) : ( 0 ) )
          << "; Link entries: " << routeResults.links.size()
          << "; Addr entries: " << routeResults.addresses.size()
          << "; Route entries: " << routeResults.routes.size() );

    if ( monitor != &_routeMon )
    {
        // We only care about updates from the route monitor.
        return;
    }

    if ( netlinkError != 0 && netlinkError->error != 0 )
    {
        LOG ( L_ERROR, "Received an error message; Request SeqNum: " << seqNum
              << "; Error code: " << netlinkError->error );

        if ( seqNum != 0 )
        {
            routeMonitorReqError ( seqNum );
        }

        return;
    }

    if ( seqNum != 0 )
    {
        if ( seqNum == _linkListReq )
        {
            _linkListReq = 0;
            _pendingLinks = routeResults.links;

            LOG ( L_DEBUG, "Requested list of links received; Size: "
                  << _pendingLinks.size() << "; (full-update, SeqNum: " << seqNum << ")" );
            return;
        }

        if ( seqNum == _addrListReq )
        {
            _addrListReq = 0;

            if ( _fullUpdate )
            {
                _pendingAddresses = routeResults.addresses;

                LOG ( L_DEBUG, "Requested list of addresses received (full-update; size: "
                      << _pendingAddresses.size() << "; SeqNum: " << seqNum << ")" );
                return;
            }

            LOG ( L_DEBUG, "Requested list of addresses received (size: " << routeResults.addresses.size()
                  << "; SeqNum: " << seqNum << "); Setting..." );

            HashSet<NetManagerTypes::Address> add;

            for ( size_t i = 0; i < routeResults.addresses.size(); ++i )
            {
                const NetlinkTypes::Address & addr = routeResults.addresses.at ( i );

                if ( addr.act == PosixNetMgrTypes::ActionAdd )
                {
                    add.insert ( addr );
                }
                else
                {
                    add.remove ( addr );
                }
            }

            setAddresses ( add );
            return;
        }

        if ( seqNum == _routeListReq )
        {
            _routeListReq = 0;

            if ( _fullUpdate )
            {
                List<NetlinkTypes::Link> links = _pendingLinks;
                List<NetlinkTypes::Address> addrs = _pendingAddresses;

                clearPending();

                LOG ( L_DEBUG, "Requested list of routes (full-update; size: " << routeResults.routes.size()
                      << "; SeqNum: " << seqNum << "); Setting ifaces, addresses and routes" );

                setNetlinkLinks ( links, addrs, routeResults.routes );
                return;
            }

            LOG ( L_DEBUG, "Requested list of routes received (size: "
                  << routeResults.routes.size() << "; SeqNum: " << seqNum << "); Setting..." );

            HashSet<NetManagerTypes::Route> add;

            for ( size_t i = 0; i < routeResults.routes.size(); ++i )
            {
                const NetlinkTypes::Route & route = routeResults.routes.at ( i );

                // On Linux (and Android) we want to ignore routes from 'LOCAL' routing table.
                if ( route.table == 255 )
                {
                    LOG ( L_DEBUG4, "Ignoring a route info for " << route.dst << "/" << route.dstPrefixLen
                          << " address from the LOCAL routing table; Action: " << route.act );

                    continue;
                }

                if ( route.act == PosixNetMgrTypes::ActionAdd )
                {
                    LOG ( L_DEBUG3, "Adding a route info for " << route.dst << "/" << route.dstPrefixLen );

                    add.insert ( route );
                }
                else
                {
                    LOG ( L_DEBUG3, "Removing a route info for " << route.dst << "/" << route.dstPrefixLen );

                    add.remove ( route );
                }
            }

            setRoutes ( add );
            return;
        }

        LOG ( L_DEBUG, "Unexpected request received; SeqNum: " << seqNum << "; Ignoring..." );
        return;
    }

    // This is an asynchronous update.

    if ( netlinkError != 0 && netlinkError->error != 0 )
    {
        LOG ( L_DEBUG, "Received a netlink error code: " << netlinkError->error << "; ErrSeqNum: "
              << netlinkError->msg.nlmsg_seq << "; Request SeqNum: " << seqNum << "; Ignoring..." );

        return;
    }

    if ( _fullUpdate )
    {
        // We are performing a full update.
        // However, we may be getting an update about something we already received during this full update!

        if ( _linkListReq == 0 )
        {
            // We already got the links.

            LOG ( L_DEBUG, "Received an asynchronous update (SeqNum: " << seqNum
                  << ") during a full update that already returned the list of links; "
                  "Appending " << routeResults.links.size() << " links to pending links" );

            for ( size_t i = 0; i < routeResults.links.size(); ++i )
            {
                _pendingLinks.append ( routeResults.links.at ( i ) );
            }
        }

        if ( _addrListReq == 0 )
        {
            // We already got the addresses.

            LOG ( L_DEBUG, "Received an asynchronous update (SeqNum: " << seqNum
                  << ") during a full update that already returned the list of addresses; "
                  "Appending " << routeResults.addresses.size() << " addresses to pending addresses" );

            for ( size_t i = 0; i < routeResults.addresses.size(); ++i )
            {
                _pendingAddresses.append ( routeResults.addresses.at ( i ) );
            }
        }

        // Since we're still performing a full update, routes shouldn't be done yet
        assert ( _routeListReq != 0 );

        LOG ( L_DEBUG, "Received an asynchronous update (SeqNum: " << seqNum
              << ") during a full update that hasn't returned anything yet; Ignoring" );

        return;
    }

    if ( routeResults.links.size() > 0 )
    {
        bool fullUpdateRequested = false;
        HashSet<int> removeIfaces;
        HashMap<int, NetManagerTypes::Interface> updateData;

        LOG ( L_DEBUG, "Received " << routeResults.links.size() << " link update(s)" );

        for ( size_t i = 0; i < routeResults.links.size(); ++i )
        {
            const NetlinkTypes::Link & link = routeResults.links.at ( i );

            if ( link.act == PosixNetMgrTypes::ActionRemove )
            {
                removeIfaces.insert ( link.id );
                updateData.remove ( link.id );
                continue;
            }

            if ( link.act != PosixNetMgrTypes::ActionAdd )
                continue;

            if ( link.isActive() && !isIfaceActive ( link.id ) )
            {
                // NETLINK-HACK: It's possible that we lost addresses/routes on an interface when it went down.
                // Whenever an interface comes back up, we have to refresh the list of addresses and routes.

                // We have a link that just became active. Let's request a full address and route update!

                if ( !fullUpdateRequested )
                {
                    fullUpdateRequested = true;

                    _addrListReq = _routeMon.getAddresses();
                    _routeListReq = _routeMon.getRoutes();
                }

                LOG ( L_DEBUG, "Link " << link.id << " (" << link.name
                      << ") becomes active. Requesting full list of addresses (SeqNum: " << _addrListReq
                      << ") and routes (SeqNum: " << _routeListReq << ")" );
            }

            removeIfaces.remove ( link.id );
            updateData.insert ( link.id, link );
        }

        // NETLINK-HACK: When interface goes down/inactive, it may lose some routes (and maybe addresses?)
        // So, basically, whenever the state of an interface changes, we clear routes and addresses
        // related to that interface.

        updateIfaces ( updateData, removeIfaces );
    }

    if ( routeResults.addresses.size() > 0 )
    {
        LOG ( L_DEBUG, "Received " << routeResults.addresses.size() << " address update(s)" );

        HashSet<NetManagerTypes::Address> add;
        HashSet<NetManagerTypes::Address> remove;

        for ( size_t i = 0; i < routeResults.addresses.size(); ++i )
        {
            const NetlinkTypes::Address & addr = routeResults.addresses.at ( i );

            if ( addr.act == PosixNetMgrTypes::ActionAdd )
            {
                add.insert ( addr );
                remove.remove ( addr );
            }
            else
            {
                remove.insert ( addr );
                add.remove ( addr );
            }
        }

        if ( !remove.isEmpty() )
        {
            // NETLINK-HACK: On NAD and CVG, when rmnet's address is removed, some routes go away too,
            // but we don't see Netlink messages for that. So when we lose addresses we refresh the routes...
            // (unless a refresh is already in progress)

            if ( _routeListReq == 0 )
            {
                _routeListReq = _routeMon.getRoutes();

                LOG ( L_DEBUG, "An address is being removed; Requesting a full route update; SeqNum: "
                      << _routeListReq );
            }
        }

        modifyAddresses ( add, remove );
    }

    if ( routeResults.routes.size() > 0 )
    {
        LOG ( L_DEBUG, "Received " << routeResults.routes.size() << " route update(s)" );

        HashSet<NetManagerTypes::Route> add;
        HashSet<NetManagerTypes::Route> remove;

        for ( size_t i = 0; i < routeResults.routes.size(); ++i )
        {
            const NetlinkTypes::Route & route = routeResults.routes.at ( i );

            // On Linux (and Android) we want to ignore routes from 'LOCAL' routing table.
            if ( route.table == 255 )
            {
                LOG ( L_DEBUG4, "Ignoring a route info for " << route.dst << "/" << route.dstPrefixLen
                      << " address from the LOCAL routing table" );
                continue;
            }

            if ( route.act == PosixNetMgrTypes::ActionAdd )
            {
                LOG ( L_DEBUG3, "Adding a route info for " << route.dst << "/" << route.dstPrefixLen );

                if ( ( route.ifaceIdIn != 0 && !isIfaceActive ( route.ifaceIdIn ) )
                     || ( route.ifaceIdOut != 0 && !isIfaceActive ( route.ifaceIdOut ) ) )
                {
                    // NETLINK-HACK: Sometimes (on Android?) we don't see interfaces coming back
                    // even though they are up again. But we will see new routes on them.
                    // So if that happens, we simply refresh the list of links (and addresses and routes)
                    // to get the correct state.

                    // We shouldn't be in the middle of a 'full update' if we get here
                    // (there is an 'if' above that takes over if _fullUpdate is set).

                    LOG ( L_WARN, "Received a new route related to an interface that we thought was inactive; "
                          "Route's 'in' iface: " << route.ifaceIdIn << "; 'out' iface: " << route.ifaceIdOut
                          << "; Refreshing the list of interfaces; SeqNum: " << _linkListReq );

                    startStateRefresh();

                    // No point in doing anything else, since we just started a full update!
                    return;
                }

                add.insert ( route );
                remove.remove ( route );
            }
            else
            {
                LOG ( L_DEBUG3, "Removing a route info for " << route.dst << "/" << route.dstPrefixLen );

                if ( getRoutes().contains ( route ) && _routeListReq == 0 )
                {
                    // NETLINK-HACK: Just in case, when we lose routes we refresh them
                    // (unless a refresh is already in progress)

                    _routeListReq = _routeMon.getRoutes();

                    LOG ( L_DEBUG, "A route that was previously active is being removed; "
                          "Requesting a full route update; SeqNum: " << _routeListReq );

                    // For now we leave the routes unchanged.
                    return;
                }

                remove.insert ( route );
                add.remove ( route );
            }
        }

        modifyRoutes ( add, remove );
    }
}
