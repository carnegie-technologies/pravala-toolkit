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

// #define DEBUG_VHOSTNET_TUN 1

extern "C"
{
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <linux/vhost.h>
}

#include <cerrno>
#include <cstdlib>

#include "VhostNet.hpp"
#include "VhostNetMgr.hpp"

// vhost-net device path
#define VHOST_NET_DEV    "/dev/vhost-net"

using namespace Pravala;

TextLog VhostNet::_log ( "vhost_net" );

#ifdef DEBUG_VHOSTNET_TUN

#define MAX_RX_DESCS    16
#define MAX_TX_DESCS    16

#else

#define MAX_RX_DESCS    512
#define MAX_TX_DESCS    512
#endif

VhostNet::VhostNet ( uint8_t memTag ):
    RegisteredMemTag ( memTag ),
    _rxRing ( MAX_RX_DESCS, memTag ), _txRing ( MAX_TX_DESCS, memTag ),
    _vhostFd ( -1 ),
    _rxCallFd ( -1 ), _rxKickFd ( -1 ), _txKickFd ( -1 ),
    _maxPktsReadPerLoop ( 64 ), // Default max 64 pkts per loop, this was arbitrarily picked
    _txNeedKick ( false ),
    _isValid ( false )
{
    // According to virtio specs, max descs must be a power of 2 and should be at least 8

    assert ( MAX_RX_DESCS >= 8 );
    assert ( MAX_TX_DESCS >= 8 );

    // checking that MaxDescs is a power of 2
    assert ( ( MAX_RX_DESCS & ( MAX_RX_DESCS - 1 ) ) == 0 );
    assert ( ( MAX_TX_DESCS & ( MAX_TX_DESCS - 1 ) ) == 0 );
}

VhostNet::~VhostNet()
{
    // Everything should have been cleared already
    assert ( _vhostFd < 0 );
    assert ( _rxCallFd < 0 );
    assert ( _rxKickFd < 0 );
    assert ( _txKickFd < 0 );
    assert ( !_txNeedKick );
    assert ( !_isValid );
}

VhostNet * VhostNet::generateNew()
{
    return new VhostNet ( VhostNetMgr::get().RegisteredMemoryTag );
}

void VhostNet::returnsToPool()
{
    close();
}

void VhostNet::close()
{
    if ( _vhostFd >= 0 )
    {
        // Unregister before closing so that no other thread can re-use/re-register with this FD number until
        // we're done with it.
        //
        // If we closed then unregistered, it's possible some other thread would open a new FD with the system re-using
        // the FD number we just closed before we unregister it. Then when we unregister the FD, we may end up
        // unregistering THEIR FD!
        //
        // By unregistering before closing, the system will guarantee that nobody else will be using this FD
        // (except for us), until we close it.
        VhostNetMgr::get().unregisterVhostNet ( _vhostFd );

        EventManager::closeFd ( _vhostFd );
        _vhostFd = -1;
    }

    if ( _rxCallFd >= 0 )
    {
        EventManager::closeFd ( _rxCallFd );
        _rxCallFd = -1;
    }

    if ( _rxKickFd >= 0 )
    {
        EventManager::closeFd ( _rxKickFd );
        _rxKickFd = -1;
    }

    if ( _txKickFd >= 0 )
    {
        EventManager::closeFd ( _txKickFd );
        _txKickFd = -1;
    }

    _txNeedKick = false;
    _isValid = false;

    _rxRing.clear();
    _txRing.clear();
}

VhostNet * VhostNet::generate ( VhostNetOwner * owner, int tunFd, ERRCODE & eCode )
{
    if ( tunFd < 0 )
    {
        eCode = Error::InvalidParameter;
        return 0;
    }

    VhostNet * vn = getFromPool ( owner );

    assert ( vn != 0 );
    assert ( !vn->isValid() );

    eCode = vn->internalSetup ( tunFd );

    if ( NOT_OK ( eCode ) )
    {
        vn->unrefOwner ( owner );
        vn = 0;

        return 0;
    }

    assert ( vn->isValid() );
    assert ( vn->_vhostFd >= 0 );
    assert ( vn->_rxCallFd >= 0 );
    assert ( vn->_rxKickFd >= 0 );
    assert ( vn->_txKickFd >= 0 );

    bool ret = VhostNetMgr::get().registerVhostNet ( vn->_vhostFd, vn );

    if ( !ret )
    {
        // Failed to set up memory regions
        eCode = Error::MemoryError;

        vn->unrefOwner ( owner );
        vn = 0;

        return 0;
    }

    return vn;
}

