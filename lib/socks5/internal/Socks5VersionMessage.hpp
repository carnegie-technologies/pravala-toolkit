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

#include "basic/List.hpp"

#include "Socks5Message.hpp"

namespace Pravala
{
/// @brief Represents a SOCKS 5 version identifier/method selection message
/// Sent from the client to the server. Identifies the SOCKS version and
/// lists the authentication methods the client supports
class Socks5VersionMessage: public Socks5Message
{
    public:
        /// @brief Default constructor.
        /// Creates an empty (invalid) SOCKS5 version identifier/method selection message
        Socks5VersionMessage();

        /// @brief Constructor for a SOCKS5 version identifier/method selection message with
        /// a list of authentication methods the client supports.
        /// The list should not have more than 255 methods.
        /// @param [in] authMethods The list of authentication methods supported
        Socks5VersionMessage ( const List<uint8_t> & authMethods );

        /// @brief Parses a MemHandle containing a SOCKS 5 version identifier/method selection message.
        /// On success it populates the underlying buffer with the data and consumes it from the MemHandle.
        /// On error, neither the underlying buffer nor the given MemHandle are modified.
        /// @param [in,out] data MemHandle containing a SOCKS5 method selection message.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns IncompleteData error),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success. May not be modified on other errors.
        /// @return Standard error code
        ERRCODE parseAndConsume ( MemHandle & data, size_t & bytesNeeded );

        /// @brief Gets the number of methods supported by the client.
        /// @return The number of methods supported by the client, or 0 if the message is invalid.
        uint8_t getNumMethods() const;

        /// @brief Checks if an authentication method is in the list of available methods.
        /// @param [in] authMethod The AuthenticationMethod to check for.
        /// @return True if it is, false if not (or if the message is invalid).
        bool containsAuthMethod ( uint8_t authMethod ) const;

        /// @brief Gets a method from the list of available methods.
        /// @param[in] methodNumber The index to get the method identifier of
        /// @return the AuthenticationMethod at the given index, or 0 if the message is invalid.
        uint8_t getMethod ( uint8_t methodNumber ) const;

        virtual String getLogId() const;
        virtual void describe ( Buffer & toBuffer ) const;
};
}
