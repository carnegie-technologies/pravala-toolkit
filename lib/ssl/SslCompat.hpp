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

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++11-extensions"
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif

extern "C"
{
#include <openssl/opensslv.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
}

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

#include "basic/SockAddr.hpp"

#if ( OPENSSL_VERSION_NUMBER < 0x10100000L ) || defined( OPENSSL_IS_BORINGSSL )

BIO_METHOD * BIO_meth_new ( int type, const char * name );

int BIO_meth_set_write ( BIO_METHOD * biom, int ( * write )( BIO *, const char *, int ) );
int BIO_meth_set_read ( BIO_METHOD * biom, int ( * read )( BIO *, char *, int ) );
int BIO_meth_set_puts ( BIO_METHOD * biom, int ( * puts )( BIO *, const char * ) );
int BIO_meth_set_ctrl ( BIO_METHOD * biom, long ( * ctrl )( BIO *, int, long, void * ) );
int BIO_meth_set_destroy ( BIO_METHOD * biom, int ( * destroy )( BIO * ) );

void BIO_set_data ( BIO * a, void * ptr );
void * BIO_get_data ( BIO * a );
void BIO_set_init ( BIO * a, int init );

const char * SSL_SESSION_get0_hostname ( const SSL_SESSION * s );
size_t SSL_SESSION_get_master_key ( const SSL_SESSION * ssl, unsigned char * out, size_t outlen );

#define TLS_client_method    TLSv1_2_client_method
#define TLS_server_method    TLSv1_2_server_method
#endif

#if ( OPENSSL_VERSION_NUMBER < 0x10100000L ) && !defined( OPENSSL_IS_BORINGSSL )

#define DTLS_client_method    DTLSv1_client_method
#define DTLS_server_method    DTLSv1_server_method
#endif

namespace Pravala
{
// Boring SSL doesn't support DTLS servers
#ifndef OPENSSL_IS_BORINGSSL
/// @brief A wrapper around DTLSv1_listen.
/// @param [in] s The SSL object to use for listening.
/// @param [out] remoteAddr The address of the host we listened to will be stored there.
/// @return The value returned by DTLSv1_listen.
int Prav_DTLSv1_listen ( SSL * s, SockAddr & remoteAddr );
#endif
}
