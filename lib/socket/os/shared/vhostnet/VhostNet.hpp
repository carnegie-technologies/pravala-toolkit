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

#include <cassert>

#include "error/Error.hpp"
#include "object/PooledOwnedObject.hpp"
#include "event/EventManager.hpp"

#include "RxVring.hpp"
#include "TxVring.hpp"

/// @brief Helper macro to:
/// 1. if not valid, return Error::Closed
/// 2. call _txRing.write (...)
/// 3. if this write fills up the txRing (i.e. 0 free descs), then we immediately call kickTx() to start transmission
/// 4. otherwise, we set _txNeedKick so that it will be kicked at the end of the loop
/// 5. always subscribe to end of loop, since we may need it to call kickTx(), and definitely need it to call cleanTx()
/// 6. return the eCode.
/// @note This macro does not return a value, instead it will return out of the parent function!
/// @param [in] ... Parameters to pass to _txRing.write
#define TX_WRITE_AND_RETURN( ... ) \
    do { \
        if ( !isValid() ) { return Error::Closed; } \
        const ERRCODE eCode = _txRing.write ( __VA_ARGS__ ); \
        if ( eCode == Error::Success ) \
        { \
            if ( _txRing.getFreeDescs() == 0 ) \
            { \
                _txNeedKick = false; \
                kickFd ( _txKickFd, "TX" ); \
            } \
            else \
            { \
                _txNeedKick = true; \
            } \
            EventManager::loopEndSubscribe ( this ); \
        } \
        return eCode; \
    } while ( false )

namespace Pravala
{
class VhostNet;

/// @brief Owner of a VhostNet object, receives callbacks from VhostNet
class VhostNetOwner
{
    protected:
        /// @brief Called when a packet has been received
        /// @param [in] vn VhostNet object that packet was received from
        /// @param [in] pkt Contains the actual packet
        /// For a TUN device, this will be a complete IP packet beginning with the IP header.
        /// For a TAP device, this will be a complete ethernet frame.
        virtual void vhostPacketReceived ( VhostNet * vn, MemHandle & pkt ) = 0;

        /// @brief Called when a VhostNet object's FD was closed.
        /// The owner can no longer use this VhostNet object for TX/RX and should clean it up.
        /// If the owner wishes, they may try to generate a new VhostNet object using the same backing FD.
        /// @param [in] vn VhostNet object that was closed
        virtual void vhostNetClosed ( VhostNet * vn ) = 0;

