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

#include "error/Error.hpp"

namespace Pravala
{
/// @brief A class that creates a pair of connected sockets.
/// @todo TODO This class probably should be moved to LibSocket.
///       However, LibEvent needs it. Core functionality could be moved into LibSys/SocketApi,
///       and code in LibEvent could use that directly.
class SocketPair
{
    public:
        /// @brief Default constructor
        SocketPair();

        /// @brief Destructor
        ~SocketPair();

        /// @brief Returns the internal descriptor of the first socket.
        /// @note The caller should NOT close this file descriptor. To "take over" takeSockA() should be used.
        /// @return Internal descriptor of the first socket.
        inline int getSockA() const
        {
            return _sockA;
        }

        /// @brief Returns the internal descriptor of the first socket, and stops using the socket.
        /// This means that the caller is now responsible for closing this socket.
        /// Subsequent calls to getSockA() will return -1.
        /// @return Internal descriptor of the first socket.
        inline int takeSockA()
        {
            const int ret = _sockA;

            _sockA = -1;

            return ret;
        }

        /// @brief Returns the internal descriptor of the second socket.
        /// @note The caller should NOT close this file descriptor. To "take over" takeSockA() should be used.
        /// @return Internal descriptor of the second socket.
        inline int getSockB() const
        {
            return _sockB;
        }

        /// @brief Returns the internal descriptor of the second socket, and stops using the socket.
        /// This means that the caller is now responsible for closing this socket.
        /// Subsequent calls to getSockB() will return -1.
        /// @return Internal descriptor of the second socket.
        inline int takeSockB()
        {
            const int ret = _sockB;

            _sockB = -1;

            return ret;
        }

        /// @brief Closes both sockets
        void close();

        /// @brief Initializes the socket pair
        /// @return Standard error code
        ERRCODE init();

    private:
        int _sockA; ///< The internal descriptor of the first socket
        int _sockB; ///< The internal descriptor of the second socket

        /// @brief Private copy constructor
        /// Not implemented
        SocketPair ( const SocketPair & );

        /// @brief Private operator=
        /// Not implemented
        SocketPair & operator= ( const SocketPair & );
};
}
