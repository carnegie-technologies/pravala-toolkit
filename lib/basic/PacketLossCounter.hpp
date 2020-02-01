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

extern "C"
{
#include <stdint.h>
#include <sys/types.h>
}

#include "NoCopy.hpp"

namespace Pravala
{
/// @brief A class used for monitoring packet loss statistics
class PacketLossCounter: public NoCopy
{
    public:

        /// @brief Default constructor
        /// @param [in] bufSize The number of packet loss entries to keep track of.
        ///                     If the value passed is less than 4, 4 will be used instead.
        PacketLossCounter ( uint16_t bufSize );

        /// @brief Destructor
        ~PacketLossCounter();

        /// @brief Returns packet loss percentage
        /// @return Packet loss percentage
        uint8_t getLossPercentage() const;

        /// @brief Clears loss counter's state
        void clear();

        /// @brief "Adds" a packet loss
        /// @param [in] packetLoss The number of packets that should have been received before the packet that
        ///                         was just received. If this packet was in order, a 0 value should be added.
        ///                         If there was 1 packet lost, a value of 1 should be passed, etc.
        void addLoss ( uint32_t packetLoss );

    private:
        /// @brief The circular buffer for storing the numbers of lost packets
        uint8_t * _buf;

        uint32_t _totalLossCount; ///< Number of lost packets

        const uint16_t _bufSize; ///< The size of the buffer

        /// @brief Current position in the circular buffer.
        ///
        /// This is the last sequence number received - the "freshest" one
        uint16_t _curBufPos;

        /// @brief "Adds" a packet loss. This internal version can only update a single buffer entry.
        /// @param [in] packetLoss The number of packets that should have been received before the packet that
        ///                         was just received. If this packet was in order, a 0 value should be added.
        ///                         If there was 1 packet lost, a value of 1 should be passed, etc.
        void intAddLoss ( uint8_t packetLoss );
};
}
