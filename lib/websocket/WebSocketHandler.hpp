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

namespace Pravala
{
class WebSocketListener;
class WebSocketConnection;

/// @brief Interface for a WebSocket handler
class WebSocketHandler
{
    protected:
        /// @brief Virtual destructor
        virtual ~WebSocketHandler()
        {
        }

        /// @brief Called by WebSocketListener to add a connection to the inheriting handler
        /// @param [in] listener Listener that generated this connection.
        /// This pointer may not be valid after this function.
        /// @param [in] conn Connection that has been established.
        virtual void addConnection ( WebSocketListener * listener, WebSocketConnection * conn ) = 0;

        friend class WebSocketListener;
};

typedef WebSocketHandler * WebSocketHandlerPtr;
}
