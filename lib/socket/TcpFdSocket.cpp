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

extern "C"
{
#ifdef SYSTEM_WINDOWS
#include <winsock2.h>
#else
// netinet/tcp.h uses preprocessor macros that are defined in sys/cdefs.h on Android,
// but does not include sys/cdefs.h itself on 64-bit platforms.
#include <sys/cdefs.h>
#include <netinet/tcp.h>
#endif
}

#include <cerrno>

#include "basic/Math.hpp"

#include "PacketDataStore.hpp"
#include "TcpFdSocket.hpp"

using namespace Pravala;

/// @brief Sets the maximum segment size in outgoing TCP connections.
/// It allows us to force TCP to use a specific MSS value
/// (to use lower MSS than it would normally negotiate).
/// This normally should NOT be set.
/// We used it to deal with a misconfigured network/server.
static ConfigLimitedNumber<uint16_t> optSetMss (
        0,
        "os.tcp.set_mss",
        "Sets the maximum segment size in outgoing TCP connections.",
        28,
        0xFFFF
);

TcpFdSocket::TcpFdSocket ( SocketOwner * owner ):
    TcpSocket ( owner ),
    _sockFd ( -1 ),
    _maxReadSize ( PacketDataStore::PacketSize )
{
}

TcpFdSocket::TcpFdSocket ( SocketOwner * owner, int sockFd, const SockAddr & localAddr, const SockAddr & remoteAddr ):
    TcpSocket ( owner, localAddr, remoteAddr ),
    _sockFd ( sockFd ),
    _maxReadSize ( PacketDataStore::PacketSize )
{
    if ( _sockFd >= 0 )
    {
        setFlags ( SockFlagValid | SockFlagConnected | SockTcpFlagConnected );

        EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead | EventManager::EventWrite );
    }
}

TcpFdSocket::TcpFdSocket ( SocketOwner * owner, TcpSocket * sock ):
    TcpSocket ( owner,
            ( sock != 0 ) ? sock->getLocalSockAddr() : EmptySockAddress,
            ( sock != 0 ) ? sock->getRemoteSockAddr() : EmptySockAddress ),
    _sockFd ( ( sock != 0 ) ? sock->stealSockFd() : -1 ),
    _maxReadSize ( PacketDataStore::PacketSize )
{
    if ( !sock || _sockFd < 0 )
        return;

    _readBuf = sock->getReadBuffer();

    if ( !_readBuf.isEmpty() )
    {
        sock->consumeReadBuffer ( _readBuf.size() );
    }

    setFlags ( SockFlagValid | SockFlagConnected | SockTcpFlagConnected );

    EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead | EventManager::EventWrite );
}

TcpFdSocket::~TcpFdSocket()
{
    close();
}

String TcpFdSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "TCP:[%2-%3,%1]" ).arg ( _sockFd ).arg ( _localAddr.toString(), _remoteAddr.toString() );
    }

    return String ( "TCP:" ).append ( _remoteAddr.toString() );
}

void TcpFdSocket::close()
{
    if ( _sockFd >= 0 )
    {
        LOG ( L_DEBUG2, getLogId() << ": Closing socket; Size of data in read buffer: " << _readBuf.size() );

        EventManager::closeFd ( _sockFd );
        _sockFd = -1;
    }

    _readBuf.clear();

    IpSocket::close();
}

uint16_t TcpFdSocket::getDetectedMtu() const
{
    return detectMtu ( _sockFd );
}

void TcpFdSocket::setMaxReadSize ( uint16_t maxReadSize )
{
    if ( maxReadSize > 0 )
    {
        _maxReadSize = maxReadSize;
    }
}

TcpFdSocket * TcpFdSocket::generateTcpFdSock ( SocketOwner * owner )
{
    if ( _sockFd < 0 )
    {
        LOG ( L_ERROR, getLogId() << ": Could not generate basic TCP socket: Underlying socket FD is invalid" );
        return 0;
    }

    TcpFdSocket * newSock = new TcpFdSocket ( owner, this );

    if ( !newSock )
        return 0;

    if ( !newSock->isValid() )
    {
        LOG ( L_ERROR, getLogId() << ": Could not generate a valid basic TCP socket" );

        newSock->unrefOwner ( owner );
        newSock = 0;
    }

    return newSock;
}

