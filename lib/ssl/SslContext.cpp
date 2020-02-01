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
#include <openssl/rand.h>
#include <openssl/err.h>
}

#include "sys/SocketApi.hpp"
#include "SslContext.hpp"
#include "DtlsBio.hpp"
#include "X509Utils.hpp"
#include "SslCompat.hpp"

using namespace Pravala;

TextLog SslContext::_log ( "ssl" );

bool SslContext::_sslIsInitialized ( false );

ConfigString SslContext::optSslCipherList (
        ConfigOpt::FlagInitializeOnly,
        "ssl.cipher_list",
        "The list of ciphers to use (see openssl ciphers)",
        "HIGH"
);

ConfigString SslContext::optSslCipherListTls (
        ConfigOpt::FlagInitializeOnly,
        "ssl.cipher_list_tls",
        "The list of ciphers to use for TLS connections; If set it will be used instead of the main one; "
        "(see openssl ciphers)"
);

SslContext::SslContext ( ContextType contextType ):
    _ctx ( 0 ),
    _verifyCallback ( 0 ),
    _verifyMode ( SSL_VERIFY_NONE ),
    _type ( contextType )
{
    // Detect if we can run properly on this machine.
    // If the system has support for AES NI and the OpenSSL version is too old it will crash inside DTLS code.
    // OpenSSL 1.0.1 doesn't work, 1.0.1c is fine.
    // Not sure about 'a' and 'b', so let's just say that 'c' or higher is required.
    // 1.0.1c is 0x1000103fL

    if ( SSLeay() < 0x1000103fL && isDtls() )
    {
        // Let's test the CPU support for AES:
        MemHandle buf;

        if ( buf.readFile ( "/proc/cpuinfo" ) )
        {
            StringList lines = buf.toStringList ( "\n" );

            for ( size_t i = 0; i < lines.size(); ++i )
            {
                if ( lines.at ( i ).startsWith ( "flags" ) )
                {
                    StringList flags = lines.at ( i ).split ( " " );

                    for ( size_t j = 0; j < flags.size(); ++j )
                    {
                        if ( flags.at ( j ) == "aes" )
                        {
                            // We have AES NI support and the OpenSSL is too old!

                            fprintf ( stderr, "This OpenSSL version (%s) is too old to use DTLS "
                                      "on a machine with AES NI support!\n", SSLeay_version ( SSLEAY_VERSION ) );

                            exit ( EXIT_FAILURE );
                            return;
                        }
                    }
                }
            }
        }
    }

    if ( !_sslIsInitialized )
    {
        OpenSSL_add_ssl_algorithms();
        SSL_load_error_strings();

        _sslIsInitialized = true;
    }

    switch ( _type )
    {
        case ContextTlsClient:
            _ctx = SSL_CTX_new ( TLS_client_method() );
            break;

        case ContextTlsServer:
            _ctx = SSL_CTX_new ( TLS_server_method() );
            break;

        case ContextDtlsClient:
            _ctx = SSL_CTX_new ( DTLS_client_method() );
            break;

        case ContextDtlsServer:
#ifdef OPENSSL_IS_BORINGSSL
            fprintf ( stderr, "BoringSSL does not support DTLS server mode\n" );
            exit ( EXIT_FAILURE );
            return;
#else
            _ctx = SSL_CTX_new ( DTLS_server_method() );

            // Set DTLS cookie generation and verification callbacks
            SSL_CTX_set_cookie_generate_cb ( _ctx, DtlsBio::generateCookie );
            SSL_CTX_set_cookie_verify_cb ( _ctx, DtlsBio::verifyCookie );
#endif
            break;
    }

    if ( !_ctx )
    {
        LOG ( L_FATAL_ERROR, "Error generating SSL context; Type: " << _type );

        logSslErrors ( "SslContext()", L_FATAL_ERROR );

        assert ( false );

        return;
    }

    SSL_CTX_set_session_cache_mode ( _ctx, SSL_SESS_CACHE_OFF );

    SSL_CTX_set_verify_depth ( _ctx, 2 );
    SSL_CTX_set_read_ahead ( _ctx, 1 );

    // Compression is nothing but trouble...
    SSL_CTX_set_options ( _ctx, SSL_CTX_get_options ( _ctx ) | SSL_OP_NO_COMPRESSION );

    if ( isTls() && optSslCipherListTls.isNonEmpty() )
    {
        const ERRCODE eCode = setCipherList ( optSslCipherListTls.value() );

        if ( NOT_OK ( eCode ) )
        {
            LOG ( L_ERROR, "Error setting TLS-specific cipher list to: '" << optSslCipherListTls.value() << "'" );
        }
    }
    else if ( optSslCipherList.isNonEmpty() )
    {
        const ERRCODE eCode = setCipherList ( optSslCipherList.value() );

        if ( NOT_OK ( eCode ) )
        {
            LOG ( L_ERROR, "Error setting cipher list to: '" << optSslCipherList.value() << "'" );
        }
    }
}

