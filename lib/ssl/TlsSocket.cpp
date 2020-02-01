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
#include <unistd.h>
#include <fcntl.h>
#include <openssl/err.h>

#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif
}

#include "socket/PacketDataStore.hpp"
#include "socket/TcpFdSocket.hpp"

#include "TlsSocket.hpp"
#include "SslContext.hpp"

using namespace Pravala;

TlsSocket::TlsSocket ( SocketOwner * owner, TlsContext & tlsContext ):
    SslSocket ( owner, tlsContext ),
    _tcpSocket ( new TcpFdSocket ( this ) )
{
    if ( _ssl != 0 )
    {
        setFlags ( SockFlagValid );
    }
}

TlsSocket::TlsSocket (
        SocketOwner * owner, TlsContext & tlsContext, int sockFd,
        const SockAddr & localAddr, const SockAddr & remoteAddr ):
    SslSocket ( owner, tlsContext ),
    _localAddr ( localAddr ),
    _remoteAddr ( remoteAddr ),
    _tcpSocket ( 0 )
{
    if ( sockFd >= 0 && _ssl != 0 )
    {
        setSslSockFd ( sockFd );
        setFlags ( SockFlagValid | SockFlagConnecting );
    }
}

TlsSocket::TlsSocket ( SocketOwner * owner, TlsContext & tlsContext, int sockFd ):
    SslSocket ( owner, tlsContext ),
    _tcpSocket ( 0 )
{
    if ( !SocketApi::getName ( sockFd, _localAddr ) )
    {
        LOG ( L_ERROR, "Could not read the local name of the socket: " << SocketApi::getLastErrorDesc() );
    }

    if ( !SocketApi::getPeerName ( sockFd, _remoteAddr ) )
    {
        LOG ( L_ERROR, "Could not read the peer name of the socket: " << SocketApi::getLastErrorDesc() );
    }

    if ( sockFd >= 0 && _ssl != 0 )
    {
        setSslSockFd ( sockFd );
        setFlags ( SockFlagValid | SockFlagConnecting );
    }
}

TlsSocket::TlsSocket ( SocketOwner * owner, TlsContext & tlsContext, TcpSocket * tcpSocket ):
    SslSocket ( owner, tlsContext ),
    _tcpSocket ( 0 )
{
    assert ( tcpSocket != 0 );

    if ( !_ssl || !tcpSocket )
        return;

    if ( tcpSocket->isValid() && ( tcpSocket->isConnecting() || tcpSocket->isConnected() ) )
    {
        setFlags ( SockFlagValid | SockFlagConnecting );

        // We need to set it regardless, since socketConnected() expects it to be set.

        _tcpSocket = tcpSocket;
        _tcpSocket->refOwner ( this );

        if ( tcpSocket->isConnected() )
        {
            socketConnected ( tcpSocket );
        }

        return;
    }

    LOG ( L_WARN, getLogId() << ": TcpSocket passed (" << tcpSocket->getLogId ( true )
          << ") is invalid, or neither connected nor connecting" );
}

TlsSocket::~TlsSocket()
{
    close();
}

String TlsSocket::getLogId ( bool extended ) const
{
    return String ( "TLS:" ).append ( SslSocket::getLogId ( extended ) );
}

TlsSocket * TlsSocket::getTlsSocket()
{
    return this;
}

void TlsSocket::socketConnected ( Socket * sock )
{
    assert ( sock != 0 );
    assert ( sock->isConnected() );
    assert ( !sock->isConnecting() );
    assert ( _tcpSocket != 0 );
    assert ( sock == _tcpSocket );
    assert ( getSslSockFd() < 0 );

    if ( _ssl != 0 && sock != 0 && sock->isConnected() && _tcpSocket != 0 )
    {
        const int fd = sock->stealSockFd();

        if ( fd >= 0 )
        {
            setSslSockFd ( fd );

            _localAddr = _tcpSocket->getLocalSockAddr();
            _remoteAddr = _tcpSocket->getRemoteSockAddr();

            LOG ( L_DEBUG2, getLogId() << ": Underlying TCP socket ["
                  << _localAddr << "-" << _remoteAddr << "] is now connected" );

            // Nothing else to do. TlsSocket is still 'connecting'.
            // It should already be set this way, but in case it isn't:
            setFlags ( SockFlagConnecting );
        }
    }

    if ( _tcpSocket != 0 )
    {
        _tcpSocket->unrefOwner ( this );
        _tcpSocket = 0;
    }
}

