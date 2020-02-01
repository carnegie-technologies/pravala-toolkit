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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "DnsCore.h"
#include "DnsWrapper.h"

// This should be before including SimpleLog.h
#define SIMPLE_LOG_TAG    "DNS/Core"

#include "simplelog/SimpleLog.h"

/// @brief A mutex used for controlling access to global state in DnsCore.
static pthread_mutex_t sDnsCoreMutex = PTHREAD_MUTEX_INITIALIZER;

/// @brief Contains thread-specific DNS state.
struct DnsThreadState
{
    pthread_t threadId; ///< The ID of the thread whose state this is.

    struct DnsThreadConfig config; ///< The configuration to be used by current thread.

    struct DnsThreadState * next; ///< The next state in the list.
};

/// @brief The head of the list with all DnsThreadState objects.
/// @note We do this instead of using thread-local global variables,
///       because clang's support for them on Android is wonky.
static struct DnsThreadState * tStateHead = 0;

void dnsThreadClear()
{
    const pthread_t tId = pthread_self();

    struct DnsThreadState * ptr;
    struct DnsThreadState * prev = 0;

    pthread_mutex_lock ( &sDnsCoreMutex );

    for ( ptr = tStateHead; ptr != 0; prev = ptr, ptr = ptr->next )
    {
        if ( tId == ptr->threadId )
        {
            if ( prev != 0 )
            {
                prev->next = ptr->next;
            }
            else
            {
                assert ( ptr == tStateHead );

                tStateHead = ptr->next;
            }

            free ( ptr );
            break;
        }
    }

    pthread_mutex_unlock ( &sDnsCoreMutex );
}

void dnsThreadSetup ( const struct DnsThreadConfig * threadConfig )
{
    const pthread_t tId = pthread_self();

    struct DnsThreadState * state = 0;
    struct DnsThreadState * ptr;

    pthread_mutex_lock ( &sDnsCoreMutex );

    for ( ptr = tStateHead; ptr != 0; ptr = ptr->next )
    {
        if ( tId == ptr->threadId )
        {
            state = ptr;
            break;
        }
    }

    if ( !state )
    {
        if ( !( state = ( struct DnsThreadState * ) calloc ( 1, sizeof ( *state ) ) ) )
        {
            pthread_mutex_unlock ( &sDnsCoreMutex );
            return;
        }

        state->threadId = tId;
        state->next = tStateHead;

        tStateHead = state;
    }

    if ( !threadConfig )
    {
        memset ( &( state->config ), 0, sizeof ( state->config ) );
    }
    else
    {
        // We copy the config object, instead of storing just the pointer.
        // Just in case something is asynchronous and gets deleted elsewhere.
        // However, this does NOT mean that the pointers in the config are copied.
        // They must remain valid until this configuration is cleared/replaced.
        // There is an appropriate warning in the comment of this function.

        state->config = *threadConfig;
    }

    pthread_mutex_unlock ( &sDnsCoreMutex );
}

/// @brief A helper function that returns the configuration of this thread.
/// @note This locks and unlocks the global mutex.
///       Returned pointer points to an entry in the global list,
///       but it should be safe since no other thread should be modifying this entry.
/// @return This thread's DnsThreadConfig object, or 0 if it's not set.
static struct DnsThreadConfig * getThreadConfig()
{
    const pthread_t tId = pthread_self();

    struct DnsThreadConfig * ret = 0;
    struct DnsThreadState * ptr;

    pthread_mutex_lock ( &sDnsCoreMutex );

    for ( ptr = tStateHead; ptr != 0; ptr = ptr->next )
    {
        if ( tId == ptr->threadId )
        {
            ret = &ptr->config;
            break;
        }
    }

    pthread_mutex_unlock ( &sDnsCoreMutex );
    return ret;
}

