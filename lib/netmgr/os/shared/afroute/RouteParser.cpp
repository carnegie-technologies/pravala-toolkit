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

#include "RouteParser.hpp"
#include "RoutePayload.hpp"

#include <cstdio>
#include <cerrno>

extern "C"
{
#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
}

using namespace Pravala;

TextLog RouteParser::_log ( "route_parser" );

bool RouteParser::CommonMsgHdr::parse ( const MemHandle & buf, MemHandle & data )
{
    if ( buf.size() < sizeof ( CommonMsgHdr ) )
        return false;

    const struct CommonMsgHdr * tmp = ( const struct CommonMsgHdr * ) ( buf.get() );
    assert ( tmp != 0 );

    if ( tmp->msgLen > buf.size() )
        return false;

    msgLen = tmp->msgLen;
    type = tmp->type;
    version = tmp->version;

    assert ( msgLen <= buf.size() );

    data = buf.getHandle ( 0, msgLen );
    assert ( data.size() == msgLen );
    assert ( data.size() <= buf.size() );

    return true;
}

ERRCODE RouteParser::processBuffer (
        RwBuffer & buf,
        List<AfRouteTypes::Link> & links,
        List<AfRouteTypes::Address> & addrs,
        List<AfRouteTypes::Route> & routes )
{
    if ( buf.size() < 1 )
        return Error::NothingToDo;

    ERRCODE ret = Error::Success;
    size_t bufOff = 0;

    // We may get more than one message in the read; iterate through them all
    while ( bufOff < buf.size() )
    {
        MemHandle bufRemaining = buf.getHandle ( bufOff );
        CommonMsgHdr hdr;
        MemHandle data;
        if ( !hdr.parse ( bufRemaining, data ) )
        {
            LOG ( L_DEBUG, "Incomplete message read; waiting for additional data before continuing" );
            ret = Error::IncompleteData;
            break;
        }

        assert ( data.size() <= bufRemaining.size() );
        // we can't parse a header with a size of 0, so if we properly parsed the message above
        // there needs to be data present.
        assert ( data.size() > 0 );

        // By this point we have a full message ready to be parsed.

        switch ( hdr.type )
        {
            case RTM_ADD:
            case RTM_DELETE:
            case RTM_CHANGE:
            case RTM_GET:
                {
                    AfRouteTypes::Route route;
                    if ( processRtMsg ( data, route ) )
                    {
                        routes.append ( route );
                    }
                    break;
                }

            case RTM_IFINFO:
                {
                    AfRouteTypes::Link link;
                    if ( processIfMsg ( data, link ) )
                    {
                        links.append ( link );
                    }
                    break;
                }

            case RTM_NEWADDR:
            case RTM_DELADDR:
                {
                    AfRouteTypes::Address addr;
                    if ( processIfaMsg ( data, addr ) )
                    {
                        addrs.append ( addr );
                    }
                    break;
                }

// This is defined on BSD, however QNX decided not to define NEWMADDR or DELMADDR as valid fields.
#ifndef SYSTEM_QNX
            case RTM_NEWMADDR:
            case RTM_DELMADDR:
                {
                    AfRouteTypes::Address addr;
                    if ( processIfmaMsg ( data, addr ) )
                    {
                        addrs.append ( addr );
                    }
                    break;
                }
#endif
            default:
                LOG ( L_DEBUG, "Received a message with unknown RTM type: " << hdr.type << "; ignoring" );
        }

        // We consume all of the data which the header thinks that this message has regardless of error code.
        // There isn't a point in parsing the message again and hoping for a different result, so we discard
        // the message and parse the next message.
        bufOff += data.size();
    }

    buf.consumeData ( bufOff );
    return ret;
}

