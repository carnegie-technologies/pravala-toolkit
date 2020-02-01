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
/// @brief Vring for receiving packets from the system
/// The RxVring on its own is NOT vhost-net specific, however the setup function is!
class RxVring: public Vring
{
    public:
        /// @brief Constructor.
        /// @param [in] maxDescs Maximum number of descriptors in this ring.
        ///                      According to the virtio specs, this must be a power of 2, and should be at least 8.
        /// @param [in] memTag The tag associated with memory blocks that can be handled.
        ///                    Vrings can only handle memory that has been properly registered.
        ///                    This tag value should be used to mark memory blocks that are within that memory.
        RxVring ( uint16_t maxDescs, uint8_t memTag );

        /// @brief Allocate and set up this Vring's data structures, and set it up with vhost-net.
        /// This setup function is vhost-net specific.
        /// This will not do anything if this Vring is already initialized. If any other error occurs,
        /// this object is cleared. setup() may be called again to try to set it up again.
        /// This should only be called after vhostFd is ready to set up for vrings, otherwise it will fail.
        /// @param [in] vhostFd vhost-net FD to set this vring up with
        /// @param [in] backendFd (Tunnel or similar) FD that this vring will interact with TX over
        /// @return Standard error code
        ERRCODE setup ( int vhostFd, int backendFd );

        /// @brief Called to give as many empty PacketDataStore backed packets to the system as possible
        /// to store packets the system receives for us.
        /// @return True if the system needs to be kicked to tell it that the ring has been refilled. False otherwise.
        bool refill();

        /// @brief Read a packet from the used ring
        /// @param [out] vhdr MemHandle to replace with one containing the virtio header.
        /// This may be cleared if an error occurs.
        /// @param [out] data MemHandle to replace with one containing the data of the packet.
        /// This may be cleared if an error occurs.
        /// For a TUN backed vring, this will contain a complete IP packet.
        /// For a TAP backed vring, this will contain a complete ethernet frame.
        /// @return Standard error code
        ///    Error::SoftFail      - Nothing to read, caller should try again later.
        ///                           i.e. caller should probably wait for an event on the "call" FD
        ///    Error::EmptyRead     - Read an empty packet, caller should try again soon (e.g. end of loop).
        ///                           The system hasn't completed writing the packet to us yet, but was in
        ///                           the process of doing that.
        ///    Error::IncompleteData- Read a packet with only virtio header, or not enough data for virtio header.
        ///                           vhdr and data will be cleared. This broken packet was skipped.
        ///                           Caller can call readPacket again to try reading the next packet.
        ERRCODE readPacket ( MemHandle & vhdr, MemHandle & data );
};
}
