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
/// @brief A base for a number of SOCKS5 messages that carry an address.
/// Several messages (request, reply, UDP request) use the exact same data format,
/// although they interpret some of configuration fields differently.
/// This class defines that format.
class Socks5AddrMessage: public Socks5Message
{
    public:
        /// @brief Parses a MemHandle containing a SOCKS5 address message.
        /// On success it populates the underlying buffer with the data and consumes it from the MemHandle.
        /// On error, neither the underlying buffer nor the given MemHandle are modified.
        /// @param [in,out] data MemHandle containing a SOCKS5 method selection message.
        /// @param [out] bytesNeeded If the data buffer is too small (this function returns IncompleteData error),
        ///                          this is set to the number of additional bytes needed in the data buffer.
        ///                          Set to 0 on success.
        ///                          Not modified on other errors.
        /// @return Standard error code
        ERRCODE parseAndConsume ( MemHandle & data, size_t & bytesNeeded );

        /// @brief Gets the type of the address stored in the message.
        /// @return The address stored in the message, or 0 if the message is invalid.
        uint8_t getAddressType() const;

        /// @brief Gets the address stored in the message.
        /// @return The IP address, or invalid address if it is missing (or the message is invalid).
        IpAddress getAddress() const;

        /// @brief Gets the port number stored in the message.
        /// @return The port number, or 0 if it is missing (or the message is invalid).
        uint16_t getPort() const;

    protected:

#pragma pack(push, 1)

        /// @brief The base of SOCKS5 address message.
        struct Socks5AddrMsgBase
        {
            union
            {
                struct
                {
                    uint8_t fields[ 3 ];  ///< Configuration fields.
                    uint8_t atyp;         ///< The address type.
                } base; ///< Used by the base class

                struct
                {
                    uint8_t ver;        ///< The SOCKS protocol version number
                    uint8_t cmd;        ///< The proxy command requested
                    uint8_t rsv;        ///< Empty reserved field (should be set to 0)
                    uint8_t atyp;       ///< The address type (the address is a "destination" address)
                } req; ///< Used by SOCKS5 request messages

                struct
                {
                    uint8_t ver;        ///< The SOCKS protocol version number
                    uint8_t rep;        ///< The proxy server's reply code
                    uint8_t rsv;        ///< Empty reserved field (should be set to 0)
                    uint8_t atyp;       ///< The address type (the address is a "server bound" address)
                } reply; ///< Used by SOCKS5 reply messages

                struct
                {
                    uint8_t rsv[ 2 ];   ///< Reserved (both should be set to 0)
                    uint8_t frag;       ///< The packet fragment number.
                    uint8_t atyp;       ///< The address type (the address is a "destination" address)
                } udpReq; ///< Used by SOCKS5 UDP request header messages
            } u; ///< The base configuration expressed in different ways.
        };

        /// @brief SOCKS5 address message with an IPv4 address and port.
        struct Socks5AddrMsgV4: public Socks5AddrMsgBase
        {
            in_addr addr;       ///< The IP address.
            uint16_t port;      ///< The port number (in network byte order).
        };

        /// @brief SOCKS5 address message with an IPv6 address and port.
        struct Socks5AddrMsgV6: public Socks5AddrMsgBase
        {
            in6_addr addr;      ///< The IP address.
            uint16_t port;      ///< The port number (in network byte order).
        };

#pragma pack(pop)

        /// @brief Default constructor.
        /// It creates an empty (invalid) SOCKS5 message.
        Socks5AddrMessage();

        /// @brief Constructor for a SOCKS5 message with a configuration and an address.
        /// @param [in] fieldA First configuration field.
        /// @param [in] fieldB Second configuration field.
        /// @param [in] fieldC Fourth configuration field.
        /// @param [in] addr Address to put in the message
        Socks5AddrMessage ( uint8_t fieldA, uint8_t fieldB, uint8_t fieldC, const SockAddr & addr );

        /// @brief Checks if the base message is valid.
        /// @param [in] msg Pointer to the message to check.
        /// @return True if the base of the message looks valid; False otherwise.
        virtual bool isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const = 0;
};
}
