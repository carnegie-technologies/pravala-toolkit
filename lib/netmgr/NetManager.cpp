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

#include "NetManager.hpp"

using namespace Pravala;

TextLog NetManager::_log ( "net_manager" );

NetManager::NetManager(): _scheduledUpdatesTaskAdded ( false )
{
    EventManager::shutdownSubscribe ( this );
}

NetManager::~NetManager()
{
    for ( size_t i = 0; i < _asyncTasks.size(); ++i )
    {
        delete _asyncTasks.at ( i );
    }

    _asyncTasks.clear();

    for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( _ifaces ); it.isValid(); it.next() )
    {
        delete it.value();
    }

    _ifaces.clear();
}

void NetManager::receiveShutdownEvent()
{
    EventManager::shutdownUnsubscribe ( this );

    delete this;
}

NetManager::AsyncTask::~AsyncTask()
{
}

NetManager::RouteMonitor::~RouteMonitor()
{
    NetManager::get().unsubscribeRoutes ( this );
}

void NetManager::subscribeRoutes ( NetManager::RouteMonitor * monitor, bool fullUpdate )
{
    if ( !monitor )
        return;

    if ( !fullUpdate )
    {
        _activeMonitors.route.insert ( monitor );
        return;
    }

    _scheduledMonitors.route.insert ( monitor );
    scheduleFullUpdates();
}

void NetManager::unsubscribeRoutes ( NetManager::RouteMonitor * monitor )
{
    if ( !monitor )
        return;

    _activeMonitors.route.remove ( monitor );
    _scheduledMonitors.route.remove ( monitor );
}

NetManager::AddressMonitor::~AddressMonitor()
{
    NetManager::get().unsubscribeAddresses ( this );
}

void NetManager::subscribeAddresses ( NetManager::AddressMonitor * monitor, bool fullUpdate )
{
    if ( !monitor )
        return;

    if ( !fullUpdate )
    {
        _activeMonitors.address.insert ( monitor );
        return;
    }

    _scheduledMonitors.address.insert ( monitor );
    scheduleFullUpdates();
}

void NetManager::unsubscribeAddresses ( NetManager::AddressMonitor * monitor )
{
    if ( monitor != 0 )
    {
        _activeMonitors.address.remove ( monitor );
        _scheduledMonitors.address.remove ( monitor );
    }
}

NetManager::IfaceMonitor::~IfaceMonitor()
{
    NetManager::get().unsubscribeIfaces ( this );
}

void NetManager::subscribeIfaces ( NetManager::IfaceMonitor * monitor, bool fullUpdate )
{
    if ( !monitor )
        return;

    if ( !fullUpdate )
    {
        _activeMonitors.iface.insert ( monitor );
        return;
    }

    _scheduledMonitors.iface.insert ( monitor );
    scheduleFullUpdates();
}

void NetManager::unsubscribeIfaces ( NetManager::IfaceMonitor * monitor )
{
    if ( monitor != 0 )
    {
        _activeMonitors.iface.remove ( monitor );
        _scheduledMonitors.iface.remove ( monitor );
    }
}

void NetManager::scheduleTask ( NetManager::AsyncTask * task )
{
    if ( task != 0 )
    {
        _asyncTasks.append ( task );
        EventManager::loopEndSubscribe ( this );
    }
}

void NetManager::scheduleFullUpdates()
{
    if ( !_scheduledUpdatesTaskAdded )
    {
        _scheduledUpdatesTaskAdded = true;
        scheduleTask ( new DoScheduledUpdatesTask() );
    }
}

void NetManager::receiveLoopEndEvent()
{
    runTasks();
}

void NetManager::runTasks()
{
    List<NetManager::AsyncTask *> tasks = _asyncTasks;
    _asyncTasks.clear();

    while ( !tasks.isEmpty() )
    {
        AsyncTask * task = tasks.first();
        tasks.removeFirst();

        if ( task != 0 )
        {
            task->runTask ( *this );
            delete task;
        }
    }
}

