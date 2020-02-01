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

#include <cassert>
#include <cerrno>

extern "C"
{
#include <fcntl.h>
#include <unistd.h>
}

#include "basic/Math.hpp"
#include "basic/MemHandle.hpp"
#include "net/IpPacket.hpp"
#include "netmgr/NetManager.hpp"
#include "socket/PacketDataStore.hpp"
#include "socket/PacketMemPool.hpp"

#include "TunIfaceDev.hpp"

using namespace Pravala;

ConfigLimitedNumber<uint8_t> TunIfaceDev::optMaxReadsPerEvent (
        0,
        "os.tun.max_reads_per_event",
        "Maximum number of packets to read per read event",
        1, 0xFFU, 64
);

ConfigNumber<bool> TunIfaceDev::optUseAsyncWrites (
        0,
        "os.tun.async_writes",
        "Set to true to enable asynchronous tunnel writes",
        false
);

ConfigLimitedNumber<uint16_t> TunIfaceDev::optQueueSize (
        0,
        "os.tun.write_queue_size",
        "The length of per-socket write queue",
        4, 1000, 16
);

ConfigLimitedNumber<uint32_t> TunIfaceDev::optMaxMemorySize
(
        0,
        "os.tun.max_memory",
        "The max amount of pre-allocated memory that can be used by a tunnel interface for reading packets "
        "(in megabytes)",
        1, 1024, 16
);

TunIfaceDev::TunIfaceDev ( TunIfaceOwner * owner ):
    TunIface ( owner ),
    _writer ( PacketWriter::BasicWriter,
            ( optUseAsyncWrites.value() ? ( PacketWriter::FlagThreaded ) : 0 ),
            optQueueSize.value() ),
    _memPool ( 0 ),
    _ifaceId ( -1 ),
    _fd ( -1 ),
    _ifaceMtu ( 0 )
{
}

TunIfaceDev::~TunIfaceDev()
{
    // We HAVE to call it here.
    // TunIface's destructor would call only its own version!
    stop();

    if ( _memPool != 0 )
    {
        _memPool->shutdown();
        _memPool = 0;
    }
}

uint16_t TunIfaceDev::getMtu() const
{
    return _ifaceMtu;
}

void TunIfaceDev::stop()
{
    _writer.clearFd();

    TunIface::stop();

    if ( _ifaceId >= 0 )
    {
        for ( HashSet<IpAddress>::Iterator it ( getAddresses() ); it.isValid(); it.next() )
        {
            NetManager::get().removeIfaceAddress ( _ifaceId, it.value() );
        }

        NetManager::get().setIfaceState ( _ifaceId, false );

        _ifaceId = -1;
    }

    _ifaceName.clear();

    if ( _fd >= 0 )
    {
        EventManager::closeFd ( _fd );
        _fd = -1;
    }
}

void TunIfaceDev::receiveFdEvent ( int fd, short events )
{
    ( void ) fd;
    assert ( fd == _fd );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        EventManager::disableWriteEvents ( fd );
    }

    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        LOG ( L_DEBUG4, "ReadEvent" );

        // Hold a reference to ourself while we try to read multiple times
        simpleRef();

        for ( uint8_t i = 0; i < optMaxReadsPerEvent.value() && _fd >= 0; ++i )
        {
            MemHandle buf = ( _memPool != 0 )
                            ? ( _memPool->getHandle() )
                            : ( PacketDataStore::getPacket ( _ifaceMtu ) );

            if ( buf.isEmpty() )
            {
                LOG ( L_ERROR, "Out of memory to read from tun" );
                break;
            }

            if ( !osRead ( buf ) )
            {
                stop();
                break;
            }

            if ( buf.isEmpty() )
            {
                // No more data to read
                break;
            }

            packetReceived ( buf );
        }

        if ( _fd < 0 )
        {
            // We got closed during the last read cycle, tell our owner
            notifyTunIfaceClosed();
        }

        simpleUnref();
        return;
    }
}