ERRCODE VhostNet::internalSetup ( int tunFd )
{
    assert ( !isValid() );
    assert ( tunFd >= 0 );
    assert ( _vhostFd < 0 );
    assert ( _rxCallFd < 0 );
    assert ( _rxKickFd < 0 );
    assert ( _txKickFd < 0 );

    if ( tunFd < 0 )
    {
        return Error::InvalidParameter;
    }

    _vhostFd = open ( VHOST_NET_DEV, O_RDWR );

    if ( _vhostFd < 0 )
    {
        LOG ( L_ERROR, "Failed to open vhost-net device: " VHOST_NET_DEV ". Error: " << strerror ( errno ) );
        return Error::NotAvailable;
    }

    _rxCallFd = eventfd ( 0, 0 );

    if ( _rxCallFd < 0 )
    {
        LOG ( L_ERROR, "Failed to set up RX call eventfd. Error: " << strerror ( errno ) );
        return Error::NotAvailable;
    }

    _rxKickFd = eventfd ( 0, 0 );

    if ( _rxKickFd < 0 )
    {
        LOG ( L_ERROR, "Failed to set up RX kick eventfd. Error: " << strerror ( errno ) );
        return Error::NotAvailable;
    }

    _txKickFd = eventfd ( 0, 0 );

    if ( _txKickFd < 0 )
    {
        LOG ( L_ERROR, "Failed to set up TX kick eventfd. Error: " << strerror ( errno ) );
        return Error::NotAvailable;
    }

    // I'm not really sure what this does, but it seems to be required
    int ret = ioctl ( _vhostFd, VHOST_SET_OWNER, 0 );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set owner on vhost-net FD: " << _vhostFd << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    // Start it off with an empty memory region
    struct vhost_memory mmapEmpty;
    memset ( &mmapEmpty, 0, sizeof ( mmapEmpty ) );

    ret = ioctl ( _vhostFd, VHOST_SET_MEM_TABLE, &mmapEmpty );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set empty memory region on vhost-net FD: " << _vhostFd
              << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    // - Enable moderated interrupts (generate less events)
    // - Enable VirtIO header (required)
    const uint64_t features = ( 1ULL << VIRTIO_RING_F_EVENT_IDX )
                              | ( 1ULL << VHOST_NET_F_VIRTIO_NET_HDR );

    ret = ioctl ( _vhostFd, VHOST_SET_FEATURES, &features );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set features on vhost-net FD: " << _vhostFd << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    struct vhost_vring_file vrf;

    vrf.index = Vring::RxVringIdx;
    vrf.fd = _rxCallFd;
    ret = ioctl ( _vhostFd, VHOST_SET_VRING_CALL, &vrf );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set call/rx fd on vhost-net FD: " << _vhostFd << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    vrf.index = Vring::RxVringIdx;
    vrf.fd = _rxKickFd;
    ret = ioctl ( _vhostFd, VHOST_SET_VRING_KICK, &vrf );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set kick/rx fd on vhost-net FD: " << _vhostFd << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    vrf.index = Vring::TxVringIdx;
    vrf.fd = _txKickFd;
    ret = ioctl ( _vhostFd, VHOST_SET_VRING_KICK, &vrf );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set kick/tx fd on vhost-net FD: " << _vhostFd << ". Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    // We can only set up the rings after _vhostFd has been set up as above.

    ERRCODE eCode = _rxRing.setup ( _vhostFd, tunFd );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Failed to setup RX ring" );
        return eCode;
    }

    eCode = _txRing.setup ( _vhostFd, tunFd );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Failed to setup TX ring" );
        return eCode;
    }

    EventManager::setFdHandler ( _rxCallFd, this, EventManager::EventRead );

    _isValid = true;

    return Error::Success;
}

void VhostNet::closeAndScheduleNotify()
{
    close();

    EventManager::loopEndSubscribe ( this );
}

void VhostNet::doClosedNotify()
{
    EventManager::loopEndUnsubscribe ( this );

    VhostNetOwner * owner = getOwner();

    if ( owner != 0 )
    {
        owner->vhostNetClosed ( this );
    }
}

void VhostNet::receiveLoopEndEvent()
{
    LOG ( L_DEBUG4, "Loop end" );

    if ( !isValid() )
    {
        doClosedNotify();
        return;
    }

    if ( _txNeedKick )
    {
        _txNeedKick = false;
        kickFd ( _txKickFd, "TX" );
    }

    cleanTx();

    doRead();

    // must return immediately after doRead()
    return;
}

void VhostNet::receiveFdEvent ( int fd, short int events )
{
    ( void ) fd;
    ( void ) events;

    LOG ( L_DEBUG4, "FD event" );

    if ( !isValid() )
    {
        doClosedNotify();
        return;
    }

    // Should only get events for the call FD
    assert ( fd == _rxCallFd );

    // Should only get read events
    assert ( ( events & EventManager::EventRead ) == EventManager::EventRead );

    uint64_t junk = 0;

    // Read to clear the "something happened" flag, must be a uint64_t (on all platforms).
    // This is a counter, cleared by read. See eventfd()
    int ret = read ( fd, &junk, sizeof ( junk ) );

    if ( ret <= 0 )
    {
        LOG ( L_ERROR, "Closing. Got error from reading vhost-net fd: "
              << fd << "; Error: " << strerror ( errno ) );

        close();
        doClosedNotify();

        return;
    }

    cleanTx();

    doRead();

    // must return immediately after doRead()
    return;
}

