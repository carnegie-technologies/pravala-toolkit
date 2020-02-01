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
#include "basic/List.hpp"
#include "basic/NoCopy.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "event/Timer.hpp"
#include "event/EventManager.hpp"
#include "net/IpPacket.hpp"

struct netif;
struct pbuf;
struct ip4_addr;
typedef ip4_addr ip4_addr_t;

struct ip6_addr;
typedef ip6_addr ip6_addr_t;
typedef int8_t err_t;

namespace Pravala
{
class IpPacket;

/// @brief Wrapper class for an lwIP interface.
///
/// This class can be used to create an lwIP interface that allows sending and receiving IP packets
/// to and from the lwIP stack.
///
/// @note This class is not thread-safe.
/// @note This interface should not be deallocated as the result of a callback from lwIP.
class LwipInterface: public NoCopy, protected EventManager::LoopEndEventHandler
{
    public:
        /// @brief Constructor.
        LwipInterface();

        /// @brief Destructor. This calls stop()
        ~LwipInterface();

        /// @brief Initialize the lwIP interface.
        /// If there is no default lwIP interface, this will be set as the default.
        /// @return Standard error code
        ERRCODE init();

        /// @brief Stop and remove the underlying lwIP interface.
        /// If this was the default lwIP interface, a new one will be set if available.
        void stop();

        /// @brief Marks this interface as the default lwIP interface.
        /// This will replace the previous default interface if already set.
        void markDefault();

        /// @brief Check if this interface is the default lwIP interface.
        /// @return True if this interface is the default lwIP interface.
        bool isDefault() const;

        /// @brief Set the IPv4 address of this interface. lwIP only supports one IPv4 interface address.
        /// @param addr The IPv4 address to set.
        /// If an non-IPv4 address is provided, the IPv4 interface address is cleared.
        void setIpV4Address ( const IpAddress & addr );

        /// @brief Set the IPv6 addresses of this interface. Supports up to LWIP_IPV6_NUM_ADDRESSES.
        /// @param addr The IPv6 addresses to set. Any non-IPv6 address in the list are ignored.
        /// Providing an empty list, or a list containing all non-IPv6, will clear the IPv6 interface addresses.
        void setIpV6Addresses ( const List<IpAddress> & addrs );

        /// @brief Checks whether this interface is up.
        /// The interface is up when it has at at least one IPv4 or IPv6 interface address configured
        /// @return True if this interface is up.
        bool isUp() const;

        /// @brief Sends an IP packet into the lwIP stack.
        /// Packets sent by this method will be injected into lwIP stack through the
        /// underlying lwIP interface, acting as an incoming interface.
        /// IP packets may not be passed into the stack immediately.
        /// IP packets will eventually be injected into the lwIP stack for further processing.
        /// @param [in] srcAddr Source address this packet came from.
        /// @param [in] dstAddr Destination address for this packet.
        /// @param [in] packet The IP packet to send.
        /// @return Standard error code.
        ERRCODE sendPacket ( const IpPacket & packet );

    protected:
        virtual void receiveLoopEndEvent();

        /// @brief Called when a packet is received by the interface.
        /// @param [in,out] mh MemHandle containing the data of the IP packet that was received.
        ///                    This MemHandle starts at the beginning of the IP packet and does not contain
        ///                    any platform specific headers. This buffer might be cleared by this function.
        /// @param [in] tag The tag to be set on constructed IpPacket.
        virtual void packetReceived ( MemHandle & mh, int32_t tag = 0 ) = 0;

    private:
        static TextLog _log; ///< Log stream

        netif * _interface; ///< The underlying lwIP network interface that is used to receive and output packets

        bool _hasV4Addr; ///< If this interface has an IPv4 address configured

        size_t _numV6Addrs; ///< The number of IPv6 address this interface has configured

        /// @brief Packets that are pending input into the lwIP stack.
        /// This queue will be injected into the lwIP stack at loop-end then emptied.
        List<IpPacket> _inputQueue;

        /// @brief Update the state for this interface depending on the IP address(es) configured on it.
        /// This interface is up if it has at at least one IPv4 or IPv6 interface address.
        void updateIfaceState();

        /// @brief Immediately injects an IP packet into the lwIP stack.
        /// @note LwipEventPoller::checkLwipTimer() should be called after calling this!
        /// @param [in] packet The IP packet to inject.
        /// @return Standard error code.
        ERRCODE injectPacket ( const IpPacket & packet );

        /// @brief Output an IP packet.
        /// The data is passed to packetReceived() call.
        /// @param buffer The IP packet buffer.
        /// @return lwIP error code.
        err_t outputIpPacket ( pbuf * buffer );

        /// @brief Called by lwIP when the interface should be initialized. This is an lwIP callback.
        /// lwIP requires this function when adding the interface, but it doesn't actually need to do anything,
        /// it just needs to return OK.
        /// @param interface The lwIP network interface that should be initialized
        /// @return lwIP error code, always returns ERR_OK
        static err_t ifaceInitFunc ( netif * interface );

        /// @brief Called by lwIP when the interface should output an IPv4 packet. This is an lwIP callback.
        /// @param interface The lwIP network interface that should output the packet
        /// @param buffer The packet buffer
        /// @param ipaddr The destination IP address
        /// @return lwIP error code
        static err_t ifaceIPv4OutputFunc ( netif * interface, pbuf * buffer, const ip4_addr_t * ipaddr );

        /// @brief Called by lwIP when the interface should output an IPv6 packet. This is an lwIP callback.
        /// @param interface The lwIP network interface that should output the packet
        /// @param buffer The packet buffer
        /// @param ipaddr The destination IP address
        /// @return lwIP error code
        static err_t ifaceIPv6OutputFunc ( netif * interface, pbuf * buffer, const ip6_addr_t * ipaddr );

        /// @brief Map an lwIP error to an ERRCODE
        /// @param error lwIP error
        /// @return Standard error code
        static ERRCODE mapLwipErrorToERRCODE ( err_t error );
};
}
