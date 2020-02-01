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

#ifdef ENABLE_VHOSTNET

#include "socket/PacketDataStore.hpp"
#include "socket/os/shared/vhostnet/VhostNetMgr.hpp"
#include "TunIfaceVhostNet.hpp"

using namespace Pravala;

ConfigNumber<bool> TunIfaceVhostNet::optEnableTunVhostNet (
        ConfigOpt::FlagInitializeOnly,
        "os.tun.enable_vhostnet",
        "True to enable vhost-net support for tun, false otherwise.",
        false
);

TunIfaceDev * TunIfaceDev::generate ( TunIfaceOwner * owner )
{
    if ( TunIfaceVhostNet::optEnableTunVhostNet.value() )
    {
        return new TunIfaceVhostNet ( owner );
    }

    return new TunIfaceDev ( owner );
}

TunIfaceVhostNet::TunIfaceVhostNet ( TunIfaceOwner * owner ):
    TunIfaceDev ( owner ),
    _vh ( 0 )
{
}

TunIfaceVhostNet::~TunIfaceVhostNet()
{
    // We HAVE to call it here.
    // TunIface's destructor would call only its own version!
    stop();
}

void TunIfaceVhostNet::configureMemPool ( int ifaceMtu )
{
    if ( optEnableTunVhostNet.value() && ifaceMtu > ( int ) PacketDataStore::PacketSize )
    {
        // We could support it, but it makes things more complicated... for now - we don't.

        LOG ( L_FATAL_ERROR, "VhostNet does not support tunnel MTUs larger than " << PacketDataStore::PacketSize
              << "; Configured MTU (" << ifaceMtu << ") will likely cause problems" );

        return;
    }

    TunIfaceDev::configureMemPool ( ifaceMtu );
}

ERRCODE TunIfaceVhostNet::setupFd ( int fd )
{
    ERRCODE eCode = TunIfaceDev::setupFd ( fd );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    // This shouldn't be possible, as TunIfaceDev::start() above should have already failed with
    // Error::AlreadyInitialized if _vh was set.
    assert ( !_vh );

    // But just in case...
    if ( _vh != 0 )
    {
        _vh->unrefOwner ( this );
        _vh = 0;
    }

    _vh = VhostNet::generate ( this, fd, eCode );

    if ( !_vh )
    {
        LOG_ERR ( L_ERROR, eCode, "Failed to start vhost-net tunnel, falling back to normal tunnel" );

        return Error::Success;
    }

    assert ( _vh != 0 );
    assert ( IS_OK ( eCode ) );

    _vh->setMaxPacketsReadPerLoop ( optMaxReadsPerEvent.value() );

    // Disable all FD events on the tunnel FD to make everything go through vhost-net
    EventManager::setFdEvents ( _fd, 0 );

    return Error::Success;
}

void TunIfaceVhostNet::stop()
{
    if ( _vh != 0 )
    {
        LOG ( L_DEBUG2, "Closing and removing VhostNet object" );

        _vh->close();
        _vh->unrefOwner ( this );
        _vh = 0;
    }

    TunIfaceDev::stop();
}

ERRCODE TunIfaceVhostNet::sendPacket ( const IpPacket & ipPacket )
{
    if ( !_vh )
    {
        // Disabled vhost-net, use the regular send:
        return TunIfaceDev::sendPacket ( ipPacket );
    }

    if ( !ipPacket.isValid() )
    {
        return Error::InvalidParameter;
    }

    const MemVector & packet = ipPacket.getPacketData();

    if ( packet.isEmpty() )
    {
        LOG_LIM ( L_ERROR, "Cannot send an empty packet: " << ipPacket );
        return Error::InvalidParameter;
    }

    if ( !_vh->canUseMemory ( packet ) )
    {
        LOG_LIM ( L_WARN, "Packet uses memory not compatible with VhostNet: " << ipPacket );

        // The memory is not from PacketDataStore, so we cannot use vhost-net.
        // This will impact the performance and could result in rearranged packets, but it still should work.
        return TunIfaceDev::sendPacket ( ipPacket );
    }

    assert ( _vh != 0 );
    assert ( _vh->canUseMemory ( packet ) );

    // A packet that is being sent must be in registered vhost memory.
    //
    // This check is rather heavy, so we should really fix any code rather than perform the check for every packet.
    assert ( VhostNetMgr::get().isInMemRange ( packet ) );

    const ERRCODE eCode = _vh->write ( packet );

    if ( IS_OK ( eCode ) )
    {
        LOG ( L_DEBUG4, "sendPacket OK, loop end subscribed" );

        updateSendDataCount ( packet.getDataSize() );
    }
    else if ( eCode == Error::SoftFail )
    {
        LOG_LIM ( L_WARN, "Tunnel write queue full, dropping packet; FD: " << _fd << "; Packet: " << ipPacket );
    }
    else if ( eCode == Error::EmptyWrite || eCode == Error::InvalidParameter )
    {
        LOG_ERR_LIM ( L_ERROR, eCode, "Tunnel write failed; Invalid data passed to the tunnel; Packet: " << ipPacket );
    }
    else
    {
        LOG_ERR_LIM ( L_ERROR, eCode, "Tunnel write failed, closing tunnel" );
        stop();
    }

    return eCode;
}

void TunIfaceVhostNet::vhostPacketReceived ( VhostNet * vh, MemHandle & pkt )
{
    ( void ) vh;
    assert ( vh == _vh );

    packetReceived ( pkt );
}

void TunIfaceVhostNet::vhostNetClosed ( VhostNet * vh )
{
    assert ( vh == _vh );

    if ( vh == _vh )
    {
        LOG ( L_ERROR, "VhostNet closed, tunnel falling back to non-vhost mode" );

        _vh->unrefOwner ( this );
        _vh = 0;

        // Re-subscribe to events on the tunnel FD
        EventManager::setFdEvents ( _fd, EventManager::EventRead | EventManager::EventWrite );

        return;
    }
}

#endif
