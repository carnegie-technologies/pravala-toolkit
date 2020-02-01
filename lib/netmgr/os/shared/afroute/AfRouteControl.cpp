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

#include "AfRouteControl.hpp"
#include "RouteParser.hpp"

#include <cerrno>

extern "C"
{
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
}

#define MIB_SIZE                     6

#define RT_MSG_SIZE                  1024

// Number of times to retry a sysctl call which has failed due to insufficient memory before aborting the call
#define SYSCTL_MEMORY_RETRY_COUNT    10

using namespace Pravala;

TextLog AfRouteControl::_log ( "afroute_control" );

AfRouteControl::AfRouteControl(): _routeSock ( -1 ), _v4Sock ( -1 ), _v6Sock ( -1 ), _rtmSeqNum ( 0 )
{
}

AfRouteControl::~AfRouteControl()
{
    if ( _routeSock >= 0 )
    {
        close ( _routeSock );
        _routeSock = -1;
    }

    if ( _v4Sock >= 0 )
    {
        close ( _v4Sock );
        _v4Sock = -1;
    }

    if ( _v6Sock >= 0 )
    {
        close ( _v6Sock );
        _v6Sock = -1;
    }
}

int AfRouteControl::getRouteSocket()
{
    if ( _routeSock < 0 )
    {
        // If we can't create it, the caller will figure this out and log the error.
        _routeSock = ::socket ( AF_ROUTE, SOCK_RAW, 0 );

        if ( _routeSock < 0 )
        {
            LOG ( L_ERROR, "Unable to create routing socket: " << ::strerror ( errno ) );
        }
    }

    return _routeSock;
}

int AfRouteControl::getUdpSocket ( IpAddress::AddressType type )
{
    if ( type == IpAddress::V6Address )
    {
        if ( _v6Sock < 0 )
        {
            _v6Sock = ::socket ( AF_INET6, SOCK_DGRAM, 0 );

            if ( _v6Sock < 0 )
            {
                LOG ( L_ERROR, "Unable to create IPv6 UDP socket: " << ::strerror ( errno ) );
            }
        }

        return _v6Sock;
    }

    // we default to v4 for unknown types
    if ( _v4Sock < 0 )
    {
        _v4Sock = ::socket ( AF_INET, SOCK_DGRAM, 0 );

        if ( _v4Sock < 0 )
        {
            LOG ( L_ERROR, "Unable to create IPv4 UDP socket: " << ::strerror ( errno ) );
        }
    }

    return _v4Sock;
}

ERRCODE AfRouteControl::getLinksAndAddresses (
        List<AfRouteTypes::Link> & links,
        List<AfRouteTypes::Address> & addrs )
{
    // clang complains about this being const, so removed for now
    static int mib[ MIB_SIZE ] = { CTL_NET, AF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };

    List<AfRouteTypes::Route> routes; // unused

    return doSysctlRequest ( mib, MIB_SIZE, links, addrs, routes );
}

ERRCODE AfRouteControl::getRoutes ( List<AfRouteTypes::Route> & routes )
{
    // clang complains about this being const, so removed for now
    static int mib[ MIB_SIZE ] = { CTL_NET, AF_ROUTE, 0, 0, NET_RT_DUMP, 0 };

    List<AfRouteTypes::Link> links; // unused
    List<AfRouteTypes::Address> addrs; // unused

    return doSysctlRequest ( mib, MIB_SIZE, links, addrs, routes );
}

