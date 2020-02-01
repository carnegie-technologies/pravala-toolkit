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

#include "basic/Thread.hpp"

#include "../../EventManager.hpp"

namespace Pravala
{
/// @brief Posix part of the Event Manager.
class PosixEventManager: public EventManager
{
    protected:

        /// @brief Default constructor
        PosixEventManager();

        /// @brief Destructor
        ~PosixEventManager();

        /// @brief Initializes signal support (if _firstManager is set to true)
        /// It should be called at the beginning of run() function.
        /// @brief True if the signals were enabled; False otherwise
        bool initSignals();

        /// @brief Processes signals received
        /// It will either check _globalSig* flags, or try to read from _signalFd.
        /// If _isPrimaryManager is false, then this function does nothing.
        /// @return True if at least one signal was received; False otherwise
        bool runProcessSignals();

        /// @brief Returns the timeout to be used by epoll_wait or equivalent
        ///
        /// It is based on TimerManager::getTimeout(), but when safe SIGCHLD processing is enabled
        /// it never returns < 0 or values greated than safe SIGCHLD interval.
        ///
        /// @return The number of milliseconds after which the runTimers should be run.
        ///         0 for "right away", -1 for "no timers scheduled"
        int getSafeTimeout();

        /// @brief Initializes the FD for use with the EventManager
        /// It currently sets 'close on exit' and 'nonblocking' flags
        static void initFd ( int fd );

        /// @brief Set to true when we receive SIGINT/SIGTERM
        static bool _globalExit;

        // SignalFD is a Linux exception, but to simplify the inheritance (epoll/linux_poll/non_linux_poll)
        // we just assume that it's part of Posix that can be enabled or not.
        // And so it happens that it will only ever be enabled on Linux...

#ifdef USE_SIGNALFD
        int _signalFd; ///< File descriptor used for signalfd operations
#else
        /// @brief Number of SIGCHLDs to process
        static THREAD_LOCAL volatile uint32_t _globalSigChld;

        /// @brief Number of SIGHUPs to process
        static THREAD_LOCAL volatile uint32_t _globalSigHup;

        /// @brief Number of SIGUSR1s to process
        static THREAD_LOCAL volatile uint32_t _globalSigUsr1;

        /// @brief Number of SIGUSR2s to process
        static THREAD_LOCAL volatile uint32_t _globalSigUsr2;

        /// @brief Legacy signal handler
        /// @param [in] sigNum signal number
        static void posixSignalHandler ( int sigNum );
#endif

    private:
        /// @brief Calls waitpid() and runs appropriate handlers
        void runChildWait();

        friend class EventManager;
};
}