void TcpFdSocket::doSockConnectFailed ( ERRCODE reason )
{
    if ( _sockFd >= 0 )
    {
        // 'connect failed' does not necessarily mean that the socket will get closed.
        // But the socket is not really usable at this point, and until something else is done from the outside,
        // we want to stop receiving FD events:

        LOG ( L_DEBUG2, getLogId() << ": Socket failed to connect; Disabling all FD events" );

        EventManager::setFdEvents ( _sockFd, 0 );
    }

    TcpSocket::doSockConnectFailed ( reason );
}

int TcpFdSocket::stealSockFd()
{
    const int sockFd = _sockFd;

    _sockFd = -1;

    if ( sockFd >= 0 )
    {
        EventManager::removeFdHandler ( sockFd );
        close();
    }

    return sockFd;
}

bool TcpFdSocket::sockInitFd ( SocketApi::SocketType sockType, int & sockFd )
{
    if ( sockFd >= 0 )
        return true;

    if ( !TcpSocket::sockInitFd ( sockType, sockFd ) )
        return false;

    if ( optSetMss.isSet() )
    {
        const int mss = optSetMss.value();

        if ( SocketApi::setOption ( sockFd, IPPROTO_TCP, TCP_MAXSEG, mss ) )
        {
            LOG ( L_DEBUG2, getLogId() << ": Set TCP MSS to " << mss );
        }
        else
        {
            // We can't set it and print a warning, but we still keep going.
            LOG ( L_WARN, getLogId()
                  << ": Could not set MSS " << mss << " in TCP socket: " << SocketApi::getLastErrorDesc() );
        }
    }

    return true;
}

bool TcpFdSocket::getOption ( int level, int optName, MemHandle & value ) const
{
    return SocketApi::getOption ( _sockFd, level, optName, value );
}

ERRCODE TcpFdSocket::bind ( const SockAddr & addr )
{
    return IpSocket::ipSockBind ( addr, _sockFd, _localAddr, _remoteAddr );
}

ERRCODE TcpFdSocket::bindToIface ( const String & ifaceName, IpAddress::AddressType addrType )
{
    return TcpFdSocket::ipSockBindToIface (
        ( addrType == IpAddress::V4Address )
        ? SocketApi::SocketStream4
        : SocketApi::SocketStream6,
        ifaceName, _sockFd, _localAddr, _remoteAddr );
}

ERRCODE TcpFdSocket::connect ( const SockAddr & addr )
{
    const ERRCODE eCode = IpSocket::ipSockConnect ( addr, _sockFd, _localAddr, _remoteAddr );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    // Needs both read events (to determine connection failure) and write events (to determine connection success)
    EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead | EventManager::EventWrite );

    LOG ( L_DEBUG2, getLogId() << ": Successfully started connection attempt; Internal connect result: " << eCode );

    // We do NOT return the actual code, which could be 'connect in progress'.
    // This API always returns success. But this 'success' always means 'in progress' even if the
    // connection is already established. 'connected' callback will be generated regardless.

    return Error::Success;
}

ERRCODE TcpFdSocket::send ( MemHandle & data )
{
    if ( data.size() < 1 )
    {
        // Nothing to send.
        return Error::Success;
    }

    const char * mem = data.get();
    size_t size = data.size();

    const ERRCODE eCode = send ( mem, size );

    if ( IS_OK ( eCode ) )
    {
        data.consume ( size );
    }

    return eCode;
}

ERRCODE TcpFdSocket::send ( MemVector & data )
{
    return streamSend ( data );
}