DtlsContext::DtlsContext ( ContextType contextType ): SslContext ( contextType )
{
    assert ( _ctx != 0 );
    assert ( isDtls() );
}

DtlsClient::DtlsClient(): DtlsContext ( ContextDtlsClient )
{
    assert ( _ctx != 0 );
}

DtlsServer::DtlsServer(): DtlsContext ( ContextDtlsServer )
{
    assert ( _ctx != 0 );
}

TlsContext::TlsContext ( ContextType contextType ): SslContext ( contextType )
{
    assert ( _ctx != 0 );
    assert ( isTls() );
}

TlsClient::TlsClient(): TlsContext ( ContextTlsClient )
{
    assert ( _ctx != 0 );
}

TlsServer::TlsServer(): TlsContext ( ContextTlsServer )
{
    assert ( _ctx != 0 );
}

SslContext::~SslContext()
{
    if ( _ctx != 0 )
    {
        SSL_CTX_free ( _ctx );
        _ctx = 0;
    }
}

const char * SslContext::getContextTypeName() const
{
    switch ( _type )
    {
        case ContextDtlsClient:
            return "DTLS-Client";
            break;

        case ContextDtlsServer:
            return "DTLS-Server";
            break;

        case ContextTlsClient:
            return "TLS-Client";
            break;

        case ContextTlsServer:
            return "TLS-Server";
            break;
    }

    return "Unknown";
}

void SslContext::setSniHostname ( const String & sniHostname )
{
    _sniHostname = sniHostname;
}

ERRCODE SslContext::setKeyPair ( const String & keyFile, const String & certFile )
{
    assert ( _ctx != 0 );

    ERRCODE ret = Error::Success;

    ERR_clear_error();

    if ( !SSL_CTX_use_certificate_file ( _ctx, certFile.c_str(), SSL_FILETYPE_PEM ) )
    {
        LOG ( L_ERROR, "Could not load certificate from '" << certFile << "'" );

        ret = Error::PkiBadCert;
    }
    else if ( !SSL_CTX_use_PrivateKey_file ( _ctx, keyFile.c_str(), SSL_FILETYPE_PEM ) )
    {
        LOG ( L_ERROR, "Could not load private key from '" << keyFile << "'" );

        ret = Error::PkiBadKey;
    }
    else if ( !SSL_CTX_check_private_key ( _ctx ) )
    {
        LOG ( L_ERROR, "Invalid private key in '" << keyFile << "'" );

        ret = Error::PkiBadKey;
    }

    if ( IS_OK ( ret ) )
    {
        LOG_ERR ( L_DEBUG, ret, "Successfully configured SSL key and certificate" );
    }
#ifndef NO_LOGGING
    else if ( _log.shouldLog ( L_ERROR ) )
    {
        logSslErrors ( "setKeyPair()" );
    }
#endif

    ERR_clear_error();

    return ret;
}