ERRCODE AfRouteControl::addIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask )
{
    LOG ( L_DEBUG, "Trying to add address: " << addr << "/" << mask << "; ifaceId: " << ifaceId );

    char ifName[ IFNAMSIZ ];
    memset ( ifName, 0, IFNAMSIZ );

    // This function will not fill in ifName with >IFNAMSIZ bytes, and null terminates ifName
    if ( !if_indextoname ( ifaceId, ifName ) )
    {
        LOG ( L_ERROR, "Unable to set address of iface ID " << ifaceId << " due to error " << strerror ( errno ) );
        return Error::InvalidParameter;
    }

    struct ifreq ifr;
    memset ( &ifr, 0, sizeof ( ifr ) );

    assert ( sizeof ( ifr.ifr_name ) <= sizeof ( ifName ) );

    strncpy ( ifr.ifr_name, ifName, sizeof ( ifr.ifr_name ) );

    const SockAddr sockAddr = addr.getSockAddr ( 0 );
    memcpy ( &( ifr.ifr_addr ), &( sockAddr.sa ), sockAddr.getSocklen() );

    int fd = getUdpSocket ( addr.getAddrType() );

    if ( fd < 0 )
    {
        LOG ( L_ERROR, "Error getting socket for ioctl call" );
        return Error::SocketFailed;
    }

    if ( ioctl ( fd, SIOCSIFADDR, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCSIFADDR: " << strerror ( errno ) << " with addr: " << addr.toString()
              << "/" << mask << " on iface ID " << ifaceId );
        return Error::IoctlFailed;
    }

    const SockAddr maskAddr = addr.getNetmaskAddress ( mask ).getSockAddr ( 0 );
    memcpy ( &( ifr.ifr_addr ), &( maskAddr.sa ), maskAddr.getSocklen() );

    if ( ioctl ( fd, SIOCSIFNETMASK, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCSIFNETMASK: " << strerror ( errno ) << " with addr: " << addr.toString()
              << "/" << mask << " on iface ID " << ifaceId );
        return Error::IoctlFailed;
    }

    return Error::Success;
}

ERRCODE AfRouteControl::removeIfaceAddress ( int ifaceId, const IpAddress & addr )
{
    LOG ( L_DEBUG, "Trying to remove address: " << addr << "; ifaceId: " << ifaceId );

    char ifName[ IFNAMSIZ ];
    if ( !if_indextoname ( ifaceId, ifName ) )
    {
        LOG ( L_ERROR, "Unable to set address of iface ID " << ifaceId << " due to error " << strerror ( errno ) );
        return Error::InvalidParameter;
    }

    struct ifreq ifr;
    memset ( &ifr, 0, sizeof ( ifr ) );

    assert ( sizeof ( ifr.ifr_name ) == sizeof ( ifName ) );

    strncpy ( ifr.ifr_name, ifName, sizeof ( ifr.ifr_name ) );

    SockAddr sockAddr = addr.getSockAddr ( 0 );
    memcpy ( &( ifr.ifr_addr ), &( sockAddr.sa ), sockAddr.getSocklen() );

    int fd = getUdpSocket ( addr.getAddrType() );

    if ( fd < 0 )
    {
        LOG ( L_ERROR, "Error getting new socket for ioctl call" );
        return Error::SocketFailed;
    }

    if ( ioctl ( fd, SIOCDIFADDR, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCDIFADDR: " << strerror ( errno ) << " with addr: " << addr.toString()
              << " on iface ID " << ifaceId );
        return Error::IoctlFailed;
    }

    return Error::Success;
}

ERRCODE AfRouteControl::addRoute ( const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId )
{
    LOG ( L_DEBUG, "Trying to add route. Dst: " << dst << "/" << mask
          << " via gateway: " << gw << "; ifaceId: " << ifaceId );

    // When adding an IP, we have to make sure that we're telling it to go somewhere
    if ( ( !gw.isValid() || gw.isZero() ) && ifaceId < 0 )
    {
        LOG ( L_ERROR, "Missing both gateway and ifaceId" );
        return Error::InvalidParameter;
    }

    return doRouteRequest ( RTM_ADD, dst, mask, gw, ifaceId );
}

ERRCODE AfRouteControl::removeRoute ( const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId )
{
    LOG ( L_DEBUG, "Trying to remove route. Dst: " << dst << "/" << mask
          << " via gateway: " << gw << "; ifaceId: " << ifaceId );

    return doRouteRequest ( RTM_DELETE, dst, mask, gw, ifaceId );
}

