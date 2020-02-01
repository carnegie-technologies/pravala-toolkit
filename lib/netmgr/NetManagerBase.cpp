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

#include "NetManagerBase.hpp"

using namespace Pravala;

void NetManagerBase::removeIfaceRouteNoCb ( const NetManagerTypes::Route & route )
{
    NetManagerTypes::InterfaceObject * iface = 0;

    if ( route.ifaceIdOut != 0
         && ( iface = _ifaces.value ( route.ifaceIdOut ) ) != 0
         && iface->_routes.remove ( route ) > 0 )
    {
        LOG ( L_DEBUG2, "Removed route " << route.toString() << " from its OUT interface with ID "
              << route.ifaceIdOut );
    }

    if ( route.ifaceIdIn != 0
         && ( iface = _ifaces.value ( route.ifaceIdIn ) ) != 0
         && iface->_routes.remove ( route ) > 0 )
    {
        LOG ( L_DEBUG2, "Removed route " << route.toString() << " from its IN interface with ID "
              << route.ifaceIdIn );
    }
}

void NetManagerBase::removeHelperRouteNoCb ( const NetManagerTypes::Route & route )
{
    LOG ( L_DEBUG3, "Checking if route " << route.toString() << " should be removed from helper sets" );

    if ( route.isHostRoute() && _hostRoutes[ route.dst ].remove ( route ) > 0 )
    {
        if ( _hostRoutes[ route.dst ].isEmpty() )
            _hostRoutes.remove ( route.dst );

        LOG ( L_DEBUG2, "Removed route " << route.toString() << " from host routes" );
    }

    if ( route.ifaceIdOut != 0 && route.isDefaultRoute() && _defaultRoutes[ route.ifaceIdOut ].remove ( route ) > 0 )
    {
        if ( _defaultRoutes[ route.ifaceIdOut ].isEmpty() )
            _defaultRoutes.remove ( route.ifaceIdOut );

        LOG ( L_DEBUG2, "Removed route " << route.toString() << " from default routes" );
    }
}

bool NetManagerBase::activateRouteNoCb ( const NetManagerTypes::Route & route )
{
    // By default routes are considered 'active'.
    bool routeActive = true;

    // Let's add the route to interfaces whether they're active or not...
    // We run this twice - the first one will have 'ifaceId' set to 'ifaceIdIn', the second - to ifaceIdOut.

    for ( int i = 0, ifaceId = route.ifaceIdIn; i < 2; ++i, ifaceId = route.ifaceIdOut )
    {
        if ( ifaceId == 0 )
        {
            // This ID is not set.
            continue;
        }

        // This route has interface ID set.

        NetManagerTypes::InterfaceObject * const iface = _ifaces.value ( ifaceId );

        if ( !iface )
        {
            // No interface found (despite route having the ID set), this route is not active.
            routeActive = false;
            continue;
        }

        // We have an interface - add the route to its list:
        iface->_routes.insert ( route );

        if ( !iface->isActive() )
        {
            // If the interface is inactive, the route is inactive as well.
            routeActive = false;
        }

        LOG ( L_DEBUG2, "Added route " << route.toString()
              << " to its " << ( ( i == 0 ) ? "IN" : "OUT" )
              << " interface with ID " << ifaceId
              << "; Iface Active: " << iface->isActive() << "; Route Active: " << routeActive );
    }

    if ( !routeActive )
    {
        // This route is NOT active - nothing else to do, return 'false'.
        return false;
    }

    // This route IS active - add it to sets of active routes (even if they already contained it, doesn't hurt us)
    // and return 'true'.

    _routes.insert ( route );

    LOG ( L_DEBUG2, "Added route " << route.toString() << " to active routes" );

    if ( route.isHostRoute() )
    {
        _hostRoutes[ route.dst ].insert ( route );

        LOG ( L_DEBUG2, "Added route " << route.toString() << " to host routes" );
    }

    if ( route.ifaceIdOut != 0 && route.isDefaultRoute() )
    {
        _defaultRoutes[ route.ifaceIdOut ].insert ( route );

        LOG ( L_DEBUG2, "Added route " << route.toString() << " to default routes" );
    }

    return true;
}

