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

// Common functions for all UdpListener implementations that use FDs
// Contains:
// - config options
// - constructor/destructor
// - create a system FD socket and bind it to a listening port
// - close the system FD
// - check validity of object

#include <cassert>
#include <cerrno>

#include "basic/MsvcSupport.hpp"
#include "sys/SocketApi.hpp"

#include "UdpFdListener.hpp"

using namespace Pravala;

ConfigNumber<uint16_t> UdpFdListener::optMaxSendSpeed (
        0,
        "max-mas-send-speed",
        0,
        "os.udp_listener.max_send_speed",
        "Maximum send spend in Mbps. It also enables writes on a separate thread. 0 means unlimited. "
        "Precision of this limiting depends on the internal bucket size. Lower values are often not enforceable, "
        "especially when a small bucket is used.",
        0
);

ConfigLimitedNumber<uint16_t> UdpFdListener::optMaxRecvPackets (
        0,
        "os.udp_listener.max_recv_packets",
        "Maximum number of packets to receive per a single UDP recv call",
        1, 1024, 64
);

ConfigLimitedNumber<uint16_t> UdpFdListener::optMaxSendPackets (
        0,
        "os.udp_listener.max_send_packets",
        "Maximum number of packets to send per a single UDP send call",
        1, 1024, 64
);

ConfigLimitedNumber<int> UdpFdListener::optRecvBufSize (
        0,
        "os.udp_listener.buffer_size.recv",
        "UDP listener's socket receive buffer size (in bytes) to try to use (if it is smaller than that)",
        1, SocketApi::MaxBufferSize );

ConfigLimitedNumber<int> UdpFdListener::optSendBufSize (
        0,
        "os.udp_listener.buffer_size.send",
        "UDP listener's socket send buffer size (in bytes) to try to use (if it is smaller than that)",
        1, SocketApi::MaxBufferSize );

UdpFdListener::UdpFdListener():
    _writer ( PacketWriter::SocketWriter,
            PacketWriter::FlagMultiWrite
            | ( ( optMaxSendSpeed.value() > 0 ) ? ( PacketWriter::FlagThreaded ) : 0 ),
            optMaxSendPackets.value(),
            optMaxSendSpeed.value() ),
    _reader ( optMaxRecvPackets.value() ),
    _fd ( -1 )
{
}

UdpFdListener::~UdpFdListener()
{
    closeSock();

    assert ( _fd < 0 );
}

String UdpFdListener::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "UDP_Listener[%1,%2]" ).arg ( _localAddr ).arg ( _fd );
    }

    return String ( "UDP_Listener[%1]" ).arg ( _localAddr );
}

UdpFdListener * UdpFdListener::generate ( const SockAddr & localAddr, ERRCODE * errCode )
{
    UdpFdListener * const ret = new UdpFdListener();

    if ( !ret )
    {
        if ( errCode != 0 )
        {
            *errCode = Error::MemoryError;
        }

        return 0;
    }

    ERRCODE eCode = ret->reinitialize ( localAddr );

    if ( errCode != 0 )
        *errCode = eCode;

    if ( IS_OK ( eCode ) && ret->isValid() )
    {
        return ret;
    }

    ret->unref();

    LOG_ERR ( L_ERROR, eCode, "Error initializing a new UdpListener listening on " << localAddr );

    return 0;
}

ERRCODE UdpFdListener::reinitializeImpl ( const SockAddr & localAddr )
{
    closeSock();

    ERRCODE eCode;

    const int fd = SocketApi::createUdpSocket ( localAddr, false, &eCode );

    if ( fd < 0 )
    {
        LOG_ERR ( L_ERROR, eCode,
                  "Could not create a UDP socket on " << localAddr << ": " << SocketApi::getLastErrorDesc() );

        return eCode;
    }

    if ( !SocketApi::getName ( fd, _localAddr ) )
    {
        LOG ( L_ERROR, "Unable to get local address of a UDP socket: " << SocketApi::getLastErrorDesc() );

        SocketApi::close ( fd );

        return Error::SocketFailed;
    }

    assert ( fd >= 0 );

    _fd = fd;
    _writer.setupFd ( fd );

    if ( optRecvBufSize.isSet() && optRecvBufSize.value() > 0 )
    {
        const int size = increaseRcvBufSize ( optRecvBufSize.value() );

        if ( size < optRecvBufSize.value() )
        {
            LOG ( L_WARN, "Failed to increase receive buffer size of UDP socket " << _fd << " to: "
                  << optRecvBufSize.value() << "; Size used: " << size );
        }
        else
        {
            LOG ( L_INFO, "Configured receive buffer size of UDP socket " << _fd << " to: " << size );
        }
    }

    if ( optSendBufSize.isSet() && optSendBufSize.value() > 0 )
    {
        const int size = increaseSndBufSize ( optSendBufSize.value() );

        if ( size < optSendBufSize.value() )
        {
            LOG ( L_WARN, "Failed to increase send buffer size of UDP socket " << _fd << " to: "
                  << optSendBufSize.value() << "; Size used: " << size );
        }
        else
        {
            LOG ( L_INFO, "Configured send buffer size of UDP socket " << _fd << " to: " << size );
        }
    }

    EventManager::setFdHandler ( _fd, this, EventManager::EventRead );

    LOG ( L_DEBUG, "UDP FD-based Listener (re)initialized; Local address: " << _localAddr );

    return Error::Success;
}

