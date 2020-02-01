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

#include "NetManagerImpl.hpp"

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

NetManagerImpl::NetManagerImpl(): _routeMon ( *this )
{
    refreshState();
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
    return _routeCtrl.removeIfaceAddress ( ifaceId, address );
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
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int /*metric*/, int /*tableId*/ )
{
    return _routeCtrl.addRoute ( dst, mask, gw, ifaceId );
}

ERRCODE NetManagerImpl::removeRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int /*metric*/, int /*tableId*/ )
{
    return _routeCtrl.removeRoute ( dst, mask, gw, ifaceId );
}

void NetManagerImpl::setAfRouteLinks (
        const List<AfRouteTypes::Link > & links,
        const List<AfRouteTypes::Address > & addrs,
        const List<AfRouteTypes::Route > & routes )
{
    // This map will contain ALL interfaces that are present and active (up & running); Key is the iface ID;
    HashMap<int, NetManagerTypes::Interface> ifaces;

    for ( size_t i = 0; i < links.size(); ++i )
    {
        const AfRouteTypes::Link & link = links.at ( i );

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
        const AfRouteTypes::Address & addr = addrs.at ( i );

        if ( !ifaces.contains ( addr.ifaceId ) )
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
        const AfRouteTypes::Route & route = routes.at ( i );

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

ERRCODE NetManagerImpl::getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface )
{
    List<AfRouteTypes::Link> links;
    List<AfRouteTypes::Address> addrs;

    ERRCODE eCode = _routeCtrl.getLinksAndAddresses ( links, addrs );

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
    List<AfRouteTypes::Link> links;
    List<AfRouteTypes::Address> addrs;

    ERRCODE eCode = _routeCtrl.getLinksAndAddresses ( links, addrs );

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
    List<AfRouteTypes::Link> links;
    List<AfRouteTypes::Address> addrs;

    ERRCODE eCode = _routeCtrl.getLinksAndAddresses ( links, addrs );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    for ( size_t i = 0; i < links.size(); ++i )
    {
        const AfRouteTypes::Link & link = links.at ( i );

        if ( link.name == ifaceName )
        {
            rxBytes = link.rxBytes;
            txBytes = link.txBytes;

            return Error::Success;
        }
    }

    return Error::NotFound;
}

ERRCODE NetManagerImpl::refreshState()
{
    List<AfRouteTypes::Link> links;
    List<AfRouteTypes::Address> addrs;

    ERRCODE eCode = _routeCtrl.getLinksAndAddresses ( links, addrs );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error reading list of links using AfRouteControl" );
        return eCode;
    }

    List<AfRouteTypes::Route> routes;

    eCode = _routeCtrl.getRoutes ( routes );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error reading list of routes using AfRouteControl" );
        return eCode;
    }

    setAfRouteLinks ( links, addrs, routes );
    return Error::Success;
}

ERRCODE NetManagerImpl::refreshRoutes()
{
    List<AfRouteTypes::Route> routes;

    ERRCODE eCode = _routeCtrl.getRoutes ( routes );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error reading list of routes using AfRouteControl" );
        return eCode;
    }

    HashSet<NetManagerTypes::Route> routeSet;

    for ( size_t i = 0; i < routes.size(); ++i )
    {
        const AfRouteTypes::Route & route = routes.at ( i );

        if ( route.act == PosixNetMgrTypes::ActionAdd )
        {
            routeSet.insert ( route );
        }
        else
        {
            routeSet.remove ( route );
        }
    }

    setRoutes ( routeSet );

    return Error::Success;
}

void NetManagerImpl::afRouteMonUpdate (
        AfRouteMonitor * monitor,
        List< AfRouteTypes::Link > & links,
        List< AfRouteTypes::Address > & addresses,
        List< AfRouteTypes::Route > & routes )
{
    assert ( monitor == &_routeMon );
    ( void ) monitor;

    LOG ( L_DEBUG2, "Received RouteMonitor update; Link entries: " << links.size()
          << "; Addr entries: " << addresses.size()
          << "; Route entries: " << routes.size() );

    if ( links.size() > 0 )
    {
        HashSet<int> removeIfaces;
        HashMap<int, NetManagerTypes::Interface> updateData;

        LOG ( L_DEBUG, "Received " << links.size() << " link update(s)" );

        for ( size_t i = 0; i < links.size(); ++i )
        {
            const AfRouteTypes::Link & link = links.at ( i );

            if ( link.act == PosixNetMgrTypes::ActionRemove )
            {
                removeIfaces.insert ( link.id );
                updateData.remove ( link.id );
                continue;
            }

            if ( link.act != PosixNetMgrTypes::ActionAdd )
                continue;

            removeIfaces.remove ( link.id );
            updateData.insert ( link.id, link );
        }

        updateIfaces ( updateData, removeIfaces );
    }

    if ( addresses.size() > 0 )
    {
        LOG ( L_DEBUG, "Received " << addresses.size() << " address update(s)" );

        HashSet<NetManagerTypes::Address> add;
        HashSet<NetManagerTypes::Address> remove;

        for ( size_t i = 0; i < addresses.size(); ++i )
        {
            const AfRouteTypes::Address & addr = addresses.at ( i );

            if ( addr.act == PosixNetMgrTypes::ActionAdd )
            {
                if ( !getIfaces().contains ( addr.ifaceId ) )
                {
                    LOG ( L_DEBUG, "Received a new address related to an interface (ID: " << addr.ifaceId
                          << ") that we don't know about. Refreshing the list of links, addresses and routes." );

                    refreshState();

                    // No point in doing anything else.
                    return;
                }

                add.insert ( addr );
                remove.remove ( addr );
            }
            else
            {
                remove.insert ( addr );
                add.remove ( addr );
            }
        }

        modifyAddresses ( add, remove );
    }

    if ( routes.size() > 0 )
    {
        LOG ( L_DEBUG, "Received " << routes.size() << " route update(s)" );

        HashSet<NetManagerTypes::Route> add;
        HashSet<NetManagerTypes::Route> remove;

        for ( size_t i = 0; i < routes.size(); ++i )
        {
            const AfRouteTypes::Route & route = routes.at ( i );

            if ( route.act == PosixNetMgrTypes::ActionAdd )
            {
                if ( route.ifaceIdIn == 0 && route.ifaceIdOut == 0 )
                {
                    LOG ( L_DEBUG, "Received a new route with no interfaces set. Refreshing the list of routes" );

                    if ( IS_OK ( refreshRoutes() ) )
                    {
                        // No point in doing anything else...
                        return;
                    }
                }
                else
                {
                    add.insert ( route );
                    remove.remove ( route );
                }
            }
            else
            {
                remove.insert ( route );
                add.remove ( route );
            }
        }

        modifyRoutes ( add, remove );
    }
}