void TlsSocket::setSslSockFd ( int sockFd )
{
    if ( !_ssl || sockFd < 0 || getSslSockFd() >= 0 )
    {
        assert ( false );
        return;
    }

    SSL_set_fd ( _ssl, sockFd );

    EventManager::setFdHandler ( sockFd, this, EventManager::EventRead | EventManager::EventWrite );
}

void TlsSocket::close()
{
    const int sockFd = SSL_get_fd ( _ssl );

    SSL_set_fd ( _ssl, -1 );

    SslSocket::close();

    if ( sockFd >= 0 )
    {
        LOG ( L_DEBUG3, getLogId() << ": Closing socket; Size of data in read buffer: " << _readBuf.size() );

        EventManager::closeFd ( sockFd );
    }

    if ( _tcpSocket != 0 )
    {
        _tcpSocket->unrefOwner ( this );
        _tcpSocket = 0;
    }

    _readBuf.clear();
}

uint16_t TlsSocket::getDetectedMtu() const
{
    return TcpFdSocket::detectMtu ( getSslSockFd() );
}

const SockAddr & TlsSocket::getLocalSockAddr() const
{
    return _localAddr;
}

const SockAddr & TlsSocket::getRemoteSockAddr() const
{
    return _remoteAddr;
}

ERRCODE TlsSocket::bind ( const SockAddr & addr )
{
    if ( !_tcpSocket || !_ssl )
        return Error::WrongState;

    return _tcpSocket->bind ( addr );
}

ERRCODE TlsSocket::bindToIface ( const String & ifaceName, IpAddress::AddressType addrType )
{
    if ( !_tcpSocket || !_ssl )
        return Error::WrongState;

    return _tcpSocket->bindToIface ( ifaceName, addrType );
}

ERRCODE TlsSocket::connect ( const SockAddr & addr )
{
    if ( !_tcpSocket || !_ssl )
        return Error::WrongState;

    setFlags ( SockFlagConnecting );

    return _tcpSocket->connect ( addr );
}

const MemHandle & TlsSocket::getReadBuffer() const
{
    return _readBuf;
}

void TlsSocket::consumeReadBuffer ( size_t size )
{
    _readBuf.consume ( size );

    const int sockFd = getSslSockFd();

    if ( _readBuf.isEmpty() && sockFd >= 0 && isConnected() )
    {
        LOG ( L_DEBUG3, getLogId() << ": Read buffer is now empty; Scheduling delayed read" );

        // Instead of re-enabling read events, we want to force the read.
        // This is because there may not be anything new to be read from the socket,
        // and some data already pending in SSL's internal buffers.
        // If that's the case, we would try reading it until the next packet arrives.
        // So we just want to try reading regardless.
        scheduleEvents ( SockTlsEventDelayedRead );
    }
}

ERRCODE TlsSocket::send ( MemHandle & data )
{
    const char * mem = data.get();
    size_t size = data.size();

    ERRCODE eCode = send ( mem, size );

    if ( NOT_OK ( eCode ) )
        return eCode;

    data.consume ( size );
    return Error::Success;
}

ERRCODE TlsSocket::send ( MemVector & data )
{
    return streamSend ( data );
}

ERRCODE TlsSocket::send ( const char * data, size_t & dataSize )
{
    const int sockFd = getSslSockFd();

    if ( sockFd < 0 || !isConnected() )
    {
        LOG ( L_ERROR, getLogId() << ": Can't send data; Socket is not connected" );
        return Error::NotConnected;
    }

    // We pass 'true' to delay callbacks:
    return tlsWrite ( sockFd, true, data, dataSize );
}

