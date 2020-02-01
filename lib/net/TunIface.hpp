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
#include "basic/HashSet.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "net/TunIpPacket.hpp"
#include "sys/Time.hpp"
#include "event/EventManager.hpp"
#include "object/OwnedObject.hpp"

namespace Pravala
{
class TunIface;

/// @brief The owner of this tun interface.
class TunIfaceOwner
{
    protected:
        /// @brief Called when the tun interface has read some data.
        /// @param [in] iface Pointer to the tun interface that generated the callback.
        /// @param [in] packet The IP packet read from the tunnel.
        virtual void tunIfaceRead ( TunIface * iface, TunIpPacket & packet ) = 0;

        /// @brief Called when the tun interface closes (due to an error)
        /// @param [in] iface Pointer to the tun interface that generated the callback.
        virtual void tunIfaceClosed ( TunIface * iface ) = 0;

        /// @brief Called when new data rate calculation is available.
        /// The tunnel should NOT be destroyed in this callback!
        /// @param [in] iface Pointer to the tun interface that generated the callback.
        /// @param [in] sendRate Send rate (in bytes/second)
        /// @param [in] rcvRate Receiving rate (in bytes/second)
        /// @param [in] sinceTime The beginning of the time window over which the rates are calculated.
        virtual void tunIfaceRateUpdate (
            TunIface * iface, uint32_t sendRate, uint32_t rcvRate, const Time & sinceTime );

        /// @brief Called when the IP addresses active on this tun interface have changed.
        /// The tunnel should NOT be destroyed in this callback!
        /// @param [in] iface Pointer to the tun interface that generated the callback.
        virtual void tunIfaceAddressesChanged ( TunIface * iface );

        // Never called, but required to compile on Android.
        virtual ~TunIfaceOwner()
        {
        }

        friend class TunIface;
};

/// @brief The tun interface
/// The interface that packets received from clients are written to in order to send to the Internet
class TunIface: public OwnedObject<TunIfaceOwner>
{
    public:
        /// @brief Adds an address to this interface
        /// @param [in] addr Address to add
        /// @return Standard error code
        ///     Error::AlreadyExists    - address already exists
        ///
        ///     Other errors possible if the add failed
        virtual ERRCODE addAddress ( const IpAddress & addr );

        /// @brief Removes an address to this interface
        /// @param [in] addr Address to remove
        /// @return True if the address was removed successfully, false if the address did not exist
        virtual bool removeAddress ( const IpAddress & addr );

        /// @brief Enables the use of the tun interface.
        /// @param [in] fd The file descriptor of the tun device to use
        /// @param [in] ipAddresses The addresses to use. The device should be already configured,
        ///                          as these addresses are only set in this object's internal state,
        ///                          not applied in any way to the device.
        /// @param [in] ifaceMtu The interface MTU configured on the device.
        ///                      A default MTU will be assumed if this value is <= 0.
        /// @return Standard error code.
        /// If the tunnel was started successfully, this will return success.
        virtual ERRCODE startUnmanaged ( int fd, const HashSet<IpAddress> & ipAddresses, int ifaceMtu = -1 ) = 0;

        /// @brief Create and setup the tunnel device, then start the tun interface in "managed" mode.
        /// The tunnel interface will add/remove system IP addresses for this tunnel device.
        /// @param [in] ifaceMtu The interface MTU to use. It is not used if the value is <= 0,
        ///                       in which case system's default will be used. This is the default behaviour.
        /// @return Standard error code
        virtual ERRCODE startManaged ( int ifaceMtu = -1 ) = 0;

        /// @brief Checks if the tunnel is operating in "managed" mode.
        /// In that mode the tunnel can control (add and remove) its own IP addresses.
        /// @note Depending on the tunnel type, this could return true even when tunnel is not running.
        ///       It is because it could still accept addresses even before being initialized.
        /// @return True if the tunnel is operating in "managed" mode;
        ///         False if it cannot control its own addresses.
        virtual bool isManaged() = 0;

