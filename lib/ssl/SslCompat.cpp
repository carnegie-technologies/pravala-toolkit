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

#include <cstdlib>

#include "SslCompat.hpp"

#if ( OPENSSL_VERSION_NUMBER < 0x10100000L ) || defined( OPENSSL_IS_BORINGSSL )

BIO_METHOD * BIO_meth_new ( int type, const char * name )
{
    BIO_METHOD * biom = ( BIO_METHOD * ) calloc ( 1, sizeof ( BIO_METHOD ) );

    if ( biom != 0 )
    {
        biom->type = type;
        biom->name = name;
    }

    return biom;
}

int BIO_meth_set_write ( BIO_METHOD * biom, int ( * write )( BIO *, const char *, int ) )
{
    biom->bwrite = write;
    return 1;
}

int BIO_meth_set_read ( BIO_METHOD * biom, int ( * read )( BIO *, char *, int ) )
{
    biom->bread = read;
    return 1;
}

int BIO_meth_set_puts ( BIO_METHOD * biom, int ( * puts )( BIO *, const char * ) )
{
    biom->bputs = puts;
    return 1;
}

int BIO_meth_set_ctrl ( BIO_METHOD * biom, long ( * ctrl )( BIO *, int, long, void * ) )
{
    biom->ctrl = ctrl;
    return 1;
}

int BIO_meth_set_destroy ( BIO_METHOD * biom, int ( * destroy )( BIO * ) )
{
    biom->destroy = destroy;
    return 1;
}

void BIO_set_data ( BIO * a, void * ptr )
{
    a->ptr = ptr;
}

void * BIO_get_data ( BIO * a )
{
    return a->ptr;
}

void BIO_set_init ( BIO * a, int init )
{
    a->init = init;
}

const char * SSL_SESSION_get0_hostname ( const SSL_SESSION * s )
{
    return s->tlsext_hostname;
}

#ifndef OPENSSL_IS_BORINGSSL

// Boring SSL includes SSL_SESSION_get_master_key()
size_t SSL_SESSION_get_master_key ( const SSL_SESSION * s, unsigned char * out, size_t outlen )
{
    if ( outlen < 1 )
        return s->master_key_length;

    if ( s->master_key_length < 1 )
        return 0;

    if ( outlen > ( size_t ) s->master_key_length )
        outlen = s->master_key_length;

    if ( out != 0 )
        memcpy ( out, s->master_key, outlen );

    return outlen;
}

// Boring SSL doesn't support DTLS servers
int Pravala::Prav_DTLSv1_listen ( SSL * s, SockAddr & remoteAddr )
{
    remoteAddr.clear();

    return DTLSv1_listen ( s, &remoteAddr );
}
#endif

#else

int Pravala::Prav_DTLSv1_listen ( SSL * s, SockAddr & remoteAddr )
{
    remoteAddr.clear();

    BIO_ADDR * peer = BIO_ADDR_new();

    const int ret = DTLSv1_listen ( s, peer );

    size_t addrLen = 0;

    if ( peer != 0 )
    {
        // We don't check the result - if it fails, addrLen will still be 0.
        BIO_ADDR_rawaddress ( peer, 0, &addrLen );

        if ( addrLen == sizeof ( remoteAddr.sa_in.sin_addr ) )
        {
            // NOTE: The API of this function is dumb and dangerous!
            //       If we pass any non-zero pointer, it will write as many bytes as it wants to.
            //       That's why we need to check required length first (the first call with 0 buffer).
            //       For consistency with functions that have sane API, we pass the same size...
            if ( BIO_ADDR_rawaddress ( peer, &remoteAddr.sa_in.sin_addr, &addrLen ) == 1 )
            {
                remoteAddr.sa_in.sin_family = AF_INET;

                // BIO_ADDR_rawport returns port number in network order, so we can just store it:
                remoteAddr.sa_in.sin_port = BIO_ADDR_rawport ( peer );

                assert ( remoteAddr.isIPv4() );
            }
        }
        else if ( addrLen == sizeof ( remoteAddr.sa_in6.sin6_addr ) )
        {
            // NOTE: The API of this function is dumb and dangerous!
            //       If we pass any non-zero pointer, it will write as many bytes as it wants to.
            //       That's why we need to check required length first (the first call with 0 buffer).
            //       For consistency with functions that have sane API, we pass the same size...
            if ( BIO_ADDR_rawaddress ( peer, &remoteAddr.sa_in6.sin6_addr, &addrLen ) == 1 )
            {
                remoteAddr.sa_in6.sin6_family = AF_INET6;

                // BIO_ADDR_rawport returns port number in network order, so we can just store it:
                remoteAddr.sa_in6.sin6_port = BIO_ADDR_rawport ( peer );

                assert ( remoteAddr.isIPv6() );
            }
        }

        BIO_ADDR_free ( peer );
        peer = 0;
    }

    return ret;
}
#endif