ERRCODE TcpFdSocket::send ( const char * data, size_t & dataSize )
{
    if ( _sockFd < 0 || !hasFlag ( SockTcpFlagConnected ) )
    {
        LOG ( L_ERROR, getLogId() << ": Can't send data; Socket is not connected" );
        return Error::NotConnected;
    }

    if ( dataSize < 1 )
    {
        // Nothing to send.
        return Error::Success;
    }

    if ( !data )
    {
        return Error::InvalidParameter;
    }

    const ssize_t ret = ::send ( _sockFd, data, dataSize, 0 );

    if ( ret > 0 )
    {
        LOG ( L_DEBUG4, getLogId() << ": Successfully sent " << ret << " out of " << dataSize << " bytes" );

        if ( ( size_t ) ret < dataSize )
        {
            dataSize = ( size_t ) ret;

            setFlags ( SockFlagSendBlocked );
            EventManager::enableWriteEvents ( _sockFd );
        }

        return Error::Success;
    }

    if ( SocketApi::isErrnoSoft() )
    {
        LOG ( L_DEBUG4, getLogId() << ": Send would block" );

        setFlags ( SockFlagSendBlocked );
        EventManager::enableWriteEvents ( _sockFd );

        return Error::SoftFail;
    }

    if ( SocketApi::isErrnoNonFatal() )
    {
        LOG ( L_ERROR, getLogId ( true )
              << ": Error sending data; Error: " << SocketApi::getLastErrorDesc() );

        return Error::WriteFailed;
    }

    LOG ( L_ERROR, getLogId ( true )
          << ": Error sending data; Scheduling the socket to be closed; Error: " << SocketApi::getLastErrorDesc() );

    scheduleEvents ( SockEventClosed );
    return Error::Closed;
}

void TcpFdSocket::consumeReadBuffer ( size_t size )
{
    _readBuf.consume ( size );

    if ( _readBuf.isEmpty() && _sockFd >= 0 && hasFlag ( SockTcpFlagConnected ) )
    {
        LOG ( L_DEBUG3, getLogId() << ": Read buffer is now empty; Re-enabling read events" );

        EventManager::enableReadEvents ( _sockFd );
    }
}

void TcpFdSocket::receiveFdEvent ( int fd, short int events )
{
    assert ( fd >= 0 );
    assert ( fd == _sockFd );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        // The first write event tells us the TCP connection is complete
        if ( !hasFlag ( SockTcpFlagConnected ) )
        {
            LOG ( L_DEBUG3, getLogId() << ": Socket connected" );

            setFlags ( SockTcpFlagConnected );
            doSockConnected();
            return;
        }

        if ( !hasFlag ( SockFlagSendBlocked ) )
        {
            // There were no failed or incomplete writes.
            EventManager::disableWriteEvents ( fd );
            return;
        }

        clearFlags ( SockFlagSendBlocked );

        // Let's notify the owner that they can write again!
        doSockReadyToSend();
        return;
    }

    // Read events are for data received
    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        // We only want to try reading if we are 'TCP connected'.
        // If we are not, read events mean that the connection attempt failed.

        if ( hasFlag ( SockTcpFlagConnected ) )
        {
            // Disable read events if the read buffer is not empty
            if ( !_readBuf.isEmpty() )
            {
                LOG ( L_DEBUG3, getLogId() << ": Unable to receive data; Read buffer is not empty" );

                EventManager::disableReadEvents ( fd );
                return;
            }

            // Get the data
            MemHandle mh = PacketDataStore::getPacket ( _maxReadSize );

            char * const w = mh.getWritable();

            if ( !w || mh.isEmpty() )
            {
                LOG ( L_ERROR, getLogId() << ": Unable to receive data; Failed to allocate memory" );
                return;
            }

            const ssize_t ret = ::recv ( fd, w, min<size_t> ( mh.size(), _maxReadSize ), 0 );

            if ( ret < 0 )
            {
                LOG ( L_ERROR, getLogId() << ": Error receiving data; Closing socket; Error: "
                      << SocketApi::getLastErrorDesc() );

                doSockClosed ( Error::ReadFailed );
                return;
            }

            if ( ret > 0 )
            {
                assert ( ( size_t ) ret <= mh.size() );

                mh.truncate ( ret );

                _readBuf = mh;

                mh.clear();

                doSockDataReceived ( _readBuf );
                return;
            }

            assert ( ret == 0 );

            LOG ( L_DEBUG3, getLogId() << ": Socket closed by remote host" );
        }
        else
        {
            LOG ( L_DEBUG3, getLogId() << ": Connection refused" );
        }

        // Socket got closed, or connection was refused.

        clearFlags ( SockTcpFlagConnected );

        if ( hasFlag ( SockFlagConnecting ) )
        {
            doSockConnectFailed ( Error::ConnectFailed );
            return;
        }

        doSockClosed ( Error::Closed );
        return;
    }
}
