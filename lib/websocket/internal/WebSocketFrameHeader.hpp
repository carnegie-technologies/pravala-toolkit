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

extern "C"
{
#include <stdint.h>
}

#include <basic/MemHandle.hpp>
#include <basic/Buffer.hpp>

namespace Pravala
{
#pragma pack(push, 1)
/// @brief Base WebSocket Frame Header (see RFC 6455)
struct WebSocketHeader
{
    /// @brief Flags and OpCode
    /// Bit layout:
    /// 1 bit   Fin (end of message) flag
    /// 1 bit   Rsv1 (reserved 1) flag
    /// 1 bit   Rsv2 (reserved 2) flag
    /// 1 bit   Rsv3 (reserved 3) flag
    /// 4 bits  OpCode
    uint8_t flagOpCode;

    /// @brief Mask flag and length
    /// Bit layout:
    /// 1 bit   Mask flag
    ///          If 1, 32-bit mask field is present after the last length field
    /// 7 bits  Length of payload in bytes
    ///          126 = Length is contained in 16-bit length field after this field
    ///          127 = Length is contained in a 64-bit length field after this field
    uint8_t maskLen1;
};

/// @brief WebSocket Frame Header with 16-bit payload length
struct WebSocketHeader16: public WebSocketHeader
{
    uint16_t len16; ///< 16-bit payload length, network byte order
};

/// @brief WebSocket Frame Header with 64-bit payload length
struct WebSocketHeader64: public WebSocketHeader
{
    uint64_t len64; ///< 64-bit payload length, network byte order
};

/// @brief WebSocket Frame Header with mask
struct WebSocketHeaderMasked: public WebSocketHeader
{
    /// @brief 4 byte XOR mask value. This is used as an arbitrary 4 byte value to mask in the same 4 byte order,
    /// as such, byte order does not matter. For details, see RFC 6455.
    uint32_t mask;
};

/// @brief WebSocket Frame Header with 16-bit payload length and mask
struct WebSocketHeader16Masked: public WebSocketHeader16
{
    /// @brief 4 byte XOR mask value. This is used as an arbitrary 4 byte value to mask in the same 4 byte order,
    /// as such, byte order does not matter. For details, see RFC 6455.
    uint32_t mask;
};

/// @brief WebSocket Frame Header with 64-bit payload length and mask
struct WebSocketHeader64Masked: public WebSocketHeader64
{
    /// @brief 4 byte XOR mask value. This is used as an arbitrary 4 byte value to mask in the same 4 byte order,
    /// as such, byte order does not matter. For details, see RFC 6455.
    uint32_t mask;
};

#pragma pack(pop)

/// @brief Represents a WebSocket Frame Header
union WebSocketFrameHeader
{
    public:
        /// @brief WebSocket OpCodes from RFC 6455
        /// All other OpCodes are reserved
        enum OpCode
        {
            OpContinuation  = 0x0,///< This frame continues the message from the last frame
            OpText          = 0x1,///< This frame contains text data
            OpBinary        = 0x2,///< This frame contains binary data
            OpClose         = 0x8,///< This frame contains requests/confirms socket closing.
            OpPing          = 0x9,///< This frame is requesting a pong response
            OpPong          = 0xA///< Response to a ping frame
        };

        /// @brief Possible flag values for the flagOpCode field
        enum FlagOpCode
        {
            FlagOpCodeFin   = 1 << 7,///< Fin flag
            FlagOpCodeRsv1  = 1 << 6,///< Rsv1 flag
            FlagOpCodeRsv2  = 1 << 5,///< Rsv2 flag
            FlagOpCodeRsv3  = 1 << 4///< Rsv3 flag
        };

        /// @brief Flag value if the mask is set in the maskLen1 field
        enum FlagMaskLen1
        {
            FlagMaskLen1Mask    = 1 << 7///< Mask flag
        };

        /// @brief Constructor
        /// This is here only for the assert to ensure the size of this union makes sense.
        /// This object is in an unknown state until parse() returns true or the required set functions are called.
        WebSocketFrameHeader();

        /// @brief Clears all fields, then sets up the header according to the specified properties
        ///
        /// This is convenient since these properties are generally all known when creating the frame,
        /// and changing the payload length can be annoying if a mask is present.
        ///
        /// @param [in] opCode OpCode
        /// @param [in] finFlag True to set the FIN flag
        /// @param [in] payloadLen Length of the payload
        /// @param [in] setMask True to generate and set the mask.
        /// @return Length of the WebSocket frame header
        uint8_t setupWebSocketFrame ( OpCode opCode, bool finFlag, uint64_t payloadLen, bool setMask );

