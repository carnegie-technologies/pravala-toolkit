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
#include <openssl/rand.h>

#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif
}

#include "basic/Math.hpp"
#include "basic/SeqNum.hpp"
#include "socket/PacketDataStore.hpp"
#include "socket/UdpSocket.hpp"

#include "DtlsSocket.hpp"
#include "DtlsProto.hpp"
#include "SslCompat.hpp"

using namespace Pravala;

MemHandle DtlsSocket::_testData;

ConfigLimitedNumber<uint16_t> DtlsSocket::optDtlsPacketLossBufferSize (
        0,
        "dtls.packet_loss.buf_size",
        "The size of buffer used for calculating packet losses",
        4, 0xFFFF, 32
);

void DtlsSocket::Owner::dtlsSocketUnexpectedDataReceived ( DtlsSocket * sock, const MemHandle & data )
{
    ( void ) data;

    if ( sock != 0 )
    {
        LOG ( L_WARN, sock->getLogId ( true )
              << ": Received unexpected data callback that was NOT handled; Dropping; Data (size: "
              << data.size() << "): " << String::hexDump ( data.get(), data.size() ) );
    }
}

void DtlsSocket::Owner::dtlsSocketListenSucceeded ( DtlsSocket * sock )
{
    if ( sock != 0 )
    {
        LOG ( L_FATAL_ERROR, sock->getLogId ( true ) << ": Callback not handled" );
    }
}

// Inheritance order is important!
// DtlsBio should be before SslSocket.
// This is because we want SslSocket's destructor to run before DtlsBio's destructor.
// This way SSL has a chance to release BIO object before we destroy it.

DtlsSocket::DtlsSocket ( Owner * owner, DtlsContext & dtlsContext, UdpSocket * udpSocket, long sslFlags ):
    DtlsBio ( udpSocket ),
    SslSocket ( owner, dtlsContext ),
    _dtlsTimer ( *this ),
    _dtlsOwner ( owner ),
    _dtlsExpectedSeqNum ( 0 )
{
    memset ( _dtlsExpectedEpoch, 0, sizeof ( _dtlsExpectedEpoch ) );
    memset ( &_stats, 0, sizeof ( _stats ) );

    assert ( _udpSocket != 0 );

    if ( !_ssl || !_udpSocket || !_udpSocket->isValid() )
        return;

    _udpSocket->refOwner ( this );

    setupBio ( _ssl );
    setFlags ( SockFlagValid );

    if ( sslFlags != 0 )
    {
        SSL_set_options ( _ssl, sslFlags );
    }

    setFlags ( SockFlagConnecting );

    if ( udpSocket->isConnecting() )
    {
        LOG ( L_DEBUG, getRemoteSockAddr() << ": Underlying UDP socket is still connecting" );
    }
    else if ( udpSocket->isConnected() )
    {
        LOG ( L_DEBUG, getRemoteSockAddr() << ": Underlying UDP socket is already connected" );

        socketConnected ( _udpSocket );
    }
    else if ( dtlsContext.isServer() )
    {
        LOG ( L_DEBUG, getRemoteSockAddr() << ": Underlying UDP socket is not connected or connecting "
              "and we got a DTLS server context; Enabling 'listening' mode" );

        setFlags ( SockDtlsFlagListenNeeded );
    }
}

DtlsSocket::~DtlsSocket()
{
    close();

    if ( _udpSocket != 0 )
    {
        _udpSocket->unrefOwner ( this );
        _udpSocket = 0;
    }
}

String DtlsSocket::getLogId ( bool extended ) const
{
    return String ( "DTLS:" ).append ( SslSocket::getLogId ( extended ) );
}

DtlsSocket * DtlsSocket::getDtlsSocket()
{
    return this;
}

void DtlsSocket::ownerChanged()
{
    _dtlsOwner = 0;
}

void DtlsSocket::setOwner ( DtlsSocket::Owner * owner )
{
    SslSocket::setOwner ( owner );

    _dtlsOwner = owner;
}

