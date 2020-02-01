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

// We want this first to configure some extra compiler pragmas:
#include "SslContext.hpp"

extern "C"
{
#include <openssl/md5.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
}

#include "SslSocket.hpp"
#include "X509Utils.hpp"
#include "SslCompat.hpp"

using namespace Pravala;

TextLog SslSocket::_log ( "ssl_socket" );

SslSocket::SslSocket ( SocketOwner * owner, SslContext & sslContext ):
    IpSocket ( owner ),
    _ssl ( SSL_new ( sslContext.getContext() ) )
{
    if ( sslContext.isClient() )
    {
        setFlags ( SockSslFlagConnectNeeded );
    }
    else if ( sslContext.isServer() )
    {
        setFlags ( SockSslFlagAcceptNeeded );
    }
    else
    {
        LOG ( L_FATAL_ERROR, "Invalid SSL context received - it is neither client nor server type" );

        assert ( false );
    }

    if ( !_ssl )
    {
        LOG ( L_ERROR, "Could not create a new SSL object" );

        return;
    }

    if ( !sslContext.getSniHostname().isEmpty() )
    {
        setSniHostname ( sslContext.getSniHostname() );
    }
}

SslSocket::~SslSocket()
{
    close();

    if ( _ssl != 0 )
    {
        SSL_free ( _ssl );
        _ssl = 0;
    }
}

TlsSocket * SslSocket::getTlsSocket()
{
    return 0;
}

DtlsSocket * SslSocket::getDtlsSocket()
{
    return 0;
}

int SslSocket::getSslSockFd() const
{
    return ( _ssl != 0 ) ? ( SSL_get_fd ( _ssl ) ) : ( -1 );
}

String SslSocket::getLogId ( bool extended ) const
{
    if ( !extended )
    {
        return getRemoteDesc();
    }

    return String ( "%1-%2[%3]" )
           .arg ( getLocalSockAddr().toString(), getRemoteSockAddr().toString(), getCertSubject() );
}

const char * SslSocket::callTypeName ( SslSocket::CallType callType )
{
    switch ( callType )
    {
        case CallDtlsListen:
            return "DTLSv1_listen()";
            break;

        case CallAccept:
            return "SSL_accept()";
            break;

        case CallConnect:
            return "SSL_connect()";
            break;

        case CallRead:
            return "SSL_read()";
            break;

        case CallWrite:
            return "SSL_write()";
            break;

        case CallUnknown:
            break;
    }

    return "SSL_unknown()";
}

String SslSocket::getSessionSniHostname() const
{
    SSL_SESSION * const s = ( _ssl != 0 ) ? ( SSL_get_session ( _ssl ) ) : 0;

    return ( s != 0 ) ? SSL_SESSION_get0_hostname ( s ) : String();
}

bool SslSocket::setSniHostname ( const String & sniHostname )
{
    if ( _ssl != 0 && !sniHostname.isEmpty() )
    {
        ERR_clear_error();

        // Stupid OpenSSL... it assumes "char *" even though it would be just fine with "const char *"
        if ( SSL_set_tlsext_host_name ( _ssl, const_cast<char *> ( sniHostname.c_str() ) ) != 0 )
        {
            return true;
        }

#ifndef NO_LOGGING
        if ( _log.shouldLog ( L_ERROR ) )
        {
            LOG ( L_ERROR, "Error calling SSL_set_tlsext_host_name" );

            long unsigned err;

            while ( ( err = ERR_get_error() ) != 0 )
            {
                LOG ( L_ERROR, "Error calling SSL_set_tlsext_host_name: " << ERR_error_string ( err, 0 ) );
            }
        }
#endif

        ERR_clear_error();
    }

    return false;
}

String SslSocket::getCertSubject() const
{
    X509 * cert = SSL_get_peer_certificate ( _ssl );

    String ret = X509Utils::getCertSubject ( cert );

    if ( cert != 0 )
    {
        X509_free ( cert );
    }

    return ret;
}

String SslSocket::getCertIssuer() const
{
    X509 * cert = SSL_get_peer_certificate ( _ssl );

    String ret = X509Utils::getCertIssuer ( cert );

    if ( cert != 0 )
    {
        X509_free ( cert );
    }

    return ret;
}