ERRCODE TunIfaceDev::sendPacket ( const IpPacket & ipPacket )
{
    if ( !ipPacket.isValid() )
    {
        return Error::InvalidParameter;
    }

    LOG ( L_DEBUG4, "Packet to tunnel iface: " << ipPacket );

    if ( ipPacket.getPacketData().isEmpty() )
    {
        LOG_LIM ( L_ERROR, "Cannot send an empty packet: " << ipPacket );
        return Error::InvalidParameter;
    }

    if ( _fd < 0 )
    {
        return Error::NotInitialized;
    }

    // We reserve 3 slots, 2 for the packet (and extPayload), one for prefix (if needed by the OS).
    MemVector vec ( 3 );

    if ( !osGetWriteData ( ipPacket, vec ) )
    {
        return Error::MemoryError;
    }

    const ERRCODE eCode = _writer.write ( vec );

    if ( IS_OK ( eCode ) )
    {
        updateSendDataCount ( ipPacket.getPacketSize() );
    }

    return eCode;
}

ERRCODE TunIfaceDev::configureIface (
        int fd, const String & ifaceName,
        int ifaceMtu, int & ifaceId )
{
    if ( fd < 0 )
        return Error::InvalidParameter;

    NetManagerTypes::Interface iface;

    // We just created this interface a few calls ago, so the Network Manager will not know about it yet
    // (it uses asynchronous updates and returns cached copy of the state). But we need to know the ID
    // of the interface we just created. Let's ask the system directly, going around the cache.
    // This will not update the cache, but will give us an ID.

    ERRCODE eCode = NetManager::get().getUncachedIface ( ifaceName, iface );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode,
                  "Could not find the TUN interface '" << ifaceName << "' in the list of network links" );

        return Error::CouldNotInitialize;
    }

    ifaceId = iface.id;

    if ( ifaceMtu > 0 )
    {
        eCode = NetManager::get().setIfaceMtu ( ifaceId, max<int> ( ifaceMtu, MinMTU ) );

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_ERROR, eCode, "Setting MTU of the tun interface to " << ifaceMtu << " failed" );
            return Error::MtuError;
        }
    }

    LOG ( L_DEBUG, "Bringing the tun interface UP" );

    eCode = NetManager::get().setIfaceState ( ifaceId, true );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Bringing the tun interface UP failed" );
        return Error::InterfaceError;
    }

    return eCode;
}

ERRCODE TunIfaceDev::setupFd ( int fd )
{
    if ( fd < 0 )
    {
        return Error::InvalidParameter;
    }

    if ( _fd >= 0 )
    {
        return Error::AlreadyInitialized;
    }

    // There should be no addresses on the tunnel yet!
    // (i.e. add/removeAddresses shouldn't have populated anything since we weren't initialized)
    assert ( getAddresses().isEmpty() );

    _fd = fd;

    EventManager::setFdHandler ( _fd, this, EventManager::EventRead );

    _writer.setupFd ( _fd );

    return Error::Success;
}

ERRCODE TunIfaceDev::startUnmanaged ( int fd, const HashSet<IpAddress> & ipAddresses, int ifaceMtu )
{
    if ( fd < 0 || ipAddresses.isEmpty() )
    {
        return Error::InvalidParameter;
    }

    ERRCODE eCode = setupFd ( fd );

    if ( IS_OK ( eCode ) )
    {
        configureMemPool ( ifaceMtu );

        for ( HashSet<IpAddress>::Iterator it ( ipAddresses ); it.isValid(); it.next() )
        {
            TunIface::addAddress ( it.value() );
        }
    }

    return eCode;
}

