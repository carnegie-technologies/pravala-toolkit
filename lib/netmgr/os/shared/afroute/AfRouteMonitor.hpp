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

#include "basic/Buffer.hpp"
#include "log/TextLog.hpp"
#include "event/EventManager.hpp"
#include "AfRouteTypes.hpp"

namespace Pravala
{
/// @brief AfRouteMonitor for receiving asynchronous events from AF_ROUTE.
class AfRouteMonitor: public EventManager::FdEventHandler
{
    public:
        /// @brief To be implemented by the owner of the AfRouteMonitor.
        class Owner
        {
            public:
                /// @brief Called whenever AfRouteMonitor receives some updates.
                /// @param [in] monitor The monitor that generated this callback.
                /// @param [in] links The list of link updates received from the OS.
                /// @param [in] addrs The list of address updates received from the OS.
                /// @param [in] routes The list of route updates received from the OS.
                virtual void afRouteMonUpdate (
                    AfRouteMonitor * monitor,
                    List<AfRouteTypes::Link> & links,
                    List<AfRouteTypes::Address> & addrs,
                    List<AfRouteTypes::Route> & routes ) = 0;

                /// @brief Destructor.
                virtual ~Owner();
        };

        /// @brief Creates a new AF_ROUTE file descriptor and listens for updates.
        /// @param [in] owner The object to notify about routing messages.
        AfRouteMonitor ( Owner & owner );

        /// @brief Destructor.
        ~AfRouteMonitor();

    protected:
        void receiveFdEvent ( int fd, short events );

    private:
        static TextLog _log; ///< Log stream

        Owner & _owner; ///< The owner

        int _sock; ///< The routing socket file descriptor

        RwBuffer _sockData; ///< The unparsed data read from the socket.

        /// @brief Close the current connection to the routing socket and reconnect.
        void reset();
};
}