MemHandle SslSocket::getCertSerialNumber() const
{
    X509 * cert = SSL_get_peer_certificate ( _ssl );

    MemHandle ret = X509Utils::getCertSerialNumber ( cert );

    if ( cert != 0 )
    {
        X509_free ( cert );
    }

    return ret;
}

List<IpAddress> SslSocket::getCertIpAddresses() const
{
    X509 * cert = SSL_get_peer_certificate ( _ssl );

    List<IpAddress> addrs = X509Utils::getCertIpAddresses ( cert );

    if ( cert != 0 )
    {
        X509_free ( cert );
    }

    return addrs;
}

String SslSocket::getCertDesc() const
{
    const String curCipher = SSL_CIPHER_get_name ( SSL_get_current_cipher ( _ssl ) );
    X509 * cert = SSL_get_peer_certificate ( _ssl );

    String desc;

    if ( cert != 0 )
    {
        desc = String ( "'%1'; Cipher: '%2'" ).arg ( X509Utils::getCertSubject ( cert ), curCipher );

        X509_free ( cert );
    }
    else
    {
        desc = String ( "'NO CERTIFICATE'; Cipher: '%1'" ).arg ( curCipher );
    }

    return desc;
}

String SslSocket::getSessionMasterKeyHash ( bool printableHex ) const
{
    SSL_SESSION * s = ( _ssl != 0 ) ? ( SSL_get_session ( _ssl ) ) : 0;

    if ( !s )
        return String();

    size_t keyLen = SSL_SESSION_get_master_key ( s, 0, 0 );

    if ( keyLen < 1 )
        return String();

    MD5_CTX ctx;

    if ( MD5_Init ( &ctx ) == 0 )
        return String();

    unsigned char * buf = new uint8_t[ keyLen ];

    keyLen = SSL_SESSION_get_master_key ( s, buf, keyLen );

    unsigned char md[ MD5_DIGEST_LENGTH ];

    MD5_Update ( &ctx, buf, keyLen );
    MD5_Final ( md, &ctx );

    delete[] buf;

    if ( printableHex )
    {
        // We use hexDump, no '0x' and empty string as the separator
        return String::hexDump ( ( const char * ) md, MD5_DIGEST_LENGTH, false, "" );
    }

    return String ( ( const char * ) md, MD5_DIGEST_LENGTH );
}

void SslSocket::scheduleClosedEvent ( ERRCODE reason )
{
    _closedReason = reason;

    scheduleEvents ( SockEventClosed );
}

bool SslSocket::runEvents ( uint16_t events )
{
    if ( ( events & SockEventClosed ) == SockEventClosed )
    {
        doSockClosed ( _closedReason );
        return true;
    }

    return IpSocket::runEvents ( events );
}

void SslSocket::socketClosed ( Socket *, ERRCODE reason )
{
    LOG_ERR ( L_ERROR, reason, "Underlying data socket has been closed" );

    doSockClosed ( reason );
}

void SslSocket::socketConnectFailed ( Socket *, ERRCODE reason )
{
    LOG_ERR ( L_ERROR, reason, "Underlying data socket failed to connect" );

    doSockConnectFailed ( reason );
}

void SslSocket::socketDataReceived ( Socket *, MemHandle & )
{
    LOG ( L_FATAL_ERROR, "Received unexpected callback" );
}

void SslSocket::socketReadyToSend ( Socket * )
{
    LOG ( L_FATAL_ERROR, "Received unexpected callback" );
}

