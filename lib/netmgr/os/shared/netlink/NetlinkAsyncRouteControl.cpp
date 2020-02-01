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

#include "NetlinkAsyncRouteControl.hpp"
#include "NetlinkMsgCreator.hpp"

using namespace Pravala;

NetlinkAsyncRouteControl::NetlinkAsyncRouteControl ( NetlinkRouteMonitor::Owner & owner ):
    NetlinkRouteMonitor ( owner, 0 ) // We specifically don't want to be receiving any multicast messages
{
}

uint32_t NetlinkAsyncRouteControl::addRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyRoute (
                PosixNetMgrTypes::ActionAdd,              // operation
                NLM_F_CREATE | NLM_F_REPLACE,              // flags
                dst,              // dst address
                mask,              // network mask
                metric,              // metric
                gw,              // gateway
                ifaceId,              // iface id
                RTN_UNICAST,
                ( tableId >= 0 && tableId <= 0xFF )
                ? ( ( uint8_t ) tableId )
                : ( ( uint8_t ) RT_TABLE_MAIN )
            ) );

    return sendMessage ( msg );
}

uint32_t NetlinkAsyncRouteControl::removeRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyRoute (
                PosixNetMgrTypes::ActionRemove,              // operation
                0,              // flags
                dst,              // dst address
                mask,              // network mask
                metric,              // metric
                gw,              // gateway
                ifaceId,              // iface id
                RTN_UNICAST,
                ( tableId >= 0 && tableId <= 0xFF )
                ? ( ( uint8_t ) tableId )
                : ( ( uint8_t ) RT_TABLE_MAIN )
            ) );

    return sendMessage ( msg );
}