void NetManager::DoScheduledUpdatesTask::runTask ( NetManager & netManager )
{
    netManager._scheduledUpdatesTaskAdded = false;

    if ( !netManager._scheduledMonitors.iface.isEmpty() )
    {
        HashSet<int> added;
        HashSet<int> inactive;
        HashSet<int> removed;

        for ( HashMap<int, NetManagerTypes::InterfaceObject *>::Iterator it ( netManager._ifaces );
              it.isValid();
              it.next() )
        {
            if ( it.value() != 0 )
            {
                if ( it.value()->isActive() )
                {
                    added.insert ( it.key() );
                }
                else
                {
                    inactive.insert ( it.key() );
                }
            }
        }

        for ( HashSet<NetManager::IfaceMonitor *>::Iterator it ( netManager._scheduledMonitors.iface );
              it.isValid();
              it.next() )
        {
            if ( it.value() != 0 && netManager._scheduledMonitors.iface.contains ( it.value() ) )
            {
                it.value()->netIfacesChanged ( added, inactive, removed );
            }
        }

        netManager._activeMonitors.iface.insertAll ( netManager._scheduledMonitors.iface );
        netManager._scheduledMonitors.iface.clear();
    }

    if ( !netManager._scheduledMonitors.address.isEmpty() )
    {
        const HashSet<NetManagerTypes::Address> added = netManager._addresses;
        const HashSet<NetManagerTypes::Address> removed;

        for ( HashSet<NetManager::AddressMonitor *>::Iterator it ( netManager._scheduledMonitors.address );
              it.isValid();
              it.next() )
        {
            if ( it.value() != 0 && netManager._scheduledMonitors.address.contains ( it.value() ) )
            {
                it.value()->netIfaceAddressesChanged ( added, removed );
            }
        }

        netManager._activeMonitors.address.insertAll ( netManager._scheduledMonitors.address );
        netManager._scheduledMonitors.address.clear();
    }

    if ( !netManager._scheduledMonitors.route.isEmpty() )
    {
        const HashSet<NetManagerTypes::Route> added = netManager._routes;
        const HashSet<NetManagerTypes::Route> removed;

        for ( HashSet<NetManager::RouteMonitor *>::Iterator it ( netManager._scheduledMonitors.route );
              it.isValid();
              it.next() )
        {
            if ( it.value() != 0 && netManager._scheduledMonitors.route.contains ( it.value() ) )
            {
                it.value()->netRoutesChanged ( added, removed );
            }
        }

        netManager._activeMonitors.route.insertAll ( netManager._scheduledMonitors.route );
        netManager._scheduledMonitors.route.clear();
    }
}

NetManager::DoScheduledUpdatesTask::~DoScheduledUpdatesTask()
{
}

void NetManager::doNotify (
        const HashSet<NetManagerTypes::Route> added,
        const HashSet<NetManagerTypes::Route> removed )
{
    // We are passing a const reference. To make things slightly safer we take a copy of each set.
    // Just to avoid accidental changes. As long as nothing is modified, this is very cheap.
    // And if it gets modified, it will be more expensive but will also mean that we dodged a bullet!

    if ( added.isEmpty() && removed.isEmpty() )
        return;

    for ( HashSet<NetManager::RouteMonitor *>::Iterator it ( _activeMonitors.route ); it.isValid(); it.next() )
    {
        if ( it.value() != 0 && _activeMonitors.route.contains ( it.value() ) )
        {
            it.value()->netRoutesChanged ( added, removed );
        }
    }
}

void NetManager::doNotify (
        const HashSet<NetManagerTypes::Address> added,
        const HashSet<NetManagerTypes::Address> removed )
{
    // We are passing a const reference. To make things slightly safer we take a copy of each set.
    // Just to avoid accidental changes. As long as nothing is modified, this is very cheap.
    // And if it gets modified, it will be more expensive but will also mean that we dodged a bullet!

    if ( added.isEmpty() && removed.isEmpty() )
        return;

    for ( HashSet<NetManager::AddressMonitor *>::Iterator it ( _activeMonitors.address ); it.isValid(); it.next() )
    {
        if ( it.value() != 0 && _activeMonitors.address.contains ( it.value() ) )
        {
            it.value()->netIfaceAddressesChanged ( added, removed );
        }
    }
}

