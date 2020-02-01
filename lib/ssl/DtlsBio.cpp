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
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <sys/time.h>
}

#include <cerrno>

#include "basic/Math.hpp"
#include "socket/UdpSocket.hpp"
#include "DtlsBio.hpp"
#include "SslCompat.hpp"

#define COOKIE_SECRET_LENGTH    16

#ifdef OPENSSL_SYS_LINUX

// Original OpenSSL does that, it may be needed (for instance OpenWRT needs that)
#ifndef IP_MTU
#define IP_MTU    14
#endif
#endif

// This is the smallest possible MTU value returned to OpenSSL, that still results in the same number
// of handshake messages. 1365 results in one additional message.
#define SAFE_MTU             1366

// 20 bytes for IPv4 header plus 8 bytes for UDP header
#define IPV4_MTU_OVERHEAD    28

// 40 bytes for IPv6 header plus 8 bytes for UDP header
#define IPV6_MTU_OVERHEAD    48

using namespace Pravala;

TextLog DtlsBio::_bLog ( "dtls_bio" );

BIO_METHOD * DtlsBio::_bioMethod = 0;

bool DtlsBio::initBio()
{
    if ( _bioMethod != 0 )
        return true;

    if ( !( _bioMethod = BIO_meth_new ( BIO_TYPE_DGRAM, "pravala datagram socket" ) ) )
    {
        SLOG ( _bLog, L_ERROR, "Could not create a new BIO_METHOD object" );
        return false;
    }

    BIO_meth_set_write ( _bioMethod, bioWrite );
    BIO_meth_set_read ( _bioMethod, bioRead );
    BIO_meth_set_puts ( _bioMethod, bioPuts );
    // No 'gets'
    BIO_meth_set_ctrl ( _bioMethod, bioCtrl );
    // No 'create'
    BIO_meth_set_destroy ( _bioMethod, bioDestroy );
    // No 'callback ctrl'

    return true;
}

DtlsBio::DtlsBio ( UdpSocket * udpSocket ):
    _udpSocket ( udpSocket ),
    _bio ( 0 ),
    _bioFlags ( 0 )
{
    if ( !_bioMethod && !initBio() )
    {
        return;
    }

    if ( !( _bio = BIO_new ( _bioMethod ) ) )
    {
        SLOG ( _bLog, L_ERROR, "Could not create a new BIO object" );
        return;
    }

    BIO_set_init ( _bio, 1 );
    BIO_set_data ( _bio, this );
}

DtlsBio::~DtlsBio()
{
    // _bio should have already been set to 0 by bioDestroy() function (called when SslSocket::_ssl is removed).
    // If it is still valid, it could mean that _ssl was not configured to use that _bio object.

    if ( _bio != 0 )
    {
        BIO_free ( _bio );
    }

    assert ( !_bio );
}

void DtlsBio::setupBio ( SSL * ssl )
{
    if ( ssl != 0 && _bio != 0 )
    {
        SSL_set_bio ( ssl, _bio, _bio );
    }
}

void DtlsBio::startTestWrite()
{
    _bioReadBuffer.clear();
    _bioFlags |= BioFlagTestWrite;
}

size_t DtlsBio::endTestWrite()
{
    if ( !( _bioFlags & BioFlagTestWrite ) )
    {
        return 0;
    }

    const size_t ret = _bioReadBuffer.size();

    _bioReadBuffer.clear();
    _bioFlags &= ~BioFlagTestWrite;

    return ret;
}

int DtlsBio::getDtlsTimeoutMs() const
{
    if ( _dtlsNextTimeout.isZero() )
    {
        return -1;
    }

    // We add +1 to increase the chance that when we think that the timer expired
    // the DTLS code actually thinks that too! Otherwise, due to rounding,
    // it could happen that we expire before DTLS does, in which case we would have to do this again.
    int ret = _dtlsNextTimeout.getDiffInMilliSeconds ( EventManager::getCurrentTime() ) + 1;

    if ( ret <= 0 )
    {
        return 0;
    }

    return ret;
}

