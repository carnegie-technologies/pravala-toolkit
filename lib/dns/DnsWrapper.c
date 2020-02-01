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

// dns.c uses _BSD_SOURCE to enable some extra features.
// That define is deprecated and generates warnings.
// To avoid that, we just define _DEFAULT_SOURCE that should be used instead.

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE    1
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE        1
#endif

#include <pthread.h>

// We also need to include all headers that declare symbols we want to redefine.
// This HAS to happen before we redefine the functions that we want to take over (close(), send(), etc.).
// Otherwise there is a risk that the originals will be used instead.
// For example, this happens on iOS/OSX with close().

// This header declares close():
#include <unistd.h>

// This header declares all socket operations:
#include <sys/socket.h>

#ifdef PLATFORM_ANDROID

// On Android we might not have atomic fetch and add/sub operations.
// We don't want dns.c to fallback to using i++ and i--,
// so we use our own method that uses a mutex.

/// @brief A mutex used for synchronizing our atomic fetch and add/sub operations.
static pthread_mutex_t sDnsAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

/// @brief Performs a synchronous fetch and add/sub operation.
/// @param [in] value The pointer to the value to modify.
/// @param [in] modBy The number to be added to the original value.
///                   If positive, this behaves like fetch-and-add, if negative this behaves like fetch-and-sub.
/// @return The original value, before it was modified.
static inline long unsigned int dnsAtomicFetchMod ( long unsigned int * value, int modBy )
{
    long unsigned int ret;

    pthread_mutex_lock ( &sDnsAtomicMutex );

    ret = *value;

    *value += modBy;

    pthread_mutex_unlock ( &sDnsAtomicMutex );

    return ret;
}

#define DNS_ATOMIC_FETCH_ADD( i )    ( dnsAtomicFetchMod ( ( i ), 1 ) )
#define DNS_ATOMIC_FETCH_SUB( i )    ( dnsAtomicFetchMod ( ( i ), -1 ) )
#endif

#include "DnsCore.h"
#include "DnsWrapper.h"

// We redefine sockets API before including dns.c.
// This way we take over socket API calls that dns.c uses.

// We want to handle these calls differently, depending on per-thread settings.
// They will result in either native calls, or vsocket API calls.

#define close( a )                     dnsw_close ( a )
#define send( a, b, c, d )             dnsw_send ( a, b, c, d )
#define recv( a, b, c, d )             dnsw_recv ( a, b, c, d )

#define bind( a, b, c )                dnsw_bind ( a, b, c )
#define connect( a, b, c )             dnsw_connect ( a, b, c )
#define getpeername( a, b, c )         dnsw_getpeername ( a, b, c )
#define setsockopt( a, b, c, d, e )    dnsw_setsockopt ( a, b, c, d, e )

// For this one we want even more special handling.
// Depending on thread settings, we either use vsocket2 which takes additional arguments
// and configure vsocket tag on the socket, or we want to use native sockets and (optionally)
// set Android network ID on them.

#define socket( a, b, c )    dnsw_socket ( a, b, c )

// We don't redefine all socket API calls we could, since dns.c does not use them.
// However, let's define them to something that will not build if dns.c
// gets updated and the new version does use them.
// This way we will not miss that (there will be a build error).

#define closesocket( a )                missing_closesocket foo ( a )
#define recvfrom( a, b, c, d, e, f )    missing_recvfrom foo ( a, b, c, d, e, f )
#define sendto( a, b, c, d, e, f )      missing_sendto foo ( a, b, c, d, e, f )
#define getsockname( a, b, c )          missing_getsockname foo ( a, b, c )
#define getsockopt( a, b, c, d, e )     missing_getsockopt foo ( a, b, c, d, e )

#if defined __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexpansion-to-defined"
#endif

#include "dnsc/src/dns.c"

#if defined __clang__
#pragma GCC diagnostic pop
#endif

int dnsResolverUsedCache ( struct dns_resolver * r )
{
    if ( !r || !r->resconf )
        return 0;

    // Resolver has a stack of operations that it performs.
    // 'which' field of the stack describes the index in the dns_resolv_conf's lookup array
    // which describes different methods of obtaining DNS answers.
    // When resolver performs a specific operation, it increments its 'which' index.
    // So the current 'which' index describes the operation that will be performed next.
    // The last operation has index 'which - 1'.
    unsigned which = r->stack[ r->sp ].which;

    // This resolver hasn't tried anything yet.
    if ( which < 1 )
        return 0;

    // If the code of the last operation (which - 1) in the lookup array is 'C' (or 'C'),
    // it means that resolver tried to use cache to obtain DNS answers.
    return ( r->resconf->lookup[ which - 1 ] == 'c' || r->resconf->lookup[ which - 1 ] == 'C' );
}

struct dns_packet * dnsGetAnswer ( struct dns_addrinfo * ai )
{
    return ( ai != 0 ) ? ( ai->answer ) : 0;
}

int dnsEnableCache ( struct dns_resolv_conf * rConf )
{
    if ( !rConf || rConf->lookup[ 0 ] == 'c' || rConf->lookup[ 0 ] == 'C' )
    {
        return 0;
    }
    // We have a dns_resolv_conf object that is not configured to use the cache first.
    // The 'lookup' array in dns_resolv_conf contains characters that describe various ways
    // of getting the DNS information. By default it doesn't use the cache object.
    // To make it use the cache, we need to add 'c' (or 'C') to that array.
    // Different methods are tried in order, and we want it to try cache first.
    // So we need to insert 'C' before the first entry, shifting everything after that.

    // First, let's move everything one slot up.
    for ( unsigned i = lengthof ( rConf->lookup ) - 1; i > 0; --i )
    {
        rConf->lookup[ i ] = rConf->lookup[ i - 1 ];
    }

    // Now let's put cache ('c') as the first method to be tried:
    rConf->lookup[ 0 ] = 'c';

    return 1;
}

void dnsPacketFree ( struct dns_packet * p )
{
    dns_p_free ( p );
}
