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

#include "Socks5Message.hpp"

namespace Pravala
{
/// @brief Represents a SOCKS 5 version method selection message
/// Sent from the server to the client. Identifies the SOCKS version and
/// specifies which of the authentication methods to use.
class Socks5MethodSelectionMessage: public Socks5Message
{
    public:
        /// @brief Default constructor.
        /// It creates an empty (invalid) SOCKS5 method selection message.
        Socks5MethodSelectionMessage();

        /// @brief Constructor for a SOCKS5 method selection message with a selected authentication method.
        /// @param [in] a The authentication method to use in the SOCKS5 negotiations
        Socks5MethodSelectionMessage ( Socks5Message::AuthenticationMethod a );

        /// @brief Parses a MemHandle containing a SOCKS5 method selection message.
        /// On success it populates the underlying buffer with the data and consumes it from the MemHandle.
        /// On error, neither the underlying buffer nor the given MemHandle are modified.
        /// @param [in,out] data MemHandle containing a SOCKS5 method selection message.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns IncompleteData error),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success.
        ///                          Not modified on other errors.
        /// @return Standard error code
        ERRCODE parseAndConsume ( MemHandle & data, size_t & bytesNeeded );

        /// @brief Gets the authentication method selected by the server.
        /// @return The method selected by the server; or 0 if the message is invalid.
        uint8_t getAuthenticationMethod() const;

        virtual String getLogId() const;
        virtual void describe ( Buffer & toBuffer ) const;
};
}