void NetManagerBase::deactivateRoutesNoCb ( HashSet<NetManagerTypes::Route> & routes, bool alsoIfaceRemove )
{
    for ( HashSet<NetManagerTypes::Route>::MutableIterator it ( routes ); it.isValid(); )
    {
        const NetManagerTypes::Route & route = it.value();

        if ( alsoIfaceRemove )
        {
            // If we want to completely remove the route,
            // let's update the interface (doesn't matter if it's active or not):

            removeIfaceRouteNoCb ( route );
        }

        if ( _routes.remove ( route ) == 0 )
        {
            // It already is inactive: Let's remove it from the set, since there is no callback required for it.
            it.remove();
        }
        else
        {
            LOG ( L_DEBUG2, "Removed route " << route.toString() << " from active routes" );

            // It was active!
            removeHelperRouteNoCb ( route );

            // We want to keep it in the set - a callback will be required.
            it.next();
        }
    }
}

void NetManagerBase::setRoutes ( HashSet<NetManagerTypes::Route> & routes )
{
    // Store existing, active, routes:
    HashSet<NetManagerTypes::Route> prevActive ( _routes );

    LOG ( L_DEBUG2, "Clearing active routes, helper route sets and interface routes" );

    _routes.clear();
    _hostRoutes.clear();
    _defaultRoutes.clear();

    // Let's clear all the routes in all the interfaces:
    for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        it.value()->_routes.clear();
    }

    for ( HashSet<NetManagerTypes::Route>::MutableIterator it ( routes ); it.isValid(); )
    {
        const NetManagerTypes::Route & route = it.value();

        // The order is important. We always want to try to 'activate' it first, and if it turns out that
        // the route is active now - we try to remove it from the routes that were previously active.
        if ( activateRouteNoCb ( route ) && prevActive.remove ( route ) == 0 )
        {
            // This route is active now but was NOT active before - we want to keep it in 'routes' for 'added' callback!
            it.next();
        }
        else
        {
            // Otherwise it is not active now, or it is active now AND it was active before - in both cases
            // we don't need 'added' callback - we should remove it from 'routes':
            it.remove();
        }
    }

    // Here 'routes' will contain all the routes that were inactive (or missing) before but are active now,
    // and prevActive will contain only those routes that were active before but are not active anymore:

    doNotify ( routes, prevActive );
}

void NetManagerBase::modifyRoutes ( HashSet<NetManagerTypes::Route> & add, HashSet<NetManagerTypes::Route> & remove )
{
    for ( HashSet<NetManagerTypes::Route>::MutableIterator it ( add ); it.isValid(); )
    {
        const NetManagerTypes::Route & route = it.value();

        if ( _routes.contains ( route ) || remove.contains ( route ) )
        {
            // It already is active, or we are about to remove it anyway.
            // No need to change anything and no need to include it in 'added' callback:

            it.remove();
            continue;
        }

        // This route was not active before AND we don't want to remove it.
        // Which means we should add and activate it (if possible).

        if ( activateRouteNoCb ( route ) )
        {
            // This route is now active - should stay in the set for 'added' callback:
            it.next();
        }
        else
        {
            // This route is inactive, no need to include in the callback.
            it.remove();
        }
    }

    if ( !remove.isEmpty() )
    {
        // We want to remove the routes from interfaces too - 2nd argument is 'true':
        deactivateRoutesNoCb ( remove, true );
    }

    doNotify ( add, remove );
}

bool NetManagerBase::activateAddressNoCb ( const NetManagerTypes::Address & addr )
{
    NetManagerTypes::InterfaceObject * const iface = ( addr.ifaceId != 0 ) ? _ifaces.value ( addr.ifaceId ) : 0;

    if ( !iface )
    {
        LOG ( L_WARN, "Received an address " << addr.localAddress << " for interface ID " << addr.ifaceId
              << " but this interface is missing; Ignoring" );

        // Not active:
        return false;
    }

    // Doesn't matter, active or not:

    iface->_addrs.insert ( addr );

    if ( iface->isActive() )
    {
        _addresses.insert ( addr );
        return true;
    }

    return false;
}