bool RouteParser::processRtMsg ( const MemHandle & data, AfRouteTypes::Route & route )
{
    if ( data.size() < sizeof ( struct rt_msghdr ) )
    {
        LOG ( L_ERROR, "Called processRtMsg with a buffer of size " << data.size()
              << "; but sizeof(struct rt_msghdr) is " << sizeof ( struct rt_msghdr )
              << "; not enough data in buffer" );

        return false;
    }

    const struct rt_msghdr * rtHdr = ( const struct rt_msghdr * ) data.get();

    if ( rtHdr->rtm_msglen != data.size() )
    {
        LOG ( L_ERROR, "Called processRtMessage with a buffer of size " << data.size()
              << "; but the message has a size of " << rtHdr->rtm_msglen
              << "; cannot parse mismatched data values" );

        return false;
    }

    assert ( !data.isEmpty() );
    assert ( data.size() == rtHdr->rtm_msglen );

// For some reason QNX uses CLONED instead of WASCLONED; they have the same numeric value however
#ifdef SYSTEM_QNX
    if ( ( rtHdr->rtm_flags & RTF_CLONED ) == RTF_CLONED )
#else
    if ( ( rtHdr->rtm_flags & RTF_WASCLONED ) == RTF_WASCLONED )
#endif
    {
        // Every time a connection is made, a route is 'cloned' over that connection so that if the
        // route goes away at some point in the future the connection will continue.
        // We don't care about these cloned routes; since they are only generated as a result
        // of a 'real' route.
        return false;
    }

    if ( ( rtHdr->rtm_type & RTM_ADD ) == RTM_ADD || ( rtHdr->rtm_type & RTM_GET ) == RTM_GET )
    {
        route.act = PosixNetMgrTypes::ActionAdd;
    }
    else if ( ( rtHdr->rtm_type & RTM_DELETE ) == RTM_DELETE )
    {
        route.act = PosixNetMgrTypes::ActionRemove;
    }
    else
    {
        LOG ( L_DEBUG, "Received a route event of type " << rtHdr->rtm_type << " which we don't notify on" );
        return false;
    }

    RoutePayload payload;
    const MemHandle payloadData = data.getHandle ( sizeof ( struct rt_msghdr ) );

    if ( !payload.setup ( rtHdr->rtm_addrs, payloadData ) )
    {
        LOG ( L_ERROR, "Unable to setup RoutePayload from received ROUTE message, skipping" );
        return false;
    }

    if ( !payload.contains ( RoutePayload::FieldDestination ) || !payload.contains ( RoutePayload::FieldGateway ) )
    {
        LOG ( L_DEBUG, "Received a ROUTE message without a destination or gateway, skipping" );
        return false;
    }

    if ( !payload.getAddress ( RoutePayload::FieldDestination, route.dst ) )
    {
        LOG ( L_ERROR, "Received a ROUTE message with an invalid destination, skipping" );
        return false;
    }
    assert ( route.dst.isValid() );

    // Gateway can sometimes be LINK, so ensure that the GATEWAY is IP before parsing the address from it.
    if ( payload.isFieldIp ( RoutePayload::FieldGateway ) )
    {
        if ( !payload.getAddress ( RoutePayload::FieldGateway, route.gw ) )
        {
            LOG ( L_ERROR, "Received a ROUTE message with an invalid gateway, skipping" );
            return false;
        }
    }
    else
    {
        route.gw = ( route.dst.isIPv4() ? IpAddress::Ipv4ZeroAddress : IpAddress::Ipv6ZeroAddress );
    }
    assert ( route.gw.isValid() );

    // Interface address is optional, and may sometimes be LINK.
    if ( payload.contains ( RoutePayload::FieldInterfaceAddress )
         && payload.isFieldIp ( RoutePayload::FieldInterfaceAddress ) )
    {
        if ( !payload.getAddress ( RoutePayload::FieldInterfaceAddress, route.src ) )
        {
            LOG ( L_ERROR, "Received a ROUTE message with an invalid interface address, skipping" );
            return false;
        }
    }
    else
    {
        route.src = ( route.dst.isIPv4() ? IpAddress::Ipv4ZeroAddress : IpAddress::Ipv6ZeroAddress );
    }
    assert ( route.src.isValid() );

    uint8_t prefixLen = ( route.dst.isIPv4() ? 32 : 128 );

    if ( payload.contains ( RoutePayload::FieldNetmask ) )
    {
        IpAddress netmask;
        if ( !payload.getNetmask ( RoutePayload::FieldNetmask, netmask ) )
        {
            LOG ( L_ERROR, "Received a ROUTE message with an invalid netmask, skipping" );
            return false;
        }

        prefixLen = netmask.toPrefix();
    }

    // If we have a destination address, then the netmask is associated with the destination address
    if ( payload.contains ( RoutePayload::FieldDestination ) && payload.isFieldIp ( RoutePayload::FieldDestination ) )
    {
        route.dstPrefixLen = prefixLen;
    }
    // Otherwise, if we have a valid source IP address, then the netmask is associated with the source address
    else if ( payload.contains ( RoutePayload::FieldInterfaceAddress )
              && payload.isFieldIp ( RoutePayload::FieldInterfaceAddress ) )
    {
        route.srcPrefixLen = prefixLen;
    }

    route.family = ( route.dst.isIPv6() ? AF_INET6 : AF_INET );
    route.ifaceIdOut = rtHdr->rtm_index;

    // default values are fine for the rest of the fields; they aren't supported here.

    return true;
}