        /// @brief Stops and closes the tun interface.
        virtual void stop();

        /// @brief Adds a packet to internal list of packets to write.
        /// @param [in] packet The packet to write.
        /// @return Standard error code.
        virtual ERRCODE sendPacket ( const IpPacket & packet ) = 0;

        /// @brief Get the name of the interface (if set)
        /// @return Reference to the TunIface's name
        virtual const String & getIfaceName() const = 0;

        /// @brief Get the ID of the interface (used used to talk to the OS)
        /// On Linux the Netlink system is used, and this is the 'index' of the interface.
        /// @return The interface ID (or -1 if not known)
        virtual int getIfaceId() const = 0;

        /// @brief Returns true if this tunnel interface has been initialized (i.e. a start function has succeeded)
        /// @return True if this tunnel interface has been initialized (i.e. a start function has succeeded)
        virtual bool isInitialized() const = 0;

        /// @brief Returns the MTU configured on a tunnel device.
        /// Default implementation returns 0 (0 means OS default MTU, which typically is 1500).
        /// @return MTU configured on the tunnel interface, or 0 if the OS default value is used.
        virtual uint16_t getMtu() const;

        /// @brief Get the IP addresses active on this tun interface
        /// @return The IP addresses active on this tun interface
        inline const HashSet<IpAddress> & getAddresses() const
        {
            return _addresses;
        }

        /// @brief Sets the interval at which data rates are calculated.
        /// @param [in] interval The interval (in seconds) at which data rates are calculated.
        ///                       Setting it to 0 (which is the default) disables rate monitoring.
        void setRateMonitoringInterval ( uint32_t interval );

    protected:
        static TextLogLimited _log; ///< Log stream
        static const TunIpPacketData _emptyTunData; ///< Default (empty) content of TunIpPacketData.

        Time _lastRateUpdate; ///< The last time we updated data rate values

        uint32_t _sendDataCount; ///< The number of bytes sent (since the last rate update)
        uint32_t _rcvDataCount; ///< The number of bytes received (since the last rate update)
        uint32_t _rateMonitoringInterval; ///< The interval between rate calculations.

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TunIface ( TunIfaceOwner * owner );

        /// @brief Destructor
        virtual ~TunIface();

        /// @brief Called when a packet is received by the tunnel.
        /// @note Our owner may have deferenced us after this function has been called!
        /// @param [in,out] mh MemHandle containing the data of the IP packet that was received.
        ///                    This MemHandle must start at the beginning of the IP packet
        ///                    and must not contain any platform specific headers.
        ///                    This buffer might be cleared by this function!
        /// @param [in] tunData TunIface-specific data to set on the generated TunIpPacket.
        void packetReceived ( MemHandle & mh, const TunIpPacketData & tunData = _emptyTunData );

        /// @brief Notify our owner that the tunnel interface has been closed.
        /// @note Our owner may have deferenced us after this function has been called!
        void notifyTunIfaceClosed();

        /// @brief Called when the IP addresses active on this tun interface have changed.
        /// This should be overridden by inheriting classes if they need to respond to this event.
        /// The default implementation notifies the owner that the addresses have changed.
        virtual void tunIfaceAddressesChanged();

        /// @brief A helper function that does the rate measurement update.
        void doRateUpdate();

        /// @brief Called when some data is sent to update the send data count
        /// @param [in] len Amount of data sent (bytes)
        inline void updateSendDataCount ( size_t len )
        {
            if ( _rateMonitoringInterval > 0 )
            {
                if ( EventManager::getCurrentTime().isGreaterEqualThan ( _lastRateUpdate, _rateMonitoringInterval ) )
                    doRateUpdate();

                _sendDataCount += len;
            }
        }

    private:
        HashSet<IpAddress> _addresses; ///< The IP addresses assigned to this TUN interface

        friend class TunIfaceOwner;
};
}