ERRCODE SslContext::setKeyPairData ( const String & keyData, const String & certData )
{
    if ( certData.isEmpty() )
    {
        LOG ( L_ERROR, "Empty SSL CERT data received" );

        return Error::InvalidDataSize;
    }

    if ( keyData.isEmpty() )
    {
        LOG ( L_ERROR, "Empty SSL KEY data received" );

        return Error::InvalidDataSize;
    }

    // HAS to be freed!
    EVP_PKEY * pKey = pemReadPrivateKey ( keyData.c_str(), keyData.length() );

    if ( !pKey )
    {
        LOG ( L_ERROR, "Invalid PEM KEY data" );

        return Error::PkiBadKey;
    }

    // HAS to be freed!
    List<X509 *> certs = X509Utils::pemReadCertificates ( certData.c_str(), certData.length(), 1 );

    if ( certs.isEmpty() )
    {
        LOG ( L_ERROR, "Invalid PEM CERT data" );

        EVP_PKEY_free ( pKey );
        pKey = 0;

        return Error::PkiBadCert;
    }

    assert ( _ctx != 0 );

    ERRCODE ret = Error::Success;

    ERR_clear_error();

    if ( !SSL_CTX_use_certificate ( _ctx, certs.first() ) )
    {
        LOG ( L_ERROR, "Invalid PEM certificate data" );

        ret = Error::PkiBadCert;
    }
    else if ( !SSL_CTX_use_PrivateKey ( _ctx, pKey ) )
    {
        LOG ( L_ERROR, "Invalid PEM key data" );

        ret = Error::PkiBadKey;
    }
    else if ( !SSL_CTX_check_private_key ( _ctx ) )
    {
        LOG ( L_ERROR, "Invalid PEM key" );

        ret = Error::PkiBadKey;
    }

    if ( IS_OK ( ret ) )
    {
        LOG_ERR ( L_DEBUG, ret, "Successfully configured SSL key and certificate" );
    }
#ifndef NO_LOGGING
    else if ( _log.shouldLog ( L_ERROR ) )
    {
        logSslErrors ( "setKeyPairData()" );
    }
#endif

    ERR_clear_error();

    X509Utils::pemClearCertificates ( certs );

    if ( pKey != 0 )
    {
        EVP_PKEY_free ( pKey );
        pKey = 0;
    }

    return ret;
}

ERRCODE SslContext::setCipherList ( const String & cipherList )
{
    assert ( _ctx != 0 );

    if ( cipherList.isEmpty() )
    {
        return Error::InvalidParameter;
    }

    ERR_clear_error();

    if ( !SSL_CTX_set_cipher_list ( _ctx, cipherList.c_str() ) )
    {
#ifndef NO_LOGGING
        if ( _log.shouldLog ( L_ERROR ) )
        {
            LOG ( L_ERROR, "Error setting requested cipher list: '" << cipherList << "'" );

            logSslErrors ( "setCipherList()" );
        }
#endif

        ERR_clear_error();

        return Error::SslBadCipherList;
    }

    return Error::Success;
}

ERRCODE SslContext::setTrustedCAs ( const String & caFile, const String & caDir )
{
    assert ( _ctx != 0 );

    ERR_clear_error();

    // 0 if caFile/caDir is empty, otherwise c_str to the path
    if ( !SSL_CTX_load_verify_locations ( _ctx,
                                          caFile.isEmpty() ? 0 : caFile.c_str(),
                                          caDir.isEmpty() ? 0 : caDir.c_str() ) )
    {
#ifndef NO_LOGGING
        if ( _log.shouldLog ( L_ERROR ) )
        {
            LOG ( L_ERROR, "Error setting trusted CAs to '" << caFile << "' / '" << caDir << "'" );

            logSslErrors ( "setTrustedCAs()" );
        }
#endif

        ERR_clear_error();
        return Error::PkiBadCert;
    }

    return Error::Success;
}

ERRCODE SslContext::setTrustedCAsData ( const String & trustedCAsData )
{
    if ( trustedCAsData.isEmpty() )
    {
        LOG ( L_ERROR, "Empty trusted CAs data received" );

        return Error::InvalidDataSize;
    }

    List<X509 *> certs = X509Utils::pemReadCertificates ( trustedCAsData.c_str(), trustedCAsData.length() );

    size_t added = 0;

    while ( !certs.isEmpty() )
    {
        ERR_clear_error();

        if ( !X509_STORE_add_cert ( SSL_CTX_get_cert_store ( _ctx ), certs.first() ) )
        {
            LOG ( L_ERROR, "Could not add a trusted CA to X509 store" );

#ifndef NO_LOGGING
            if ( _log.shouldLog ( L_ERROR ) )
            {
                logSslErrors ( "setTrustedCAsData()" );
            }
#endif

            ERR_clear_error();
        }
        else
        {
            ++added;
        }

        X509_free ( certs.first() );

        certs.removeFirst();
    }

    if ( added < 1 )
    {
        return Error::PkiBadCert;
    }

    LOG ( L_DEBUG, "Added " << added << " trusted CAs to X509 certificate store" );

    return Error::Success;
}

