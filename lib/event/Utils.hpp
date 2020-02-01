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

#include "basic/MsvcSupport.hpp"
#include "error/Error.hpp"

namespace Pravala
{
class Utils
{
    public:
        /// @brief Switches program to the 'daemon' mode
        ///
        /// If it is called in a daemon program, nothing
        /// is done and Error::NothingToDo is returned.
        /// Otherwise it will fork
        /// and return Error::ForkParent in the parent process
        /// (or Error::ForkError if the fork fails).
        /// In the child it will perform following steps:
        /// create new SID
        /// change the working directory to / (to unlock any directories)
        /// redirect input, output and error streams to /dev/null.
        ///
        /// Possible error codes in the child process:
        /// Error::SetSidFailed
        /// Error::ChdirFailed
        /// Error::ForkChild - if everything went fine.
        ///
        /// Some messages can also be printed out to stderr (before it's redirected)
        ///
        /// @param [in] autoParentExit If true (and the fork succeeds)
        ///  this function prints a message that the program is running
        ///  as a daemon and calls exit(EXIT_SUCCESS)
        ///  So, if everything goes well, this function doesn't exit in the parent.
        /// @return Standard error code
        static ERRCODE daemonize ( bool autoParentExit = true );

        /// @brief Fork a child process
        ///
        /// This function is basically just a wrapper around fork. It does not create a new SID,
        /// change directories, or redirect any streams.
        ///
        /// Possible error codes in the child:
        /// Error::ForkChild - everything went ok
        ///
        /// Possible error codes in the parent:
        /// Error::ForkFailed
        /// Error::ForkParent - everything went ok
        ///
        /// @param [out] childPid If not 0 and fork succeeds, then the child PID will be written here on the parent,
        /// and 0 will be written on the child. Otherwise this value is untouched.
        /// @param [out] commFd If not 0, a SocketPair is created between the parent and the child and its
        ///                      descriptor (different in the parent and in the child) is placed here.
        /// @return Standard error code
        static ERRCODE forkChild ( pid_t * childPid = 0, int * commFd = 0 );

        /// @brief Sets up core file size limit to max
        /// @return False if the core file size limit could not be modified, true otherwise
        static bool setupDebugCore();

        /// @brief Sets some low-level parameters that affect OS configuration and memory management
        static void setup();
};
}