long int DtlsBio::bioCtrl ( int cmd, long int num, void * ptr )
{
    assert ( _bio != 0 );

    // This switch only contains those control commands from the original datagram BIO
    // which were not doing anything, or were used from by other parts of OpenSSL.
    // It is possible that we need more than that, in which case they will be added.
    //
    // If we need something more just for our code, then it should be done directly through the object,
    // not through the SSL code.

    switch ( cmd )
    {
        case BIO_CTRL_POP:
        case BIO_CTRL_PUSH:
        case BIO_CTRL_RESET:
        case BIO_C_FILE_SEEK:
        case BIO_C_FILE_TELL:
        case BIO_CTRL_INFO:
        case BIO_CTRL_PENDING:
        case BIO_CTRL_WPENDING:
            // case BIO_CTRL_GET_CLOSE:
            // case BIO_CTRL_SET_CLOSE:
            return 0;
            break;

        case BIO_CTRL_DUP:
        case BIO_CTRL_FLUSH:
            return 1;
            break;

        case BIO_CTRL_DGRAM_SET_MTU:
            // This is used by SSL. The original BIO sets the internal value and then returns
            // it when BIO_CTRL_DGRAM_GET_MTU is used. However, nothing uses it!
            // It also sets it in BIO_CTRL_DGRAM_QUERY_MTU.
            // Here we don't do anything, but return the value passed (the same as the original code does)
            return num;
            break;

        case BIO_C_GET_FD:
            return -1;
            break;

        case BIO_C_SET_FD:
            // We ignore the FD (we don't need it), but we should return '1' anyways.
            return 1;
            break;

#ifdef BIO_CTRL_DGRAM_MTU_DISCOVER
        case BIO_CTRL_DGRAM_MTU_DISCOVER:
            // The original datagram BIO sets the socket option to use the "don't fragment" bit.
            // This does not work very well anyway and we don't want it to be used.
            // Also, read the comment of the BIO_CTRL_DGRAM_QUERY_MTU case.
            return 0;
            break;
#endif

        case BIO_CTRL_DGRAM_QUERY_MTU:
            // When this was using a socket FD directly, we were using getsockopt() to figure it out.
            // However it wasn't always working (on the server, for example), and SAFE_MTU was our fallback.
            // If we want to do this, we may want to implement a proper MTU discovery.
            // For now let's just always use SAFE_MTU.
            return SAFE_MTU;
            break;

#ifdef BIO_CTRL_DGRAM_GET_MTU_OVERHEAD
        case BIO_CTRL_DGRAM_GET_MTU_OVERHEAD:
            if ( _udpSocket != 0
                 && _udpSocket->getLocalSockAddr().isIPv6()
                 && !_udpSocket->getLocalSockAddr().isIPv6MappedIPv4() )
            {
                return IPV6_MTU_OVERHEAD;
            }

            return IPV4_MTU_OVERHEAD;
            break;
#endif

#ifdef BIO_CTRL_DGRAM_GET_FALLBACK_MTU
        case BIO_CTRL_DGRAM_GET_FALLBACK_MTU:
            if ( _udpSocket != 0
                 && _udpSocket->getLocalSockAddr().isIPv6()
                 && !_udpSocket->getLocalSockAddr().isIPv6MappedIPv4() )
            {
                // Minimum allowed IPv6 packet size (1280) minus the overhead.
                return 1280 - IPV6_MTU_OVERHEAD;
            }

            // For IPv4 we use 576 minus the overhead.
            return 576 - IPV4_MTU_OVERHEAD;
            break;
#endif

#ifdef BIO_CTRL_DGRAM_SET_PEER
        case BIO_CTRL_DGRAM_SET_PEER:
            // We don't need to set it, since we use the one in UDP socket.
            return 1;
            break;
#endif

        case BIO_CTRL_DGRAM_GET_PEER:

            if ( !ptr || !_udpSocket )
            {
                return 0;
            }

            {
                const SockAddr & sAddr = _udpSocket->getRemoteSockAddr();
                const int ret = sAddr.getSocklen();

                if ( num == 0 || num > ret )
                {
                    num = ret;
                }

                memcpy ( ptr, &sAddr, num );
            }

            return num;
            break;

        case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:
            {
                const struct timeval & tval = *( ( const struct timeval * ) ptr );

                if ( tval.tv_sec == 0 && tval.tv_usec == 0 )
                {
                    _dtlsNextTimeout.clear();
                }
                else
                {
                    // OpenSSL uses gettimeofday for timeouts.
                    // We need to get the length of the 'timeout' to convert it to our time.
                    // The timeout length is 'now' obtained using gettimeofday minus 'tval' received from OpenSSL
                    struct timeval now;

                    gettimeofday ( &now, 0 );

                    _dtlsNextTimeout = EventManager::getCurrentTime ( true );

                    if ( tval.tv_sec > now.tv_sec )
                    {
                        _dtlsNextTimeout.increaseMilliseconds ( ( tval.tv_sec - now.tv_sec ) * 1000 );
                    }

                    if ( tval.tv_usec > now.tv_usec )
                    {
                        _dtlsNextTimeout.increaseMilliseconds ( ( tval.tv_usec - now.tv_usec ) / 1000 );
                    }

                    // It is possible that we make it too long - if 'now.usec' > 'tval.usec'
                    // it means that one of the seconds that we added above is not 'full'.
                    // But those are sub second differences and we don't care that much...
                }
            }

            return 0;
            break;

        case BIO_CTRL_DGRAM_MTU_EXCEEDED:

            if ( ( _bioFlags & BioFlagMtuExceeded ) != 0 )
            {
                _bioFlags &= ~BioFlagMtuExceeded;
                return 1;
            }

            return 0;
            break;

#if defined( BIO_CTRL_DGRAM_SET_PEEK_MODE ) || defined( BIO_CTRL_DGRAM_SCTP_SET_IN_HANDSHAKE )
            // We need both, because when BIO_CTRL_DGRAM_SET_PEEK_MODE was defined
            // it was incorrectly using the same value as BIO_CTRL_DGRAM_SCTP_SET_IN_HANDSHAKE.
            // That has now been fixed, but for binary compatibility we need to support both...
            // See: https://github.com/openssl/openssl/commit/9924087573cfbc8d2bc97088f36d1a81ca00cda3
#ifdef BIO_CTRL_DGRAM_SCTP_SET_IN_HANDSHAKE
        case BIO_CTRL_DGRAM_SCTP_SET_IN_HANDSHAKE:
#endif
#ifdef BIO_CTRL_DGRAM_SET_PEEK_MODE
        case BIO_CTRL_DGRAM_SET_PEEK_MODE:
#endif

            if ( num != 0 )
            {
                _bioFlags |= BioFlagPeekMode;
            }
            else
            {
                _bioFlags &= ~BioFlagPeekMode;
            }
            return 1;
            break;
#endif
    }

    SLOG ( _bLog, L_WARN, "Unsupported BIO-Control command: " << cmd << "; Ignoring" );

    return 0;
}

