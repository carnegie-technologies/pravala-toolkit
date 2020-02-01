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

#include "IpPacket.hpp"

namespace Pravala
{
/// @brief Contains TunIface-specific data for TunIpPacket.
struct TunIpPacketData
{
    /// @brief A "tag" that can be set on TunIpPacket objects.
    /// It is not based on data received over the network, nor is sent over it.
    /// It is meant to be used locally to mark different packets and possibly handle them differently.
    /// The meaning of this tag is not specified here.
    int32_t tag;

    /// @brief Default constructor.
    TunIpPacketData();

    /// @brief Clears the content of this object.
    void clear();
};

/// @brief A wrapper around IpPacket, that contains additional, TunIface-specific data.
class TunIpPacket: public IpPacket
{
    public:
        /// @brief Default constructor.
        TunIpPacket();

        /// @brief Constructor.
        /// If the data passed is invalid, the packet will also be invalid.
        /// @param [in] data The data of the IP packet (see IpPacket(MemHandle))
        TunIpPacket ( const MemHandle & data );

        /// @brief Constructor.
        /// If the data passed is invalid, the packet will also be invalid.
        /// @param [in] data The data of the IP packet (see IpPacket(MemHandle))
        /// @param [in] tunData The TunIpPacketData to set.
        TunIpPacket ( const MemHandle & data, const TunIpPacketData & tunData );

        /// @brief Constructor.
        /// @param [in] ipPacket the IpPacket to create a copy of.
        TunIpPacket ( const IpPacket & ipPacket );

        /// @brief Assignment operator.
        /// @param [in] ipPacket the IpPacket to create a copy of.
        /// @return A reference to this packet.
        TunIpPacket & operator= ( const IpPacket & ipPacket );

        /// @brief Clears the packet and associated memory buffer.
        void clear();

        /// @brief Returns the tag that this TunIpPacket was marked with.
        /// Tags are not based on data received over the network, nor are sent over it.
        /// They are meant to be used locally, to mark packets in different ways to allow different
        /// handling of specific packets. The meaning of tag values is not defined here.
        /// @return This TunIpPacket's tag value.
        inline int32_t getTag() const
        {
            return _tunData.tag;
        }

    protected:
        TunIpPacketData _tunData; ///< TunIpPacketData associated with this packet.
};

/// @brief Streaming operator
/// Appends an TunIpPacket's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value TunIpPacket for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const TunIpPacket & value );
}