void DtlsSocket::refOwner ( DtlsSocket::Owner * owner )
{
    SslSocket::refOwner ( owner );

    if ( owner != 0 )
    {
        _dtlsOwner = owner;
    }
}

size_t DtlsSocket::testEncryptionOverhead ( size_t size )
{
    if ( size < 1 )
    {
        return 0;
    }

    if ( _testData.size() < size )
    {
        _testData = MemHandle ( size );

        // We want to use random data, in case OpenSSL does something smart to compress it.

        if ( !_testData.getWritable()
             || RAND_bytes ( ( unsigned char * ) _testData.getWritable(), _testData.size() ) < 1 )
        {
            _testData.setZero();
        }

        if ( _testData.size() < size )
        {
            return 0;
        }
    }

    startTestWrite();

    assert ( _testData.size() >= size );

    doSendEncrypted ( _testData.get(), size );

    return endTestWrite();
}

void DtlsSocket::socketConnected ( Socket * sock )
{
    assert ( sock != 0 );
    assert ( sock->isConnected() );
    assert ( !sock->isConnecting() );
    assert ( _udpSocket != 0 );
    assert ( sock == _udpSocket );
    assert ( getSslSockFd() < 0 );

    if ( _ssl != 0 && sock != 0 && sock->isConnected() && _udpSocket != 0 )
    {
        LOG ( L_DEBUG2, getLogId() << ": Underlying UDP socket is now connected" );

        // DtlsSocket is still 'connecting'. But we want to perform the initial accept/connect.
        // We can't run this right away, because we may have been called synchronously!
        scheduleEvents ( SockDtlsEventInitialOp );
    }
}

void DtlsSocket::close()
{
    _dtlsTimer.stop();

    if ( _udpSocket != 0 )
    {
        _udpSocket->close();
    }

    SslSocket::close();
}

const SockAddr & DtlsSocket::getLocalSockAddr() const
{
    return ( _udpSocket != 0 ) ? _udpSocket->getLocalSockAddr() : EmptySockAddress;
}

const SockAddr & DtlsSocket::getRemoteSockAddr() const
{
    return ( _udpSocket != 0 ) ? _udpSocket->getRemoteSockAddr() : EmptySockAddress;
}

ERRCODE DtlsSocket::bind ( const SockAddr & )
{
    return Error::Unsupported;
}

ERRCODE DtlsSocket::bindToIface ( const String &, IpAddress::AddressType )
{
    return Error::Unsupported;
}

ERRCODE DtlsSocket::connect ( const SockAddr & )
{
    return Error::Unsupported;
}

bool DtlsSocket::runEvents ( uint16_t events )
{
    if ( SslSocket::runEvents ( events ) )
        return true;

    if ( ( events & SockDtlsEventInitialOp ) == SockDtlsEventInitialOp )
    {
        scheduleEvents ( events & ~SockDtlsEventInitialOp );
        doInitialOp();
        return true;
    }

    return false;
}

void DtlsSocket::checkDtlsTimer ( SslSocket::CallType callType )
{
    ( void ) callType;

    int toutMs = getDtlsTimeoutMs();

    if ( toutMs > 0 )
    {
        LOG ( L_DEBUG2, callTypeName ( callType ) << ": " << getLogId()
              << ": Starting a DTLS timer (" << toutMs << " ms)" );

        // There is a DTLS timer set, let's setup the timeout.
        _dtlsTimer.start ( toutMs );
    }
    else if ( toutMs == 0 )
    {
        LOG ( L_DEBUG2, callTypeName ( callType ) << ": " << getLogId()
              << ": Scheduling a DTLS timeout in the next timer slot" );

        // The DLTS timer was set, and should fire right away.
        // However, we don't want to call that code from this function directly.
        // If we do that and it fails, the owner is notified about the problem.
        // This may cause problems with other callbacks that could be called after the checkDtlsTimeout().

        _dtlsTimer.start ( 1 );
    }
}