int UdpFdListener::increaseRcvBufSize ( int size )
{
    return SocketApi::increaseRcvBufSize ( _fd, size );
}

int UdpFdListener::increaseSndBufSize ( int size )
{
    return SocketApi::increaseSndBufSize ( _fd, size );
}

bool UdpFdListener::isValid() const
{
    return ( _fd >= 0 );
}

void UdpFdListener::closeSock()
{
    if ( _fd >= 0 )
    {
        LOG ( L_DEBUG, getLogId() << ": Closing the socket" );

        _writer.clearFd();

        EventManager::closeFd ( _fd );
        _fd = -1;
    }
}

void UdpFdListener::receiveFdEvent ( int fd, short int events )
{
    assert ( fd >= 0 );
    assert ( fd == _fd );
    assert ( isValid() );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        // We shouldn't be getting those, but in case we do:
        EventManager::disableWriteEvents ( fd );
    }

    if ( ( events & EventManager::EventRead ) != EventManager::EventRead )
    {
        return;
    }

    uint16_t numPackets = 0;
    const ERRCODE eCode = _reader.readPackets ( fd, *this, numPackets );

    LOG_ERR ( L_DEBUG4, eCode, getLogId() << ": Read packets: " << numPackets );

    if ( eCode == Error::Closed )
    {
        closeSock();

        assert ( !isValid() );

        notifyClosed();
        return;
    }
    else if ( eCode == Error::SoftFail || ( IS_OK ( eCode ) && numPackets < 1 ) )
    {
        // Temporary error; Nothing to do!
        return;
    }
    else if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error reading packets; Local: " << _localAddr
                  << "; Error: " << strerror ( errno ) << "; Closing socket" );

        closeSock();

        assert ( !isValid() );

        notifyClosed();
        return;
    }

    // Take a reference in case someone unrefs us while we're looping
    ref();

    for ( uint16_t i = 0; i < numPackets; ++i )
    {
        MemHandle data;
        SockAddr addr;

        if ( !_reader.getPacket ( i, data, addr ) || data.isEmpty() )
        {
            continue;
        }

        dataReceived ( addr, data );
    }

    unref();
    return;
}

ERRCODE UdpFdListener::handleWriteResult ( ERRCODE eCode, const SockAddr & addr )
{
    ( void ) addr;

    if ( eCode == Error::Closed )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending data; Local: " << _localAddr << "; Remote: " << addr << "; Closing socket" );

        closeSock();
        notifyClosed();

        return Error::Closed;
    }
    else if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending data; Local: " << _localAddr << "; Requested destination: " << addr );
    }

    return eCode;
}

ERRCODE UdpFdListener::send ( const SockAddr & addr, MemVector & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( data.isEmpty() )
    {
        return Error::Success;
    }

    // write() checks if the address makes sense:
    return handleWriteResult ( _writer.write ( addr, data ), addr );
}

ERRCODE UdpFdListener::send ( const SockAddr & addr, MemHandle & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( data.isEmpty() )
    {
        return Error::Success;
    }

    // write() checks if the address makes sense:
    return handleWriteResult ( _writer.write ( addr, data ), addr );
}

ERRCODE UdpFdListener::send ( const SockAddr & addr, const char * data, size_t dataSize )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( dataSize < 1 )
    {
        return Error::Success;
    }

    // write() checks if the address makes sense:
    return handleWriteResult ( _writer.write ( addr, data, dataSize ), addr );
}
