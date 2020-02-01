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
#include "error/Error.hpp"

namespace Pravala
{
namespace Ctrl
{
class Message;
class Request;
class Update;
}

// TODO This file must be renamed to match the class name. Waiting to finish code review first
//      to keep patchset diffs readable
/// @brief A synchronous control link
class CtrlLinkSync
{
    public:

        /// @brief default constructor
        CtrlLinkSync();

        /// @brief destructor
        ~CtrlLinkSync();

        /// @brief Open a local socket
        ///
        /// @param [in] sockName The path to the socket to use
        /// @return Standard error code
        ERRCODE connect ( const String & sockName );

        /// @brief Close a socket
        void close();

        /// @brief Synchronously send a request to the server and wait for a response
        ///        If a specific type of message is passed, this will keep reading messages until
        ///        a message of that type is received. Messages of other types are discarded.
        ///
        /// @param [in] req The ctrl message to send
        /// @param [out] resp The ctrl message that was received
        /// @return Standard error code
        ERRCODE request ( Ctrl::Request & req, Ctrl::Update & resp );

        /// @brief Synchronously send a control message
        ///
        /// @param [in] msg The message to send
        /// @return Standard error code
        ERRCODE send ( Ctrl::Message & msg );

        /// @brief Synchronously receive a control message
        ///
        /// @param [out] msg The message that was received. Only set if no error was returned.
        /// @return Standard error code
        ERRCODE recv ( Ctrl::Message & msg );

    private:

        Buffer _readBuffer; ///< A buffer to store received data in

        String _localSockName; ///< The path to the local socket

        /// @todo use SimpleSocket
        int _socketFd; ///< The file descriptor of the open socket
};
}