void NetManagerBase::deactivateAddressesNoCb ( HashSet<NetManagerTypes::Address> & addresses, bool alsoIfaceRemove )
{
    for ( HashSet<NetManagerTypes::Address>::MutableIterator it ( addresses ); it.isValid(); )
    {
        const NetManagerTypes::Address & addr = it.value();
        NetManagerTypes::InterfaceObject * const iface = ( addr.ifaceId != 0 ) ? _ifaces.value ( addr.ifaceId ) : 0;

        if ( !iface )
        {
            LOG ( L_WARN, "Removing an address " << addr.localAddress << " from interface ID " << addr.ifaceId
                  << " but this interface is missing; Ignoring" );

            // No need to keep for the callback:
            it.remove();
            continue;
        }

        if ( alsoIfaceRemove )
        {
            // Active or not, remove it:

            iface->_addrs.remove ( addr );
        }

        if ( _addresses.remove ( addr ) == 0 )
        {
            // Not removed - it wasn't active and no need to keep for the callback:
            it.remove();
        }
        else
        {
            // It was active and got removed - let's include it in the set for 'removed' callback:
            it.next();
        }
    }
}

void NetManagerBase::setAddresses ( HashSet<NetManagerTypes::Address> & addresses )
{
    HashSet<NetManagerTypes::Address> prevActive ( _addresses );

    // Let's clear everything:

    _addresses.clear();

    for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
    {
        NetManagerTypes::InterfaceObject * const iface = it.value();

        if ( iface != 0 )
        {
            iface->_addrs.clear();
        }
    }

    for ( HashSet<NetManagerTypes::Address>::MutableIterator it ( addresses ); it.isValid(); )
    {
        const NetManagerTypes::Address & addr = it.value();

        // The order is important. We always want to try to activate the address,
        // but we remove it from 'previously active' only if it is active now.
        if ( activateAddressNoCb ( addr ) && prevActive.remove ( addr ) == 0 )
        {
            // This address IS active and removing from prevActive failed - this means that this is an active address
            // that was NOT active before - we want to keep it in the set for 'added' callbacK:
            it.next();
        }
        else
        {
            // Otherwise it is NOT active now, or it is active now but it was also active before - no need
            // to include it in 'added' callback:
            it.remove();
        }
    }

    // Here 'addresses' will include all addresses that are now active but were not active before.
    // prevActive includes those that were active before, but are not active now.

    doNotify ( addresses, prevActive );
}

void NetManagerBase::modifyAddresses (
        HashSet<NetManagerTypes::Address> & add,
        HashSet<NetManagerTypes::Address> & remove )
{
    for ( HashSet<NetManagerTypes::Address>::MutableIterator it ( add ); it.isValid(); )
    {
        const NetManagerTypes::Address & addr = it.value();

        if ( _addresses.contains ( addr ) || remove.contains ( addr ) )
        {
            // It already is active, or we are about to remove it anyway.
            // No need to change anything and no need to include it in 'added' callback:

            it.remove();
            continue;
        }

        // This address was not active before AND we don't want to remove it.
        // Which means we should add and activate it (if possible).

        if ( activateAddressNoCb ( addr ) )
        {
            // This address is now active - should stay in the set for 'added' callback:
            it.next();
        }
        else
        {
            // This address is inactive, no need to include in the callback.
            it.remove();
        }
    }

    if ( !remove.isEmpty() )
    {
        // We want to remove the addresses from interfaces too - 2nd argument is 'true':
        deactivateAddressesNoCb ( remove, true );
    }

    doNotify ( add, remove );
}

void NetManagerBase::removeIface ( const int ifaceId )
{
    NetManagerTypes::InterfaceObject * iface = ( ifaceId != 0 ) ? _ifaces.value ( ifaceId ) : 0;

    if ( !iface )
    {
        LOG ( L_WARN, "Could not remove non-existing interface with ID " << ifaceId );
        return;
    }

    // First, let's notify about routes and addresses going away with the interface (before removing the interface).

    // We need copies of those sets, since the 'deactivate' method will remove them
    // from sets in interfaces - and we are passing one of those sets as an argument.

    HashSet<NetManagerTypes::Route> routes ( iface->_routes );

    if ( !routes.isEmpty() )
    {
        deactivateRoutesNoCb ( routes, true );

        // deactivateRoutesNoCb modified 'routes' set,
        // leaving only the routes that were active and are not active anymore:
        doNotify ( HashSet<NetManagerTypes::Route>(), routes );
    }

    HashSet<NetManagerTypes::Address> addrs ( iface->_addrs );

    if ( !addrs.isEmpty() )
    {
        deactivateAddressesNoCb ( addrs, true );

        // deactivateAddressesNoCb modified 'addrs' set,
        // leaving only the addresses that were active and are not active anymore:
        doNotify ( HashSet<NetManagerTypes::Address>(), addrs );
    }

    // Now we can actually remove the interface:

    _ifaces.remove ( ifaceId );

    delete iface;
    iface = 0;

    // And notify about it:

    if ( !_activeMonitors.iface.isEmpty() )
    {
        HashSet<int> removed;

        removed.insert ( ifaceId );

        doNotify ( HashSet<int>(), HashSet<int>(), removed );
    }
}

