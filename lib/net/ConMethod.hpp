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
#include "basic/String.hpp"

namespace Pravala
{
/// @brief Used for parsing "connection method" strings in following format:
/// PROTOCOL:IP_ADDR/PORT/EXTRA/ARGS
/// For example:
/// UDP:127.0.0.1/9999/A (UDP connection method)
/// TCP:127.0.0.1/9999/A/B/C (TCP connection method)
/// 127.0.0.1/9999/X/Y (both UDP and TCP; PROTO: is optional)
class ConMethod
{
    public:
        /// @brief Default constructor
        ConMethod();

        /// @brief Constructor
        /// @param [in] str configuration string
        ConMethod ( const String & str );

        /// @brief Clears (and invalidates) the content
        void clear();

        /// @brief Returns true if the method is valid
        /// @return true if the method is valid; false otherwise
        inline bool isValid() const
        {
            return ( _proto != ProtoInvalid && _port > 0 && _addr.isValid() );
        }

        /// @brief Returns true if the protocol is 'TCP' or 'BOTH'
        /// @return true if the protocol is 'TCP' or 'BOTH'
        inline bool useTcp() const
        {
            return ( _proto == ProtoBoth || _proto == ProtoTcp );
        }

        /// @brief Returns true if the protocol is 'UDP' or 'BOTH'
        /// @return true if the protocol is 'UDP' or 'BOTH'
        inline bool useUdp() const
        {
            return ( _proto == ProtoBoth || _proto == ProtoUdp );
        }

        /// @brief Returns true if both TCP and UDP are to be used
        /// @return true if both TCP and UDP are to be used
        inline bool useBoth() const
        {
            return ( _proto == ProtoBoth );
        }

        /// @brief Returns the IpAddress to use
        /// @return the IpAddress to use
        inline const IpAddress & getAddress() const
        {
            return _addr;
        }

        /// @brief Returns the port number to use
        /// @return port number to use
        inline const uint16_t & getPort() const
        {
            return _port;
        }

        /// @brief Returns extra arguments read from the config string
        /// @return extra arguments read from the config string
        inline const StringList & getArgs() const
        {
            return _args;
        }

        /// @brief Configures this ConMethod
        /// @param [in] str configuration string
        /// @return True if it was configured properly, false otherwise
        bool setup ( const String & str );

    private:
        StringList _args; ///< Extra args
        IpAddress _addr; ///< The address
        uint16_t _port; ///< Port number

        /// @brief The protocol type to use
        enum
        {
            ProtoInvalid, ///< Invalid
            ProtoUdp, ///< UDP only
            ProtoTcp, ///< TCP only
            ProtoBoth ///< Both UDP and TCP
        } _proto;
};
}
