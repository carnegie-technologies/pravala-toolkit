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
/// @brief Represents a SOCKS 5 request message
/// Sent from the client. Identifies the SOCKS version and specifies the command
/// the client wishes the proxy server to execute - connect, bind, or UDP associate.
class Socks5RequestMessage: public Socks5AddrMessage
{
    public:
        /// @brief The commands that the client can request of the proxy server.
        /// Values are from RFC1928.
        enum Command
        {
            CmdTcpConnect = 0x01,        ///< Connect to a TCP server and relay TCP packets
            CmdTcpBind = 0x02,           ///< Bind a port to listen for TCP connections
            CmdUDPAssociate = 0x03       ///< Generate an associated UDP socket to relay data
        };

        /// @brief Checks if a field is a valid Command as per RFC1928.
        /// @return True if the Command is valid, false otherwise
        static inline bool isValidCommand ( uint8_t c )
        {
            return ( 0x00 < c && c < 0x04 );
        }

        /// @brief Default constructor.
        /// It creates an empty (invalid) SOCKS5 request message.
        Socks5RequestMessage();

        /// @brief Constructor for a SOCKS5 request message with a command and a destination address.
        /// @param [in] command The command to request (connect, bind, or UDP associate)
        /// @param [in] destAddr Destination address to put in the request
        Socks5RequestMessage ( Command c, const SockAddr & destAddr );

        /// @brief Gets the value of the command field.
        /// @return The value from command field, or 0 if the message is invalid.
        uint8_t getCommand() const;

        virtual String getLogId() const;
        virtual void describe ( Buffer & toBuffer ) const;

    protected:
        virtual bool isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const;
};
}
