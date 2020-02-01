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

#include "event/EventManager.hpp"
#include "event/Timer.hpp"
#include "../../WifiMgrMonitor.hpp"

#include "WpaSuppCore.hpp"

namespace Pravala
{
class WifiMgrMonitorPriv: public WpaSuppCore, public EventManager::FdEventHandler, public Timer::Receiver
{
    public:
        /// @brief Constructor
        /// @param [in] owner The monitor to notify when things arrive
        /// @param [in] ctrlInfo The control socket to use
        WifiMgrMonitorPriv ( WifiMgrMonitor & owner, const String & ctrlInfo );

        /// @brief Destructor
        ~WifiMgrMonitorPriv();

    protected:
        /// @brief Handles read or write events on the socket
        /// @param [in] fd File descriptor the event occurred on
        /// @param [in] events Bitmask of all detected events
        void receiveFdEvent ( int fd, short events );

        /// @brief Handles timer events
        /// @param [in] timer The timer event
        void timerExpired ( Timer * timer );

        /// @brief Clean up and reset
        virtual void reset();

    private:
        /// @brief Tries to connect; otherwise sets a timer.
        void restart();

        WifiMgrMonitor & _owner; ///< The monitor to call back to.

        SimpleTimer _reconnTimer; ///< The reconnect timer.

        SimpleTimer _disconnectCbTimer; ///< Timer to delay the disconnect callback
        SimpleTimer _scanResultsCbTimer; ///< Timer to delay the scan results callback
};
}
