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

extern "C"
{
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
}

#include "basic/List.hpp"

#include "NetlinkTypes.hpp"
#include "NetlinkMessage.hpp"

namespace Pravala
{
/// @brief Data types and helper function for dealing with NETLINK_ROUTE message family.
namespace NetlinkRoute
{
/// @brief Contains a set of "route" results from Netlink.
struct RouteResults
{
    List<NetlinkTypes::Link> links; ///< The list of link updates received.
    List<NetlinkTypes::Address> addresses; ///< The list of address updates received.
    List<NetlinkTypes::Route> routes; ///< The list of route updates received.

    /// @brief Clears the content of the structure.
    inline void clear()
    {
        links.clear();
        addresses.clear();
        routes.clear();
    }
};

/// @brief Processes a netlink message part as NETLINK_ROUTE data.
/// @param [in] msg The message to process.
/// @param [in,out] result Processed result. Data is appended to it.
/// @return If the message was recognized and parsed; False otherwise.
bool parseRouteMessage ( const NetlinkMessage & msg, RouteResults & result );

/// @brief Parse a RTM LINK message and add it to links (if the message is ok)
/// @param [in] hdr Pointer to the header of the Netlink message (the actual message is expected
/// to exist after the header)
/// @param [out] links The list of link data to append the link read from the message to
/// @return True if it succeeded, false if the message was incorrect
bool parseRtmLink ( const nlmsghdr * hdr, List<NetlinkTypes::Link> & links );

/// @brief Parse a RTM ADDR message and add it to addresses (if the message is ok)
/// @param [in] hdr Pointer to the header of the Netlink message (the actual message is expected
/// to exist after the header)
/// @param [out] addresses The list of addresses to append the address read from the message to
/// @return True if it succeeded, false if the message was incorrect
bool parseRtmAddr ( const nlmsghdr * hdr, List<NetlinkTypes::Address> & addresses );

/// @brief Parse a RTM ROUTE message and add it to routes (if the message is ok)
/// @param [in] hdr Pointer to the header of the Netlink message (the actual message is expected
/// to exist after the header)
/// @param [out] addresses The list of routes to append the address read from the message to
/// @return True if it succeeded, false if the message was incorrect
bool parseRtmRoute ( const nlmsghdr * hdr, List<NetlinkTypes::Route> & routes );

/// @brief Converts an RTA into an IpAddress
/// @param [in] rta Pointer to the rta struct
/// @param [in] family Protocol family
/// @param [out] ip Variable that will be updated with the IP address
/// @return true if successful, false if not
bool rtaToIpAddress ( const struct rtattr * rta, uint8_t family, IpAddress & ip );

/// @brief Converts an RTA into an int
/// @param [in] rta Pointer to the rta struct
/// @param [out] val Variable that will be updated with the int
/// @return true if successful, false if not
bool rtaToInt ( const rtattr * rta, int & val );

/// @brief Converts an RTA into an uint32_t
/// @param [in] rta Pointer to the rta struct
/// @param [out] val Variable that will be updated with the uint32_t
/// @return true if successful, false if not
bool rtaToUint32 ( const rtattr * rta, uint32_t & val );

/// @brief Locate the start of RTA given ancillary message header
/// @param [in] amhdr pointer to ancillary message header
/// @param [in] amhdrLen length of the ancillary message header
/// @return Pointer to the first rtattr
inline const struct rtattr * getMsgRta ( const char * amhdr, int amhdrLen )
{
    assert ( amhdr != 0 );
    assert ( amhdrLen > 0 );

    return ( const struct rtattr * ) ( amhdr + NLMSG_ALIGN ( amhdrLen ) );
}
}
}
