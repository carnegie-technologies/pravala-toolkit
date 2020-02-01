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

#include "PacketDataStore.hpp"
#include "UdpFdSocket.hpp"

using namespace Pravala;

ConfigNumber<bool> UdpFdSocket::optUseAsyncWrites (
        0,
        "os.udp.fd.async_writes",
        "Set to true to enable asynchronous UDP writes over individual FDs (if possible)",
        false
);

ConfigNumber<bool> UdpFdSocket::optUseMultiWrites (
        0,
        "os.udp.fd.multi_writes",
        "Set to true to enable writing multiple UDP packets at a time (if possible)",
        false
);

ConfigLimitedNumber<uint16_t> UdpFdSocket::optQueueSize (
        0,
        "os.udp.fd.write_queue_size",
        "The length of per-socket write queue (only used if needed)",
        4, 1000, 16
);

ConfigLimitedNumber<uint16_t> UdpFdSocket::optMultiReadSize (
        0,
        "os.udp.fd.multi_read_size",
        "The max number of messages to read at a time",
        8, 1000, 16
);

UdpFdSocket::UdpFdSocket ( SocketOwner * owner ):
    UdpSocket ( owner ),
    _writer ( PacketWriter::SocketWriter,
            ( optUseAsyncWrites.value() ? ( PacketWriter::FlagThreaded ) : 0 )
            | ( optUseMultiWrites.value() ? ( PacketWriter::FlagMultiWrite ) : 0 ),
            optQueueSize.value() ),
    _reader ( optMultiReadSize.value() ),
    _sockFd ( -1 )
{
}

UdpFdSocket::~UdpFdSocket()
{
    close();
}

bool UdpFdSocket::sockInitFd ( SocketApi::SocketType sockType, int & sockFd )
{
    if ( !UdpSocket::sockInitFd ( sockType, sockFd ) )
        return false;

    if ( sockFd >= 0 )
    {
        _writer.setupFd ( sockFd );
    }
    else
    {
        _writer.clearFd();
    }

    return true;
}

String UdpFdSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "UDP(FD):[%1-%2]" ).arg ( _localAddr.toString(), _remoteAddr.toString() );
    }

    return String ( "UDP(FD):" ).append ( _remoteAddr.toString() );
}

void UdpFdSocket::close()
{
    if ( _sockFd >= 0 )
    {
        LOG ( L_DEBUG, getLogId() << ": Closing the socket" );

        _writer.clearFd();

        EventManager::closeFd ( _sockFd );
        _sockFd = -1;
    }

    UdpSocket::close();
}

int UdpFdSocket::stealSockFd()
{
    const int sockFd = _sockFd;

    _sockFd = -1;

    if ( sockFd >= 0 )
    {
        _writer.clearFd();

        EventManager::removeFdHandler ( sockFd );
        close();
    }

    return sockFd;
}

bool UdpFdSocket::getOption ( int level, int optName, MemHandle & value ) const
{
    return SocketApi::getOption ( _sockFd, level, optName, value );
}

int UdpFdSocket::increaseRcvBufSize ( int size )
{
    return SocketApi::increaseRcvBufSize ( _sockFd, size );
}

int UdpFdSocket::increaseSndBufSize ( int size )
{
    return SocketApi::increaseSndBufSize ( _sockFd, size );
}

const SockAddr & UdpFdSocket::getLocalSockAddr() const
{
    return _localAddr;
}

const SockAddr & UdpFdSocket::getRemoteSockAddr() const
{
    return _remoteAddr;
}

ERRCODE UdpFdSocket::bind ( const SockAddr & addr )
{
    return UdpSocket::ipSockBind ( addr, _sockFd, _localAddr, _remoteAddr );
}

ERRCODE UdpFdSocket::bindToIface ( const String & ifaceName, IpAddress::AddressType addrType )
{
    return UdpFdSocket::ipSockBindToIface (
        ( addrType == IpAddress::V4Address )
        ? SocketApi::SocketDgram4
        : SocketApi::SocketDgram6,
        ifaceName, _sockFd, _localAddr, _remoteAddr );
}