        /// @brief Parse a WebSocket frame and consume the data from the buffer if successful.
        ///
        /// @param [in] buf MemHandle that contains a WebSocket frame.
        /// This is not modified if the parse failed (incomplete data).
        ///
        /// @param [out] payload This will be set to a handle containing the raw, potentially masked, payload (if any).
        /// This is not modified if the parse failed (incomplete data).
        ///
        /// @return True if the parse succeeded and this object is now in a valid state, false otherwise.
        /// If parse fails, buf and payload are untouched.
        bool parseAndConsume ( MemHandle & buf, MemHandle & payload );

        /// @brief Returns the size of the header of this WebSocket frame
        /// @note This operation is somewhat expensive, as such, repeated calls should be avoided.
        /// @return The size of the header of this WebSocket frame
        uint8_t getHdrSize() const;

        /// @brief Returns the size of the payload
        /// @note This operation is somewhat expensive, as such, repeated calls should be avoided.
        /// @return The size of the payload
        size_t getPayloadSize() const;

        /// @brief Returns true if the Fin flag is set
        /// @return True if the Fin flag is set
        inline bool isFin() const
        {
            return ( _norm.flagOpCode & FlagOpCodeFin ) == FlagOpCodeFin;
        }

        /// @brief Set the Fin flag
        /// @param [in] val Value to set the Fin flag to
        inline void setFin ( bool val )
        {
            if ( val )
            {
                _norm.flagOpCode |= FlagOpCodeFin;
            }
            else
            {
                _norm.flagOpCode &= ~FlagOpCodeFin;
            }
        }

        /// @brief Perform the masking operation over the payload as per RFC 6455 in place,
        /// using the mask value in this header.
        /// This is used to mask data when sending from the client, or to unmask received data on the server
        /// @param [in] start First byte to mask (in place)
        /// @param [in] len Number of bytes to mask
        inline void mask ( char * start, size_t len )
        {
            maskAndCopy ( start, start, len );
        }

        /// @brief Perform the masking operation over the payload as per RFC 6455 using the mask value in this header,
        /// and put the result to dst.
        /// This is used to mask data when sending from the client, or to unmask received data on the server
        /// @param [in] dst Where to put the masked data.
        /// @param [in] src Pointer to the first byte of the source to mask
        /// @param [in] len Number of bytes to copy and mask
        void maskAndCopy ( char * dst, const char * src, size_t len );

        /// @brief Returns true if this frame header has a mask
        /// @return True if this frame header has a mask
        inline bool hasMask() const
        {
            return ( _norm.maskLen1 & FlagMaskLen1Mask ) == FlagMaskLen1Mask;
        }

        /// @brief Gets the value of the mask.
        /// @note This operation is somewhat expensive, as such, repeated calls should be avoided.
        /// @return The value of the mask. This will not crash, but the result is undefined, if hasMask() is false.
        uint32_t getMask() const;

        /// @brief Gets the OpCode
        /// @return The OpCode
        inline uint8_t getOpCode() const
        {
            return _norm.flagOpCode & 0x0f;
        }

        /// @brief Sets the OpCode
        /// @param [in] code OpCode to set
        inline void setOpCode ( OpCode code )
        {
            _norm.flagOpCode &= ~0x7f;
            _norm.flagOpCode |= code;
        }

    private:
        WebSocketHeader _norm; ///< WebSocket frame header with payload length < 125 and no mask
        WebSocketHeader16 _norm16; ///< WebSocket frame header with 16-bit payload length and no mask
        WebSocketHeader64 _norm64; ///< WebSocket frame header with 64-bit payload length and no mask
        WebSocketHeaderMasked _masked; ///< WebSocket frame header with length < 125 and a mask
        WebSocketHeader16Masked _masked16; ///< WebSocket frame header with 16-bit payload length and a mask
        WebSocketHeader64Masked _masked64; ///< WebSocket frame header with 64-bit payload length and a mask

        /// @brief Returns the value of the first length field
        /// @return The value of the first length field
        inline uint8_t getLen1() const
        {
            return _norm.maskLen1 & 0x7f;
        }
};
}