        friend class VhostNet;
};

/// @brief Class containing functionality to transmit/receive data using a vhost-net ring
class VhostNet: public PooledOwnedObject<VhostNet, VhostNetOwner>,
    public EventManager::FdEventHandler,
    public EventManager::LoopEndEventHandler
{
    public:
        /// @brief Memory tag that should be set on memory blocks registered with VhostNetMgr.
        const uint8_t RegisteredMemTag;

        /// @brief Set up a VhostNet object
        /// Calls internalSetup and cleans up if it fails
        /// @param [in] owner Owner of this VhostNet object
        /// @param [in] tunFd Tunnel FD
        /// @param [out] eCode Standard error code.
        /// @return VhostNet object, or 0 if generation/setup failed.
        static VhostNet * generate ( VhostNetOwner * owner, int tunFd, ERRCODE & eCode );

        /// @brief Free all memory and close all file descriptors, except for the tunnel FD.
        /// Also unregister from VhostNetMgr if we previously registered.
        void close();

        /// @brief Calls close, then subscribe to notify our owner next loop end that we've been closed.
        void closeAndScheduleNotify();

        /// @brief Immediately notifies our owner that we've closed.
        /// This also unsubscribes us from end of loop.
        /// @note This should only be called when we are on a callback codepath,
        /// e.g. receiveLoopEndEvent or receiveFdEvent
        void doClosedNotify();

        /// @brief Returns true if this object is ready to be used
        /// @return True if this object is ready to be used
        inline bool isValid() const
        {
            return _isValid;
        }

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
        ///     Error::Closed           - VhostNet was closed
        inline ERRCODE write ( const MemHandle & data )
        {
            TX_WRITE_AND_RETURN ( data );
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
        ///     Error::Closed           - VhostNet was closed
        inline ERRCODE write ( const MemVector & data )
        {
            TX_WRITE_AND_RETURN ( data );
        }

        /// @brief Sets the maximum number of packets to read per event/loop.
        /// @param [in] count Number of packets to read per event/loop. Must be >0.
        ///
        /// This value should not be too large as having it excessively large can introduce excessive latency
        /// into the event loop. What "excessively large" is depends on hardware, platform, etc.
        inline void setMaxPacketsReadPerLoop ( uint16_t count )
        {
            assert ( count > 0 );

            if ( count < 1 )
                return;

            _maxPktsReadPerLoop = count;
        }

        /// @brief Checks whether given memory object can be handled by the VhostNet.
        /// It will check if given memory object uses memory pools, and whether its tag is the same as the one
        /// registered in the constructor.
        /// @param [in] mem The memory object to check.
        /// @tparam T The type of the object to check, either MemHandle or MemVector.
        /// @return True if the memory object has the right type and tag; False otherwise.
        template<typename T> inline bool canUseMemory ( const T & mem ) const
        {
            return Vring::canUseMemory ( mem, RegisteredMemTag );
        }

    protected:
        virtual void receiveLoopEndEvent();
        virtual void receiveFdEvent ( int fd, short events );

        /// @brief Called when this object returns to pool
        void returnsToPool();

        /// @brief Allocates a new VhostNet object
        /// @return A new VhostNet object
        static VhostNet * generateNew();

    private:
        static TextLog _log; ///< Log stream

        RxVring _rxRing; ///< Vring for receiving packets from the system
        TxVring _txRing; ///< Vring for transmitting packets to the system

        /// @brief vhost-net FD
        /// If this is >= 0, we need to unregister from VhostNetMgr.
        /// If this is <0, we are not "ready" and cannot be used.
        int _vhostFd;

        int _rxCallFd; ///< FD that will have something to read when a packet is available to be read

        /// @brief FD that we should write something to when we have new descriptors available for the system to
        /// read into AND the system is asking for a kick
        int _rxKickFd;

        int _txKickFd; ///< FD that we should write something to when we want to the system to TX packets

        uint16_t _maxPktsReadPerLoop; ///< Max packets read per event/loop

        bool _txNeedKick; ///< True if kickTx() should be called at the end of loop

        bool _isValid; ///< True if this object is valid and can be used for TX/RX, false otherwise

        /// @brief Constructor.
        /// @param [in] memTag The tag associated with memory blocks that can be handled.
        ///                    Vrings can only handle memory that has been properly registered.
        ///                    This tag value should be used to mark memory blocks that are within that memory.
        VhostNet ( uint8_t memTag );

        /// @brief Destructor
        /// Calls clear()
        ~VhostNet();

        /// @brief Set up a VhostNet object
        /// @param [in] tunFd Tunnel FD
        /// @return Standard error code.
        /// If this is not Error::Success, then this object will be in an undefined state and cannot be used.
        /// close() can be called to return this object to a defined (closed) state, or it can be destroyed.
        ERRCODE internalSetup ( int tunFd );

        /// @brief Sends a uint64_t with the value of 1 over the specified FD. This is used for "kick"ing the kernel
        /// to tell it that there are available descriptors on a ring.
        /// @param [in] fd FD to send kick over
        /// @param [in] log Type of FD that was passed (for logging purposes only)
        void kickFd ( int fd, const char * log );

        /// @brief Cleans used (transmitted) packets from the TX ring
        void cleanTx();

        /// @brief Perform a read cycle, reading up to _maxPktsReadPerLoop packets and calling the vhostPacketReceived
        /// callback potentially multiple times.
        ///
        /// This may also enable/disable read events on _rxCallFd and subscribe to end of loop.
        ///
        /// @note This should only be called on a callback codepath e.g. receiveLoopEndEvent or receiveFdEvent
        void doRead();

        POOLED_OWNED_OBJECT_HDR ( VhostNet, VhostNetOwner );
};
}
