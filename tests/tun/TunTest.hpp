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

#include "net/TunIface.hpp"
#include "event/Timer.hpp"
#include "config/ConfigIpAddress.hpp"
#include "config/ConfigAddrSpec.hpp"
#include "config/ConfigNumber.hpp"
#include "net/IpPacket.hpp"

namespace Pravala
{
class TunTest: public TunIfaceOwner, public Timer::Receiver
{
    public:
        /// @brief IP address of the tunnel
        static ConfigIpAddress optTunIpAddr;

        /// @brief IP address/port to send junk to
        static ConfigAddrSpec optDestIpPort;

        /// @brief Time to wait between sending packets (ms)
        static ConfigNumber<uint16_t> optSendDelay;

        /// @brief The number of tunnel interfaces.
        static ConfigLimitedNumber<uint16_t> optNumIfaces;

        TunTest();

        virtual ~TunTest();

        bool start();

        static String getTunDesc ( TunIface * iface );

    protected:
        List<TunIface *> _tuns;

        SimpleTimer _timer;

        static TextLog _log;

        IpPacket _pkt;

        virtual void tunIfaceRead ( TunIface * iface, TunIpPacket & packet );
        virtual void tunIfaceClosed ( TunIface * iface );
        virtual void tunIfaceRateUpdate (
            TunIface * iface, uint32_t sendRate, uint32_t rcvRate,
            const Time & sinceTime );

        virtual void timerExpired ( Timer * timer );
};
}
