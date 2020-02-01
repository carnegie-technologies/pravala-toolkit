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
/// @brief Represents a SOCKS 5 UDP request header
/// Used by both the client and the server. Goes in front of any UDP packet sent between them.
/// Contains the address to whom the packet is meant to be sent (when received by the server)
/// or from whom the packet actually originated (when received by the client).
class Socks5UdpRequestHeader: public Socks5AddrMessage
{
    public:
        /// @brief Default constructor.
        /// It creates an empty (invalid) SOCKS5 UDP request header.
        Socks5UdpRequestHeader();

        /// @brief Constructor for a SOCKS5 UDP request header with a destination address. The payload data
        /// should be appended to this header before being sent.
        /// @param [in] sa The destination address and port to set
        Socks5UdpRequestHeader ( const SockAddr & destAddr );

        /// @brief Gets the fragment number.
        /// @return The fragment number, or 0 if the message is invalid.
        uint8_t getFragment() const;

        virtual String getLogId() const;
        virtual void describe ( Buffer & toBuffer ) const;

    protected:
        virtual bool isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const;
};
}
