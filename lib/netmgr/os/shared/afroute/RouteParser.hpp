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

#include "basic/Buffer.hpp"
#include "basic/String.hpp"
#include "log/TextLog.hpp"
#include "AfRouteTypes.hpp"

namespace Pravala
{
/// @brief A helper class for parsing route messages.
class RouteParser
{
    public:
        /// @brief Process a memory buffer containing one or more messages to retrieve the route, link and
        /// address updates.
        ///
        /// It will only consume full messages; once a message is parsed it will 'consume' it from the buffer
        /// so at the end there will only be the unparsed data left in buf.
        /// @param [in] buf The buffer to process
        /// @param [out] links The links parsed from the buffer; appended to whatever was passed in
        /// @param [out] addrs The addresses parsed from the buffer; appended to whatever was passed in
        /// @param [out] routes The routes parsed from the buffer; appended to whatever was passed in
        /// @return True on success; false on failure
        static ERRCODE processBuffer (
            RwBuffer & buf,
            List<AfRouteTypes::Link> & links,
            List<AfRouteTypes::Address> & addrs,
            List<AfRouteTypes::Route> & routes );

    private:
#pragma pack(push, 1)
        /// @brief The header which is common to all of the messages read from the routing socket.
        struct CommonMsgHdr
        {
            uint16_t msgLen; ///< The length of the message, including header and payload.
            uint8_t version; ///< The version of the message.
            uint8_t type; ///< The type of message.

            /// @brief Given a buffer, parse it into the common msg hdr and the resulting payload
            /// @param [in] buf The buffer to parse
            /// @param [out] data A memhandle to the first message contained in buf. Not set if ret is false.
            /// This points to the beginning of the header of the packet.
            /// @return True if parsing succeeded (buf contained at least one full packet).
            /// On success, msgLen, version & type are filled in,
            /// and data is pointed to a read-only copy of the data in this packet (header + payload).
            /// False means that there isn't enough data to form both the header and the payload,
            /// more data is required (buf isn't complete).
            bool parse ( const MemHandle & buf, MemHandle & data );
        };

#pragma pack(pop)

        static TextLog _log; ///< Log

        /// @brief Process an rt_msg message (route updates)
        /// @param [in] buf The message to parse
        /// @param [out] route The parsed routing message
        /// @return true if the route was successfully parsed and the record should be used; false otherwise
        /// for example cloned routes are not considered relevant (they are auto-generated), so we return false
        static bool processRtMsg ( const MemHandle & buf, AfRouteTypes::Route & route );

        /// @brief Process an if_msg message (interface updates)
        /// @param [in] buf The message to parse
        /// @param [out] link The parsed link message
        /// @return true if the interface update was successfully parsed
        /// and the record should be used; false otherwise
        static bool processIfMsg ( const MemHandle & buf, AfRouteTypes::Link & link );

        /// @brief Process an ifa_msg message (interface address updates)
        /// @param [in] buf The message to parse
        /// @param [out] addr The parsed address message
        /// @return true if the interface address update was successfully parsed
        /// and the record should be used; false otherwise
        static bool processIfaMsg ( const MemHandle & buf, AfRouteTypes::Address & addr );

        /// @brief Process an ifma_msg message (interface multicast address updates)
        /// @param [in] buf The message to parse
        /// @param [out] addr The parsed multicast address message
        /// @return true if the interface multicast address update was successfully parsed
        /// and the record should be used; false otherwise
        static bool processIfmaMsg ( const MemHandle & buf, AfRouteTypes::Address & addr );
};
}
