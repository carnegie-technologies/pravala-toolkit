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

#ifndef ENABLE_VHOSTNET
#error "TunIfaceVhostNet cannot be included when ENABLE_VHOSTNET is not defined"
#endif

#include "basic/MemHandle.hpp"
#include "config/ConfigNumber.hpp"
#include "socket/os/shared/vhostnet/VhostNet.hpp"
#include "../../TunIfaceDev.hpp"

namespace Pravala
{
/// @brief Tunnel implementation using vhost-net
class TunIfaceVhostNet: public TunIfaceDev, public VhostNetOwner
{
    public:
        /// @brief True to enable using vhost-net for Tun, false otherwise.
        static ConfigNumber<bool> optEnableTunVhostNet;

    protected:
        virtual ERRCODE setupFd ( int fd );
        virtual void stop();

        virtual ERRCODE sendPacket ( const IpPacket & packet );

        virtual void vhostPacketReceived ( VhostNet * vn, MemHandle & pkt );
        virtual void vhostNetClosed ( VhostNet * vn );

        virtual void configureMemPool ( int ifaceMtu );

    private:
        /// @brief VhostNet object to perform I/O using vhost-net.
        /// This must be set to use vhost-net.
        VhostNet * _vh;

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TunIfaceVhostNet ( TunIfaceOwner * owner );

        /// @brief Destructor
        virtual ~TunIfaceVhostNet();

        friend class TunIfaceDev;
};
}
