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

#include "../../GpsMonitor.hpp"

#include "event/EventManager.hpp"
#include "log/TextLog.hpp"
#include "event/SimpleSocket.hpp"

namespace Pravala
{
/// @brief gpsd-specific implementation of the GPS monitor interface.
/// We are either connected & receiving updates or we are not connected;
/// we don't support being connected to gpsd but not receiving updates.
/// There isn't a current need to be connected but not receiving updates.
class GpsMonitorPriv: public NoCopy, protected EventManager::FdEventHandler
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The owning GPS monitor object.
        GpsMonitorPriv ( GpsMonitor & owner );

        /// @brief Start listening for updates from gpsd.
        /// @return true on success; false on error (gpsd not available, incorrect connection info, etc.)
        bool start();

        /// @brief Stop listening for updates.
        void stop();

    protected:
        /// @brief Receives events on the file descriptor open to the gpsd socket
        /// @param [in] fd The file descriptor the event occurred on; should match _fd
        /// @param [in] events Bitmask of received events; both read and write (messages are sent & received).
        virtual void receiveFdEvent ( int fd, short events );

    private:
        static TextLog _log; ///< Log

        GpsMonitor & _owner; ///< The object to notify about changes.

        SimpleSocket _sock; ///< The connection to the gpsd daemon

        RwBuffer _toWrite; ///< The data which is to be written; empty if there is no pending data to write

        RwBuffer _toProcess; ///< The data which has been read from the socket but hasn't yet been processed

        /// @brief Send a WATCH command to start or stop updates from gpsd.
        /// @param [in] isActive If true, request updates be sent; if false, request updates be stopped.
        void sendWatchCommand ( bool isActive );

        /// @brief Process the data received from gpsd.
        void processData();
};
}