void NetManager::doNotify (
        const HashSet< int > added,
        const HashSet< int > deactivated,
        const HashSet< int > removed )
{
    // We are passing a const reference. To make things slightly safer we take a copy of each set.
    // Just to avoid accidental changes. As long as nothing is modified, this is very cheap.
    // And if it gets modified, it will be more expensive but will also mean that we dodged a bullet!

    if ( added.isEmpty() && deactivated.isEmpty() && removed.isEmpty() )
        return;

    for ( HashSet<NetManager::IfaceMonitor *>::Iterator it ( _activeMonitors.iface ); it.isValid(); it.next() )
    {
        if ( it.value() != 0 && _activeMonitors.iface.contains ( it.value() ) )
        {
            it.value()->netIfacesChanged ( added, deactivated, removed );
        }
    }
}

NetManager::RouteOpTask::RouteOpTask (
        NetManager::RouteOpTask::Type type,
        const IpAddress & dst,
        uint8_t mask,
        const IpAddress & gw,
        int ifaceId,
        int metric,
        int tableId ):
    _type ( type ),
    _dst ( dst ),
    _gw ( gw ),
    _ifaceId ( ifaceId ),
    _metric ( metric ),
    _tableId ( tableId ),
    _mask ( mask )
{
}

void NetManager::RouteOpTask::runTask ( NetManager & netManager )
{
    if ( _type == AddRoute )
    {
        netManager.addRoute ( _dst, _mask, _gw, _ifaceId, _metric, _tableId );
    }
    else if ( _type == RemoveRoute )
    {
        netManager.removeRoute ( _dst, _mask, _gw, _ifaceId, _metric, _tableId );
    }
}

void NetManager::addRouteAsync (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    scheduleTask ( new RouteOpTask ( RouteOpTask::AddRoute, dst, mask, gw, ifaceId, metric, tableId ) );
}

void NetManager::removeRouteAsync (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    scheduleTask ( new RouteOpTask ( RouteOpTask::RemoveRoute, dst, mask, gw, ifaceId, metric, tableId ) );
}

NetManager::IfaceAddressOpTask::IfaceAddressOpTask ( Type type, int ifaceId, const IpAddress & address ):
    _type ( type ), _ifaceId ( ifaceId ), _addr ( address )
{
}

void NetManager::IfaceAddressOpTask::runTask ( NetManager & netManager )
{
    if ( _type == AddAddress )
    {
        netManager.addIfaceAddress ( _ifaceId, _addr );
    }
    else if ( _type == RemoveAddress )
    {
        netManager.removeIfaceAddress ( _ifaceId, _addr );
    }
}

void NetManager::addIfaceAddressAsync ( int ifaceId, const IpAddress & address )
{
    scheduleTask ( new IfaceAddressOpTask ( IfaceAddressOpTask::AddAddress, ifaceId, address ) );
}

void NetManager::removeIfaceAddressAsync ( int ifaceId, const IpAddress & address )
{
    scheduleTask ( new IfaceAddressOpTask ( IfaceAddressOpTask::RemoveAddress, ifaceId, address ) );
}

NetManager::IfaceOpTask::IfaceOpTask ( Type type, int ifaceId, int value ):
    _type ( type ), _ifaceId ( ifaceId ), _value ( value )
{
}

void NetManager::IfaceOpTask::runTask ( NetManager & netManager )
{
    if ( _type == SetMtu )
    {
        netManager.setIfaceMtu ( _ifaceId, _value );
    }
    else if ( _type == SetState )
    {
        netManager.setIfaceState ( _ifaceId, _value != 0 );
    }
}

void NetManager::setIfaceMtuAsync ( int ifaceId, int mtu )
{
    scheduleTask ( new IfaceOpTask ( IfaceOpTask::SetMtu, ifaceId, mtu ) );
}

void NetManager::setIfaceStateAsync ( int ifaceId, bool isUp )
{
    scheduleTask ( new IfaceOpTask ( IfaceOpTask::SetState, ifaceId, isUp ? 1 : 0 ) );
}
