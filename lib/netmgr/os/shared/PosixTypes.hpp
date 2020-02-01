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
#include <net/if.h>
}

#include "../../NetManagerTypes.hpp"

namespace Pravala
{
namespace PosixNetMgrTypes
{
/// @brief Whether we got an added message or a removed message
enum Action
{
    ActionUnknown = 0, ///< Unknown
    ActionAdd, ///< We got a message about a new object (like a link, address or a route), or we want to add it
    ActionRemove ///< We got a message about a removed route, or we want to send a remove route message
};

/// @brief The type of the address
enum AddressType
{
    AddrLocal = 0,  ///< Local address (e.g. address assigned to the interface)
    AddrPeerBroadcast ///< Peer (for PTP interfaces), or Broadcast (for broadcast interfaces)
};

/// @brief Information about an interface and its configuration on the system.
/// Implemented by RTM_{NEW,DEL,GET}LINK in Netlink; RTM_IFINFO in AF_ROUTE messages
struct Link: public NetManagerTypes::Interface
{
    /// @brief Whether link was added/removed.
    /// @note This is also used when the link changes its properties. It also doesn't necessarily mean
    ///        that the link is usable. isUp() and isRunning() should be used to get detailed information.
    Action act;

    /// @brief Sets platform-independent interface flags based on the Posix interface flags.
    /// @param [in] ifaceFlags The Posix flags to use.
    void setFlags ( unsigned int ifaceFlags )
    {
        flags = 0;

        if ( ( ifaceFlags & IFF_UP ) == IFF_UP )
            flags |= NetManagerTypes::Interface::FlagIsUp;

        if ( ( ifaceFlags & IFF_RUNNING ) == IFF_RUNNING )
            flags |= NetManagerTypes::Interface::FlagIsRunning;

        if ( ( ifaceFlags & IFF_LOOPBACK ) == IFF_LOOPBACK )
            flags |= NetManagerTypes::Interface::FlagIsLoopback;

        if ( ( ifaceFlags & IFF_POINTOPOINT ) == IFF_POINTOPOINT )
            flags |= NetManagerTypes::Interface::FlagIsPtp;
    }

    /// @brief Default constructor.
    Link(): act ( ActionUnknown )
    {
    }
};

/// @brief Information about an interface IP address
/// Implemented by RTM_{NEW,DEL,GET}ADDR in Netlink, RTM_{NEWADDR,DELADDR} in AF_ROUTE messages
struct Address: public NetManagerTypes::Address
{
    Action act; ///< whether address was added/removed

    uint8_t family; ///< Protocol family, e.g. AF_INET or AF_INET6
    uint8_t flags;  ///< Flags, see linux/if_addr.h IFA_F_*

    /// @brief Default constructor.
    Address(): act ( ActionUnknown ), family ( 0 ), flags ( 0 )
    {
    }
};

/// @brief RouteData with 'action' field that describes the operation
struct Route: public NetManagerTypes::Route
{
    Action act; ///< whether address was added/removed

    uint8_t family; ///< Protocol family, e.g. AF_INET or AF_INET6

    /// @brief Default constructor.
    Route(): act ( ActionUnknown ), family ( 0 )
    {
    }
};
}
}
