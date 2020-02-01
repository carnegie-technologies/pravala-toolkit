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

#include "Socket.hpp"

using namespace Pravala;

TextLog Socket::_log ( "socket" );

SocketOwner::~SocketOwner()
{
}

void SocketOwner::socketWriteSizeHintUpdated ( Socket *, size_t )
{
}

Socket::Socket ( SocketOwner * owner ):
    OwnedObject<SocketOwner> ( owner ),
    _sockFlags ( 0 ),
    _sockEvents ( 0 )
{
}

Socket::~Socket()
{
}

String Socket::getLocalDesc() const
{
    return String::EmptyString;
}

String Socket::getRemoteDesc() const
{
    return String::EmptyString;
}

IpSocket * Socket::getIpSocket()
{
    return 0;
}

LocalSocket * Socket::getLocalSocket()
{
    return 0;
}

int Socket::stealSockFd()
{
    return -1;
}

void Socket::close()
{
    clearAllFlags();
    clearAllEvents();
}

bool Socket::getOption ( int, int, MemHandle & ) const
{
    return false;
}

int Socket::increaseRcvBufSize ( int )
{
    return -1;
}

int Socket::increaseSndBufSize ( int )
{
    return -1;
}

const MemHandle & Socket::getReadBuffer() const
{
    return MemHandle::EmptyHandle;
}

void Socket::consumeReadBuffer ( size_t )
{
}

size_t Socket::getWriteSizeHint() const
{
    return 0;
}

void Socket::scheduleEvents ( uint16_t events )
{
    if ( events != 0 )
    {
        _sockEvents |= events;
        EventManager::loopEndSubscribe ( this );
    }
}

void Socket::receiveLoopEndEvent()
{
    const uint16_t events = _sockEvents;

    _sockEvents = 0;

    if ( events != 0 && !runEvents ( events ) )
    {
        LOG ( L_WARN, getLogId() << ": Events not handled: " << events );
    }
}

bool Socket::runEvents ( uint16_t events )
{
    if ( ( events & SockEventClosed ) == SockEventClosed )
    {
        doSockClosed ( Error::Closed );
        return true;
    }

    if ( ( events & SockEventConnectFailed ) == SockEventConnectFailed )
    {
        doSockConnectFailed ( Error::ConnectFailed );
        return true;
    }

    if ( ( events & SockEventConnected ) == SockEventConnected )
    {
        scheduleEvents ( events & ~SockEventConnected );
        doSockConnected();
        return true;
    }

    // This is perfectly fine, maybe there are events scheduled at higher levels.
    // We just want to say we didn't handle it (by returning false).

    return false;
}

bool Socket::sockInitFd ( SocketApi::SocketType sockType, int & sockFd )
{
    if ( sockFd >= 0 )
        return true;

    sockFd = SocketApi::create ( sockType );

    if ( sockFd < 0 )
    {
        LOG ( L_ERROR, getLogId()
              << ": Error initializing new " << SocketApi::getSockTypeName ( sockType )
              << ": " << SocketApi::getLastErrorDesc() );

        return false;
    }

    if ( !SocketApi::setNonBlocking ( sockFd, true ) )
    {
        LOG ( L_ERROR, getLogId() << ": Error enabling non-blocking mode on a new "
              << SocketApi::getSockTypeName ( sockType ) << " socket: " << SocketApi::getLastErrorDesc() );

        SocketApi::close ( sockFd );
        sockFd = -1;

        return false;
    }

    setFlags ( SockFlagValid );

    LOG ( L_DEBUG2, getLogId() << ": Successfully initialized new " << SocketApi::getSockTypeName ( sockType )
          << "; Socket's FD:" << sockFd );

    return true;
}

void Socket::doSockClosed ( ERRCODE reason )
{
    close();

    if ( getOwner() != 0 )
    {
        getOwner()->socketClosed ( this, reason );
    }
}

void Socket::doSockConnectFailed ( ERRCODE reason )
{
    clearFlags ( SockFlagConnecting | SockFlagConnected );

    if ( getOwner() != 0 )
    {
        getOwner()->socketConnectFailed ( this, reason );
    }
}

void Socket::doSockConnected()
{
    clearFlags ( SockFlagConnecting );
    setFlags ( SockFlagConnected );

    if ( getOwner() != 0 )
    {
        getOwner()->socketConnected ( this );
    }
}

void Socket::doSockReadyToSend()
{
    clearFlags ( SockFlagSendBlocked );

    if ( getOwner() != 0 )
    {
        getOwner()->socketReadyToSend ( this );
    }
}

void Socket::doSockDataReceived ( MemHandle & data )
{
    if ( !isValid() )
        return;

    size_t prevSize = 0;

    // We will be (potentially) calling the callback multiple times.
    // Let's create a self-reference to make sure we don't get removed.

    simpleRef();

    while ( data.size() > 0 && prevSize != data.size() && isValid() )
    {
        prevSize = data.size();

        if ( getOwner() != 0 )
        {
            getOwner()->socketDataReceived ( this, data );
        }
    }

    simpleUnref();
}

void Socket::doSockWriteSizeHintUpdated ( size_t sizeHint )
{
    if ( getOwner() != 0 )
    {
        getOwner()->socketWriteSizeHintUpdated ( this, sizeHint );
    }
}

ERRCODE Socket::streamSend ( MemVector & data )
{
    if ( data.isEmpty() )
    {
        // Nothing to send.
        return Error::Success;
    }

    size_t totalWritten = 0;
    const struct iovec * chunks = data.getChunks();

    assert ( chunks != 0 );
    assert ( data.getNumChunks() > 0 );

    for ( size_t i = 0; i < data.getNumChunks(); ++i )
    {
        size_t size = chunks[ i ].iov_len;

        assert ( size > 0 );

        const ERRCODE eCode = send ( reinterpret_cast<const char *> ( chunks[ i ].iov_base ), size );

        if ( NOT_OK ( eCode ) )
        {
            if ( totalWritten > 0 )
            {
                // We still managed to write something, let's say it went fine.
                // If this is a serious error, it will be reported next time someone tries to write,
                // or a callbacks (socket closed) will be generated anyway.
                break;
            }

            return eCode;
        }

        totalWritten += size;

        if ( size < chunks[ i ].iov_len )
        {
            // Partial write.
            break;
        }
    }

    data.consume ( totalWritten );
    return Error::Success;
}
