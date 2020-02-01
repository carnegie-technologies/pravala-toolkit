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

#include <cstdio>
#include "NetMgrTest.hpp"

using namespace Pravala;

ConfigSwitch NetMgrTest::swOnlyIpv4 ( "ipv4", '4', "IPv4-only mode; Ignore IPv6 routes and addresses" );

void NetMgrTest::start()
{
    NetManager::get().subscribeIfaces ( this, true );
    NetManager::get().subscribeAddresses ( this, true );
    NetManager::get().subscribeRoutes ( this, true );
}

void NetMgrTest::netIfacesChanged (
        const HashSet< int > & activated, const HashSet< int > & deactivated,
        const HashSet< int > & removed )
{
    for ( HashSet<int>::Iterator it ( activated ); it.isValid(); it.next() )
    {
        NetManagerTypes::InterfaceObject * iface = NetManager::get().getIface ( it.value() );

        if ( !iface )
        {
            printf ( "Interface %d activated\n", it.value() );
        }
        else
        {
            const NetManagerTypes::Interface & desc = iface->getData();

            printf ( "Interface %d (%s) activated; MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x; "
                     "Up: %d; Running: %d; PTP: %d, Loopback: %d\n",
                     desc.id,
                     desc.name.c_str(),
                     desc.hwAddr[ 0 ],
                     desc.hwAddr[ 1 ],
                     desc.hwAddr[ 2 ],
                     desc.hwAddr[ 3 ],
                     desc.hwAddr[ 4 ],
                     desc.hwAddr[ 5 ],
                     desc.isUp(),
                     desc.isRunning(),
                     desc.isPtp(),
                     desc.isLoopback() );
        }
    }

    for ( HashSet<int>::Iterator it ( deactivated ); it.isValid(); it.next() )
    {
        NetManagerTypes::InterfaceObject * iface = NetManager::get().getIface ( it.value() );

        if ( !iface )
        {
            printf ( "Interface %d deactivated\n", it.value() );
        }
        else
        {
            const NetManagerTypes::Interface & desc = iface->getData();

            printf ( "Interface %d (%s) deactivated; MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x; "
                     "Up: %d; Running: %d; PTP: %d, Loopback: %d\n",
                     desc.id,
                     desc.name.c_str(),
                     desc.hwAddr[ 0 ],
                     desc.hwAddr[ 1 ],
                     desc.hwAddr[ 2 ],
                     desc.hwAddr[ 3 ],
                     desc.hwAddr[ 4 ],
                     desc.hwAddr[ 5 ],
                     desc.isUp(),
                     desc.isRunning(),
                     desc.isPtp(),
                     desc.isLoopback() );
        }
    }

    for ( HashSet<int>::Iterator it ( removed ); it.isValid(); it.next() )
    {
        NetManagerTypes::InterfaceObject * iface = NetManager::get().getIface ( it.value() );

        if ( !iface )
        {
            printf ( "Interface %d removed\n", it.value() );
        }
        else
        {
            printf ( "Interface %d (%s) removed\n", it.value(), iface->getData().name.c_str() );
        }
    }
}

void NetMgrTest::netIfaceAddressesChanged (
        const HashSet< NetManagerTypes::Address > & added,
        const HashSet< NetManagerTypes::Address > & removed )
{
    for ( HashSet<NetManagerTypes::Address>::Iterator it ( added ); it.isValid(); it.next() )
    {
        const NetManagerTypes::Address & a = it.value();

        if ( swOnlyIpv4.isSet() && ( a.localAddress.isIPv6() || a.broadcastAddress.isIPv6() ) )
            continue;

        NetManagerTypes::InterfaceObject * iface = NetManager::get().getIface ( a.ifaceId );

        if ( !iface )
        {
            printf ( "Interface %d has a new address: %s/%d\n",
                     a.ifaceId,
                     a.localAddress.toString().c_str(),
                     a.prefixLen );
        }
        else
        {
            printf ( "Interface %d (%s) has a new address: %s/%d (bcast: %s)\n",
                     a.ifaceId,
                     iface->getData().name.c_str(),
                     a.localAddress.toString().c_str(),
                     a.prefixLen,
                     a.broadcastAddress.toString().c_str() );
        }
    }

    for ( HashSet<NetManagerTypes::Address>::Iterator it ( removed ); it.isValid(); it.next() )
    {
        const NetManagerTypes::Address & a = it.value();

        if ( swOnlyIpv4.isSet() && ( a.localAddress.isIPv6() || a.broadcastAddress.isIPv6() ) )
            continue;

        NetManagerTypes::InterfaceObject * iface = NetManager::get().getIface ( a.ifaceId );

        if ( !iface )
        {
            printf ( "Interface %d lost address: %s/%d\n",
                     a.ifaceId,
                     a.localAddress.toString().c_str(),
                     a.prefixLen );
        }
        else
        {
            printf ( "Interface %d (%s) lost address: %s/%d (bcast: %s)\n",
                     a.ifaceId,
                     iface->getData().name.c_str(),
                     a.localAddress.toString().c_str(),
                     a.prefixLen,
                     a.broadcastAddress.toString().c_str() );
        }
    }
}

const char * routeType ( const NetManagerTypes::Route & route )
{
    if ( route.isDefaultRoute() )
        return "default ";

    if ( route.isHostRoute() )
        return "host ";

    return "";
}

static String getRouteOverDesc ( const NetManagerTypes::Route & r )
{
    String desc;
    NetManagerTypes::InterfaceObject * iface = 0;

    if ( r.gw.isValid() && !r.gw.isZero() )
    {
        desc = r.gw.toString();
    }

    if ( r.ifaceIdIn != 0 || r.ifaceIdOut != 0 )
    {
        if ( !desc.isEmpty() )
            desc.append ( " " );

        desc.append ( "[ " );

        if ( r.ifaceIdIn != 0 )
        {
            desc.append ( String ( "IN:%1 " ).arg ( r.ifaceIdIn ) );

            if ( ( iface = NetManager::get().getIface ( r.ifaceIdIn ) ) != 0 )
                desc.append ( String ( "(%1) " ).arg ( iface->getData().name ) );
        }

        if ( r.ifaceIdOut != 0 )
        {
            desc.append ( String ( "OUT:%1 " ).arg ( r.ifaceIdOut ) );

            if ( ( iface = NetManager::get().getIface ( r.ifaceIdOut ) ) != 0 )
                desc.append ( String ( "(%1) " ).arg ( iface->getData().name ) );
        }
        desc.append ( "]" );
    }

    return desc;
}

void NetMgrTest::netRoutesChanged (
        const HashSet< NetManagerTypes::Route > & added,
        const HashSet< NetManagerTypes::Route > & removed )
{
    for ( int i = 0; i < 2; ++i )
    {
        for ( HashSet<NetManagerTypes::Route>::Iterator it ( ( i == 0 ) ? added : removed ); it.isValid(); it.next() )
        {
            const NetManagerTypes::Route & r = it.value();

            if ( swOnlyIpv4.isSet() && ( r.dst.isIPv6() || r.src.isIPv6() || r.gw.isIPv6() ) )
                continue;

            const String overDesc = getRouteOverDesc ( r );

            printf ( "%s %sroute to %s/%d over %s (metric: %d)\n",
                     ( i == 0 ) ? "Added" : "Removed",
                     routeType ( r ),
                     r.dst.toString().c_str(),
                     r.dstPrefixLen,
                     overDesc.c_str(),
                     r.metric );
        }
    }
}