void NetManagerBase::updateIfaces ( HashMap<int, NetManagerTypes::Interface> & updateData, HashSet<int> & removeIfaces )
{
    intUpdateIfaces ( updateData, removeIfaces, 0, 0 );
}

void NetManagerBase::setIfaces (
        HashMap<int, NetManagerTypes::Interface> & ifaces,
        HashSet<NetManagerTypes::Address> & addresses,
        HashSet<NetManagerTypes::Route> & routes )
{
    HashSet<int> removeIfaces;

    for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
    {
        if ( !ifaces.contains ( it.key() ) )
        {
            removeIfaces.insert ( it.key() );
        }
    }

    intUpdateIfaces ( ifaces, removeIfaces, &addresses, &routes );
}

void NetManagerBase::intDeactivateIfaceRoutes (
        HashMap< int, NetManagerTypes::Interface > & updateData,
        HashSet< int > & removeIfaces,
        const HashSet< NetManagerTypes::Route > * removeRoutesExcept )
{
    // Routes that were removed or deactivated - for the 'routes removed' callback:
    HashSet<NetManagerTypes::Route> cbRoutesRemoved;

    // We need to remove routes on all interfaces that are being removed or are becoming non active.

    // First - let's check all interfaces that are being removed:
    for ( HashSet<int>::MutableIterator it ( removeIfaces ); it.isValid(); )
    {
        const int ifaceId = it.value();
        NetManagerTypes::InterfaceObject * const iface = _ifaces.value ( ifaceId );

        if ( !iface )
        {
            LOG ( L_WARN, "Could not remove an interface with ID " << ifaceId << " - it does not exist" );

            // We remove that element from the set.
            it.remove();
            continue;
        }

        // In case we are getting the same interface as removed and updated, 'remove' takes precedence:
        updateData.remove ( ifaceId );

        // We need a copy, since iface->_routes will be modified inside:
        HashSet<NetManagerTypes::Route> ifRoutes ( iface->_routes );

        // We want to remove those routes too, not just de-active them:
        deactivateRoutesNoCb ( ifRoutes, true );

        // And add them to the callback set:
        cbRoutesRemoved.insertAll ( ifRoutes );

        it.next();
    }

    // Now let's check which interfaces are becoming inactive:
    for ( HashMap<int, NetManagerTypes::Interface>::Iterator it ( updateData ); it.isValid(); it.next() )
    {
        const int ifaceId = it.key();
        NetManagerTypes::InterfaceObject * iface = 0;

        // We should have removed all 'removed' IDs from updateData set:
        assert ( !removeIfaces.contains ( ifaceId ) );

        if ( !it.value().isActive() && ( iface = _ifaces.value ( ifaceId ) ) != 0 && iface->isActive() )
        {
            // This interface was active but is about to be deactivated.

            // We need a copy, because we don't want to remove those routes from the original inside interface
            // (they are deactivated, not removed):
            HashSet<NetManagerTypes::Route> ifRoutes ( iface->_routes );

            // But we are not actually removing them - pass 'false':
            deactivateRoutesNoCb ( ifRoutes, false );

            // And add them to the callback set:
            cbRoutesRemoved.insertAll ( ifRoutes );
        }
    }

    // Now let's remove all the routes that are not in removeRoutesExcept set (if it's used):
    if ( removeRoutesExcept != 0 )
    {
        // We need to remove all the routes that are not in removeRoutesExcept set.
        // The routes to remove may be active or not. So let's inspect all routes of all the interfaces.

        for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
        {
            const NetManagerTypes::InterfaceObject * const iface = it.value();

            assert ( iface != 0 );

            for ( HashSet<NetManagerTypes::Route>::Iterator jt ( iface->_routes ); jt.isValid(); jt.next() )
            {
                // jt is a 'const iterator', so it actually holds a implicitly shared copy of _routes,
                // so it's safe to remove it from interfaces.

                const NetManagerTypes::Route & route = jt.value();

                if ( removeRoutesExcept->contains ( route ) )
                {
                    // We want to keep this route.
                    continue;
                }

                // New set of routes doesn't include that route. Let's get rid of it from its interfaces:
                removeIfaceRouteNoCb ( route );

                // If it's active, we also need to remove it from _routes and helper routes:

                if ( _routes.remove ( route ) != 0 )
                {
                    LOG ( L_DEBUG2, "Removed route " << route.toString() << " from active routes" );

                    // It was, in fact, active!
                    // Remove from helper routes:
                    removeHelperRouteNoCb ( route );

                    // And add to callback set:
                    cbRoutesRemoved.insert ( route );
                }
            }
        }
    }

    // We are finished removing/deactivating routes. Let's call the 'routes removed' callback:
    if ( !cbRoutesRemoved.isEmpty() )
    {
        doNotify ( HashSet<NetManagerTypes::Route>(), cbRoutesRemoved );
    }
}

