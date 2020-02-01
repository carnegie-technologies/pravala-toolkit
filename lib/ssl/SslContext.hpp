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

#pragma once

#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "config/ConfigString.hpp"

// Compilers are not happy with OpenSSL code when 'pedantic' is enabled...

#ifndef _MSC_VER
#pragma GCC diagnostic push
#if ( __GNUC__ >= 6 ) || ( defined __clang__ )
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

// Different versions:
//
// 0.9.8m = 0x009080dfL
// 1.0.0  = 0x1000000fL
// 1.0.0a = 0x1000001fL
// 1.0.0f = 0x1000006fL
// 1.0.1  = 0x1000100fL
// 1.0.1a = 0x1000101fL
// 1.0.1b = 0x1000102fL
// 1.0.1c = 0x1000103fL

#if ( OPENSSL_VERSION_NUMBER < 0x1000000fL )
#error "OpenSSL is too old! Please use 1.0.0 or later"
#endif

typedef struct evp_pkey_st EVP_PKEY;

namespace Pravala
{
/// @brief A wrapper around SSL_CTX
/// Also provides some general, helper functions
class SslContext
{
    public:
        static ConfigString optSslCipherList; ///< the cipher list string passed to setCipherList()
        static ConfigString optSslCipherListTls; ///< the cipher list string passed to setCipherList() for TLS context

        /// @brief Returns internal SSL_CTX pointer
        /// @return Internal SSL_CTX pointer
        inline SSL_CTX * getContext()
        {
            return _ctx;
        }

        /// @brief Sets the key pair for SSL context to use
        /// @param [in] keyFile Key file
        /// @param [in] certFile Certificate file
        /// @return Standard error code
        ERRCODE setKeyPair ( const String & keyFile, const String & certFile );

        /// @brief Sets the key pair for SSL context to use
        /// @param [in] keyData The key data in PEM format.
        /// @param [in] certData The certificate data in PEM format.
        /// @return Standard error code
        ERRCODE setKeyPairData ( const String & keyData, const String & certData );

        /// @brief Sets the list of ciphers that can be used
        /// @param [in] cipherList The list of available ciphers. See ciphers(1) manual.
        /// @return Standard error code
        ERRCODE setCipherList ( const String & cipherList );

        /// @brief Sets the file and/or the directory with the list of CAs that should be trusted
        ///
        /// This is the list of CAs used to build a cert chain.
        ///
        /// Required for both client and server when using client certs. It HAS TO be called
        /// after the setup() functions.
        ///
        /// @param [in] trustedCAsFile Full path to the file containing certificates (can be empty)
        /// @param [in] trustedCAsDir Full path to the directory containing certificates (can be empty)
        ///                    See 'man SSL_CTX_load_verify_locations' for file naming format in this directory
        /// @return Standard error code
        ERRCODE setTrustedCAs ( const String & trustedCAsFile, const String & trustedCAsDir );

        /// @brief Sets the CAs that should be trusted.
        ///
        /// This is the list of CAs used to build a cert chain.
        ///
        /// Required for both client and server when using client certs. It HAS TO be called
        /// after the setup() functions.
        ///
        /// @param [in] trustedCAsData Data in PEM format that contains the certificates.
        /// @return Standard error code
        ERRCODE setTrustedCAsData ( const String & trustedCAsData );

        /// @brief Sets the list of allowed CAs to be loaded from certFile
        ///
        /// Calling this function also switches from the default SSL_VERIFY_NONE to SSL_VERIFY_PEER mode
        /// (see documentation for SSL_CTX_set_verify() ). The server context will also use
        /// SSL_VERIFY_FAIL_IF_NO_PEER_CERT and SSL_VERIFY_CLIENT_ONCE in addition to SSL_VERIFY_PEER.
        ///
        /// It uses SSL_CTX_set_verify() function. It preserves the 'verify callback' already set
        /// (so the order of setVerifyCallback() and setAllowedCAs() calls doesn't matter).
        ///
        /// @param [in] certFile Path to the PEM encoded certificates file
        /// @return Standard error code
        ERRCODE setAllowedCAs ( const String & allowedCAsFile );

        /// @brief Sets the SSL verify callback
        ///
        /// It uses SSL_CTX_set_verify() function. It preserves the 'verify mode' already set
        /// (so the order of setVerifyCallback() and setAllowedCAs() calls doesn't matter).
        ///
        /// @param [in] verifyCallback Function pointer to a static callback function with parameters:
        ///                int: 0 if it failed pre-verify, 1 if pre-verify succeeded (see SSL_set_verify docs)
        ///                X509_STORE_CTX *: pointer to X509_STORE_CTX with certificate data
        ///                It must return either a 1 (verify ok) or a 0 (verify failed)
        void setVerifyCallback ( int ( * verifyCallback )( int, X509_STORE_CTX * ) );

#if 0
        /// @brief Configures SSL to use the compression (or not)
        /// @param [in] enabled If true, the compression is enabled, otherwise it is disabled.
        void setCompressionEnabled ( bool enabled );
#endif

