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

#include "../PosixEventManager.hpp"

namespace Pravala
{
/// @brief LibEvent version of Event Manager.
/// @warning This implementation is thread UNSAFE at the moment!
class LibEventManager: public PosixEventManager
{
    protected:

        /// @brief Groups all libevent signal event structures together
        struct
        {
            struct event sigChld; ///< Required by libevent for receiving SIGCHLD signals.
            struct event sigInt; ///< Required by libevent for receiving SIGINT signals.
            struct event sigTerm; ///< Required by libevent for receiving SIGTERM signals.
            struct event sigPipe; ///< Required by libevent for receiving SIGPIPE signals.
            struct event sigHup; ///< Required by libevent for receiving SIGHUP signals.
            struct event sigUsr1; ///< Required by libevent for receiving SIGUSR1 signals.
            struct event sigUsr2; ///< Required by libevent for receiving SIGUSR2 signals.
        } _sigEvents;

        /// @brief Base for libevent operations.
        struct event_base * _eventBase;

        /// @brief Constructor.
        /// @param [in] eventBase Base for libevent operations.
        LibEventManager ( struct event_base * eventBase );

        /// @brief Destructor
        ~LibEventManager();

        virtual void implRun();
        virtual void implSetFdHandler ( int fd, FdEventHandler * handler, int events );
        virtual void implSetFdEvents ( int fd, int events );
        virtual void implRemoveFdHandler ( int fd );

        /// @brief Handles events from libevent and dispatches to FdEventInfo.handler
        ///
        /// @param [in] fd file descriptor of event
        /// @param [in] event bit sum of events
        /// @param [in] arg event argument - not used at the moment
        static void fdEventHandlerCallback ( int fd, short event, void * arg );

        /// @brief Signal handler for events from libevent
        ///
        /// @param [in] sigNum signal number
        /// @param [in] event event type (should be EV_SIGNAL)
        /// @param [in] arg event argument (not used)
        static void evSigHandler ( int sigNum, short event, void * arg );

        friend class EventManager;
};
}
