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

#include "NetMgrCtrlTest.hpp"

#include "netmgr/NetManager.hpp"

#include <cstdio>

using namespace Pravala;

ERRCODE NetMgrCtrlTest::run()
{
    List<int> ifaceList;
    HashMap<int, NetManagerTypes::InterfaceObject * > ifaces = NetManager::get().getIfaces();

    for ( HashMap<int, NetManagerTypes::InterfaceObject * >::Iterator itr ( ifaces ); itr.isValid(); itr.next() )
    {
        NetManagerTypes::InterfaceObject * iface = itr.value();

        if ( !iface )
            continue;

        const NetManagerTypes::Interface & desc = iface->getData();

        printf ( "Iface: %s, MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, Index: %d, PTP: %d, Running: %d, Up: %d\n",
                 desc.name.c_str(),
                 desc.hwAddr[ 0 ],
                 desc.hwAddr[ 1 ],
                 desc.hwAddr[ 2 ],
                 desc.hwAddr[ 3 ],
                 desc.hwAddr[ 4 ],
                 desc.hwAddr[ 5 ],
                 desc.id,
                 desc.isPtp(),
                 desc.isRunning(),
                 desc.isUp() );

        for ( HashSet<NetManagerTypes::Address >::Iterator it ( iface->getAddresses() ); it.isValid(); it.next() )
        {
            const NetManagerTypes::Address & addr = it.value();

            printf ( "  Addr; Index: %d, Address: %s, Broadcast: %s\n",
                     addr.ifaceId,
                     addr.localAddress.toString().c_str(),
                     addr.broadcastAddress.toString().c_str()
            );
        }
    }

    for ( HashSet<NetManagerTypes::Route>::Iterator it ( NetManager::get().getRoutes() );
          it.isValid(); it.next() )
    {
        const NetManagerTypes::Route & rt = it.value();

        printf ( "Route; OutIfIdx: %d, Src: %s/%d, ",
                 rt.ifaceIdOut,
                 rt.src.toString().c_str(),
                 rt.srcPrefixLen );

        printf ( "Dest: %s/%d, ",
                 rt.dst.toString().c_str(),
                 rt.dstPrefixLen );

        printf ( "Gw: %s, Metric: %d, Table: %d\n",
                 rt.gw.toString().c_str(),
                 rt.metric,
                 rt.table );
    }

    IpAddress testRoute ( "123.235.176.14" );
    uint8_t testMask = 32;
    IpAddress testGw ( "192.168.81.1" );

    fprintf ( stdout, "Adding route to %s/%d via %s\n", testRoute.toString().c_str(),
              testMask,
              testGw.toString().c_str() );

    ERRCODE eCode = NetManager::get().addRoute ( testRoute, testMask, testGw );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Could not add system routes: %s\n", eCode.toString() );
        return eCode;
    }

    return Error::Success;
}