void DtlsSocket::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer != 0 );
    assert ( timer == &_dtlsTimer );

    LOG ( L_DEBUG2, getLogId() << ": Running DTLS timer" );

    int ret = DTLSv1_handle_timeout ( _ssl );

    // ret < 0:  there was an error

    if ( ret < 0 )
    {
        // There was an error
        LOG ( L_ERROR, getLogId() << ": DTLSv1_handle_timeout returned: " << ret );

        doSockClosed ( Error::SslUnknown );
        return;
    }

    // ret == 0: timer is not running, or has not expired yet (according to DTLS)
    // ret > 0:  handle_timeout succeeded, but we still may need to setup the timer:

    LOG ( L_DEBUG2, getLogId() << ": DTLSv1_handle_timeout returned: " << ret );

    int toutMs = getDtlsTimeoutMs();

    if ( toutMs < 0 )
    {
        // No timer to set, exit
        return;
    }

    // Otherwise we need to setup a new timer. It is possible that toutMs is 0, which
    // means DtlsBio thinks we should run right away (it could be possible if for some timing reasons
    // DtlsBio thinks we should timeout right away, and DTLS doesn't).
    // Let's setup a very short timer. Even if we use value '1' it will be fired in the next timer slot
    if ( toutMs < 1 )
    {
        toutMs = 1;
    }

    LOG ( L_DEBUG2, getLogId() << ": Restarting the DTLS timer (again): " << toutMs << " ms" );

    _dtlsTimer.start ( toutMs );

    return;
}

void DtlsSocket::doInitialOp()
{
    if ( hasFlag ( SockDtlsFlagListenNeeded ) )
    {
        doDtlsListen();
        return;
    }

    int ret = 0;
    SslSocket::CallType cType = CallUnknown;

    ERR_clear_error();

    if ( hasFlag ( SockSslFlagAcceptNeeded ) )
    {
        cType = CallAccept;
        ret = SSL_accept ( _ssl );
    }
    else if ( hasFlag ( SockSslFlagConnectNeeded ) )
    {
        cType = CallConnect;
        ret = SSL_connect ( _ssl );
    }
    else
    {
        LOG ( L_FATAL_ERROR, getLogId()
              << ": Called without either 'accept needed' or 'connect needed' flag!" );
        return;
    }

    checkDtlsTimer ( cType );

    if ( ret != 1 )
    {
        handleSslError ( cType, ret, -1, false );

        // We HAVE to return immediately after calling handleSslError!
        // It may run a callback in the owner which, in turn, could remove us!
        return;
    }

    // Connected!

    LOG ( L_DEBUG, getLogId() << ": Connected successfully (using DTLS)" );

    clearFlags ( SockSslFlagAcceptNeeded | SockSslFlagConnectNeeded );

    doSockConnected();
    return;
}

ERRCODE DtlsSocket::send ( const char * data, size_t & dataSize )
{
    // It's either an error (including incomplete write which technically is a partial write),
    // or all the data is accepted - no need to modify dataSize.
    return doSendEncrypted ( data, dataSize );
}

