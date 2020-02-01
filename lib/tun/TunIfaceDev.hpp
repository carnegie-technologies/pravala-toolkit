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

#include "net/TunIface.hpp"
#include "event/EventManager.hpp"
#include "socket/PacketWriter.hpp"

namespace Pravala
{
class PacketMemPool;

/// @brief Base implementation of the tun interface that uses a system device (e.g. /dev/tun)
class TunIfaceDev: public TunIface, protected EventManager::FdEventHandler
{
    public:
        /// @brief Max number of slabs (each slab is a collection of blocks) per TunIface.
        static const uint8_t PacketMaxSlabs = 16;

        /// @brief Min MTU allowed. This is based on policy restrictions, for IPv6 it should be at least 1280.
        static const uint16_t MinMTU = 512;

        /// @brief Max number of packets to read per read event
        static ConfigLimitedNumber<uint8_t> optMaxReadsPerEvent;

        /// @brief Used for enabling/disabling asynchronous writes
        static ConfigNumber<bool> optUseAsyncWrites;

        /// @brief The length of the write queue.
        static ConfigLimitedNumber<uint16_t> optQueueSize;

        /// @brief The max amount of memory that can be used by a TunIface (in megabytes).
        static ConfigLimitedNumber<uint32_t> optMaxMemorySize;

        /// @brief Generates a new, reference-counted instance of the TunIfaceDev.
        /// @param [in] owner The initial owner to set.
        static TunIfaceDev * generate ( TunIfaceOwner * owner );

        virtual ERRCODE startUnmanaged ( int fd, const HashSet<IpAddress> & ipAddresses, int ifaceMtu = -1 );
        virtual ERRCODE startManaged ( int ifaceMtu = -1 );
        virtual bool isManaged();
        virtual void stop();

        virtual ERRCODE addAddress ( const IpAddress & addr );
        virtual bool removeAddress ( const IpAddress & addr );

        virtual ERRCODE sendPacket ( const IpPacket & packet );

        virtual bool isInitialized() const;

        virtual int getIfaceId() const;
        virtual const String & getIfaceName() const;
        virtual uint16_t getMtu() const;

    protected:
        PacketWriter _writer; ///< Packet writer used by this object.

        /// @brief Memory pool used by the tunnel interface.
        /// It may be unset, in which case the standard PacketDataStore will be used.
        /// It is used if the tunnel's MTU is large and requires non-standard data segments.
        PacketMemPool * _memPool;

        String _ifaceName; ///< Interface name

        /// @brief The ID of the interface, used to talk to the OS.
        /// This is the NetManager's link ID.
        /// If this is < 0, then we are running in "unmanaged" mode.
        int _ifaceId;

        int _fd; ///< File descriptor of the tunnel (may need to change for others)

        uint16_t _ifaceMtu; ///< The MTU configured, 0 means OS default is used (typically 1500).

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TunIfaceDev ( TunIfaceOwner * owner );

        /// @brief Destructor
        virtual ~TunIfaceDev();

        virtual void receiveFdEvent ( int fd, short events );

        /// @brief Configures the tun interface
        /// It reads this interface's ID (and exposes it through the ifaceId parameter),
        /// assigns the provided IP addresses to the interface, sets up the MTU and brings the interface up.
        /// @param [in] fd The FD of the tun interface
        /// @param [in] ifaceName The name of the interface
        /// @param [in] ifaceMtu The MTU of the interface to set. It is ignored (and not modified) if < 0 value is used.
        /// @param [out] ifaceId The ID of the interface that is being configured.
        /// @return Standard error code
        static ERRCODE configureIface ( int fd, const String & ifaceName, int ifaceMtu, int & ifaceId );

        /// @brief Creates and initializes the tun interface
        /// It is OS-specific function
        /// @param [out] ifaceFd The FD of the created device; Not modified if there is an error
        /// @param [out] ifaceName The name of the the created device; Not modified if there is an error
        /// @return Standard error code
        static ERRCODE osCreateTunDevice ( int & ifaceFd, String & ifaceName );

        /// @brief Reads a packet from the tunnel interface.
        /// It is OS-specific function
        /// @param [in,out] buffer Buffer to put the data in. Memory should already be allocated!
        /// @return True if it succeeded; False if it failed and the tunnel should be closed.
        ///         True doesn't mean that there is any data in the buffer.
        ///         If there was a non-critical error (like EAGAIN), it will still report success (since there is
        ///         no need to close the tunnel), but the buffer will be empty.
        bool osRead ( MemHandle & buffer );

        /// @brief Generates the data to write to the tunnel device.
        /// @param [in] ipPacket The IP packet to generate the data for.
        /// @param [in,out] vec The memory vector to append the data to.
        /// @return True if the data was successfully generated; False otherwise.
        bool osGetWriteData ( const IpPacket & ipPacket, MemVector & vec );

        /// @brief Helper function to set up an FD for use by the tunnel interface
        /// @param [in] fd FD to set up for use by the tunnel interface
        /// @return Standard Error Code
        virtual ERRCODE setupFd ( int fd );

        /// @brief Configures internal memory pool.
        /// Depending on the MTU configured on the tunnel, the memory pool may need to be changed.
        /// This function will (re)configure custom memory pool, if needed. After calling this,
        /// memory pool may be unset (if a custom pool is not required).
        /// @param [in] ifaceMtu The MTU configured on the tunnel.
        virtual void configureMemPool ( int ifaceMtu );
};
}