ERRCODE AfRouteControl::setIfaceMtu ( int ifaceId, int mtu )
{
    char ifName[ IFNAMSIZ ];
    if ( !if_indextoname ( ifaceId, ifName ) )
    {
        LOG ( L_ERROR, "Unable to set MTU of iface ID " << ifaceId << " due to error " << strerror ( errno ) );
        return Error::InvalidParameter;
    }

    struct ifreq ifr;
    memset ( &ifr, 0, sizeof ( ifr ) );

    assert ( sizeof ( ifr.ifr_name ) == sizeof ( ifName ) );

    strncpy ( ifr.ifr_name, ifName, sizeof ( ifr.ifr_name ) );

    ifr.ifr_mtu = mtu;

    int fd = getUdpSocket();
    if ( fd < 0 )
    {
        LOG ( L_ERROR, "Error getting socket for ioctl call" );
        return Error::SocketFailed;
    }

    if ( ioctl ( fd, SIOCSIFMTU, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCSIFMTU: " << strerror ( errno ) << " on ifaceId " << ifaceId
              << " with MTU " << mtu );
        return Error::IoctlFailed;
    }

    return Error::Success;
}

ERRCODE AfRouteControl::setIfaceState ( int ifaceId, bool isUp )
{
    char ifName[ IFNAMSIZ ];

    if ( !if_indextoname ( ifaceId, ifName ) )
    {
        LOG ( L_ERROR, "Unable to set state of iface ID " << ifaceId << " due to error " << strerror ( errno ) );
        return Error::InvalidParameter;
    }

    struct ifreq ifr;
    memset ( &ifr, 0, sizeof ( ifr ) );

    assert ( sizeof ( ifr.ifr_name ) == sizeof ( ifName ) );

    strncpy ( ifr.ifr_name, ifName, sizeof ( ifr.ifr_name ) );

    int fd = getUdpSocket();
    if ( fd < 0 )
    {
        LOG ( L_ERROR, "Error getting socket for ioctl call" );
        return Error::SocketFailed;
    }

    if ( ioctl ( fd, SIOCGIFFLAGS, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCGIFFLAGS: " << strerror ( errno ) << " on ifaceId " << ifaceId
              << " to state " << ( isUp ? "true" : "false" ) );
        return Error::IoctlFailed;
    }

    if ( isUp )
    {
        ifr.ifr_flags |= ( IFF_UP | IFF_RUNNING );
    }
    else
    {
        ifr.ifr_flags &= ( ~( IFF_UP | IFF_RUNNING ) );
    }

    if ( ioctl ( fd, SIOCSIFFLAGS, ( void * ) &ifr ) < 0 )
    {
        LOG ( L_ERROR, "Error calling SIOCSIFFLAGS: " << strerror ( errno ) << " on ifaceId " << ifaceId
              << " to state " << ( isUp ? "true" : "false" ) );
        return Error::IoctlFailed;
    }

    return Error::Success;
}

ERRCODE AfRouteControl::doSysctlRequest (
        int mib[], size_t mibSize,
        List<AfRouteTypes::Link> & links,
        List<AfRouteTypes::Address > & addrs,
        List<AfRouteTypes::Route > & routes )
{
    routes.clear();
    addrs.clear();
    links.clear();

    size_t payloadSize = 0;

    int ret = ::sysctl ( mib, mibSize, 0, &payloadSize, 0, 0 );

    if ( ret != 0 )
    {
        LOG ( L_ERROR, "Unable to invoke sysctl to determine buffer size for interface list dump: "
              << ::strerror ( errno ) );
        return Error::SysctlFailed;
    }

    RwBuffer payload;

    // We don't increase the payload size the first time because the payload size returned by sysctl has already
    // been increased by a margin to decrease the chances of the sysctl call failing due to a lack of memory.
    for ( size_t i = 0; i < SYSCTL_MEMORY_RETRY_COUNT; ++i )
    {
        char * payloadData = payload.getAppendable ( payloadSize );

        if ( !payloadData )
        {
            LOG ( L_ERROR, "Unable to allocate payload data" );
            return Error::MemoryError;
        }

        ret = ::sysctl ( mib, MIB_SIZE, payloadData, &payloadSize, 0, 0 );

        // Either we succeeded in the call, or we have encountered a non-memory related error
        // (we handle memory errors by retrying with more memory), so we should break out to handle this condition.
        if ( ret == 0 || ( ret == -1 && errno != ENOMEM ) )
        {
            break;
        }

        // sysctl only returns 0 or -1, so if we see another value here our above code is wrong
        assert ( ret == -1 );

        // We add a 50% buffer, in case the system changes in between calls to sysctl.
        payloadSize += 1 + ( payloadSize / 2 );
    }

    if ( ret != 0 )
    {
        LOG ( L_ERROR, "Unable to invoke sysctl to dump the interface list: " << ::strerror ( errno ) );
        return Error::SysctlFailed;
    }

    payload.markAppended ( payloadSize );

    ERRCODE eCode = RouteParser::processBuffer ( payload, links, addrs, routes );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Unexpected error when processing route buffer" );
        return Error::InternalError;
    }

    return Error::Success;
}

