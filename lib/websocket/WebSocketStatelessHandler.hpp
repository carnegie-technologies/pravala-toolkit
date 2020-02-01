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

#include "basic/MemHandle.hpp"
#include "basic/HashSet.hpp"
#include "basic/String.hpp"
#include "object/SimpleObject.hpp"

#include "WebSocketHandler.hpp"
#include "WebSocketConnection.hpp"

namespace Pravala
{
/// @brief Base for a simple server handler that requires no state.
/// The inheriting class must implement functions that this class inherits that aren't implemented.
/// For example: wsRead
class WebSocketStatelessHandler:
    public WebSocketHandler,
    public WebSocketConnectionOwner
{
    protected:
        /// @brief All sockets managed by this WebSocketHandler
        /// Objects must be ref/unref'ed when they are added/removed from this set!
        HashSet<WebSocketConnection *> _allSocks;

        virtual ~WebSocketStatelessHandler();

        virtual void addConnection ( WebSocketListener * listener, WebSocketConnection * conn );

        virtual void wsClosed ( WebSocketConnection * sock );

        /// @brief Broadcast some data to all sockets
        /// @param [in] data Data to broadcast
        /// @param [in] isText True if the data being broadcast is text, false if it is binary
        void broadcast ( const MemHandle & data, bool isText );

        /// @brief Broadcast some data to all sockets
        /// @param [in] data Data to broadcast
        /// @param [in] len Length of data to broadcast
        /// @param [in] isText True if the data being broadcast is text, false if it is binary
        void broadcast ( const char * data, size_t len, bool isText );

        /// @brief Broadcast a null terminated text string to all sockets
        /// @param [in] data Null terminated text string to broadcast
        void broadcast ( const char * data );

        /// @brief Broadcast a string to all sockets
        /// @param [in] data String to broadcast
        inline void broadcast ( const String & data )
        {
            broadcast ( data.c_str(), data.length(), true );
        }
};
}