        /// @brief Returns true if this is DTLS context
        /// @return true if this is DTLS context; false otherwise
        inline bool isDtls() const
        {
            return ( _type == ContextDtlsClient || _type == ContextDtlsServer );
        }

        /// @brief Returns true if this is TLS context
        /// @return true if this is TLS context; false otherwise
        inline bool isTls() const
        {
            return ( _type == ContextTlsClient || _type == ContextTlsServer );
        }

        /// @brief Returns true if this is client-side context
        /// @return true if this is client-side context; false otherwise
        inline bool isClient() const
        {
            return ( _type == ContextDtlsClient || _type == ContextTlsClient );
        }

        /// @brief Returns true if this is server-side context
        /// @return true if this is server-side context; false otherwise
        inline bool isServer() const
        {
            return ( _type == ContextDtlsServer || _type == ContextTlsServer );
        }

        /// @brief Returns the SNI hostname
        /// @return the SNI hostname (empty by default)
        inline const String & getSniHostname() const
        {
            return _sniHostname;
        }

        /// @brief Sets the new SNI hostname
        /// @param [in] sniHostname The new SNI hostname to use
        void setSniHostname ( const String & sniHostname );

        /// @brief Destructor
        virtual ~SslContext();

        /// @brief Returns the name of the context type.
        /// @return The name of this context's type.
        const char * getContextTypeName() const;

        /// @brief A helper function that extracts a private key stored in PEM format.
        /// @param [in] data Pointer to PEM data.
        /// @param [in] dataSize The size of data.
        /// @return A pointer to athe private key extracted from PEM data.
        ///          This pointer has to be freed by the caller using EVP_PKEY_free() SSL function.
        static EVP_PKEY * pemReadPrivateKey ( const char * data, size_t dataSize );

    protected:
        /// @brief The type of this context
        enum ContextType
        {
            ContextDtlsClient, ///< DTLS client's context
            ContextDtlsServer, ///< DTLS server's context
            ContextTlsClient, ///< TLS client's context
            ContextTlsServer ///< TLS server's context
        };

        static TextLog _log; ///< Log stream

        static bool _sslIsInitialized; ///< A flag used for initializing SSL library

        SSL_CTX * _ctx; ///< The internal pointer to SSL_CTX

        String _sniHostname; ///< SNI hostname of this context

        int (* _verifyCallback) ( int, X509_STORE_CTX * );  ///< The callbacked used by SSL_CTX_set_verify function
        int _verifyMode; ///< The mode used by SSL_CTX_set_verify function

        const ContextType _type; ///< The context type (client/server)

        /// @brief Constructor
        /// The first time it is run (using _sslIsInitialized flag) it initialized the SSL library.
        /// @note By default the SSL context is in SSL_VERIFY_NONE mode, so CAs are not verified.
        /// @warning This function may actually kill the program (call exit()) if it detects some serious problems.
        ///          Right now it checks for problematic version on hardware with AES NI support.
        /// @param [in] contextType The context type, either client or server
        SslContext ( ContextType contextType );

        /// @brief Sets the verify mode using SSL_CTX_set_verify function
        /// It uses current content of _verifyMode and _verifyCallback
        void setSslVerify();

        /// @brief Logs SSL errors.
        /// @note It also clears the SSL error queue.
        /// @param [in] forMethod Method's name to be included in the log entries.
        /// @param [in] logLevel The log level of the message; L_ERROR by default.
        static void logSslErrors ( const char * forMethod, const Log::LogLevel & logLevel = L_ERROR );
};

/// @brief DTLS version of the SslContext object
class DtlsContext: public SslContext
{
    protected:
        /// @brief Constructor
        /// @param [in] contextType The context type, either client or server
        DtlsContext ( ContextType contextType );
};

/// @brief DTLS Client version of the SslContext object
class DtlsClient: public DtlsContext
{
    public:
        /// @brief Default Constructor
        DtlsClient();
};

/// @brief DTLS Server version of the SslContext object
class DtlsServer: public DtlsContext
{
    public:
        /// @brief Default Constructor
        DtlsServer();
};

/// @brief TLS version of the SslContext object
class TlsContext: public SslContext
{
    protected:
        /// @brief Constructor
        /// @param [in] contextType The context type, either client or server
        TlsContext ( ContextType contextType );
};

/// @brief TLS Client version of the SslContext object
class TlsClient: public TlsContext
{
    public:
        /// @brief Default Constructor
        TlsClient();
};

/// @brief TLS Server version of the SslContext object
class TlsServer: public TlsContext
{
    public:
        /// @brief Default Constructor
        TlsServer();
};
}
