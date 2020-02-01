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

#include "NetlinkRouteControl.hpp"
#include "NetlinkMsgCreator.hpp"
#include "NetlinkRoute.hpp"

using namespace Pravala;

NetlinkRouteControl::NetlinkRouteControl(): NetlinkSyncSocket ( NetlinkCore::Route )
{
}

ERRCODE NetlinkRouteControl::getLinks ( List<NetlinkTypes::Link> & links )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    NetlinkMessage msg ( NetlinkMsgCreator::createRtmGetLink() );

    List<NetlinkMessage> response;
    struct nlmsgerr netlinkError;

    memset ( &netlinkError, 0, sizeof ( netlinkError ) );

    const ERRCODE eCode = execMessage ( msg, response, netlinkError );

    if ( NOT_OK ( eCode ) )
        return eCode;

    NetlinkRoute::RouteResults routeResults;

    for ( size_t i = 0; i < response.size(); ++i )
    {
        NetlinkRoute::parseRouteMessage ( response.at ( i ), routeResults );
    }

    LOG ( L_DEBUG2, "Read " << routeResults.links.size() << " links" );

    links = routeResults.links;

    return Error::Success;
}

ERRCODE NetlinkRouteControl::getAddresses ( List<NetlinkTypes::Address > & addresses, IpAddress::AddressType addrType )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    NetlinkMessage msg ( NetlinkMsgCreator::createRtmGetAddr ( addrType ) );

    List<NetlinkMessage> response;
    struct nlmsgerr netlinkError;

    memset ( &netlinkError, 0, sizeof ( netlinkError ) );

    const ERRCODE eCode = execMessage ( msg, response, netlinkError );

    if ( NOT_OK ( eCode ) )
        return eCode;

    NetlinkRoute::RouteResults routeResults;

    for ( size_t i = 0; i < response.size(); ++i )
    {
        NetlinkRoute::parseRouteMessage ( response.at ( i ), routeResults );
    }

    LOG ( L_DEBUG2, "Read " << routeResults.addresses.size() << " addresses" );

    addresses = routeResults.addresses;

    return Error::Success;
}

ERRCODE NetlinkRouteControl::getRoutes ( List< NetlinkTypes::Route > & routes, uint8_t rtTable )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    NetlinkMessage msg ( NetlinkMsgCreator::createRtmGetRoute ( rtTable ) );

    List<NetlinkMessage> response;
    struct nlmsgerr netlinkError;

    memset ( &netlinkError, 0, sizeof ( netlinkError ) );

    const ERRCODE eCode = execMessage ( msg, response, netlinkError );

    if ( NOT_OK ( eCode ) )
        return eCode;

    NetlinkRoute::RouteResults routeResults;

    for ( size_t i = 0; i < response.size(); ++i )
    {
        NetlinkRoute::parseRouteMessage ( response.at ( i ), routeResults );
    }

    LOG ( L_DEBUG2, "Returning messages to owner" );

    LOG ( L_DEBUG2, "Read " << routeResults.routes.size() << " routes" );

    routes = routeResults.routes;

    return Error::Success;
}

ERRCODE NetlinkRouteControl::addIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyIfaceAddr (
                PosixNetMgrTypes::ActionAdd,
                NLM_F_CREATE,
                addr,
                mask,
                ifaceId ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to add iface address: " << strerror ( -err ) );
    }

    return ret;
}

ERRCODE NetlinkRouteControl::removeIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyIfaceAddr (
                PosixNetMgrTypes::ActionRemove,
                NLM_F_CREATE,
                addr,
                mask,
                ifaceId ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to remove iface address: " << strerror ( -err ) );
    }

    return ret;
}

ERRCODE NetlinkRouteControl::addRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;

    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyRoute (
                PosixNetMgrTypes::ActionAdd,              // Operation
                NLM_F_CREATE | NLM_F_REPLACE,              // Flags
                dst,              // Address
                mask,              // Netmask length
                metric,              // Metric
                gw,              // Gateway
                ifaceId,               // Iface ID (Netlink's iface index)
                RTN_UNICAST,
                ( tableId >= 0 && tableId <= 0xFF )
                ? ( ( uint8_t ) tableId )
                : ( ( uint8_t ) RT_TABLE_MAIN )
            ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to add route: " << strerror ( -err ) );
    }

    return ret;
}

ERRCODE NetlinkRouteControl::removeRoute (
        const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId, int metric, int tableId )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;

    NetlinkMessage msg ( NetlinkMsgCreator::createRtmModifyRoute (
                PosixNetMgrTypes::ActionRemove,              // Operation
                0,              // Flags
                dst,              // Address
                mask,              // Netmask length
                metric,              // Metric
                gw,              // Gateway
                ifaceId,               // Iface ID (Netlink's iface index)
                RTN_UNICAST,
                ( tableId >= 0 && tableId <= 0xFF )
                ? ( ( uint8_t ) tableId )
                : ( ( uint8_t ) RT_TABLE_MAIN )
            ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to remove route: " << strerror ( -err ) );
    }

    return ret;
}

ERRCODE NetlinkRouteControl::setIfaceMtu ( int ifaceId, int mtu )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmSetIfaceMtu ( ifaceId, mtu ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to set iface MTU: " << strerror ( -err ) );
    }

    return ret;
}

ERRCODE NetlinkRouteControl::setIfaceState ( int ifaceId, bool isUp )
{
    if ( _sock < 0 )
        return Error::NotInitialized;

    if ( _family != NetlinkCore::Route )
        return Error::InvalidSocketType;

    int err = 0;
    NetlinkMessage msg ( NetlinkMsgCreator::createRtmSetIfaceState ( ifaceId, isUp ) );

    const ERRCODE ret = execMessage ( msg, &err );

    if ( NOT_OK ( ret ) )
    {
        LOG_ERR ( L_ERROR, ret, "Unable to set iface state: " << strerror ( -err ) );
    }

    return ret;
}