int DtlsBio::bioDestroy()
{
    if ( !_bio )
        return 0;

    _bio = 0;
    return 1;
}

int DtlsBio::bioRead ( char * const buf, const int size )
{
    assert ( _bio != 0 );
    assert ( buf != 0 );
    assert ( size > 0 );

    BIO_clear_retry_flags ( _bio );

    if ( _bioReadBuffer.isEmpty() )
    {
        errno = EAGAIN;

        BIO_set_retry_read ( _bio );

        return -1;
    }

    errno = 0;

    const size_t dataSize = min ( _bioReadBuffer.size(), ( size_t ) ( ( size > 0 ) ? size : 0 ) );

    memcpy ( buf, _bioReadBuffer.get(), dataSize );

    if ( ( _bioFlags & BioFlagPeekMode ) == 0 )
    {
        _bioReadBuffer.clear();
    }

    return dataSize;
}

int DtlsBio::bioWrite ( const char * buf, int size )
{
    assert ( _bio != 0 );
    assert ( buf != 0 );
    assert ( size > 0 );

    errno = 0;

    if ( _bioFlags & BioFlagTestWrite )
    {
        _bioReadBuffer = MemHandle ( size );

        if ( ( int ) _bioReadBuffer.size() != size )
        {
            _bioReadBuffer.clear();
        }

        // We actually don't need to store the data. All we use is the buffer size.

        return size;
    }

    if ( !buf || size < 1 )
    {
        return 0;
    }

    if ( !_udpSocket )
    {
        errno = EBADF;
        return -1;
    }

    size_t sSize = size;

    const ERRCODE eCode = _udpSocket->send ( buf, sSize );

    BIO_clear_retry_flags ( _bio );

    if ( IS_OK ( eCode ) )
    {
        return ( int ) sSize;
    }

    if ( eCode == Error::TooMuchData )
    {
        _bioFlags |= BioFlagMtuExceeded;

        BIO_set_retry_write ( _bio );
    }
    else if ( eCode == Error::SoftFail )
    {
        BIO_set_retry_write ( _bio );
    }

    return -1;
}

DtlsBio * DtlsBio::getDtlsBio ( BIO * bio )
{
    assert ( bio != 0 );

    DtlsBio * const dtlsBio = reinterpret_cast<DtlsBio *> ( BIO_get_data ( bio ) );

    assert ( dtlsBio != 0 );
    assert ( dtlsBio->_bio != 0 );
    assert ( dtlsBio->_bio == bio );

    return dtlsBio;
}

long int DtlsBio::bioCtrl ( BIO * bio, int cmd, long int arg1, void * arg2 )
{
    return getDtlsBio ( bio )->bioCtrl ( cmd, arg1, arg2 );
}

int DtlsBio::bioRead ( BIO * bio, char * buf, int size )
{
    return getDtlsBio ( bio )->bioRead ( buf, size );
}

