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

#include "NetlinkRoute.hpp"
#include "NetlinkCore.hpp"

// This is not defined in Android NDK
#ifndef RTA_TABLE
#define RTA_TABLE    ( RTA_MP_ALGO + 1 )
#endif

/// @brief Equivalent to RTA_DATA, but operates on const pointers
#define RTA_CONST_DATA( rta )    ( ( const void * ) ( ( ( const char * ) ( rta ) ) + RTA_LENGTH ( 0 ) ) )

/// @brief Equivalent to RTA_NEXT, but operates on const pointers
#define RTA_CONST_NEXT( rta, attrlen ) \
    ( ( attrlen ) -= RTA_ALIGN ( ( rta )->rta_len ),  \
      ( const struct rtattr * ) ( ( ( const char * ) ( rta ) ) + RTA_ALIGN ( ( rta )->rta_len ) ) )

#define CASE_RTA( rtype ) \
    case rtype: \
        LOG ( L_DEBUG4, "Got unhandled rta type: " #rtype << " [" << rta->rta_type \
              << "]; Size: " << RTA_PAYLOAD ( rta ) \
              << "; Data: " << String::hexDump ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) ) \
              << "; Printable: '" \
              << TextLog::dumpPrintable ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) ) \
              << "'" ); \
        break

using namespace Pravala;

static TextLog _log ( "netlink_route" );

/// @brief Returns an empty IP address that matches the given address family.
/// @param [in] family The type of IP address to return; Either AF_INET or AF_INET6.
/// @return IPv4 zero address, IPv6 zero address, or Invalid address - depending on the family type received.
inline static const IpAddress & getZeroAddress ( uint8_t family )
{
    if ( family == AF_INET )
    {
        return IpAddress::Ipv4ZeroAddress;
    }
    else if ( family == AF_INET6 )
    {
        return IpAddress::Ipv6ZeroAddress;
    }
    else
    {
        return IpAddress::IpEmptyAddress;
    }
}