ERRCODE SslSocket::handleSslError ( SslSocket::CallType callType, int callRet, int fd, bool delayCallbacks )
{
    assert ( callRet < 1 );

    ERRCODE eCode = Error::Unknown;

    switch ( SSL_get_error ( _ssl, callRet ) )
    {
        case SSL_ERROR_NONE:
            // Not really an error!
            LOG ( L_DEBUG2, callTypeName ( callType ) << ": " << getLogId() << ": SSL_ERROR_NONE" );

            eCode = Error::SoftFail;
            break;

        case SSL_ERROR_WANT_READ:
            // Just try again
            LOG ( L_DEBUG2, callTypeName ( callType ) << ": " << getLogId() << ": SSL_ERROR_WANT_READ" );

            if ( callType == CallWrite )
            {
                setFlags ( SockSslFlagDoWriteOnRead );
            }

            if ( fd >= 0 )
            {
                // Enable read, disable write.
                // We disable write events, because OpenSSL needs to read more data before it can do anything else,
                // so writes would fail anyway.
                EventManager::setFdEvents ( fd, EventManager::EventRead );
            }

            eCode = Error::SslNeedRead;
            break;

        case SSL_ERROR_WANT_WRITE:
            // Try again, but first we need to enable write events
            LOG ( L_DEBUG2, callTypeName ( callType ) << ": " << getLogId()
                  << ": SSL_ERROR_WANT_WRITE - enabling write events on FD: " << fd );

            if ( callType == CallRead )
            {
                setFlags ( SockSslFlagDoReadOnWrite );
            }

            if ( fd >= 0 )
            {
                EventManager::enableWriteEvents ( fd );
            }

            eCode = Error::SslNeedWrite;
            break;

        case SSL_ERROR_ZERO_RETURN:
            LOG ( L_ERROR, callTypeName ( callType ) << ": " << getLogId() << ": SSL_ERROR_ZERO_RETURN" );

            ERR_clear_error();

            if ( !delayCallbacks )
            {
                doSockClosed ( Error::Closed );
            }
            else
            {
                scheduleClosedEvent ( Error::Closed );
            }
            return Error::Closed;
            break;

        case SSL_ERROR_SYSCALL:
            {
                long unsigned err = ERR_get_error();

                if ( err == 0 && callRet == 0 )
                {
                    LOG ( L_DEBUG, callTypeName ( callType ) << ": " << getLogId()
                          << ": SSL failed: Socket closed" );

                    ERR_clear_error();

                    if ( !delayCallbacks )
                    {
                        doSockClosed ( Error::Closed );
                    }
                    else
                    {
                        scheduleClosedEvent ( Error::Closed );
                    }
                    return Error::Closed;
                }

#ifndef NO_LOGGING
                if ( _log.shouldLog ( L_INFO ) )
                {
                    LOG ( L_INFO, callTypeName ( callType ) << ": " << getLogId()
                          << ": SSL_ERROR_SYSCALL: [" << errno << "] " << strerror ( errno ) );

                    if ( err != 0 )
                    {
                        do
                        {
                            LOG ( L_INFO, callTypeName ( callType ) << ": " << getLogId()
                                  << ": Syscall error: " << ERR_error_string ( err, 0 ) );
                        }
                        while ( ( err = ERR_get_error() ) != 0 );
                    }
                    else if ( callRet == -1 )
                    {
                        LOG ( L_INFO, callTypeName ( callType ) << ": " << getLogId()
                              << ": Syscall error: " << strerror ( errno ) );
                    }
                    else
                    {
                        LOG ( L_INFO, callTypeName ( callType ) << ": " << getLogId()
                              << ": Syscall error, code: " << callRet );
                    }
                }
#endif

                ERR_clear_error();

                if ( !delayCallbacks )
                {
                    doSockClosed ( Error::SslSyscallError );
                }
                else
                {
                    scheduleClosedEvent ( Error::SslSyscallError );
                }
                return Error::SslSyscallError;
            }
            break;

        case SSL_ERROR_SSL:
#ifndef NO_LOGGING
            if ( _log.shouldLog ( L_ERROR ) )
            {
                unsigned long err;

                while ( ( err = ERR_get_error() ) != 0 )
                {
                    LOG ( L_ERROR, callTypeName ( callType ) << ": " << getLogId()
                          << ": SSL_ERROR_SSL: " << ERR_error_string ( err, 0 ) );
                }
            }
#endif

            ERR_clear_error();

            if ( !delayCallbacks )
            {
                doSockClosed ( Error::SslError );
            }
            else
            {
                scheduleClosedEvent ( Error::SslError );
            }
            return Error::SslError;
            break;

        default:
            LOG ( L_ERROR, callTypeName ( callType ) << ": " << getLogId()
                  << ": Unexpected error (" << callRet << ") - socket failed" );

            ERR_clear_error();

            if ( !delayCallbacks )
            {
                doSockClosed ( Error::SslUnknown );
            }
            else
            {
                scheduleClosedEvent ( Error::SslUnknown );
            }
            return Error::SslUnknown;
            break;
    }

    ERR_clear_error();

    return eCode;
}
