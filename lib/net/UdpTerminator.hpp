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
#include "config/ConfigNumber.hpp"
#include "net/IpFlow.hpp"
#include "log/TextLog.hpp"
#include "event/Timer.hpp"

namespace Pravala
{
class IpPacket;

/// @brief A UDP terminator that allows for handling UDP traffic incoming as IP packets over the tunnel interface.
/// It makes it possible to create UDP servers that operate using IP packets sent/received over the tunnel interface,
/// instead of using regular socket FDs.
/// @note When using IpFlow::packetReceived() call, it expects packets 'DefaultDescType' passed as the 'user data'.
///       Otherwise packets will be dropped. User pointer is always ignored.
class UdpTerminator: protected IpFlow, protected Timer::Receiver
{
    public:
        /// @brief The time of inactivity (in seconds) after the terminator will be removed.
        static ConfigLimitedNumber<uint32_t> optMaxInactivityTime;

        /// @brief IP address of this flow's client (the client sending the IP packets).
        const IpAddress ClientAddr;

        /// @brief IP address of this flow's destination (the server the client wanted to talk to).
        const IpAddress ServerAddr;

        /// @brief Port number of this flow's client (the client sending the IP packets).
        const uint16_t ClientPort;

        /// @brief Port number of this flow's destination (the server the client wanted to talk to).
        const uint16_t ServerPort;

        /// @brief Destructor.
        /// @note The UDP terminator in certain conditions will destroy itself by calling 'delete this'.
        ///       The class inheriting this must be able to handle it.
        virtual ~UdpTerminator();

        virtual String getLogId() const;
        virtual ERRCODE packetReceived ( IpPacket & packet, int32_t userData, void * userPtr );

    protected:
        static TextLogLimited _log; ///< Log stream.

        FixedTimer _timer; ///< Timer used for controlling inactivity.

        /// @brief Constructor.
        /// Constructor performs initialization, and also adds the terminator into the flow map.
        /// The caller should verify that everything succeeded, by calling isInitialized().
        /// If that function reports an error, this object should be deleted, since it's not a part of the flow map.
        /// @param [in] flowDesc FlowDesc object describing this flow.
        ///                      It MUST describe a TCPv4 or TCPv6 packet!
        UdpTerminator ( const FlowDesc & flowDesc );

        /// @brief Helper method that restarts the inactivity timer (if needed).
        void restartTimer();

        /// @brief Sends data as this streams UDP payload.
        /// @param [in] data The data to be sent. It is consumed.
        /// @return Standard error code.
        ERRCODE sendData ( MemHandle & data );

        /// @brief Receives (and consumes) data received over the UDP channel.
        /// The terminator MAY be destroyed inside this call.
        /// @param [in] data The data to receive.
        /// @return Standard error code.
        virtual ERRCODE receiveData ( MemVector & data ) = 0;

        /// @brief Sends IP packet to UDP client handled by this terminator.
        /// @param [in] packet The packet to write.
        /// @return Standard error code.
        virtual ERRCODE sendPacket ( const IpPacket & packet ) = 0;

        virtual void flowRemoved();
        virtual void timerExpired ( Timer * timer );
};
}
