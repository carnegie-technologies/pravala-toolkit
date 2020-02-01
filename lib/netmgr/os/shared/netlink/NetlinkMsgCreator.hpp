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

#include "NetlinkMessage.hpp"
#include "NetlinkTypes.hpp"
#include "basic/IpAddress.hpp"

extern "C"
{
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
}

namespace Pravala
{
class NetlinkMsgCreator
{
    public:
        /// @brief Creates an RTM_GETLINK message
        /// @return RTM_GETLINK message
        static NetlinkPayloadMessage<struct rtgenmsg> createRtmGetLink();

        /// @return RTM_GETADDR message
        static NetlinkPayloadMessage<struct ifaddrmsg> createRtmGetAddr (
            IpAddress::AddressType addrType = IpAddress::EmptyAddress
        );

        /// @brief Creates an RTM_GETROUTE message
        /// @param [in] rtTable Routing table number (see /etc/iproute2/rt_tables)
        /// @return RTM_GETROUTE message
        static NetlinkPayloadMessage<struct rtmsg> createRtmGetRoute (
            uint8_t rtTable = RT_TABLE_MAIN
        );

        /// @brief Creates an RTM_NEWADDR or RTM_DELADDR message
        /// @param [in] action Add or Remove
        /// @param [in] flags The flags to use (from linux/netlink.h):
        ///                    NLM_F_REPLACE - Override existing
        ///                    NLM_F_EXCL    - Do not touch, if it exists
        ///                    NLM_F_CREATE  - Create, if it does not exist
        ///                    NLM_F_APPEND  - Add to end of list
        /// @param [in] address The IP address to set
        /// @param [in] netmaskLen The length of the netmask
        /// @param [in] ifaceIndex The index of the interface to modify
        /// @param [in] addrType Type of address to modify (e.g. local address, peer/broadcast address)
        /// @return Generated RTM_NEWADDR or RTM_DELADDR message
        static NetlinkPayloadMessage<struct ifaddrmsg> createRtmModifyIfaceAddr (
            PosixNetMgrTypes::Action action, uint16_t flags,
            const IpAddress & address, uint8_t netmaskLen, int ifaceIndex,
            PosixNetMgrTypes::AddressType addrType = PosixNetMgrTypes::AddrLocal
        );

        /// @brief Creates an RTM_SETLINK message to modify the MTU of the interface
        /// This actually brings the interface down...
        /// @param [in] ifaceIndex The index of the interface to modify
        /// @param [in] mtu The MTU to use
        /// @param [in] flags The flags to use (from linux/netlink.h):
        ///                    NLM_F_REPLACE - Override existing
        ///                    NLM_F_EXCL    - Do not touch, if it exists
        ///                    NLM_F_CREATE  - Create, if it does not exist
        ///                    NLM_F_APPEND  - Add to end of list
        /// @return Generated RTM_SETLINK message
        static NetlinkPayloadMessage<struct ifinfomsg> createRtmSetIfaceMtu (
            int ifaceIndex, unsigned int mtu, uint16_t flags = 0 );

        /// @brief Creates an RTM_SETLINK message to modify the state of the interface
        /// @param [in] ifaceIndex The index of the interface to modify
        /// @param [in] setUp If true, the interface state will be set to UP, otherwise - to DOWN
        /// @param [in] flags The flags to use (from linux/netlink.h):
        ///                    NLM_F_REPLACE - Override existing
        ///                    NLM_F_EXCL    - Do not touch, if it exists
        ///                    NLM_F_CREATE  - Create, if it does not exist
        ///                    NLM_F_APPEND  - Add to end of list
        /// @return Generated RTM_SETLINK message
        static NetlinkPayloadMessage<struct ifinfomsg> createRtmSetIfaceState (
            int ifaceIndex, bool setUp, uint16_t flags = 0
        );

        /// @brief Creates an RTM_NEWROUTE message
        /// It uses the NLM_F_CREATE flag - Create, if it does not exist
        /// @param [in] ifaceIndex The index of the interface
        /// @param [in] address The destination IP address
        /// @param [in] netmaskLen The length of the netmask to use
        /// @param [in] metric Metric to use
        /// @param [in] rtType The type of the route (RTN_UNICAST, RTN_LOCAL, RTN_BROADCAST, ...)
        /// @param [in] rtTable The routing table
        /// @return Generated RTM_NEWROUTE or RTM_DELROUTE message
        static NetlinkPayloadMessage<struct rtmsg> createRtmAddIfaceRoute (
            int ifaceIndex, const IpAddress & address, uint8_t netmaskLen, int metric = 0,
            uint8_t rtType = RTN_UNICAST, uint8_t rtTable = RT_TABLE_MAIN
        );

        /// @brief Creates an RTM_NEWROUTE or RTM_DELROUTE message
        /// @param [in] action Add or Remove
        /// @param [in] flags The flags to use (from linux/netlink.h):
        ///                    NLM_F_REPLACE - Override existing
        ///                    NLM_F_EXCL    - Do not touch, if it exists
        ///                    NLM_F_CREATE  - Create, if it does not exist
        ///                    NLM_F_APPEND  - Add to end of list
        /// @param [in] address The destination IP address
        /// @param [in] netmaskLen The length of the netmask to use
        /// @param [in] metric Metric to use
        /// @param [in] gatewayAddr The gateway address
        /// @param [in] ifaceIndex The index of the interface
        /// @param [in] rtType The type of the route (RTN_UNICAST, RTN_LOCAL, RTN_BROADCAST, ...)
        /// @param [in] rtTable The routing table
        /// @return Generated RTM_NEWROUTE or RTM_DELROUTE message
        static NetlinkPayloadMessage<struct rtmsg> createRtmModifyRoute (
            PosixNetMgrTypes::Action action, uint16_t flags,
            const IpAddress & address, uint8_t netmaskLen, int metric,
            const IpAddress & gatewayAddr, int ifaceIndex = -1,
            uint8_t rtType = RTN_UNICAST, uint8_t rtTable = RT_TABLE_MAIN
        );

    private:
        static TextLog _log; ///< Log stream
};
}