void VhostNet::doRead()
{
    assert ( isValid() );
    assert ( _rxCallFd >= 0 );

    LOG ( L_DEBUG4, "doRead called" );

    ERRCODE eCode = Error::Success;

    // Self reference while we potentially read and call back multiple times
    // DO NOT RETURN BEFORE DEREF!
    simpleRef();

    for ( uint8_t i = 0; i < _maxPktsReadPerLoop; ++i )
    {
        LOG ( L_DEBUG4, "Reading packet " << String::number ( i ) );

        // readPacket will set these buffers to its buffers
        MemHandle vhdr, buf;

        eCode = _rxRing.readPacket ( vhdr, buf );

        if ( NOT_OK ( eCode ) )
        {
            // All error handling is done outside the loop
            LOG_ERR ( L_DEBUG4, eCode, "Read failed" );
            break;
        }

        assert ( !vhdr.isEmpty() );
        assert ( !buf.isEmpty() );

        LOG ( L_DEBUG4, "Read packet, vhdr length: " << vhdr.size() << "; packet length: " << buf.size() );

        // We don't use the virtio header right now
        vhdr.clear();

        VhostNetOwner * owner = getOwner();

        if ( owner != 0 )
        {
            owner->vhostPacketReceived ( this, buf );
        }

        if ( !isValid() )
        {
            simpleUnref();
            return;
        }
    }

    assert ( isValid() );
    assert ( _rxCallFd >= 0 );
    assert ( _rxKickFd >= 0 );

    if ( _rxRing.refill() )
    {
        kickFd ( _rxKickFd, "RX" );
    }

    LOG_ERR ( L_DEBUG4, eCode, "Done reading packets" );

    if ( eCode == Error::Success || eCode == Error::EmptyRead )
    {
        // We stopped reading because we hit our read limit (Error::Success).
        // In this case, there might be more packets to read, so make sure events are disabled,
        // and subscribe to loop end to read more!
        //
        // Alternatively, we read a packet that hadn't been fully written to our memory yet (Error::EmptyRead),
        // it will almost certainly be there next loop, so subscribe to end of loop to read it.
        //
        // This is preferable to disabling/re-enabling read events because:
        // - If we hit our read limit, chances are there's more data to be read
        // - Even if there isn't more data now, there will likely be more data to read by next loop end
        //   (i.e. between calling epoll or similar)
        // - If read events had already been disabled, then disableReadEvents is a no-op, which avoids
        //   the syscall to enable/disable events when we expect there will be more data very soon.
        EventManager::disableReadEvents ( _rxCallFd );
        EventManager::loopEndSubscribe ( this );
    }
    else if ( eCode == Error::SoftFail )
    {
        // We stopped reading because there are no more packets to read, re-enable read events
        EventManager::enableReadEvents ( _rxCallFd );
    }
    else if ( eCode == Error::IncompleteData )
    {
        // Got incomplete data, this is really odd since it means the system sent us a packet without enough data
        // for the virtio header or with an empty payload.
        LOG ( L_ERROR, "Got incomplete data. Re-enabling read events." );
        EventManager::enableReadEvents ( _rxCallFd );

        assert ( false );
    }
    else
    {
        // Some other error, close it
        LOG_ERR ( L_ERROR, eCode, "Error reading, clearing VhostNet" );

        close();
        doClosedNotify();
    }

    // Self unreference
    simpleUnref();
}

void VhostNet::kickFd ( int fd, const char * log )
{
    ( void ) log;
    assert ( fd >= 0 );
    assert ( log != 0 );

    static const uint64_t kick = 1; // eventfd expects a uint64

    LOG ( L_DEBUG4, "Kicking " << log << ". FD: " << fd );

    assert ( isValid() );

    int ret = ::write ( fd, &kick, sizeof ( kick ) );

    if ( ret <= 0 )
    {
        LOG ( L_ERROR, "Failed writing to kick FD for " << log << ". Closing. KickFd: " << fd );

        // This will notify our owner that we've been closed at end of loop.
        closeAndScheduleNotify();
    }
}

void VhostNet::cleanTx()
{
    LOG ( L_DEBUG4, "Cleaning TX" );

    if ( !_txRing.cleanUsed() )
    {
        LOG ( L_DEBUG4, "More TX to clean" );

        // Subscribe to loop end if there are packets that need to be cleaned later,
        // i.e. packets that haven't been transmitted yet.
        //
        // This is preferable to using events on the TX ring since the underlying hardware
        // tends to be able to "transmit" packets fairly quickly (i.e. take it out of our
        // queue and put it into its internal queue), so by next loop end it's likely done
        // and ready to clean... and we avoid the syscalls of having to change the event mask or to consume the event.
        EventManager::loopEndSubscribe ( this );
    }
}