bool NetlinkRoute::parseRouteMessage ( const NetlinkMessage & msg, NetlinkRoute::RouteResults & result )
{
    const struct nlmsghdr * const hdr = msg.getNlmsghdr();

    if ( !hdr )
    {
        return false;
    }

    switch ( hdr->nlmsg_type )
    {
        case RTM_NEWLINK:
            LOG ( L_DEBUG4, "Got RTM_NEWLINK; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmLink ( hdr, result.links );
            break;

        case RTM_DELLINK:
            LOG ( L_DEBUG4, "Got RTM_DELLINK; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmLink ( hdr, result.links );
            break;

        case RTM_NEWADDR:
            LOG ( L_DEBUG4, "Got RTM_NEWADDR; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmAddr ( hdr, result.addresses );
            break;

        case RTM_DELADDR:
            LOG ( L_DEBUG4, "Got RTM_DELADDR; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmAddr ( hdr, result.addresses );
            break;

        case RTM_NEWROUTE:
            LOG ( L_DEBUG4, "Got RTM_NEWROUTE; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmRoute ( hdr, result.routes );
            break;

        case RTM_DELROUTE:
            LOG ( L_DEBUG4, "Got RTM_DELROUTE; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            parseRtmRoute ( hdr, result.routes );
            break;

        case NLMSG_ERROR:
        case NLMSG_DONE:
            break;

        default:
            LOG ( L_ERROR, "Got unexpected message type: " << hdr->nlmsg_type
                  << "; SeqNum: " << hdr->nlmsg_seq << "; Flags: " << hdr->nlmsg_flags );

            return false;
            break;
    }

    return true;
}

bool NetlinkRoute::parseRtmLink ( const nlmsghdr * hdr, List<NetlinkTypes::Link> & links )
{
    assert ( hdr != 0 );

    // this function should only be called if one of these is true
    assert ( hdr->nlmsg_type == RTM_NEWLINK || hdr->nlmsg_type == RTM_DELLINK );

    // retrieve ifinfomsg from msg
    const struct ifinfomsg * ifim = ( const struct ifinfomsg * ) NLMSG_CONST_DATA ( hdr );

    assert ( ifim != 0 );

    LOG ( L_DEBUG3, "RTM_LINK message; nlmsg_type: " << hdr->nlmsg_type
          << "; ifi_type: " << ifim->ifi_type
          << "; ifi_index: " << ifim->ifi_index
          << "; ifi_flags: " << ifim->ifi_flags );

    NetlinkTypes::Link nlLink;

    nlLink.type = ifim->ifi_type;
    nlLink.id = ifim->ifi_index;

    nlLink.setFlags ( ifim->ifi_flags );

    if ( hdr->nlmsg_type == RTM_NEWLINK )
    {
        nlLink.act = PosixNetMgrTypes::ActionAdd;
    }
    else
    {
        nlLink.act = PosixNetMgrTypes::ActionRemove;
    }

    const struct rtattr * rta = getMsgRta ( ( const char * ) ifim, sizeof ( struct ifinfomsg ) );

    assert ( rta != 0 );

    /// @note rtalen is updated by RTA_NEXT
    uint32_t rtaLen = NetlinkCore::getMsgPayloadLength ( hdr, sizeof ( struct ifinfomsg ) );

    // retrieve rtattr payload
    for ( ; RTA_OK ( rta, rtaLen ); rta = RTA_CONST_NEXT ( rta, rtaLen ) )
    {
        switch ( rta->rta_type )
        {
            case IFLA_ADDRESS:
                if ( RTA_PAYLOAD ( rta ) > sizeof ( nlLink.hwAddr ) )
                {
                    LOG ( L_ERROR, "IFLA_ADDRESS length " << RTA_PAYLOAD ( rta )
                          << " exceeds hwAddr max size " << sizeof ( nlLink.hwAddr ) );
                }
                else
                {
                    memcpy ( &nlLink.hwAddr, RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) );
                    nlLink.hwAddrLen = RTA_PAYLOAD ( rta );

                    LOG ( L_DEBUG4, "Got IFLA_ADDRESS" );
                }
                break;

            case IFLA_BROADCAST:
                if ( RTA_PAYLOAD ( rta ) > sizeof ( nlLink.hwBroadcastAddr ) )
                {
                    LOG ( L_ERROR, "IFLA_BROADCAST length " << RTA_PAYLOAD ( rta )
                          << " exceeds hwBroadcastAddr max size " << sizeof ( nlLink.hwBroadcastAddr ) );
                }
                else
                {
                    memcpy ( &nlLink.hwBroadcastAddr, RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) );
                    nlLink.hwBroadcastAddrLen = RTA_PAYLOAD ( rta );

                    LOG ( L_DEBUG4, "Got IFLA_BROADCAST" );
                }
                break;

            case IFLA_IFNAME:
                assert ( nlLink.name.isEmpty() );

                nlLink.name.append ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) - 1 );

                LOG ( L_DEBUG4, "Got IFLA_IFNAME " << nlLink.name );
                break;

            case IFLA_MTU:
                LOG ( L_DEBUG4, "Got IFLA_MTU" );

                rtaToUint32 ( rta, nlLink.mtu );
                break;

            case IFLA_LINK:
                LOG ( L_DEBUG4, "Got IFLA_LINK" );

                rtaToInt ( rta, nlLink.realId );
                break;

                CASE_RTA ( IFLA_UNSPEC );
                CASE_RTA ( IFLA_QDISC );
                CASE_RTA ( IFLA_STATS );
                CASE_RTA ( IFLA_COST );
                CASE_RTA ( IFLA_PRIORITY );
                CASE_RTA ( IFLA_MASTER );
                CASE_RTA ( IFLA_WIRELESS );
                CASE_RTA ( IFLA_PROTINFO );
                CASE_RTA ( IFLA_TXQLEN );
                CASE_RTA ( IFLA_MAP );
                CASE_RTA ( IFLA_WEIGHT );
                CASE_RTA ( IFLA_OPERSTATE );
                CASE_RTA ( IFLA_LINKMODE );

#ifndef PLATFORM_ANDROID
                CASE_RTA ( IFLA_LINKINFO );
                CASE_RTA ( IFLA_NET_NS_PID );
                CASE_RTA ( IFLA_IFALIAS );
                CASE_RTA ( IFLA_NUM_VF );
                CASE_RTA ( IFLA_VFINFO_LIST );
                CASE_RTA ( IFLA_STATS64 );
                CASE_RTA ( IFLA_VF_PORTS );
                CASE_RTA ( IFLA_PORT_SELF );
                CASE_RTA ( IFLA_AF_SPEC );
                CASE_RTA ( IFLA_GROUP );
                CASE_RTA ( IFLA_NET_NS_FD );
                CASE_RTA ( IFLA_EXT_MASK );
#endif

                CASE_RTA ( __IFLA_MAX );

            default:
                LOG ( L_DEBUG3, "Got unknown rta type: [" << rta->rta_type
                      << "]; Size: " << RTA_PAYLOAD ( rta )
                      << "; Data: " << String::hexDump ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "; Printable: '"
                      << TextLog::dumpPrintable ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "'" );
                break;
        }
    }

    links.append ( nlLink );

    return true;
}

bool NetlinkRoute::parseRtmAddr ( const nlmsghdr * hdr, List<NetlinkTypes::Address > & addresses )
{
    assert ( hdr != 0 );

    // this function should only be called if one of these is true
    assert ( hdr->nlmsg_type == RTM_NEWADDR || hdr->nlmsg_type == RTM_DELADDR );

    const struct ifaddrmsg * ifim = ( const struct ifaddrmsg * ) NLMSG_CONST_DATA ( hdr );
    const struct rtattr * rta = getMsgRta ( ( const char * ) ifim, sizeof ( struct ifaddrmsg ) );

    LOG ( L_DEBUG3, "RTM_ADDR message; nlmsg_type: " << hdr->nlmsg_type
          << "; ifa_index: " << ifim->ifa_index
          << "; ifa_family: " << ifim->ifa_family
          << "; ifa_flags: " << ifim->ifa_flags );

    assert ( rta != 0 );

    /// @note rtalen is updated by RTA_NEXT
    uint32_t rtaLen = NetlinkCore::getMsgPayloadLength ( hdr, sizeof ( struct ifaddrmsg ) );

    NetlinkTypes::Address nlAddr;

    if ( hdr->nlmsg_type == RTM_NEWADDR )
    {
        nlAddr.act = PosixNetMgrTypes::ActionAdd;
    }
    else
    {
        nlAddr.act = PosixNetMgrTypes::ActionRemove;
    }

    nlAddr.family = ifim->ifa_family;
    nlAddr.prefixLen = ifim->ifa_prefixlen;
    nlAddr.ifaceId = ifim->ifa_index;
    nlAddr.flags = ifim->ifa_flags;

    if ( nlAddr.family != AF_INET && nlAddr.family != AF_INET6 )
    {
        LOG ( L_ERROR, "Unsupported address family: " << nlAddr.family );

        return false;
    }

    // retrieve rtattr payload
    for ( ; RTA_OK ( rta, rtaLen ); rta = RTA_CONST_NEXT ( rta, rtaLen ) )
    {
        switch ( rta->rta_type )
        {
            case IFA_ADDRESS:
                // If the interface is PtP, this is the remote address, so we ignore it.
                // Otherwise, it's the same as IFF_LOCAL, and it's also used by IPv6.
                if ( ( nlAddr.flags & IFF_POINTOPOINT ) == IFF_POINTOPOINT )
                {
                    LOG ( L_DEBUG4, "Got IFA_ADDRESS on PtP interface, ignoring." );
                }
                else
                {
                    LOG ( L_DEBUG4, "Got IFA_ADDRESS on non-PtP interface, using." );

                    rtaToIpAddress ( rta, nlAddr.family, nlAddr.localAddress );
                }
                break;

            case IFA_LOCAL:
                // If the interface is PtP, IFF_LOCAL is our local address.
                // Otherwise, it's the same as IFF_ADDRESS, so we'll just ignore it. IPv6 doesn't seem to set this.
                if ( ( nlAddr.flags & IFF_POINTOPOINT ) == IFF_POINTOPOINT )
                {
                    LOG ( L_DEBUG4, "Got IFA_LOCAL on PtP interface, using." );

                    rtaToIpAddress ( rta, nlAddr.family, nlAddr.localAddress );
                }
                else
                {
                    LOG ( L_DEBUG4, "Got IFA_LOCAL on non-PtP interface, ignoring." );
                }
                break;

            case IFA_BROADCAST:
                LOG ( L_DEBUG4, "Got IFA_BROADCAST" );

                rtaToIpAddress ( rta, nlAddr.family, nlAddr.broadcastAddress );
                break;

            default:
                LOG ( L_DEBUG3, "Got unknown rta type: [" << rta->rta_type
                      << "]; Size: " << RTA_PAYLOAD ( rta )
                      << "; Data: " << String::hexDump ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "; Printable: '"
                      << TextLog::dumpPrintable ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "'" );

                // we don't care about other fields
                break;
        }
    }

    addresses.append ( nlAddr );

    return true;
}

bool NetlinkRoute::parseRtmRoute ( const nlmsghdr * hdr, List< NetlinkTypes::Route > & routes )
{
    assert ( hdr != 0 );

    // this function should only be called if one of these is true
    assert ( hdr->nlmsg_type == RTM_NEWROUTE || hdr->nlmsg_type == RTM_DELROUTE );

    const struct rtmsg * rtm = ( const struct rtmsg * ) NLMSG_CONST_DATA ( hdr );
    const struct rtattr * rta = getMsgRta ( ( const char * ) rtm, sizeof ( struct rtmsg ) );

    LOG ( L_DEBUG3, "RTM_ROUTE message; nlmsg_type: " << hdr->nlmsg_type
          << "; rtm_family: " << rtm->rtm_family
          << "; rtm_table: " << rtm->rtm_table
          << "; rtm_src_len: " << rtm->rtm_src_len
          << "; rtm_dst_len: " << rtm->rtm_dst_len );

    assert ( rta != 0 );

    /// @note rtalen is updated by RTA_NEXT
    uint32_t rtaLen = NetlinkCore::getMsgPayloadLength ( hdr, sizeof ( struct rtmsg ) );

    NetlinkTypes::Route nlRoute;

    if ( hdr->nlmsg_type == RTM_NEWROUTE )
    {
        nlRoute.act = PosixNetMgrTypes::ActionAdd;
    }
    else
    {
        nlRoute.act = PosixNetMgrTypes::ActionRemove;
    }

    nlRoute.family = rtm->rtm_family;
    nlRoute.srcPrefixLen = rtm->rtm_src_len;
    nlRoute.dstPrefixLen = rtm->rtm_dst_len;
    nlRoute.table = rtm->rtm_table;
    nlRoute.routingProtocol = rtm->rtm_protocol;

    if ( nlRoute.family != AF_INET && nlRoute.family != AF_INET6 )
    {
        LOG ( L_ERROR, "Unsupported address family: " << nlRoute.family );

        return false;
    }

    // retrieve rtattr payload
    for ( ; RTA_OK ( rta, rtaLen ); rta = RTA_CONST_NEXT ( rta, rtaLen ) )
    {
        switch ( rta->rta_type )
        {
            case RTA_IIF:
                LOG ( L_DEBUG4, "Got RTA_IIF" );

                rtaToInt ( rta, nlRoute.ifaceIdIn );
                break;

            case RTA_OIF:
                LOG ( L_DEBUG4, "Got RTA_OIF" );

                rtaToInt ( rta, nlRoute.ifaceIdOut );
                break;

            case RTA_PRIORITY:
                LOG ( L_DEBUG4, "Got RTA_PRIORITY" );

                rtaToInt ( rta, nlRoute.metric );
                break;

            case RTA_SRC:
                LOG ( L_DEBUG4, "Got RTA_SRC" );

                rtaToIpAddress ( rta, nlRoute.family, nlRoute.src );
                break;

            case RTA_DST:
                LOG ( L_DEBUG4, "Got RTA_DST" );

                rtaToIpAddress ( rta, nlRoute.family, nlRoute.dst );
                break;

            case RTA_GATEWAY:
                LOG ( L_DEBUG4, "Got RTA_GATEWAY" );

                rtaToIpAddress ( rta, nlRoute.family, nlRoute.gw );
                break;

            case RTA_TABLE:
                LOG ( L_DEBUG4, "Got RTA_TABLE" );

                {
                    // RTA carries INT, but it should fit in uint8_t
                    int table = 0;
                    rtaToInt ( rta, table );

                    nlRoute.table = table & 0xFF;
                }
                break;

                CASE_RTA ( RTA_UNSPEC );
                CASE_RTA ( RTA_PREFSRC );
                CASE_RTA ( RTA_METRICS );
                CASE_RTA ( RTA_MULTIPATH );
                CASE_RTA ( RTA_PROTOINFO );
                CASE_RTA ( RTA_FLOW );
                CASE_RTA ( RTA_CACHEINFO );
                CASE_RTA ( RTA_SESSION );
                CASE_RTA ( RTA_MP_ALGO );

#ifndef PLATFORM_ANDROID
                CASE_RTA ( RTA_MARK );
                CASE_RTA ( __RTA_MAX );
#endif

            default:
                LOG ( L_DEBUG3, "Got unknown rta type: [" << rta->rta_type
                      << "]; Size: " << RTA_PAYLOAD ( rta )
                      << "; Data: " << String::hexDump ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "; Printable: '"
                      << TextLog::dumpPrintable ( ( const char * ) RTA_CONST_DATA ( rta ), RTA_PAYLOAD ( rta ) )
                      << "'" );

                // we don't care about other fields
                break;
        }
    }

    if ( !nlRoute.src.isValid() )
    {
        nlRoute.src = getZeroAddress ( nlRoute.family );
    }

    if ( !nlRoute.dst.isValid() )
    {
        nlRoute.dst = getZeroAddress ( nlRoute.family );
    }

    if ( !nlRoute.gw.isValid() )
    {
        nlRoute.gw = getZeroAddress ( nlRoute.family );
    }

    routes.append ( nlRoute );

    return true;
}

bool NetlinkRoute::rtaToIpAddress ( const rtattr * rta, uint8_t family, IpAddress & ip )
{
    in_addr addr4;
    in6_addr addr6;

    if ( family == AF_INET && RTA_PAYLOAD ( rta ) == 4 )
    {
        memcpy ( &addr4, RTA_CONST_DATA ( rta ), 4 );
        ip = addr4;
    }
    else if ( family == AF_INET6 && RTA_PAYLOAD ( rta ) == 16 )
    {
        memcpy ( &addr6, RTA_CONST_DATA ( rta ), 16 );
        ip = addr6;
    }
    else
    {
        LOG ( L_ERROR, "Unexpected family and IP address size: " << family << ", " << RTA_PAYLOAD ( rta ) );

        return false;
    }

    LOG ( L_DEBUG4, "Address: " << ip );

    return true;
}

bool NetlinkRoute::rtaToInt ( const rtattr * rta, int & val )
{
    if ( RTA_PAYLOAD ( rta ) != sizeof ( int ) )
    {
        LOG ( L_ERROR, "Length " << RTA_PAYLOAD ( rta ) << " != int size" );

        return false;
    }

    val = *( const int * ) RTA_CONST_DATA ( rta );

    LOG ( L_DEBUG4, "Value: " << val );

    return true;
}

bool NetlinkRoute::rtaToUint32 ( const rtattr * rta, uint32_t & val )
{
    if ( RTA_PAYLOAD ( rta ) != sizeof ( uint32_t ) )
    {
        LOG ( L_ERROR, "Length " << RTA_PAYLOAD ( rta ) << " != uint32_t size" );

        return false;
    }

    val = *( const int * ) RTA_CONST_DATA ( rta );

    LOG ( L_DEBUG4, "Value: " << val );

    return true;
}
