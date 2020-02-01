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

// Very verbose logging...
// #define SIMPLE_LOG_ENABLE_DEBUG2

// This can be defined before including this file:
#ifndef SIMPLE_LOG_TAG
#define SIMPLE_LOG_TAG    "DEBUG"
#endif

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#include <sys/socket.h>

#if defined( NDEBUG ) || defined( NO_LOGGING )

// No logging at all!

#define SIMPLE_LOG_ERR( FORMAT_STRING, ... )
#define SIMPLE_LOG_ERR_NP( STRING )
#define SIMPLE_LOG_DEBUG( FORMAT_STRING, ... )
#define SIMPLE_LOG_DEBUG_NP( STRING )

#elif defined( PLATFORM_ANDROID )

#include <android/log.h>

// __android_log_print already appends a newline

#define SIMPLE_LOG_ERR( FORMAT_STRING, ... ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            __android_log_print ( ANDROID_LOG_ERROR, SIMPLE_LOG_TAG, "%s:%d " FORMAT_STRING, \
                                  __FUNCTION__, __LINE__, ## __VA_ARGS__ ); \
        } \
    } while ( 0 )

#define SIMPLE_LOG_ERR_NP( FORMAT_STRING )      SIMPLE_LOG_ERR ( FORMAT_STRING )

#define SIMPLE_LOG_DEBUG( FORMAT_STRING, ... ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            __android_log_print ( ANDROID_LOG_DEBUG, SIMPLE_LOG_TAG, "%s:%d " FORMAT_STRING, \
                                  __FUNCTION__, __LINE__, ## __VA_ARGS__ ); \
        } \
    } while ( 0 )

#define SIMPLE_LOG_DEBUG_NP( FORMAT_STRING )    SIMPLE_LOG_DEBUG ( FORMAT_STRING )

#elif defined( PLATFORM_IOS )

#include "os/Apple/PravalaNSLog.h"

// Pravala_NSLog already appends a newline

#define SIMPLE_LOG_ERR( FORMAT_STRING, ... ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            Pravala_NSLog ( "%s|%s:%d " FORMAT_STRING, SIMPLE_LOG_TAG, __FUNCTION__, __LINE__, ## __VA_ARGS__ ); \
        } \
    } while ( 0 )

#define SIMPLE_LOG_ERR_NP( FORMAT_STRING )      SIMPLE_LOG_ERR ( FORMAT_STRING )

#define SIMPLE_LOG_DEBUG( FORMAT_STRING, ... ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            Pravala_NSLog ( "%s|%s:%d " FORMAT_STRING, SIMPLE_LOG_TAG, __FUNCTION__, __LINE__, ## __VA_ARGS__ ); \
        } \
    } while ( 0 )

#define SIMPLE_LOG_DEBUG_NP( FORMAT_STRING )    SIMPLE_LOG_DEBUG ( FORMAT_STRING )

#else

#include <stdio.h>

#define SIMPLE_LOG_ERR( FORMAT_STRING, ... ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            fprintf ( stderr, SIMPLE_LOG_TAG ": %s:%d " FORMAT_STRING "\n", \
                      __FUNCTION__, __LINE__, ## __VA_ARGS__ ); \
        } \
    } while ( 0 )

// C++11 requires that you call variadic macros with at least one variadic argument.
// In order log a string with no parameters, use the _NP versions which only accept a string
#define SIMPLE_LOG_ERR_NP( STRING ) \
    do { \
        if ( simpleLogsEnabled() != 0 ) { \
            fprintf ( stderr, SIMPLE_LOG_TAG ": %s:%d " STRING "\n", \
                      __FUNCTION__, __LINE__ ); \
        } \
    } while ( 0 )

#define SIMPLE_LOG_DEBUG        SIMPLE_LOG_ERR
#define SIMPLE_LOG_DEBUG_NP     SIMPLE_LOG_ERR_NP
#endif

#ifdef SIMPLE_LOG_ENABLE_DEBUG2
#define SIMPLE_LOG_DEBUG2       SIMPLE_LOG_DEBUG
#define SIMPLE_LOG_DEBUG2_NP    SIMPLE_LOG_DEBUG_NP
#else
#define SIMPLE_LOG_DEBUG2( FORMAT_STRING, ... )
#define SIMPLE_LOG_DEBUG2_NP
#endif

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

#ifndef PRAV_PUBLIC_API

#if ( defined( __unix__ ) || defined( __APPLE__ ) ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
#define PRAV_PUBLIC_API    __attribute__( ( __visibility__ ( "default" ) ) )
#elif defined( WIN32 ) && defined( BUILDING_DLL )
#define PRAV_PUBLIC_API    __declspec ( dllexport )
#elif defined( WIN32 )
#define PRAV_PUBLIC_API    __declspec ( dllimport )
#else
#define PRAV_PUBLIC_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// @brief A helper structure to easily allocate memory for address description.
struct SimpleLogAddrDescBuf
{
    /// @brief Buffer to store string representation of an IP address.
    /// The size is sufficient for IPv6 address plus :port plus NULL.
    /// POSIX defines INET6_ADDRSTRLEN to be 46.
    /// We need additional byte for ':', 5 bytes for port number, and one byte for \0.
    /// This brings us up to 53. To make it round (4B aligned) let's use 56.
    char data[ 56 ];
};

/// @brief A helper function that generates a description of the given address.
/// @param [in] addr The address whose description to generate.
/// @param [in] addrLen The length of the address.
/// @param [in,out] buf The buffer to store the description in.
/// @return The description of the address.
PRAV_PUBLIC_API const char * simpleLogAddrDesc (
        const struct sockaddr * addr, socklen_t addrLen, struct SimpleLogAddrDescBuf * buf );

/// @brief Checks if SimpleLogs are enabled.
/// @return 1 if SimpleLogs are enabled; 0 otherwise.
PRAV_PUBLIC_API int simpleLogsEnabled ( void );

/// @brief Enables/disables SimpleLogs.
/// @param [in] enabled 1 to enable SimpleLogs, 0 to disable.
PRAV_PUBLIC_API void simpleLogsSetEnabled ( int enabled );

#ifdef __cplusplus
}
#endif