/// @def DO_API_CALL Calls (and returns the result of) specified function, using custom API if it's configured.
#define DO_API_CALL( FUNC_NAME, ... ) \
    do { \
        struct DnsThreadConfig * tConfig = getThreadConfig(); \
        if ( tConfig != 0 && tConfig->sockApiExtCalls != 0 && tConfig->sockApiExtCalls->f_ ## FUNC_NAME != 0 ) { \
            SIMPLE_LOG_DEBUG2 ( "Using custom " #FUNC_NAME " (extended)" ); \
            return tConfig->sockApiExtCalls->f_ ## FUNC_NAME ( __VA_ARGS__, tConfig->sockApiUserData ); \
        } \
        else if ( tConfig != 0 && tConfig->sockApiCalls != 0 && tConfig->sockApiCalls->f_ ## FUNC_NAME != 0 ) { \
            SIMPLE_LOG_DEBUG2 ( "Using custom " #FUNC_NAME " (basic)" ); \
            return tConfig->sockApiCalls->f_ ## FUNC_NAME ( __VA_ARGS__ ); \
        } \
        SIMPLE_LOG_DEBUG2 ( "Using native " #FUNC_NAME ); \
        return FUNC_NAME ( __VA_ARGS__ ); \
    } while ( 0 )

int dnsw_socket ( int family, int type, int protocol )
{
    DO_API_CALL ( socket, family, type, protocol );
}

int dnsw_bind ( int sockFd, const struct sockaddr * addr, socklen_t addrLen )
{
    DO_API_CALL ( bind, sockFd, addr, addrLen );
}

int dnsw_close ( int sockFd )
{
    DO_API_CALL ( close, sockFd );
}

ssize_t dnsw_send ( int sockFd, const void * buf, size_t len, int flags )
{
    DO_API_CALL ( send, sockFd, buf, len, flags );
}

ssize_t dnsw_recv ( int sockFd, void * buf, size_t len, int flags )
{
    DO_API_CALL ( recv, sockFd, buf, len, flags );
}

int dnsw_getpeername ( int sockFd, struct sockaddr * addr, socklen_t * addrLen )
{
    DO_API_CALL ( getpeername, sockFd, addr, addrLen );
}

int dnsw_setsockopt ( int sockFd, int level, int optName, const void * optVal, socklen_t optLen )
{
    DO_API_CALL ( setsockopt, sockFd, level, optName, optVal, optLen );
}

int dnsw_connect ( int sockFd, const struct sockaddr * addr, socklen_t addrLen )
{
    // dns.c does not fully initialize addresses, it only sets fields it cares about.
    // The rest is not initialized, causing valgrind warnings.
    // So we "sanitize" those addresses, to be initialized to 0 first.

    if ( addrLen >= sizeof ( struct sockaddr_in ) && addr != 0 && addr->sa_family == AF_INET )
    {
        struct sockaddr_in tmpAddr = { 0 };

        tmpAddr.sin_family = AF_INET;
        tmpAddr.sin_addr = ( ( const struct sockaddr_in * ) addr )->sin_addr;
        tmpAddr.sin_port = ( ( const struct sockaddr_in * ) addr )->sin_port;

#ifdef SYSTEM_UNIX
        tmpAddr.sin_len = sizeof ( tmpAddr );
#endif

        DO_API_CALL ( connect, sockFd, ( const struct sockaddr * ) &tmpAddr, sizeof ( tmpAddr ) );
    }
    else if ( addrLen >= sizeof ( struct sockaddr_in6 ) && addr != 0 && addr->sa_family == AF_INET6 )
    {
        struct sockaddr_in6 tmpAddr = { 0 };

        tmpAddr.sin6_family = AF_INET6;
        tmpAddr.sin6_addr = ( ( const struct sockaddr_in6 * ) addr )->sin6_addr;
        tmpAddr.sin6_port = ( ( const struct sockaddr_in6 * ) addr )->sin6_port;

#ifdef SYSTEM_UNIX
        tmpAddr.sin6_len = sizeof ( tmpAddr );
#endif

        DO_API_CALL ( connect, sockFd, ( const struct sockaddr * ) &tmpAddr, sizeof ( tmpAddr ) );
    }
    else
    {
        DO_API_CALL ( connect, sockFd, addr, addrLen );
    }
}