void NetManagerBase::intDeactivateIfaceAddresses (
        HashMap< int, NetManagerTypes::Interface > & updateData,
        HashSet< int > & removeIfaces,
        const HashSet< NetManagerTypes::Address > * removeAddressesExcept )
{
    // Addresses that were removed or deactivated - for the 'addresses removed' callback:
    HashSet<NetManagerTypes::Address> cbAddressesRemoved;

    // We need to remove addresses on all interfaces that are being removed or are becoming non active.

    // First - let's check all interfaces that are being removed:
    for ( HashSet<int>::MutableIterator it ( removeIfaces ); it.isValid(); )
    {
        const int ifaceId = it.value();
        NetManagerTypes::InterfaceObject * const iface = _ifaces.value ( ifaceId );

        if ( !iface )
        {
            LOG ( L_WARN, "Could not remove an interface with ID " << ifaceId << " - it does not exist" );

            // We remove that element from the set.
            it.remove();
            continue;
        }

        // In case we are getting the same interface as removed and updated, 'remove' takes precedence:
        updateData.remove ( ifaceId );

        // We need a copy, since iface->_addrs will be modified inside:
        HashSet<NetManagerTypes::Address> ifAddresses ( iface->_addrs );

        // We want to remove those addresses too, not just de-active them:
        deactivateAddressesNoCb ( ifAddresses, true );

        // And add them to the callback set:
        cbAddressesRemoved.insertAll ( ifAddresses );

        it.next();
    }

    // Now let's check which interfaces are becoming inactive:
    for ( HashMap<int, NetManagerTypes::Interface>::Iterator it ( updateData ); it.isValid(); it.next() )
    {
        const int ifaceId = it.key();
        NetManagerTypes::InterfaceObject * iface = 0;

        // We should have removed all 'removed' IDs from updateData set:
        assert ( !removeIfaces.contains ( ifaceId ) );

        if ( !it.value().isActive() && ( iface = _ifaces.value ( ifaceId ) ) != 0 && iface->isActive() )
        {
            // This interface was active but is about to be deactivated.

            // We need a copy, because we don't want to remove those addresses from the original inside interface
            // (they are deactivated, not removed):
            HashSet<NetManagerTypes::Address> ifAddresses ( iface->_addrs );

            // But we are not actually removing them: pass 'false':
            deactivateAddressesNoCb ( ifAddresses, false );

            // And add them to the callback set:
            cbAddressesRemoved.insertAll ( ifAddresses );
        }
    }

    // Now let's remove all the addresses that are not in removeAddressesExcept set (if it's used):
    if ( removeAddressesExcept != 0 )
    {
        // We need to remove all the addresses that are not in removeAddressesExcept set.
        // The addresses to remove may be active or not. So let's inspect all addresses of all the interfaces.

        for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
        {
            NetManagerTypes::InterfaceObject * const iface = it.value();

            assert ( iface != 0 );

            for ( HashSet<NetManagerTypes::Address>::MutableIterator jt ( iface->_addrs ); jt.isValid(); )
            {
                const NetManagerTypes::Address & addr = jt.value();

                if ( removeAddressesExcept->contains ( addr ) )
                {
                    // We want to keep this address.
                    jt.next();
                    continue;
                }

                // We want to get rid of this address.

                if ( _addresses.remove ( addr ) != 0 )
                {
                    // This address was active - add to callback set:
                    cbAddressesRemoved.insert ( addr );
                }

                // And finally remove from this interface's set:
                jt.remove();
            }
        }
    }

    // We are finished removing/deactivating addresses. Let's call the 'addresses removed' callback:
    if ( !cbAddressesRemoved.isEmpty() )
    {
        doNotify ( HashSet<NetManagerTypes::Address>(), cbAddressesRemoved );
    }
}