ERRCODE UdpFdSocket::connect ( const SockAddr & addr )
{
    if ( addr.sa.sa_family == AF_UNSPEC )
    {
        // UDP socket is being disconnected.
        if ( _sockFd < 0 )
            return Error::NotInitialized;

        if ( 0 == ::connect ( _sockFd, const_cast<struct sockaddr *> ( &addr.sa ), ( socklen_t ) sizeof ( addr.sa ) ) )
        {
            _remoteAddr.clear();

            clearFlags ( SockFlagConnected | SockFlagConnecting );
            clearEvents ( SockEventConnected | SockEventConnectFailed );

            return Error::Success;
        }

        LOG ( L_ERROR, getLogId() << ": Error disconnecting the socket: " << SocketApi::getLastErrorDesc() );

        return Error::ConnectFailed;
    }

    if ( _remoteAddr.hasIpAddr() )
    {
        // UDP sockets can be re-connected to a different address.
        _remoteAddr.clear();
    }

    const ERRCODE eCode = IpSocket::ipSockConnect ( addr, _sockFd, _localAddr, _remoteAddr );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead );

    LOG ( L_DEBUG, getLogId() << ": Successfully connected socket to " << addr );

    // Even though this UDP socket would always connect (or fail) right away,
    // we want to say 'connecting' with a 'connected' callback later.
    // This simplifies the API and allows us to create UDP sockets that actually can take some time to connect
    // (for example when they use SOCKS5 proxies).
    //
    // UDP can be used right away, but if this class is inherited by something else that needs additional steps,
    // we won't become connected until later. We do, however, want to allow the inheriting class to be able to send
    // data over UDP to perform its handshake.
    // To achieve that, we set 'UDP connected' right away (and we check it in send()).

    clearFlags ( SockFlagConnected );
    setFlags ( SockFlagConnecting | SockUdpFlagConnected );

    scheduleEvents ( SockEventConnected );
    return Error::Success;
}

ERRCODE UdpFdSocket::send ( const char * data, size_t & dataSize )
{
    if ( hasFlag ( SockUdpFlagConnected ) )
    {
        return handleWriteResult ( _writer.write ( data, dataSize ), _remoteAddr );
    }
    else if ( _remoteAddr.hasIpAddr() && _remoteAddr.hasPort() )
    {
        return handleWriteResult ( _writer.write ( _remoteAddr, data, dataSize ), _remoteAddr );
    }
    else
    {
        return Error::NotConnected;
    }
}

ERRCODE UdpFdSocket::send ( MemHandle & data )
{
    if ( hasFlag ( SockUdpFlagConnected ) )
    {
        return handleWriteResult ( _writer.write ( data ), _remoteAddr );
    }
    else if ( _remoteAddr.hasIpAddr() && _remoteAddr.hasPort() )
    {
        return handleWriteResult ( _writer.write ( _remoteAddr, data ), _remoteAddr );
    }
    else
    {
        return Error::NotConnected;
    }
}

ERRCODE UdpFdSocket::send ( MemVector & data )
{
    if ( hasFlag ( SockUdpFlagConnected ) )
    {
        return handleWriteResult ( _writer.write ( data ), _remoteAddr );
    }
    else if ( _remoteAddr.hasIpAddr() && _remoteAddr.hasPort() )
    {
        return handleWriteResult ( _writer.write ( _remoteAddr, data ), _remoteAddr );
    }
    else
    {
        return Error::NotConnected;
    }
}

ERRCODE UdpFdSocket::sendTo ( const SockAddr & addr, MemHandle & data )
{
    return handleWriteResult ( _writer.write ( addr, data ), addr );
}

ERRCODE UdpFdSocket::sendTo ( const SockAddr & addr, MemVector & data )
{
    return handleWriteResult ( _writer.write ( addr, data ), addr );
}

ERRCODE UdpFdSocket::sendTo ( const SockAddr & addr, const char * data, size_t dataSize )
{
    return handleWriteResult ( _writer.write ( addr, data, dataSize ), addr );
}