void TlsSocket::receiveFdEvent ( int fd, short int events )
{
    assert ( fd >= 0 );
    assert ( getSslSockFd() == fd );

    if ( fd < 0 || getSslSockFd() != fd )
    {
        LOG ( L_ERROR, getLogId() << ": Invalid FD in SSL state; Received FD: "
              << fd << "; Our own FD: " << getSslSockFd() );

        return;
    }

    const bool acceptNeeded = hasFlag ( SockSslFlagAcceptNeeded );

    if ( acceptNeeded || hasFlag ( SockSslFlagConnectNeeded ) )
    {
        const int ret = acceptNeeded ? SSL_accept ( _ssl ) : SSL_connect ( _ssl );

#ifdef USE_VALGRIND_MEMCHECK
        VALGRIND_MAKE_MEM_DEFINED ( &ret, sizeof ( ret ) );
#endif

        if ( ret != 1 )
        {
            handleSslError ( acceptNeeded ? CallAccept : CallConnect, ret, fd, false );

            // We HAVE to return immediately after calling handleSslError!
            // It may run a callback in the owner which, in turn, could remove us!
            return;
        }

        // Connected!

        LOG ( L_DEBUG2, getLogId() << ": Connected successfully (using TLS)" );

        clearFlags ( SockSslFlagAcceptNeeded | SockSslFlagConnectNeeded );
        EventManager::setFdEvents ( fd, EventManager::EventRead | EventManager::EventWrite );

        doSockConnected();
        return;
    }

    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        if ( !hasFlag ( SockSslFlagDoWriteOnRead ) )
        {
            // Regular 'read on read'.
            tlsRead ( fd );
            return;
        }

        clearFlags ( SockSslFlagDoWriteOnRead );

        // In case previous 'write' was blocked by read request
        EventManager::enableWriteEvents ( fd );

        size_t tmp = 0;

        tlsWrite ( fd, false, 0, tmp );

        // We HAVE to return after calling that.
        return;
    }

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        if ( !hasFlag ( SockSslFlagDoReadOnWrite ) )
        {
            // This is a regular 'write on write'
            tlsHandleRegularWriteEvent ( fd );
            return;
        }

        // We want to call 'SSL_read' instead of 'SSL_write'. SSL is doing something behind the scenes!

        assert ( hasFlag ( SockSslFlagDoReadOnWrite ) );

        clearFlags ( SockSslFlagDoReadOnWrite );

        tlsRead ( fd );
        return;
    }

    LOG ( L_ERROR, getLogId() << ": Ignoring unknown FD event (" << events << ") on socket: " << fd );
    return;
}

void TlsSocket::tlsHandleRegularWriteEvent ( int sockFd )
{
    assert ( sockFd >= 0 );

    if ( !hasFlag ( SockFlagSendBlocked ) )
    {
        // There were no failed or incomplete writes, so nobody is expecting a callback.
        // We don't need more write events!
        EventManager::disableWriteEvents ( sockFd );
        return;
    }

    // Previous send() got blocked, let's notify the owner that it can write again!

    clearFlags ( SockFlagSendBlocked );
    doSockReadyToSend();
}

ERRCODE TlsSocket::tlsWrite ( int sockFd, bool delayCallbacks, const char * data, size_t & dataSize )
{
    assert ( sockFd >= 0 );

    if ( !data && dataSize > 0 )
    {
        return Error::InvalidParameter;
    }

    ERR_clear_error();

    const int ret = SSL_write ( _ssl, data, dataSize );

    if ( ret == 0 && dataSize == 0 )
    {
        // We use empty writes to call SSL_write when it's requested by OpenSSL.
        return Error::Success;
    }

    if ( ret > 0 )
    {
        LOG ( L_DEBUG4, getLogId() << ": Successfully sent " << ret << " out of " << dataSize << " bytes" );

        if ( ( size_t ) ret < dataSize )
        {
            dataSize = ( size_t ) ret;

            setFlags ( SockFlagSendBlocked );
            EventManager::enableWriteEvents ( sockFd );
        }

        return Error::Success;
    }

    return handleSslError ( CallWrite, ret, sockFd, delayCallbacks );
}

