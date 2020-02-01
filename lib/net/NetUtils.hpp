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
#include "basic/String.hpp"

namespace Pravala
{
/// @brief A class for helper network-related functions
class NetUtils
{
    public:
        /// @brief Structure that describes a single flow
        struct FlowDesc
        {
            uint32_t uid; ///< User ID
            uint16_t localPort; ///< Local port number (UDP/TCP)
            uint16_t remotePort; ///< Remote port number (UDP/TCP)

            /// @brief Compares two FlowDescs
            /// @param [in] other The second FlowDesc to compare
            /// @return True if they contain different values
            inline bool isDifferent ( const FlowDesc & other ) const
            {
                return ( uid != other.uid
                         || localPort != other.localPort
                         || remotePort != other.remotePort );
            }

            /// @brief Default constructor
            FlowDesc(): uid ( 0 ), localPort ( 0 ), remotePort ( 0 )
            {
            }
        };

        /// @brief The type of flow
        enum FlowType
        {
            FlowUDP4, ///< UDP over IPv4
            FlowUDP6, ///< UDP over IPv6
            FlowTCP4, ///< TCP over IPv4
            FlowTCP6 ///< TCP over IPv6
        };

        /// @brief Reads and interprets the content of /proc/net/[udp,udp6,tcp,tcp6] files
        /// @param [in] flowType The type of flows to read
        /// @return A list of flow descriptors
        static List<FlowDesc> readFlows ( FlowType flowType );

        /// @brief Reads the packet count for given device
        /// @return Current packet count
        static long unsigned int readPacketCount ( const String & ifaceName );

        /// @brief Reads the wireless quality for given device
        /// @return Current quality
        static uint16_t readWirelessQuality ( const String & ifaceName );
};
}
