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

#include "basic/IpAddress.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "AfRouteTypes.hpp"

namespace Pravala
{
/// @brief Performs various network operations.
/// It's not really AF_ROUTE-related, but it's specific to platforms with AF_ROUTE.
class AfRouteControl
{
    public:
        /// @brief Constructor.
        AfRouteControl();

        /// @brief Destructor.
        ~AfRouteControl();

        /// @brief Adds an address to a interface.
        /// @param [in] ifaceId The ID of the interface to add the address to.
        /// @param [in] addr The address to add.
        /// @param [in] mask Mask of the address to use.
        /// @return Standard error code.
        ERRCODE addIfaceAddress ( int ifaceId, const IpAddress & addr, uint8_t mask );

        /// @brief Removes an address from a interface.
        /// @param [in] ifaceId The ID of the interface to add the address to.
        /// @param [in] addr The address to removed.
        /// @return Standard error code.
        ERRCODE removeIfaceAddress ( int ifaceId, const IpAddress & addr );

        /// @brief Adds a system route.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use.
        /// @param [in] ifaceId Interface ID to use.
        /// @return Standard error code.
        ERRCODE addRoute ( const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId );

        /// @brief Removes a system route.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use.
        /// @param [in] ifaceId Interface ID to use.
        /// @return Standard error code.
        ERRCODE removeRoute ( const IpAddress & dst, uint8_t mask, const IpAddress & gw, int ifaceId );

        /// @brief Sets interface's MTU value.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] mtu MTU value to use.
        /// @return Standard error code.
        ERRCODE setIfaceMtu ( int ifaceId, int mtu );

        /// @brief Sets interface's state.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] isUp Whether the interface should be "UP" (true), or "DOWN" (false).
        /// @return Standard error code.
        ERRCODE setIfaceState ( int ifaceId, bool isUp );

        /// @brief Reads all the links and their addresses from the OS.
        /// @param [out] links The list of links read. It is cleared first.
        /// @param [out] addrs The list of addresses read. It is cleared first.
        /// @return Standard error code.
        ERRCODE getLinksAndAddresses (
            List<AfRouteTypes::Link> & links,
            List<AfRouteTypes::Address> & addrs );

        /// @brief Reads all the routes from the OS.
        /// @param [out] routes The list of routes read. It is cleared first.
        /// @return Standard error code.
        ERRCODE getRoutes ( List<AfRouteTypes::Route> & routes );

        /// @brief Execute a system route request
        /// @param [in] requestType Type of request (e.g. RTM_ADD, RTM_DELETE)
        /// @param [in] dst Destination IP address
        /// @param [in] mask Destination IP mask (up to 32 for IPv4 dst, up to 128 for IPv6 dst)
        /// @param [in] gw Gateway address (invalid or zero for a link route)
        /// @param [in] ifaceId Iface id (ignored if gw is valid and non-zero, otherwise used for link routes)
        /// @return standard error code
        ERRCODE doRouteRequest (
            const int requestType,
            const IpAddress & dst, uint8_t mask,
            const IpAddress & gw, int ifaceId );

        /// @brief Execute a system sysctl request using the provided MIB.
        /// This parses the result using RouteParser; only invoke for things structured like route messages.
        /// @param [in] mib The array of int values to pass to the systctl function
        /// @param [in] mibSize The number of entries in the mib array.
        /// @param [out] links The parsed links. Will be cleared.
        /// @param [out] addrs The parsed addresses. Will be cleared.
        /// @param [out] routes The parsed route entries. Will be cleared.
        /// @return standard error code
        ERRCODE doSysctlRequest (
            int mib[], size_t mibSize,
            List<AfRouteTypes::Link> & links,
            List<AfRouteTypes::Address> & addrs,
            List<AfRouteTypes::Route> & routes );

        /// @brief Get a UDP socket for use in ioctl calls.
        /// Do not bind or use this socket for non-ioctl operations. Do not manually close.
        /// @param [in] type The address family for the socket (v4 or v6). Defaults to v4 if not specified.
        /// @return file descriptor, or -1 on error
        int getUdpSocket ( IpAddress::AddressType type = IpAddress::V4Address );

        /// @brief Get a routing socket for use in ioctl calls.
        /// Do not use this socket for non-ioctl operations. Do not manually close.
        /// @return file descriptor, or -1 on error
        int getRouteSocket();

    private:
        static TextLog _log; ///< Log stream.
        int _routeSock; ///< File descriptor of a created routing socket
        int _v4Sock; ///< File descriptor of a created V4 datagram socket
        int _v6Sock; ///< File descriptor of a created V6 datagram socket

        int _rtmSeqNum; ///< The last used sequence number for the route request.
};
}
