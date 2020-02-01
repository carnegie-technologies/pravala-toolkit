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

#include "NetlinkMsgCreator.hpp"

using namespace Pravala;

TextLog NetlinkMsgCreator::_log ( "netlink_msg_factory" );

NetlinkPayloadMessage<struct ifaddrmsg> NetlinkMsgCreator::createRtmGetAddr ( IpAddress::AddressType addrType )
{
    NetlinkPayloadMessage<struct ifaddrmsg> ret (
            RTM_GETADDR, // type
            NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST // flags
    );

    if ( addrType == IpAddress::V4Address )
    {
        ret.getPayloadMessage()->ifa_family = AF_INET;
    }
    else if ( addrType == IpAddress::V6Address )
    {
        ret.getPayloadMessage()->ifa_family = AF_INET6;
    }

    /// @todo Do we need to do something for 'ANY' address type???

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage<struct rtgenmsg> NetlinkMsgCreator::createRtmGetLink()
{
    NetlinkPayloadMessage<struct rtgenmsg> ret (
            RTM_GETLINK, // type
            NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST // flags
    );

    ret.getPayloadMessage()->rtgen_family = AF_INET;

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage<struct rtmsg> NetlinkMsgCreator::createRtmGetRoute ( uint8_t rtTable )
{
    NetlinkPayloadMessage<struct rtmsg> ret (
            RTM_GETROUTE, // type
            NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST // flags
    );

    struct rtmsg * msg = ret.getPayloadMessage();

    assert ( msg != 0 );

    msg->rtm_dst_len = 0;
    msg->rtm_src_len = 0;
    msg->rtm_table = rtTable;

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage<struct ifaddrmsg> NetlinkMsgCreator::createRtmModifyIfaceAddr (
        PosixNetMgrTypes::Action action, uint16_t flags,
        const IpAddress & address, uint8_t netmaskLen,
        int ifaceIndex, PosixNetMgrTypes::AddressType addrType )
{
    NetlinkPayloadMessage<struct ifaddrmsg> ret (
            ( action == PosixNetMgrTypes::ActionAdd ) ? RTM_NEWADDR : RTM_DELADDR, // type
            NLM_F_REQUEST | flags, // flags
            RTA_SPACE ( sizeof ( struct in6_addr ) ) // RTA payload size - one IP address (IPv4 or v6)
    );

    LOG ( L_DEBUG, "action: " << ( int ) action
          << "; flags: " << flags
          << "; address: " << address
          << "; netmaskLen: " << ( int ) netmaskLen
          << "; ifaceIndex: " << ifaceIndex
          << "; addrType: " << ( int ) addrType );

    if ( action != PosixNetMgrTypes::ActionAdd && action != PosixNetMgrTypes::ActionRemove )
    {
        assert ( false );
        return ret;
    }

    if ( addrType != PosixNetMgrTypes::AddrLocal && addrType != PosixNetMgrTypes::AddrPeerBroadcast )
    {
        assert ( false );
        return ret;
    }

    struct ifaddrmsg * msg = ret.getPayloadMessage();

    assert ( msg != 0 );

    msg->ifa_flags = IFA_F_PERMANENT;
    msg->ifa_scope = RT_SCOPE_UNIVERSE;
    msg->ifa_index = ifaceIndex;
    msg->ifa_prefixlen = netmaskLen;

    if ( address.isIPv4() )
    {
        msg->ifa_family = AF_INET;

        if ( !ret.appendRta ( ( addrType == PosixNetMgrTypes::AddrLocal ) ? IFA_LOCAL : IFA_BROADCAST,
                              &address.getV4(), sizeof ( struct in_addr ) ) )
            return ret;
    }
    else if ( address.isIPv6() )
    {
        msg->ifa_family = AF_INET6;

        if ( !ret.appendRta ( ( addrType == PosixNetMgrTypes::AddrLocal ) ? IFA_LOCAL : IFA_BROADCAST,
                              &address.getV6(), sizeof ( struct in6_addr ) ) )
            return ret;
    }
    else
    {
        assert ( false );
        return ret;
    }

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage<struct ifinfomsg> NetlinkMsgCreator::createRtmSetIfaceMtu (
        int ifaceIndex, unsigned int mtu, uint16_t flags )
{
    NetlinkPayloadMessage<struct ifinfomsg> ret (
            RTM_SETLINK, // type
            NLM_F_REQUEST | flags, // flags
            RTA_SPACE ( sizeof ( mtu ) ) // RTA payload size - one MTU value
    );

    struct ifinfomsg * msg = ret.getPayloadMessage();

    assert ( msg != 0 );

    msg->ifi_family = AF_UNSPEC;
    msg->ifi_type = 0;
    msg->ifi_index = ifaceIndex;
    msg->ifi_flags = 0;

    // "ifi_change is reserved for future use and should be always set to 0xFFFFFFFF."
    msg->ifi_change = 0xFFFFFFFF;

    if ( !ret.appendRta ( IFLA_MTU, &mtu, sizeof ( mtu ) ) )
        return ret;

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage<struct ifinfomsg> NetlinkMsgCreator::createRtmSetIfaceState (
        int ifaceIndex, bool setUp, uint16_t flags )
{
    NetlinkPayloadMessage<struct ifinfomsg> ret (
            RTM_SETLINK, // type
            NLM_F_REQUEST | flags // flags
    );

    struct ifinfomsg * msg = ret.getPayloadMessage();

    assert ( msg != 0 );

    msg->ifi_family = AF_UNSPEC;
    msg->ifi_type = 0;
    msg->ifi_index = ifaceIndex;
    msg->ifi_flags = setUp ? IFF_UP : 0;

    // "ifi_change is reserved for future use and should be always set to 0xFFFFFFFF."
    msg->ifi_change = 0xFFFFFFFF;

    ret.setValid();

    return ret;
}

NetlinkPayloadMessage< rtmsg > NetlinkMsgCreator::createRtmAddIfaceRoute (
        int ifaceIndex, const IpAddress & address,
        uint8_t netmaskLen, int metric,
        uint8_t rtType, uint8_t rtTable )
{
    NetlinkPayloadMessage<struct rtmsg> ret = createRtmModifyRoute (
        PosixNetMgrTypes::ActionAdd,
        NLM_F_CREATE,
        address,
        netmaskLen,
        metric,
        IpAddress(),
        ifaceIndex,
        rtType,
        rtTable );

    // This constructor doesn't use the gateway, we NEED the iface index!
    if ( ifaceIndex < 0 )
    {
        assert ( false );

        ret.setValid ( false );
        return ret;
    }

    return ret;
}

NetlinkPayloadMessage<struct rtmsg> NetlinkMsgCreator::createRtmModifyRoute (
        PosixNetMgrTypes::Action action, uint16_t flags,
        const IpAddress & address, uint8_t netmaskLen, int metric,
        const IpAddress & gatewayAddr, int ifaceIndex,
        uint8_t rtType, uint8_t rtTable )
{
    NetlinkPayloadMessage<struct rtmsg> ret (
            ( action == PosixNetMgrTypes::ActionAdd ) ? RTM_NEWROUTE : RTM_DELROUTE, // type
            NLM_F_REQUEST | flags, // flags
            // RTA payload size:
            RTA_SPACE ( sizeof ( struct in6_addr ) ) // Destination IP address (IPv4 or v6 - so use bigger)
            + RTA_SPACE ( sizeof ( struct in6_addr ) ) // Gateway IP address (IPv4 or v6 - so use bigger)
            + RTA_SPACE ( sizeof ( ifaceIndex ) ) // + iface index
            + RTA_SPACE ( sizeof ( metric ) ) // + metric
    );

    LOG ( L_DEBUG, "action: " << ( int ) action
          << "; flags: " << flags
          << "; address: " << address
          << "; netmaskLen: " << ( int ) netmaskLen
          << "; gatewayAddr: " << gatewayAddr
          << "; ifaceIndex: " << ifaceIndex
          << "; metric: " << metric
          << "; rtType: " << ( int ) rtType
          << "; rtTable: " << ( int ) rtTable );

    if ( action != PosixNetMgrTypes::ActionAdd && action != PosixNetMgrTypes::ActionRemove )
    {
        assert ( false );
        return ret;
    }

    if ( !address.isValid() || ( !address.isIPv4() && !address.isIPv6() ) )
    {
        LOG ( L_ERROR, "Invalid address provided: " << address );

        return ret;
    }

    if ( address.isIPv4() && netmaskLen > 32 )
    {
        LOG ( L_ERROR, "Invalid prefixLen for IPv4: " << ( int ) netmaskLen );

        return ret;
    }

    if ( address.isIPv6() && netmaskLen > 128 )
    {
        LOG ( L_ERROR, "Invalid prefixLen for IPv6: " << ( int ) netmaskLen );

        return ret;
    }

    unsigned char scope = RT_SCOPE_NOWHERE;

    // To use the gateway, it needs to be valid ant non-zero
    const bool gatewayUsed = ( gatewayAddr.isValid() && !gatewayAddr.isZero() );

    if ( action == PosixNetMgrTypes::ActionAdd )
    {
        if ( gatewayUsed )
        {
            if ( address.getAddrType() != gatewayAddr.getAddrType() )
            {
                LOG ( L_ERROR, "gateway address " << gatewayAddr
                      << " has different type than destination address " << address );

                return ret;
            }

            // globally scoped route since we have a gateway
            // (i.e. if the gateway IP moves to a different interface, this route could still be valid)
            scope = RT_SCOPE_UNIVERSE;
        }
        else
        {
            if ( ifaceIndex < 0 )
            {
                LOG ( L_ERROR, "gateway address is not used, but ifaceIndex is incorrect: " << ifaceIndex );

                return ret;
            }

            // link scoped route, since we don't have a gateway
            // (i.e. if the link goes away, this route will disappear with it)
            scope = RT_SCOPE_LINK;
        }
    }

    struct rtmsg * msg = ret.getPayloadMessage();

    assert ( msg != 0 );

    msg->rtm_protocol = RTPROT_STATIC;
    msg->rtm_scope = scope;
    msg->rtm_dst_len = netmaskLen;
    msg->rtm_src_len = 0;
    msg->rtm_table = rtTable;
    msg->rtm_type = rtType;

    if ( address.isIPv4() )
    {
        msg->rtm_family = AF_INET;

        if ( !ret.appendRta ( RTA_DST, &address.getV4(), sizeof ( struct in_addr ) ) )
            return ret;

        if ( gatewayUsed )
        {
            assert ( gatewayAddr.isIPv4() );

            if ( !ret.appendRta ( RTA_GATEWAY, &gatewayAddr.getV4(), sizeof ( struct in_addr ) ) )
            {
                return ret;
            }
        }
    }
    else if ( address.isIPv6() )
    {
        msg->rtm_family = AF_INET6;

        if ( !ret.appendRta ( RTA_DST, &address.getV6(), sizeof ( struct in6_addr ) ) )
            return ret;

        if ( gatewayUsed )
        {
            assert ( gatewayAddr.isIPv6() );

            if ( !ret.appendRta ( RTA_GATEWAY, &gatewayAddr.getV6(), sizeof ( struct in6_addr ) ) )
            {
                return ret;
            }
        }
    }
    else
    {
        assert ( false );

        return ret;
    }

    // add interface index (if used)
    if ( ifaceIndex >= 0 )
    {
        if ( !ret.appendRta ( RTA_OIF, &ifaceIndex, sizeof ( ifaceIndex ) ) )
            return ret;
    }

    // add the metric (if used)
    if ( metric >= 0 )
    {
        if ( !ret.appendRta ( RTA_PRIORITY, &metric, sizeof ( metric ) ) )
            return ret;
    }

    ret.setValid();

    return ret;
}
