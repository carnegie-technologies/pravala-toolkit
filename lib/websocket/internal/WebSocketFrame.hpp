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
#include "basic/Buffer.hpp"
#include "WebSocketFrameHeader.hpp"

namespace Pravala
{
/// @brief Represents a WebSocket frame that will be sent over a WebSocket connection
class WebSocketFrame
{
    public:
        /// @brief Constructor for a WebSocket Frame with no payload
        /// @param [in] opCode OpCode to set in the header
        /// @param [in] isFin True to set the FIN flag in the header
        WebSocketFrame ( WebSocketFrameHeader::OpCode opCode, bool isFin );

        /// @brief Constructor for a WebSocket frame with payload
        /// @note This version will copy payload if needsMask is set, otherwise it will only hold a reference
        /// @todo support extensions
        /// @param [in] opCode OpCode to set in the header
        /// @param [in] isFin True to set the FIN flag in the header
        /// @param [in] mask True if the payload needs to be masked before it is sent
        /// @param [in] payload The payload to send
        WebSocketFrame ( WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask, const MemHandle & payload );

        /// @brief Constructor for a WebSocket frame with payload
        /// @note This version will copy payload
        /// @todo support extensions
        /// @param [in] opCode OpCode to set in the header
        /// @param [in] isFin True to set the FIN flag in the header
        /// @param [in] mask True if the payload needs to be masked before it is sent
        /// @param [in] payload The payload to send
        /// @param [in] payloadLen Number of bytes to send from the payload
        WebSocketFrame (
            WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask,
            const char * payload, size_t payloadLen );

        /// @brief Returns true if this WebSocketFrame is empty
        /// @return True if this WebSocketFrame is empty
        inline bool isEmpty() const
        {
            return _data1.isEmpty() && _data2.isEmpty();
        }

        /// @brief Append any internal MemHandles that have data to a list, and clear the internal handles
        /// @param [out] out List to append internal MemHandles to
        inline void appendHandles ( List<MemHandle> & out )
        {
            if ( !_data1.isEmpty() )
            {
                out.append ( _data1 );
                _data1.clear();
            }

            if ( !_data2.isEmpty() )
            {
                out.append ( _data2 );
                _data2.clear();
            }
        }

    private:
        /// @brief This contains the header, and may also contain the payload
        MemHandle _data1;

        /// @brief If not empty, this contains the payload
        MemHandle _data2;

        /// @brief Set up this object, copying the payload
        /// @param [in] opCode OpCode to set in the header
        /// @param [in] isFin True to set the FIN flag in the header
        /// @param [in] mask True if the payload needs to be masked before it is sent
        /// @param [in] payload The payload to send
        /// @param [in] payloadLen Number of bytes to send from the payload
        void setupCopyPayload (
            WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask,
            const char * payload, size_t payloadLen );
};
}