ERRCODE TunIfaceDev::startManaged ( int ifaceMtu )
{
    if ( _fd >= 0 )
    {
        return Error::AlreadyInitialized;
    }

    assert ( _ifaceName.isEmpty() );
    assert ( getAddresses().isEmpty() );

    int tunFd = -1;
    int ifaceId = -1;
    String ifaceName;

    ERRCODE eCode = osCreateTunDevice ( tunFd, ifaceName );

    UNTIL_ERROR ( eCode, configureIface ( tunFd, ifaceName, ifaceMtu, ifaceId ) );
    UNTIL_ERROR ( eCode, setupFd ( tunFd ) );

    if ( NOT_OK ( eCode ) )
    {
        // setupFd (the last step above) only sets _fd if it succeeds. Since it failed,
        // _fd should still be < 0.
        assert ( _fd < 0 );

        if ( tunFd >= 0 )
        {
            ::close ( tunFd );
            tunFd = -1;
        }

        return eCode;
    }

    // setupFd succeeded, so _fd should be set

    assert ( _fd == tunFd );

    _ifaceId = ifaceId;
    _ifaceName = ifaceName;

    configureMemPool ( ifaceMtu );

    assert ( ifaceId >= 0 );

    return Error::Success;
}

bool TunIfaceDev::isInitialized() const
{
    return ( _fd >= 0 );
}

bool TunIfaceDev::isManaged()
{
    return ( _ifaceId >= 0 );
}

ERRCODE TunIfaceDev::addAddress ( const IpAddress & addr )
{
    // If we're in unmanaged mode, we can't add addresses!
    if ( !isManaged() )
    {
        return Error::AddrError;
    }

    ERRCODE eCode = TunIface::addAddress ( addr );

    if ( IS_OK ( eCode ) )
    {
        eCode = NetManager::get().addIfaceAddress ( _ifaceId, addr );

        if ( NOT_OK ( eCode ) )
        {
            TunIface::removeAddress ( addr );
        }
    }

    return eCode;
}

bool TunIfaceDev::removeAddress ( const IpAddress & addr )
{
    // If we're in unmanaged mode, we can't remove addresses!
    if ( !isManaged() )
    {
        return false;
    }

    if ( !TunIface::removeAddress ( addr ) )
        return false;

    NetManager::get().removeIfaceAddressAsync ( _ifaceId, addr );

    return true;
}

int TunIfaceDev::getIfaceId() const
{
    return _ifaceId;
}

const String & TunIfaceDev::getIfaceName() const
{
    return _ifaceName;
}

void TunIfaceDev::configureMemPool ( int ifaceMtu )
{
    if ( ifaceMtu <= 0 )
    {
        _ifaceMtu = 0;
    }
    else if ( ifaceMtu >= 0xFFFF )
    {
        _ifaceMtu = 0xFFFF;
    }
    else if ( ( uint16_t ) ifaceMtu <= MinMTU )
    {
        _ifaceMtu = MinMTU;
    }
    else
    {
        _ifaceMtu = ( uint16_t ) ifaceMtu;
    }

    if ( _ifaceMtu <= PacketDataStore::PacketSize )
    {
        // "default" MTU, or smaller than PacketDataStore's packet size - no need for a custom memory pool.

        if ( _memPool != 0 )
        {
            _memPool->shutdown();
            _memPool = 0;
        }

        return;
    }

    assert ( _ifaceMtu > 0 );

    if ( _memPool != 0 )
    {
        if ( _ifaceMtu <= _memPool->PayloadSize )
        {
            // We currently have a custom memory pool that uses large enough packets.
            return;
        }

        _memPool->shutdown();
        _memPool = 0;
    }

    assert ( !_memPool );

    // We need a custom memory pool!

    const uint32_t blocksPerSlab
        = optMaxMemorySize.value() * 1024 * 1024
          / PacketMaxSlabs
          / ( _ifaceMtu + MemPool::DefaultPayloadOffset );

    _memPool = new PacketMemPool ( _ifaceMtu, blocksPerSlab, PacketMaxSlabs );
}
