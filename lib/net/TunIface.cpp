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

#include "basic/MemHandle.hpp"

#include "TunIpPacket.hpp"
#include "TunIface.hpp"

using namespace Pravala;

TextLogLimited TunIface::_log ( "tun_iface" );

const TunIpPacketData TunIface::_emptyTunData;

TunIface::TunIface ( TunIfaceOwner * owner ):
    OwnedObject ( owner ),
    _sendDataCount ( 0 ),
    _rcvDataCount ( 0 ),
    _rateMonitoringInterval ( 0 )
{
}

TunIface::~TunIface()
{
    stop();

    _rateMonitoringInterval = 0;
}

uint16_t TunIface::getMtu() const
{
    return 0;
}

void TunIface::setRateMonitoringInterval ( uint32_t interval )
{
    _rateMonitoringInterval = interval;
    _sendDataCount = 0;
    _rcvDataCount = 0;

    _lastRateUpdate = EventManager::getCurrentTime();
}

void TunIface::stop()
{
    _addresses.clear();

    // Keep the rate monitoring interval, but clear the counters
    _sendDataCount = 0;
    _rcvDataCount = 0;

    _lastRateUpdate.clear();
}

void TunIface::packetReceived ( MemHandle & mh, const TunIpPacketData & tunData )
{
    TunIpPacket ipPacket ( mh, tunData );

    if ( !ipPacket.isValid() )
    {
        LOG_LIM ( L_ERROR, "The IP packet read from the tunnel interface is invalid. Dropping" );

        return;
    }

    LOG ( L_DEBUG4, "Packet from tunnel iface: " << ipPacket );

    // We used to check here whether the memory is from PacketDataStore.
    // This is needed when data is written to the tunnel (in case vhostnet is used),
    // but if the tunnel generates regular memory (going in the opposite direction),
    // we are OK with that.

    if ( _rateMonitoringInterval > 0 )
    {
        if ( EventManager::getCurrentTime().isGreaterEqualThan ( _lastRateUpdate, _rateMonitoringInterval ) )
            doRateUpdate();

        _rcvDataCount += mh.size();
    }

    // We clear the original buffer.
    // The IP packet should have its own reference. If the owner wants to modify the IP packet inside
    // the callback, it would result in copying all the data. Since we don't care about
    // our own reference to that memory (stored in mh), we clear() it. This way the IP packet
    // contains the only reference to that data and can be modified without copying the data!
    mh.clear();

    TunIfaceOwner * const owner = getOwner();

    if ( owner != 0 )
    {
        // We may be unreferenced inside, so we have to return after calling this:
        owner->tunIfaceRead ( this, ipPacket );
        return;
    }
}

void TunIface::doRateUpdate()
{
    const uint32_t sndRate = EventManager::getCurrentTime().calcBytesPerSecond ( _sendDataCount, _lastRateUpdate );
    const uint32_t rcvRate = EventManager::getCurrentTime().calcBytesPerSecond ( _rcvDataCount, _lastRateUpdate );
    const Time lastUpdate = _lastRateUpdate;

    _lastRateUpdate = EventManager::getCurrentTime();
    _sendDataCount = 0;
    _rcvDataCount = 0;

    TunIfaceOwner * owner = getOwner();

    if ( owner != 0 )
    {
        owner->tunIfaceRateUpdate ( this, sndRate, rcvRate, lastUpdate );
    }
}

void TunIface::notifyTunIfaceClosed()
{
    TunIfaceOwner * owner = getOwner();

    if ( owner != 0 )
    {
        owner->tunIfaceClosed ( this );
    }
}

ERRCODE TunIface::addAddress ( const IpAddress & addr )
{
    if ( !addr.isValid() )
        return Error::InvalidParameter;

    if ( !isInitialized() )
        return Error::NotInitialized;

    const size_t oldAddrSize = _addresses.size();

    _addresses.insert ( addr );

    if ( oldAddrSize == _addresses.size() )
    {
        return Error::AlreadyExists;
    }

    tunIfaceAddressesChanged();

    return Error::Success;
}

bool TunIface::removeAddress ( const IpAddress & addr )
{
    if ( !addr.isValid() )
        return false;

    if ( !isInitialized() )
        return false;

    if ( _addresses.remove ( addr ) < 1 )
        return false;

    tunIfaceAddressesChanged();

    return true;
}

void TunIface::tunIfaceAddressesChanged()
{
    TunIfaceOwner * owner = getOwner();

    if ( owner != 0 )
    {
        owner->tunIfaceAddressesChanged ( this );
    }
}

void TunIfaceOwner::tunIfaceRateUpdate ( TunIface *, uint32_t, uint32_t, const Time & )
{
    SLOG ( TunIface::_log, L_WARN, "Rate measurements are enabled, but the callback function is not overloaded!" );
}

void TunIfaceOwner::tunIfaceAddressesChanged ( TunIface * )
{
}