int DtlsBio::bioPuts ( BIO * bio, const char * str )
{
    return getDtlsBio ( bio )->bioWrite ( str, strlen ( str ) );
}

int DtlsBio::bioWrite ( BIO * bio, const char * buf, int size )
{
    return getDtlsBio ( bio )->bioWrite ( buf, size );
}

int DtlsBio::bioDestroy ( BIO * bio )
{
    return ( bio != 0 ) ? ( getDtlsBio ( bio )->bioDestroy() ) : 0;
}

int DtlsBio::generateCookie ( unsigned char * cookie, unsigned int * cookieLen ) const
{
    static bool secretInitialized = false;
    static unsigned char mySecret[ COOKIE_SECRET_LENGTH ];
    static unsigned char result[ sizeof ( SockAddr ) ];

    if ( !secretInitialized )
    {
        /* Initialize a random secret */
        if ( RAND_bytes ( mySecret, COOKIE_SECRET_LENGTH ) < 1 )
        {
            SLOG ( _bLog, L_ERROR, "Error setting a random cookie secret" );

            return 0;
        }

        secretInitialized = true;
    }

    // Create buffer with peer's address and port
    // We use _addr - it should be the last peer we talked to!
    size_t resLength = 0;

    if ( !_udpSocket )
        return 0;

    const SockAddr & sAddr = _udpSocket->getRemoteSockAddr();

    if ( sAddr.isIPv4() )
    {
        resLength = sizeof ( sAddr.sa_in.sin_addr ) + sizeof ( sAddr.sa_in.sin_port );

        assert ( resLength <= sizeof ( result ) );

        memcpy ( result,
                 &sAddr.sa_in.sin_port,
                 sizeof ( sAddr.sa_in.sin_port ) );

        memcpy ( result + sizeof ( sAddr.sa_in.sin_port ),
                 &sAddr.sa_in.sin_addr,
                 sizeof ( sAddr.sa_in.sin_addr ) );
    }
    else if ( sAddr.isIPv6() )
    {
        resLength = sizeof ( sAddr.sa_in6.sin6_addr ) + sizeof ( sAddr.sa_in6.sin6_port );

        assert ( resLength <= sizeof ( result ) );

        memcpy ( result,
                 &sAddr.sa_in6.sin6_port,
                 sizeof ( sAddr.sa_in6.sin6_port ) );

        memcpy ( result + sizeof ( sAddr.sa_in6.sin6_port ),
                 &sAddr.sa_in6.sin6_addr,
                 sizeof ( sAddr.sa_in6.sin6_addr ) );
    }
    else
    {
        return 0;
    }

    /* Calculate HMAC of buffer using the secret */
    HMAC ( EVP_sha1(), mySecret, COOKIE_SECRET_LENGTH, result, resLength, cookie, cookieLen );

    return 1;
}

int DtlsBio::generateCookie ( SSL * ssl, unsigned char * cookie, unsigned int * cookieLen )
{
    BIO * const bio = ( ssl != 0 ) ? ( SSL_get_rbio ( ssl ) ) : 0;
    DtlsBio * const dtlsBio = ( bio != 0 ) ? ( reinterpret_cast<DtlsBio *> ( BIO_get_data ( bio ) ) ) : 0;

    return ( dtlsBio != 0
             && dtlsBio->_bio == bio
             && dtlsBio->generateCookie ( cookie, cookieLen ) == 1 ) ? 1 : 0;
}

#ifndef OPENSSL_IS_BORINGSSL

// BoringSSL does not support DTLS server mode
int DtlsBio::verifyCookie ( SSL * ssl, const unsigned char * cookie, unsigned int cookieLen )
{
    static unsigned char genCookie[ DTLS1_COOKIE_LENGTH ];
    unsigned int genLength = DTLS1_COOKIE_LENGTH;

    BIO * const bio = ( ssl != 0 ) ? ( SSL_get_rbio ( ssl ) ) : 0;
    DtlsBio * const dtlsBio = ( bio != 0 ) ? ( reinterpret_cast<DtlsBio *> ( BIO_get_data ( bio ) ) ) : 0;

    return ( dtlsBio != 0
             && dtlsBio->_bio == bio
             && dtlsBio->generateCookie ( genCookie, &genLength ) == 1
             && genLength == cookieLen
             && memcmp ( genCookie, cookie, genLength ) == 0 ) ? 1 : 0;
}

int DtlsBio::verifyCookie ( SSL * ssl, unsigned char * cookie, unsigned int cookieLen )
{
    return verifyCookie ( ssl, ( const unsigned char * ) cookie, cookieLen );
}
#endif
