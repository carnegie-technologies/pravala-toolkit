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

#include "basic/IpAddress.hpp"
#include "basic/MemHandle.hpp"
#include "log/TextLog.hpp"
#include "error/Error.hpp"

namespace Pravala
{
/// @brief The base class for all SOCKS5 messages.
class Socks5Message
{
    public:
        /// @brief Socks protocol version
        static const uint8_t Socks5Version = 0x05;

        /// @brief The constants to be set in the reserved fields
        static const uint8_t Socks5ReservedVal = 0x00;

        /// @brief  Specifies which authentication subnegotiation will occur before
        /// the client can send any requests. Values are from RFC1928.
        enum AuthenticationMethod
        {
            AuthNoneRequired = 0x00,     ///< Skips the authentication
            AuthGSSAPI = 0x01,           ///< Uses GSSAPI for authentication
            AuthUsernamePassword = 0x02, ///< Uses a username and password for authentication

            // 0x03 through 0x7F are IANA reserved
            // 0x80 through 0xFE are reserved for private methods

            AuthNoneAcceptable = 0xFF    ///< Server response when none of the given methods are acceptable
        };

        /// @brief The address type used in a message. Values are from RFC1928.
        enum AddressType
        {
            AddrIPv4 = 0x01,              ///< Uses an IPv4 address and port number
            AddrDomainName = 0x03,        ///< Uses a fully qualified domain name and port number
            AddrIPv6 = 0x04               ///< Uses an IPv6 address and port number
        };

        /// @brief Checks if a value is a valid AddressType
        /// @param [in] atyp The address type to check.
        /// @return True if the AddressType is valid, false otherwise
        static inline bool isValidAddressType ( uint8_t atyp )
        {
            return ( atyp == AddrIPv4 || atyp == AddrDomainName || atyp == AddrIPv6 );
        }

        /// @brief Checks if the message is valid (contains data)
        /// Invalid messages will always be empty, @see _data.
        /// @return True if the message is valid, false otherwise
        inline bool isValid() const
        {
            return ( _data.size() > 0 );
        }

        /// @brief Returns the MemHandle to this message's data.
        /// @return This message's data.
        inline const MemHandle & getData() const
        {
            return _data;
        }

        /// @brief Gets the size of the message.
        /// @return The size of the message.
        inline size_t getSize() const
        {
            return _data.size();
        }

        /// @brief Appends the description of this message to the buffer.
        /// @param [in,out] toBuffer The buffer to append the description to. It is not cleared first.
        virtual void describe ( Buffer & toBuffer ) const = 0;

    protected:
        static TextLog _log; ///< Log stream

        /// @brief The internal buffer that contains the data
        /// If its size is > 0, it means that it contains data that has been verified to contain a
        /// correct SOCKS5 Message and the size of the buffer is the size of that message
        MemHandle _data;

        /// @brief A helper function that consumes a single SOCKS5 message from the start of a MemHandle and
        ///        sets it in the underlying buffer.
        /// @note This overwrites the current content of the underlying buffer.
        /// @param [in,out] data MemHandle beginning with a SOCKS5 message. First msgSize bytes will be consumed.
        /// @param [in] msgSize The exact size of the SOCKS5 message.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns IncompleteData error),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success.
        /// @return IncompleteData if there were less bytes than msgSize; Success otherwise.
        ERRCODE setAndConsume ( MemHandle & data, size_t msgSize, size_t & bytesNeeded );

        /// @brief A helper function that consumes a single SOCKS5 message from the start of a MemHandle and
        ///        sets it in the underlying buffer.
        /// @note This overwrites the current content of the underlying buffer.
        /// @param [in,out] data MemHandle beginning with a SOCKS5 message.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns IncompleteData error),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success.
        /// @tparam T The type of the message to consume.
        /// @return IncompleteData if there were less bytes than the size of T; Success otherwise.
        template<typename T> inline ERRCODE setAndConsume ( MemHandle & data, size_t & bytesNeeded )
        {
            return setAndConsume ( data, sizeof ( T ), bytesNeeded );
        }

        /// @brief A helper function that exposes this message data as a specific message type.
        /// @tparam T The type of the message to return the data as.
        /// @return Pointer to the message data, or 0 if the data is invalid/too small.
        template<typename T> inline const T * getMessage() const
        {
            if ( _data.size() < sizeof ( T ) )
                return 0;

            return reinterpret_cast<const T *> ( _data.get() );
        }

        /// @brief A helper function that exposes this message data as a specific message type.
        /// @tparam T The type of the message to return the data as.
        /// @return Pointer to the message data, or 0 if the data is invalid/too small.
        template<typename T> inline T * getMessage()
        {
            if ( _data.size() < sizeof ( T ) )
                return 0;

            return reinterpret_cast<T *> ( _data.getWritable() );
        }

        /// @brief A helper function that returns data in the buffer as a message pointer.
        /// @param [in] data Buffer with the memory to return as a message pointer.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns 0),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success.
        /// @tparam T The type of the message to return the data as.
        /// @return Pointer to the message data, or 0 if the data is too small.
        template<typename T> inline const T * getMessage ( const MemHandle & data, size_t & bytesNeeded ) const
        {
            if ( data.size() < sizeof ( T ) )
            {
                LOG ( L_DEBUG4, getLogId()
                      << ": Not enough message data bytes; Size: " << data.size()
                      << "; Needed: " << sizeof ( T ) );

                bytesNeeded = ( sizeof ( T ) - data.size() );
                return 0;
            }

            bytesNeeded = 0;
            return reinterpret_cast<const T *> ( data.get() );
        }

        /// @brief Returns the ID of the socket (for logging).
        /// @return The ID of the socket (for logging).
        virtual String getLogId() const = 0;
};
}
