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
/// @brief Android-specific socket API calls.
class AndroidSocketApi
{
    public:
        /// @brief Pointer to a function that will bind socket to Android Network.
        /// @param [in] sockFd The socket FD to bind.
        /// @param [in] netId The ID of the network to bind the socket to.
        /// @param [in] userPtr User pointer, as configured using setBindSocketToNetworkFunc(). Could be 0.
        /// @return The result of the operation: 0 on success, -1 on failure.
        typedef int (* BindSocketToNetworkFType) ( int sockFd, int64_t netId, void * userPtr );

        /// @brief Configures the "bindSocketToNetwork" function.
        /// @param [in] func The function pointer to use from now on. It could be 0 to unset it.
        /// @param [in] userPtr The user pointer that will be passed with every call to "func". Can be 0.
        static void setBindSocketToNetworkFunc ( BindSocketToNetworkFType func, void * userPtr );

        /// @brief Binds socket to specific network.
        /// @param [in] sockFd The socket FD to bind to a network.
        /// @param [in] netId The ID of the network to bind the socket to.
        /// @return Standard error code.
        static ERRCODE bindSocketToNetwork ( int sockFd, int64_t netId );
};
}