bool RouteParser::processIfMsg ( const MemHandle & data, AfRouteTypes::Link & link )
{
    if ( data.size() < sizeof ( struct if_msghdr ) )
    {
        LOG ( L_ERROR, "Called processIfMsg with a buffer of size " << data.size()
              << "; but sizeof(struct if_msghdr) is " << sizeof ( struct if_msghdr )
              << "; not enough data in buffer" );

        return false;
    }

    const struct if_msghdr * ifHdr = ( const struct if_msghdr * ) data.get();

    if ( ifHdr->ifm_msglen != data.size() )
    {
        LOG ( L_ERROR, "Called processIfMessage with a buffer of size " << data.size()
              << "; but the message has a size of " << ifHdr->ifm_msglen
              << "; cannot parse mismatched data values" );

        return false;
    }

    assert ( !data.isEmpty() );
    assert ( ifHdr->ifm_msglen == data.size() );

    if ( ( ifHdr->ifm_flags & IFF_UP ) == IFF_UP )
    {
        link.act = PosixNetMgrTypes::ActionAdd;
    }
    else
    {
        link.act = PosixNetMgrTypes::ActionRemove;
    }

    link.id = ifHdr->ifm_index;

    link.setFlags ( ifHdr->ifm_flags );

    char ifname[ IFNAMSIZ ];

    char * ifnameRet = if_indextoname ( ifHdr->ifm_index, ifname );

    if ( !ifnameRet )
    {
        LOG ( L_ERROR, "Error converting index " << ifHdr->ifm_index << " to name: " << strerror ( errno ) );
        return false;
    }

    link.name = ifnameRet;

    link.rxBytes = ifHdr->ifm_data.ifi_ibytes;
    link.txBytes = ifHdr->ifm_data.ifi_obytes;

    link.type = 0; // TODO add
    link.hwAddrLen = 0; // TODO add
    link.hwBroadcastAddrLen = 0; // TODO add
    link.mtu = 0; // TODO add

    return true;
}

bool RouteParser::processIfaMsg ( const MemHandle & data, AfRouteTypes::Address & addr )
{
    if ( data.size() < sizeof ( struct ifa_msghdr ) )
    {
        LOG ( L_ERROR, "Called processIfaMsg with a buffer of size " << data.size()
              << "; but sizeof(struct ifa_msghdr) is " << sizeof ( struct ifa_msghdr )
              << "; not enough data in buffer" );

        return false;
    }

    const struct ifa_msghdr * ifaHdr = ( const struct ifa_msghdr * ) data.get();

    if ( ifaHdr->ifam_msglen != data.size() )
    {
        LOG ( L_ERROR, "Called processIfaMessage with a buffer of size " << data.size()
              << "; but the message has a size of " << ifaHdr->ifam_msglen
              << "; cannot parse mismatched data values" );

        return false;
    }

    assert ( !data.isEmpty() );
    assert ( ifaHdr->ifam_msglen == data.size() );

    if ( ( ifaHdr->ifam_type & RTM_NEWADDR ) == RTM_NEWADDR )
    {
        addr.act = PosixNetMgrTypes::ActionAdd;
    }
    else if ( ( ifaHdr->ifam_type & RTM_DELADDR ) == RTM_DELADDR )
    {
        addr.act = PosixNetMgrTypes::ActionRemove;
    }
    else
    {
        LOG ( L_DEBUG, "Received an address event of type " << ifaHdr->ifam_type
              << " which we don't understand, ignoring" );
        return false;
    }

    RoutePayload payload;
    const MemHandle payloadData = data.getHandle ( sizeof ( struct ifa_msghdr ) );

    if ( !payload.setup ( ifaHdr->ifam_addrs, payloadData ) )
    {
        LOG ( L_ERROR, "Unable to setup PayloadData from provided ADDRESS message, skipping" );
        return false;
    }

    if ( !payload.contains ( RoutePayload::FieldInterfaceAddress ) )
    {
        LOG ( L_ERROR, "Received an ADDRESS message without an interface address; skipping" );
        return false;
    }

    // We can get link addresses in the InterfaceAddress field, we want to ignore these
    if ( payload.isFieldLink ( RoutePayload::FieldInterfaceAddress ) )
    {
        LOG ( L_DEBUG4, "Recieved an ADDRESS message with a LL address; skipping" );
        return false;
    }

    if ( !payload.getAddress ( RoutePayload::FieldInterfaceAddress, addr.localAddress ) )
    {
        LOG ( L_ERROR, "Recieved an ADDRESS message with an invalid interface address; skipping" );

        return false;
    }
    assert ( addr.localAddress.isValid() );

    if ( payload.contains ( RoutePayload::FieldNetmask ) )
    {
        IpAddress netmask;
        if ( !payload.getNetmask ( RoutePayload::FieldNetmask, netmask ) )
        {
            LOG ( L_ERROR, "Received an ADDRESS message with an invalid netmask, skipping" );
            return false;
        }

        addr.prefixLen = netmask.toPrefix();
    }
    else
    {
        addr.prefixLen = ( addr.localAddress.isIPv6() ? 128 : 32 );
    }

    if ( payload.contains ( RoutePayload::FieldBroadcast ) )
    {
        if ( !payload.getAddress ( RoutePayload::FieldBroadcast, addr.broadcastAddress ) )
        {
            LOG ( L_ERROR, "Received an ADDRESS message with an invalid broadcast address; skipping" );
            return false;
        }
    }
    else
    {
        addr.broadcastAddress = addr.localAddress.getBcastAddress ( addr.prefixLen );
    }
    assert ( addr.broadcastAddress.isValid() );

    addr.family = ( addr.localAddress.isIPv6() ? AF_INET6 : AF_INET );

    addr.flags = ifaHdr->ifam_flags;
    addr.ifaceId = ifaHdr->ifam_index;

    LOG ( L_DEBUG4, "Received an ADDRESS message with a address: " << addr.localAddress );

    return true;
}