void NetManagerBase::intUpdateIfaces (
        HashMap<int, NetManagerTypes::Interface> & updateData,
        HashSet<int> & removeIfaces,
        HashSet<NetManagerTypes::Address> * setAddressesTo,
        HashSet<NetManagerTypes::Route> * setRoutesTo )
{
    // First, let's deal with routes that are going away.
    // We have to do this twice, first to remove routes, then to remove addresses.
    // This is because when we call 'routes' callback, all the addresses should still be there.

    intDeactivateIfaceRoutes ( updateData, removeIfaces, setRoutesTo );

    // Now we want to do the exact same thing for addresses:

    intDeactivateIfaceAddresses ( updateData, removeIfaces, setAddressesTo );

    // Now we can modify the interfaces.
    // Note that 'removeIfaces' doesn't include any nonexistent interfaces
    // (those were filtered out by intDeactivateIfaceRoutes / intDeactivateIfaceAddresses.
    // Also, 'updateIfaces' doesn't include any interfaces that should be removed
    // (those were also filtered out in the same methods).
    //
    // So at this point 'removeIfaces' only includes existing interfaces that should be removed,
    // and 'updateIfaces' only interfaces that should be updated/created.

    // Let's remove all interfaces that should be removed
    // (removeIface is what we will use in the callback, no need to create a new set):

    for ( HashSet<int>::Iterator it ( removeIfaces ); it.isValid(); it.next() )
    {
        NetManagerTypes::InterfaceObject * iface = 0;

        bool ret = _ifaces.findAndRemove ( it.value(), iface );

        ( void ) ret;

        assert ( ret );
        assert ( iface != 0 );

        delete iface;
    }

    HashSet<int> ifacesActivated;
    HashSet<int> ifacesDeactivated;

    // And now let's add and update interfaces:
    for ( HashMap<int, NetManagerTypes::Interface>::Iterator it ( updateData ); it.isValid(); it.next() )
    {
        const int ifaceId = it.key();

        NetManagerTypes::InterfaceObject * iface = _ifaces.value ( ifaceId );

        if ( !iface )
        {
            // It's a new interface:
            _ifaces.insert ( it.key(), new NetManagerTypes::InterfaceObject ( it.value() ) );

            if ( it.value().isActive() )
            {
                ifacesActivated.insert ( ifaceId );
            }
            else
            {
                ifacesDeactivated.insert ( ifaceId );
            }
        }
        else
        {
            // This interface already exists. Let's update it:

            const bool wasActive = iface->isActive();

            iface->updateData ( it.value() );

            if ( iface->isActive() != wasActive )
            {
                // The 'active' state changed.

                if ( iface->isActive() )
                {
                    ifacesActivated.insert ( ifaceId );
                }
                else
                {
                    ifacesDeactivated.insert ( ifaceId );
                }
            }
        }
    }

    doNotify ( ifacesActivated, ifacesDeactivated, removeIfaces );

    // And now we need to update new addresses and routes (if needed).
    // We can use 'modify' functions, since all removing has already been done.

    if ( setAddressesTo != 0 )
    {
        HashSet<NetManagerTypes::Address> tmp;

        modifyAddresses ( *setAddressesTo, tmp );
    }

    if ( setRoutesTo != 0 )
    {
        HashSet<NetManagerTypes::Route> tmp;

        modifyRoutes ( *setRoutesTo, tmp );
    }
}