ERRCODE AfRouteControl::doRouteRequest (
        const int requestType,
        const IpAddress & dst, uint8_t mask,
        const IpAddress & gw, int ifaceId )
{
    if ( !dst.isValid() )
    {
        LOG ( L_ERROR, "Invalid destination IP" );
        return Error::InvalidParameter;
    }

    if ( gw.isValid() && dst.getAddrType() != gw.getAddrType() )
    {
        LOG ( L_ERROR, "Mismatch between address types of dst (" << dst << ") and gw (" << gw << ")" );
        return Error::InvalidParameter;
    }

    assert ( dst.isIPv4() || dst.isIPv6() );

    if ( ( dst.isIPv4() && mask > 32 ) || ( mask > 128 ) )
    {
        LOG ( L_ERROR, "Invalid mask (" << mask << ") provided for address" );
        return Error::InvalidParameter;
    }

    if ( gw.isValid() && !gw.isZero() && ifaceId >= 0 )
    {
        LOG ( L_WARN, "Route has both gateway (" << gw << ") and ifaceId (" << ifaceId
              << "), ignoring ifaceId" );
        ifaceId = -1;
    }

    // This field is large enough to hold any-sized request we could make.
    // We will verify this through a series of asserts before memcpys to this object.
    char msg[ RT_MSG_SIZE ];
    memset ( msg, 0, RT_MSG_SIZE );

    assert ( RT_MSG_SIZE >= sizeof ( struct rt_msghdr ) );

    struct rt_msghdr * msgHdr = ( struct rt_msghdr * ) msg;
    msgHdr->rtm_msglen = sizeof ( struct rt_msghdr );
    msgHdr->rtm_version = RTM_VERSION;

    msgHdr->rtm_type = requestType;

    // rtm_flags describe the 'settings' of the route we are adding or removing (i.e. static, up, etc.)
    msgHdr->rtm_flags = RTF_UP | RTF_STATIC;

    msgHdr->rtm_seq = ++_rtmSeqNum;

    // rtm_addrs describes which fields are included as part of the request (i.e. which structs follow the header).
    msgHdr->rtm_addrs = RTA_DST | RTA_NETMASK;

    msgHdr->rtm_pid = getpid();

    const SockAddr dstSA = dst.getSockAddr ( 0 );
    const SockAddr maskSA = dst.getNetmaskAddress ( mask ).getSockAddr ( 0 );

    assert ( dstSA.getSocklen() == maskSA.getSocklen() );

    // The order of messages is important here!
    // It has to be in ascending order according to the value of the RTA constant.

    size_t msgIdx = sizeof ( struct rt_msghdr );

    assert ( RT_MSG_SIZE >= ( msgIdx + dstSA.getSocklen() ) );

    memcpy ( msg + msgIdx, &dstSA, dstSA.getSocklen() );
    msgIdx += dstSA.getSocklen();
    msgHdr->rtm_msglen += dstSA.getSocklen();

    if ( gw.isValid() && !gw.isZero() )
    {
        // We have a valid gateway IP, just use that

        // Set RTF_GATEWAY only if gw is a non-zero IP address
        msgHdr->rtm_flags |= RTF_GATEWAY;

        // Since we are appending a gateway field, we must set the bit
        msgHdr->rtm_addrs |= RTA_GATEWAY;

        const SockAddr gwSA = gw.getSockAddr ( 0 );

        assert ( dstSA.getSocklen() == gwSA.getSocklen() );
        assert ( RT_MSG_SIZE >= ( msgIdx + gwSA.getSocklen() ) );

        memcpy ( msg + msgIdx, &gwSA, gwSA.getSocklen() );
        msgIdx += gwSA.getSocklen();
        msgHdr->rtm_msglen += gwSA.getSocklen();
    }
    else if ( ifaceId >= 0 )
    {
        // We have a valid interface ID

        // Since we are appending a gateway field (via an interface), we must set the bit
        msgHdr->rtm_addrs |= RTA_GATEWAY;
        msgHdr->rtm_index = ifaceId;

        char ifName[ IFNAMSIZ ];
        if ( !::if_indextoname ( ifaceId, ifName ) )
        {
            LOG ( L_ERROR, "Cannot map ifaceId '" << ifaceId << "' to a name" );
            return Error::InvalidParameter;
        }

        struct sockaddr_dl sdl;
        memset ( &sdl, 0, sizeof ( sdl ) );
        sdl.sdl_len = sizeof ( sdl );
        sdl.sdl_family = AF_LINK;

        ::link_addr ( ifName, &sdl );

        // If this is 0, then we didn't find anything
        if ( sdl.sdl_alen < 1 )
        {
            LOG ( L_ERROR, "Cannot map ifaceName '" << ifName << "' to a system index" );
            return Error::InternalError;
        }

        assert ( RT_MSG_SIZE >= ( msgIdx + sizeof ( sdl ) ) );

        memcpy ( msg + msgIdx, &sdl, sizeof ( sdl ) );
        msgIdx += sizeof ( sdl );
        msgHdr->rtm_msglen += sizeof ( sdl );
    }

    assert ( RT_MSG_SIZE >= ( msgIdx + maskSA.getSocklen() ) );

    // Set RTF_HOST if this is a host route
    if ( ( dst.isIPv4() && mask == 32 ) || ( dst.isIPv6() && mask == 128 ) )
    {
        msgHdr->rtm_flags |= RTF_HOST;
    }

    memcpy ( msg + msgIdx, &maskSA, maskSA.getSocklen() );
    msgIdx += maskSA.getSocklen();
    msgHdr->rtm_msglen += maskSA.getSocklen();

    // We use this instead of _routeSock directly so that it will initialize the socket if it doesn't already exist
    int fd = getRouteSocket();

    if ( fd < 0 )
    {
        LOG ( L_ERROR, "Unable to create routing socket to add route" );
        return Error::SocketFailed;
    }

    int ret = ::write ( fd, msg, msgHdr->rtm_msglen );

    // If the route already exists, we have 'successfully' added it (end result is the route exists).
    if ( ret < 0 && errno == EEXIST )
    {
        return Error::Success;
    }

    // We should be able to write a full message without error; if we can't we reset the socket
    if ( ret < 0 || ret != msgHdr->rtm_msglen )
    {
        if ( ret < 0 )
        {
            LOG ( L_ERROR, "Unable to write routing message: " << ::strerror ( errno ) );
        }
        else if ( ret != msgHdr->rtm_msglen )
        {
            LOG ( L_ERROR, "Unable to fully write message to routing socket. Message was " << msgHdr->rtm_msglen
                  << " bytes but the socket only wrote " << ret << " bytes, resetting" );
        }
        ::close ( fd );
        // set to -1 so that next call to getRouteSock() will re-initialize the socket
        _routeSock = -1;
        return Error::WriteFailed;
    }

    return Error::Success;
}