bool TlsSocket::runEvents ( uint16_t events )
{
    if ( SslSocket::runEvents ( events ) )
        return true;

    if ( ( events & SockTlsEventDelayedRead ) == SockTlsEventDelayedRead )
    {
        scheduleEvents ( events & ~SockTlsEventDelayedRead );

        LOG ( L_DEBUG3, getLogId() << ": Running delayed read" );

        tlsRead ( getSslSockFd() );
        return true;
    }

    return false;
}

void TlsSocket::tlsRead ( int sockFd )
{
    assert ( sockFd >= 0 );

    ERR_clear_error();

    if ( !_readBuf.isEmpty() )
    {
        // If _readBuf is not empty, we don't want to read any payload data - but still call SSL_read in case
        // SSL wants to do something on its own. But we want to disable read events before.
        // They will be re-enabled in handleSslError if there is 'want read' error.

        EventManager::disableReadEvents ( sockFd );

        const int ret = SSL_read ( _ssl, 0, 0 );

#ifdef USE_VALGRIND_MEMCHECK
        VALGRIND_MAKE_MEM_DEFINED ( &ret, sizeof ( ret ) );
#endif

        if ( ret < 0 )
        {
            handleSslError ( CallRead, ret, sockFd, false );

            // We HAVE to return immediately after calling handleSslError - it may run a callback
            // in the owner which, in turn, could remove us!
            return;
        }

        return;
    }

    assert ( _readBuf.isEmpty() );

    _readBuf = PacketDataStore::getPacket();

    if ( _readBuf.isEmpty() )
    {
        LOG ( L_FATAL_ERROR, getLogId() << ": Could not allocate memory for TLS read buffer" );
        return;
    }

    size_t bufOffset = 0;
    int readRet = 0;

    do
    {
        readRet = SSL_read ( _ssl, _readBuf.getWritable() + bufOffset, _readBuf.size() - bufOffset );

#ifdef USE_VALGRIND_MEMCHECK
        VALGRIND_MAKE_MEM_DEFINED ( &readRet, sizeof ( readRet ) );
#endif

        if ( readRet > 0 )
        {
            bufOffset += readRet;
        }
    }
    while ( readRet > 0 && bufOffset < _readBuf.size() );

    _readBuf.truncate ( bufOffset );

    if ( readRet <= 0 )
    {
        // There was an error. That could simply be a 'want read' since we've read all there was to be read.

        if ( _readBuf.size() < 1 )
        {
            // No need to delay callbacks, no data has been read...
            handleSslError ( CallRead, readRet, sockFd, false );
            return;
        }

        // We need to deliver some data, so let's delay callbacks, and deliver the data we have!
        handleSslError ( CallRead, readRet, sockFd, true );
    }
    else
    {
        // The last read actually returned some data.
        // Which means we stopped reading because we ran out of space in _readBuf.
        // The data is most likely already in SSL's internal buffers,
        // which means that we won't get a read event for it.
        // So let's force a delayed read operation:

        LOG ( L_DEBUG3, getLogId() << ": SSL might have more data for us; Scheduling delayed read" );

        scheduleEvents ( SockTlsEventDelayedRead );
    }

#ifdef USE_VALGRIND_MEMCHECK
    // Because OpenSSL uses uninitialized stack as a source of entropy, all the data generated by it
    // will be "contaminated" - marked by Valgrind as 'undefined'.
    // This is not really an error, but will generate a Valgrind warning.
    // To get rid of that warning, we specifically mark that data as 'defined':
    VALGRIND_MAKE_MEM_DEFINED ( _readBuf.get(), _readBuf.size() );
#endif

    doSockDataReceived ( _readBuf );
}

SocketApi::SocketType TlsSocket::ipSockGetType ( const SockAddr & forAddr )
{
    if ( forAddr.isIPv4() )
    {
        return SocketApi::SocketStream4;
    }
    else if ( forAddr.isIPv6() )
    {
        return SocketApi::SocketStream6;
    }

    return SocketApi::SocketInvalid;
}
