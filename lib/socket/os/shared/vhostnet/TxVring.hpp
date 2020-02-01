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

#include "Vring.hpp"

namespace Pravala
{
/// @brief Vring for transmitting packets to the system
/// The TxVring on its own is NOT vhost-net specific, however the setup function is!
class TxVring: public Vring
{
    public:
        /// @brief Constructor.
        /// @param [in] maxDescs Maximum number of descriptors in this ring.
        ///                      According to the virtio specs, this must be a power of 2, and should be at least 8.
        /// @param [in] memTag The tag associated with memory blocks that can be handled.
        ///                    Vrings can only handle memory that has been properly registered.
        ///                    This tag value should be used to mark memory blocks that are within that memory.
        TxVring ( uint16_t maxDescs, uint8_t memTag );

        /// @brief Allocate and set up this Vring's data structures, and set it up with vhost-net.
        /// This setup function is vhost-net specific.
        /// This will not do anything if this Vring is already initialized. If any other error occurs,
        /// this object is cleared. setup() may be called again to try to set it up again.
        /// This should only be called after vhostFd is ready to set up for vrings, otherwise it will fail.
        /// @param [in] vhostFd vhost-net FD to set this vring up with
        /// @param [in] backendFd (Tunnel or similar) FD that this vring will interact with TX over
        /// @return Standard error code
        ERRCODE setup ( int vhostFd, int backendFd );

        /// @brief Called to write a packet to the system
        /// @note The system might need to be "kicked" to transmit this data - see VhostNet
        /// @param [in] vhdr MemHandle containing virtio header portion of the packet.
        /// Cannot be empty. Must be from PacketDataStore.
        /// @param [in] data MemHandle containing data portion of the packet.
        /// Cannot be empty. Must be from PacketDataStore.
        /// For a TUN backed vring, this should contain a complete IP packet.
        /// For a TAP backed vring, this should contain a complete ethernet frame.
        /// @return Standard error code
        ///     Error::InvalidParameter - one of the parameters wasn't from PacketDataStore
        ///     Error::EmptyWrite       - one of the parameters was empty
        ///     Error::SoftFail         - ring full, try again later
        ERRCODE writeData ( const MemHandle & vhdr, const MemHandle & data );

        /// @brief Called to write a packet to the system
        /// @note The system might need to be "kicked" to transmit this data - see VhostNet
        /// @param [in] vhdr MemHandle containing virtio header portion of the packet.
        /// Cannot be empty. Must be from PacketDataStore.
        /// @param [in] data MemVector containing data portion of the packet.
        /// Cannot be empty. Each chunk must be from PacketDataStore.
        /// For a TUN backed vring, this should contain a complete IP packet.
        /// For a TAP backed vring, this should contain a complete ethernet frame.
        /// @return Standard error code
        ///     Error::InvalidParameter - one of the parameters wasn't from PacketDataStore
        ///     Error::EmptyWrite       - one of the parameters was empty
        ///     Error::SoftFail         - ring full, try again later
        ERRCODE writeData ( const MemHandle & vhdr, const MemVector & data );

        /// @brief Called to write a packet to the system
        /// @note The system might need to be "kicked" to transmit this data - see VhostNet
        /// An zero virtio header is prepended to this packet.
        /// @param [in] data MemHandle containing data portion of the packet.
        /// Cannot be empty. Must be from PacketDataStore.
        /// For a TUN backed vring, this should contain a complete IP packet.
        /// For a TAP backed vring, this should contain a complete ethernet frame.
        /// @return Standard error code
        ///     Error::InvalidParameter - one of the parameters wasn't from PacketDataStore
        ///     Error::EmptyWrite       - one of the parameters was empty
        ///     Error::SoftFail         - ring full, try again later
        inline ERRCODE write ( const MemHandle & data )
        {
            return writeData ( _zeroVheader, data );
        }

        /// @brief Called to write a packet to the system
        /// @note The system might need to be "kicked" to transmit this data - see VhostNet
        /// An zero virtio header is prepended to this packet.
        /// @param [in] data MemVector containing data portion of the packet.
        /// Cannot be empty. Each chunk must be from PacketDataStore.
        /// For a TUN backed vring, this should contain a complete IP packet.
        /// For a TAP backed vring, this should contain a complete ethernet frame.
        /// @return Standard error code
        ///     Error::InvalidParameter - one of the parameters wasn't from PacketDataStore
        ///     Error::EmptyWrite       - one of the parameters was empty
        ///     Error::SoftFail         - ring full, try again later
        inline ERRCODE write ( const MemVector & data )
        {
            return writeData ( _zeroVheader, data );
        }

        /// @brief Clear used packets from the ring
        /// i.e. clear transmitted packets.
        /// @return True if the ring is now empty, false otherwise
        bool cleanUsed();

    private:
        /// @brief Zero virtio header, PacketDataStore backed.
        ///
        /// This is _vheaderLen long, filled with 0, which means "no special properties".
        ///
        /// This header is normally used to set checksum/offload flags, however if we don't need to set any of them,
        /// we can send out this same MemHandle with all packets and avoid repeatedly getting and zeroing packets
        /// from PacketDataStore.
        ///
        /// A VirtIO header is present at the beginning of every received packet, and must be present at the
        /// beginning of every sent packet (required by the system API).
        ///
        MemHandle _zeroVheader;

        /// @brief Append a segment to the current set of descriptors to TX
        /// The caller must ensure that there are sufficient descriptors available to TX the entire packet!
        /// @param [in] mh MemHandle to append.
        /// The caller must ensure that this MemHandle is from PacketDataStore and that it isn't empty.
        /// @param [in] last True if this is the last segment, false if the packet continues onto a next segment
        /// @return Index of descriptor that was appended
        uint16_t append ( const MemHandle & mh, bool last );

        /// @brief Offer the packet to the system.
        /// The caller should only call this after it has successfully appended some segments.
        /// @param [in] index Index of the first descriptor of the packet
        /// This caller must ensure that this index is valid.
        void offer ( uint16_t index );
};
}