#ifdef SYSTEM_QNX
// QNX does not support Ifma flags, so we don't even try parsing it.
bool RouteParser::processIfmaMsg ( const MemHandle &, AfRouteTypes::Address & )
{
    return false;
}

#else
bool RouteParser::processIfmaMsg ( const MemHandle & data, AfRouteTypes::Address & addr )
{
    if ( data.size() < sizeof ( struct ifma_msghdr ) )
    {
        LOG ( L_ERROR, "Called processIfamMsg with a buffer of size " << data.size()
              << "; but sizeof(struct ifma_msghdr) is " << sizeof ( struct ifma_msghdr )
              << "; not enough data in buffer" );

        return false;
    }

    const struct ifma_msghdr * ifmaHdr = ( const struct ifma_msghdr * ) data.get();

    if ( ifmaHdr->ifmam_msglen != data.size() )
    {
        LOG ( L_ERROR, "Called processIfamMessage with a buffer of size " << data.size()
              << "; but the message has a size of " << ifmaHdr->ifmam_msglen
              << "; cannot parse mismatched data values" );

        return false;
    }

    assert ( !data.isEmpty() );
    assert ( ifmaHdr->ifmam_msglen == data.size() );

    if ( ( ifmaHdr->ifmam_type & RTM_NEWMADDR ) == RTM_NEWMADDR )
    {
        addr.act = PosixNetMgrTypes::ActionAdd;
    }
    else if ( ( ifmaHdr->ifmam_type & RTM_DELMADDR ) == RTM_DELMADDR )
    {
        addr.act = PosixNetMgrTypes::ActionRemove;
    }
    else
    {
        LOG ( L_DEBUG, "Received an address event of type " << ifmaHdr->ifmam_type
              << " which we don't understand, ignoring" );
        return false;
    }

    RoutePayload payload;
    const MemHandle payloadData = data.getHandle ( sizeof ( struct ifma_msghdr ) );

    if ( !payload.setup ( ifmaHdr->ifmam_addrs, payloadData ) )
    {
        LOG ( L_ERROR, "Unable to setup PayloadData from provided MADDRESS message, skipping" );
        return false;
    }

    if ( !payload.contains ( RoutePayload::FieldInterfaceAddress ) )
    {
        LOG ( L_ERROR, "Received an MADDRESS message with an interface address; skipping" );
        return false;
    }

    if ( !payload.getAddress ( RoutePayload::FieldInterfaceAddress, addr.localAddress ) )
    {
        LOG ( L_ERROR, "Recieved an MADDRESS message with an invalid interface address; skipping" );
        return false;
    }
    assert ( addr.localAddress.isValid() );

    if ( payload.contains ( RoutePayload::FieldNetmask ) )
    {
        IpAddress netmask;
        if ( !payload.getNetmask ( RoutePayload::FieldNetmask, netmask ) )
        {
            LOG ( L_ERROR, "Received an MADDRESS message with an invalid netmask, skipping" );
            return false;
        }

        addr.prefixLen = netmask.toPrefix();
    }
    else
    {
        addr.prefixLen = ( addr.localAddress.isIPv6() ? 128 : 32 );
    }

    if ( payload.contains ( RoutePayload::FieldBroadcast ) )
    {
        if ( !payload.getAddress ( RoutePayload::FieldBroadcast, addr.broadcastAddress ) )
        {
            LOG ( L_ERROR, "Received an MADDRESS message with an invalid broadcast address; skipping" );
            return false;
        }
    }
    else
    {
        addr.broadcastAddress = addr.localAddress.getBcastAddress ( addr.prefixLen );
    }
    assert ( addr.broadcastAddress.isValid() );

    addr.family = ( addr.localAddress.isIPv6() ? AF_INET6 : AF_INET );
    addr.ifaceId = ifmaHdr->ifmam_index;

    addr.flags = ifmaHdr->ifmam_flags;

    return true;
}
#endif