ERRCODE SslContext::setAllowedCAs ( const String & certFile )
{
    assert ( _ctx != 0 );
    assert ( !certFile.isEmpty() );

    if ( certFile.isEmpty() )
    {
        return Error::InvalidParameter;
    }

    ERR_clear_error();

    STACK_OF ( X509_NAME ) * clientCAs = SSL_load_client_CA_file ( certFile.c_str() );

    if ( !clientCAs )
    {
#ifndef NO_LOGGING
        if ( _log.shouldLog ( L_ERROR ) )
        {
            LOG ( L_ERROR, "Error setting allowed CAs to '" << certFile << "'" );

            logSslErrors ( "setAllowedCAs()" );
        }
#endif

        ERR_clear_error();
        return Error::PkiBadCert;
    }

    ERR_clear_error();

    SSL_CTX_set_client_CA_list ( _ctx, clientCAs );

    switch ( _type )
    {
        case ContextTlsClient:
        case ContextDtlsClient:
            // SSL_VERIFY_FAIL_IF_NO_PEER_CERT and SSL_VERIFY_CLIENT_ONCE are ignored on the client
            _verifyMode = SSL_VERIFY_PEER;
            break;

        case ContextTlsServer:
        case ContextDtlsServer:
            _verifyMode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE;
            break;
    }

    setSslVerify();

    return Error::Success;
}

#if 0
void SslContext::setCompressionEnabled ( bool enabled )
{
    assert ( _ctx != 0 );

    long opts = SSL_CTX_get_options ( _ctx );

    if ( enabled )
    {
        opts &= ~SSL_OP_NO_COMPRESSION;
    }
    else
    {
        opts |= SSL_OP_NO_COMPRESSION;
    }

    SSL_CTX_set_options ( _ctx, opts );
}
#endif

void SslContext::setVerifyCallback ( int ( * verifyCallback )( int, X509_STORE_CTX * ) )
{
    assert ( _ctx != 0 );

    _verifyCallback = verifyCallback;

    setSslVerify();
}

void SslContext::setSslVerify()
{
    assert ( _ctx != 0 );

    SSL_CTX_set_verify ( _ctx, _verifyMode, _verifyCallback );
}

void SslContext::logSslErrors ( const char * forMethod, const Log::LogLevel & logLevel )
{
    ( void ) forMethod;
    ( void ) logLevel;

#ifndef NO_LOGGING
    if ( _log.shouldLog ( logLevel ) )
    {
        long unsigned err;

        while ( ( err = ERR_get_error() ) != 0 )
        {
            LOG_NFUNC ( logLevel, forMethod << ": " << ERR_error_string ( err, 0 ) );
        }
    }
#endif

    ERR_clear_error();
}

EVP_PKEY * SslContext::pemReadPrivateKey ( const char * data, size_t dataSize )
{
    if ( !data || dataSize < 1 )
    {
        LOG ( L_ERROR, "Empty PEM data received" );

        return 0;
    }

    BIO * memBio = BIO_new ( BIO_s_mem() );

    const int ret = BIO_write ( memBio, data, dataSize );

    if ( ret < 0 || ( size_t ) ret != dataSize )
    {
        LOG ( L_ERROR, "Error appending PEM data to SSL memory BIO; Return code: "
              << ret << "; Expected: " << dataSize );

        BIO_free ( memBio );

        memBio = 0;

        return 0;
    }

    EVP_PKEY * const pKey = PEM_read_bio_PrivateKey ( memBio, 0, 0, 0 );

    BIO_free ( memBio );

    memBio = 0;

    return pKey;
}