ERRCODE UdpFdSocket::handleWriteResult ( ERRCODE eCode, const SockAddr & addr )
{
    ( void ) addr;

    if ( eCode == Error::Closed )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending data; Scheduling the socket to be closed; Local: " << _localAddr
                  << "; Remote: " << _remoteAddr << "; Requested destination: " << addr
                  << "; Error: " << SocketApi::getLastErrorDesc() );

        scheduleEvents ( SockEventClosed );
    }
    else if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending data; Local: " << _localAddr << "; Remote: " << _remoteAddr
                  << "; Requested destination: " << addr
                  << "; Error: " << SocketApi::getLastErrorDesc() );
    }

    return eCode;
}

UdpSocket * UdpFdSocket::generateConnectedSock ( SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode )
{
    ERRCODE tmpCode;

    if ( !errCode )
        errCode = &tmpCode;

    if ( _sockFd < 0 )
    {
        *errCode = Error::NotInitialized;
        return 0;
    }

    if ( !_localAddr.hasIpAddr() )
    {
        *errCode = Error::ConfigError;
        return 0;
    }

    if ( !remoteAddr.hasIpAddr() || !remoteAddr.hasPort() )
    {
        *errCode = Error::InvalidParameter;
        return 0;
    }

    UdpFdSocket * const newSock = new UdpFdSocket ( owner );

    if ( !newSock )
    {
        *errCode = Error::MemoryError;
        return 0;
    }

    *errCode = newSock->bind ( _localAddr );

    if ( IS_OK ( *errCode ) )
    {
        *errCode = newSock->connect ( remoteAddr );
    }

    if ( NOT_OK ( *errCode ) )
    {
        LOG ( L_ERROR, getLogId() << ": Error generating a new socket connected to " << remoteAddr
              << "; Error: " << ( *errCode ) );

        newSock->unrefOwner ( owner );
        return 0;
    }

    newSock->clearFlags ( SockFlagConnecting );
    newSock->setFlags ( SockFlagConnected | SockUdpFlagConnected );

    newSock->clearEvents ( SockEventConnected );

    return newSock;
}

void UdpFdSocket::receiveFdEvent ( int fd, short int events )
{
    assert ( fd >= 0 );
    assert ( fd == _sockFd );

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
        doSockClosed ( eCode );
        return;
    }
    else if ( NOT_OK ( eCode ) || numPackets < 1 )
    {
        return;
    }

    // We will be (potentially) calling the callback multiple times.
    // Let's create a self-reference to make sure we don't get removed.

    simpleRef();

    for ( uint16_t i = 0; i < numPackets && getOwner() != 0; ++i )
    {
        MemHandle data;
        SockAddr addr;

        if ( !_reader.getPacket ( i, data, addr ) || data.isEmpty() )
        {
            continue;
        }

        if ( addr.hasIpAddr() )
        {
            _remoteAddr = addr;
        }

        size_t prevSize = 0;

        while ( data.size() > 0 && prevSize != data.size() && isValid() )
        {
            prevSize = data.size();
            callSockDataReceived ( data );
        }
    }

    simpleUnref();
    return;
}

bool UdpFdSocket::getDestAddrToUse ( const SockAddr & destAddr, SockAddr & destAddrToUse )
{
    if ( hasFlag ( SockUdpFlagConnected ) )
    {
        // This socket is connected. We don't need an address at all.
        // But we still need to check if 'destAddr' makes sense.
        // We are OK with an invalid destination address,
        // or a valid address that matches the one we are connected to.

        if ( destAddr.hasIpAddr() && destAddr != _remoteAddr )
        {
            LOG ( L_ERROR, getLogId() << ": Refusing to send data to " << destAddr
                  << " over UDP socket connected to a different remote address: " << _remoteAddr );

            return false;
        }

        // Note: We clear the address to use, to let the caller know that they should use send() instead of sendto().
        // While on Linux sendto() with an address on connected UDP socket works,
        // on iOS/OSX it does not (even if the address is the same as what the socket is connected to).
        // So we don't want to use it at all in connected mode.

        destAddrToUse.clear();
        return true;
    }

    if ( !destAddr.hasIpAddr() || !destAddr.hasPort() )
    {
        // We are not connected, so we need an actual address!
        LOG ( L_ERROR, getLogId() << ": Invalid destination address for unconnected UDP socket: " << destAddr );
        return false;
    }

    destAddrToUse = destAddr;
    return true;
}
