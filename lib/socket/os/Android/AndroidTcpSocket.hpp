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

#include "socket/TcpFdSocket.hpp"

namespace Pravala
{
/// @brief Android-specific wrapper around TcpFdSocket that binds the socket to a specific network.
class AndroidTcpSocket: public TcpFdSocket
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] networkId The ID of the network to bind this socket to. This is not used when < 0.
        AndroidTcpSocket ( SocketOwner * owner, int64_t networkId );

    protected:
        /// @brief Destructor
        virtual ~AndroidTcpSocket();

        virtual bool sockInitFd ( SocketApi::SocketType sockType, int & sockFd );

    private:
        const int64_t _netId; ///< The ID of the network to bind this socket to. This is not used when < 0.
};
}
