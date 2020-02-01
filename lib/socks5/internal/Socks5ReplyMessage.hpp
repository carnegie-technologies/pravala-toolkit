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

#include "Socks5AddrMessage.hpp"

namespace Pravala
{
/// @brief Represents a SOCKS 5 reply message
/// Sent from the server. Identifies the SOCKS version used and includes the result of the client's request, along
/// with the server bound address from the command. The use of the server-bound address depends on the request:
///
/// For connect, the reply's address and port are the address and port the SOCKS proxy assigned to connect to
/// the target host.
///
/// During bind, two replies are received. The first reply's address and port denote where the SOCKS proxy is
/// listening for new connections. The second reply's address and port are those of the connecting host.
///
/// For UDP associate, the bound port and address indicate where the client should send UDP packets to be relayed
class Socks5ReplyMessage: public Socks5AddrMessage
{
    public:
        /// @brief The proxy server's reply code when given a request. Values are from RFC1928.
        enum Reply
        {
            ReplySuccess = 0x00,                     ///< The request completed successfully
            ReplyGeneralSocksServerFailure = 0x01,   ///< General failure on the server end
            ReplyConnectionNotAllowed = 0x02,        ///< Server rules do not permit the request
            ReplyNetworkUnreachable = 0x03,          ///< The network could not be reached
            ReplyHostUnreachable = 0x04,             ///< The target host could not be reached
            ReplyConnectionRefused = 0x05,           ///< Target host refused the connection
            ReplyTTLExpired = 0x06,                  ///< Time to live expired
            ReplyCommandNotSupported = 0x07,         ///< Given command is unsupported
            ReplyAddressTypeNotSupported = 0x08      ///< Given address type is unsupported
        };

        /// @brief Default constructor.
        /// It creates an empty (invalid) SOCKS5 reply message.
        Socks5ReplyMessage();

        /// @brief Constructor for a SOCKS5 reply message with a reply code and an address.
        /// @param [in] r The reply code to set
        /// @param [in] boundAddr The server-bound address and port
        Socks5ReplyMessage ( Socks5ReplyMessage::Reply r, const SockAddr & boundAddr );

        /// @brief Gets the reply field.
        /// @return The reply field, or 0 if the message is invalid.
        uint8_t getReply() const;

        virtual String getLogId() const;
        virtual void describe ( Buffer & toBuffer ) const;

    protected:
        virtual bool isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const;
};
}