ERRCODE DtlsSocket::send ( MemHandle & data )
{
    const ERRCODE eCode = doSendEncrypted ( data.get(), data.size() );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE DtlsSocket::send ( MemVector & data )
{
    const ERRCODE eCode = doSendEncrypted ( data );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE DtlsSocket::doSendEncrypted ( const char * data, size_t dataSize )
{
    if ( !_udpSocket || !isConnected() )
    {
        LOG ( L_ERROR, getLogId() << ": Can't send data; Socket is not connected" );
        return Error::NotConnected;
    }

    if ( dataSize < 1 )
    {
        LOG ( L_DEBUG, getLogId() << ": No data to send" );
        return Error::Success;
    }

    ERR_clear_error();

    const int ret = SSL_write ( _ssl, data, dataSize );

    checkDtlsTimer ( CallWrite );

    if ( ret <= 0 )
    {
        // Here we pass 'true' to delay callbacks - because this function is called synchronously!
        return handleSslError ( CallWrite, ret, -1, true );
    }

    return ( ( size_t ) ret < dataSize ) ? ( Error::IncompleteWrite ) : ( Error::Success );
}

ERRCODE DtlsSocket::doSendEncrypted ( const MemVector & data )
{
    if ( !_udpSocket || !isConnected() )
    {
        LOG ( L_ERROR, getLogId() << ": Can't send data; Socket is not connected" );
        return Error::NotConnected;
    }

    if ( data.isEmpty() )
    {
        LOG ( L_DEBUG, getLogId() << ": No data to send" );
        return Error::Success;
    }

    // First, we need a continuous memory (to be able to pass it to SSL).
    // MemVector provides a method to generate a continuous memory segment.
    // It is expensive (unless there is only one chunk in the vector), but we have no choice...

    MemHandle contMem;

    if ( data.getNumChunks() > 1 )
    {
        // We only need to allocate memory if there are multiple chunks.
        contMem = PacketDataStore::getPacket ( data.getDataSize() );
    }

    if ( !data.storeContinuous ( contMem ) )
    {
        return Error::MemoryError;
    }

    assert ( contMem.size() == data.getDataSize() );

    return doSendEncrypted ( contMem.get(), contMem.size() );
}

void DtlsSocket::socketDataReceived ( Socket * sock, MemHandle & data )
{
    ( void ) sock;

    assert ( sock != 0 );
    assert ( _udpSocket != 0 );
    assert ( sock == _udpSocket );

    doDeliverData ( data );

    // doDeliverData() may partially consume the 'data' object.
    // We MUST clear() it, or it will be delivered again.
    data.clear();
}

void DtlsSocket::doDeliverData ( MemHandle & data )
{
    if ( data.size() < sizeof ( DtlsMiniHeader ) )
    {
        LOG ( L_WARN, getLogId()
              << ": Received a message which is too small (size: " << data.size()
              << "); Data: " << String::hexDump ( data.get(), data.size() ) );

        if ( _dtlsOwner != 0 )
        {
            _dtlsOwner->dtlsSocketUnexpectedDataReceived ( this, data );
        }

        return;
    }

    const DtlsMiniHeader * const miniHdr = reinterpret_cast<const DtlsMiniHeader *> ( data.get() );

    if ( hasFlag ( SockDtlsFlagListenNeeded | SockSslFlagAcceptNeeded | SockSslFlagConnectNeeded ) )
    {
        if ( !miniHdr->isDtls() )
        {
            LOG ( L_DEBUG, getLogId() << ": Received non-DTLS data on a pending DTLS socket; Data (size: "
                  << data.size() << "): " << String::hexDump ( data.get(), data.size() ) );

            if ( _dtlsOwner != 0 )
            {
                _dtlsOwner->dtlsSocketUnexpectedDataReceived ( this, data );
            }

            return;
        }

        // Let's store the data for DTLS to read it. It also clears the original data.
        setBioReadBuffer ( data );

        doInitialOp();
        return;
    }

    if ( !miniHdr->isDtls() )
    {
        LOG ( L_DEBUG, getLogId() << ": Received non-DTLS data on a DTLS socket; Data (size: "
              << data.size() << "): " << String::hexDump ( data.get(), data.size() ) );

        if ( _dtlsOwner != 0 )
        {
            _dtlsOwner->dtlsSocketUnexpectedDataReceived ( this, data );
        }

        return;
    }

    // DTLS data - we should perform regular SSL operations.

    const DtlsHeader * const dtlsHeader = reinterpret_cast<const DtlsHeader *> ( data.get() );
    const uint16_t dtlsSeqNum = dtlsHeader->getPartialSeqNum();

    if ( memcmp ( dtlsHeader->epoch, _dtlsExpectedEpoch, sizeof ( dtlsHeader->epoch ) ) != 0 )
    {
        // The epoch changed, we can't tell anything about packet losses...

        memcpy ( _dtlsExpectedEpoch, dtlsHeader->epoch, sizeof ( dtlsHeader->epoch ) );

        // Overflows are OK. So is 0, since it has no special meaning.
        // It is part of DTLS' sequence number, so we will see 0 values and that's OK.

        _dtlsExpectedSeqNum = dtlsSeqNum + 1;
        ++_stats.packetsReceived;
    }
    else
    {
        const int32_t seqNumDiff = compareSeqNums ( dtlsSeqNum, _dtlsExpectedSeqNum );

        if ( seqNumDiff >= 0 )
        {
            // Received sequence number is in the future.
            // We record a loss (which, if it is the same as expected, is "no loss").
            // If this sequence number is in the past, it means that this is a reordered packet.
            // In that case we ignore the sequence number, but we still want to deliver the packet.
            // This is because IP packets can be reordered, and depending on the higher level protocol
            // they may still be useful.

            ++_stats.packetsReceived;
            _stats.packetsLost += seqNumDiff;

            // Overflows are OK. So is 0, since it has no special meaning.
            // It is part of DTLS' sequence number, so we will see 0 values and that's OK.
            _dtlsExpectedSeqNum = dtlsSeqNum + 1;
        }
    }

    // Let's store the data for DTLS to read it. It also clears the original data.
    setBioReadBuffer ( data );

    // Let's get rid of this reference.
    data.clear();

    if ( hasFlag ( SockSslFlagDoWriteOnRead ) )
    {
        clearFlags ( SockSslFlagDoWriteOnRead );

        ERR_clear_error();

        const int ret = SSL_write ( _ssl, 0, 0 );

        checkDtlsTimer ( CallWrite );

        if ( ret <= 0 )
        {
            handleSslError ( CallWrite, ret, -1, false );
        }

        return;
    }

    MemHandle buffer = PacketDataStore::getPacket();
    char * const mem = buffer.getWritable();

    int ret = -1;

    ERR_clear_error();

    if ( !mem )
    {
        errno = ENOMEM;
    }
    else
    {
        ret = SSL_read ( _ssl, mem, buffer.size() );

#ifdef USE_VALGRIND_MEMCHECK
        VALGRIND_MAKE_MEM_DEFINED ( &ret, sizeof ( ret ) );
#endif

        checkDtlsTimer ( CallRead );
    }

    if ( ret <= 0 )
    {
        handleSslError ( CallRead, ret, -1, false );

        // We HAVE to return immediately after calling handleSslError - it may run a callback in the owner which,
        // in turn, could remove us!
        return;
    }

    // ret is positive - we managed to read something!
    // However, the actual value of 'ret' is not important.
    // If we received a packet, it is stored in the buffer.
    // Otherwise, we don't have enough data yet.

#ifdef USE_VALGRIND_MEMCHECK
    // Because OpenSSL uses uninitialized stack as a source of entropy, all the data generated by it
    // will be "contaminated" - marked by Valgrind as 'undefined'.
    // This is not really an error, but will generate a Valgrind warning.
    // To get rid of that warning, we specifically mark that data as 'defined':
    VALGRIND_MAKE_MEM_DEFINED ( mem, ret );
#endif

    if ( ret > 0 && getOwner() != 0 )
    {
        buffer.truncate ( ret );
        doSockDataReceived ( buffer );
        return;
    }
}

void DtlsSocket::doDtlsListen()
{
#ifdef OPENSSL_IS_BORINGSSL
    LOG ( L_FATAL_ERROR, "BoringSSL does not support DTLS server mode" );
    return;
#else
    SockAddr remoteAddr;

    ERR_clear_error();

    const int ret = Prav_DTLSv1_listen ( _ssl, remoteAddr );

    if ( ret <= 0 )
    {
#ifndef NO_LOGGING
        if ( _log.shouldLog ( L_ERROR ) )
        {
            // If we don't want to log L_ERROR, we certainly don't want to log L_DEBUG

            const int sslErr = SSL_get_error ( _ssl, ret );

            if ( sslErr == SSL_ERROR_WANT_READ )
            {
                // SSL_ERROR_WANT_READ - this is normal (client sends a hello request without a cookie)

                LOG ( L_DEBUG, getLogId() << ": DTLSv1_listen ret: " << ret
                      << "; Error: SSL_ERROR_WANT_READ" );
            }
            else if ( sslErr == SSL_ERROR_SSL )
            {
                // SSL_ERROR_SSL. This is more interesting!
                // (we already know that we want to log L_ERROR)

                unsigned long err;

                while ( ( err = ERR_get_error() ) != 0 )
                {
                    LOG ( L_ERROR, getLogId() << ": DTLSv1_listen ret: " << ret
                          << "; Error: SSL_ERROR_SSL: " << ERR_error_string ( err, 0 ) );
                }
            }
            else if ( sslErr == SSL_ERROR_SYSCALL )
            {
                // SSL_ERROR_SYSCALL. This is more interesting!
                // (we already know that we want to log L_ERROR)

                unsigned long err = ERR_get_error();

                if ( err != 0 )
                {
                    do
                    {
                        LOG ( L_ERROR, getLogId() << ": DTLSv1_listen ret: " << ret
                              << "; Error: SSL_ERROR_SYSCALL: " << ERR_error_string ( err, 0 ) );
                    }
                    while ( ( err = ERR_get_error() ) != 0 );
                }
                else if ( ret == 0 )
                {
                    LOG ( L_DEBUG, getLogId() << ": DTLSv1_listen ret: " << ret
                          << "; Socket closed" );
                }
                else
                {
                    LOG ( L_ERROR, getLogId() << ": DTLSv1_listen ret: " << ret
                          << "; Error: SSL_ERROR_SYSCALL: " << strerror ( errno ) );
                }
            }
            else
            {
                // Some other error. This also could be important

                LOG ( L_ERROR, getLogId() << ": DTLSv1_listen ret: " << ret
                      << "; Error: " << sslErr );
            }
        }
#endif

        ERR_clear_error();
        return;
    }

    // We are now connected!

    LOG ( L_DEBUG, getLogId() << ": DTLSv1_listen succeeded; Remote: " << remoteAddr );

    clearFlags ( SockDtlsFlagListenNeeded );

    assert ( _udpSocket != 0 );

    ERRCODE eCode;
    UdpSocket * prevSocket = _udpSocket;

    _udpSocket = prevSocket->generateConnectedSock ( this, remoteAddr, &eCode );

    prevSocket->unrefOwner ( this );
    prevSocket = 0;

    if ( !_udpSocket )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": DTLSv1_listen succeeded, but generating UDP socket connected to " << remoteAddr << " failed" );

        doSockClosed ( eCode );
        return;
    }

    if ( _udpSocket->isConnecting() )
    {
        LOG ( L_DEBUG, getRemoteSockAddr() << ": New UDP socket is still connecting" );
    }
    else if ( _udpSocket->isConnected() )
    {
        LOG ( L_DEBUG, getRemoteSockAddr() << ": New UDP socket is already connected" );

        socketConnected ( _udpSocket );
    }

    if ( !_dtlsOwner )
    {
        LOG ( L_FATAL_ERROR, getLogId() << ": DTLSv1_listen succeeded, but DTLS owner is NOT set" );

        doSockClosed ( Error::WrongState );
        return;
    }

    _dtlsOwner->dtlsSocketListenSucceeded ( this );

    // Return after the callback!
    return;
#endif
}

SocketApi::SocketType DtlsSocket::ipSockGetType ( const SockAddr & forAddr )
{
    if ( forAddr.isIPv4() )
    {
        return SocketApi::SocketDgram4;
    }
    else if ( forAddr.isIPv6() )
    {
        return SocketApi::SocketDgram6;
    }

    return SocketApi::SocketInvalid;
}
