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

struct dns_resolver;
struct dns_addrinfo;
struct dns_resolv_conf;

#include "Dns.h"

typedef int (* FType_socket) ( int family, int type, int protocol );
typedef int (* FType_close) ( int fd );
typedef ssize_t (* FType_send) ( int sockfd, const void * buf, size_t len, int flags );
typedef ssize_t (* FType_recv) ( int sockfd, void * buf, size_t len, int flags );
typedef int (* FType_bind) ( int fd, const struct sockaddr * addr, socklen_t addrLen );
typedef int (* FType_connect) ( int fd, const struct sockaddr * addr, socklen_t addrLen );
typedef int (* FType_getpeername) ( int fd, struct sockaddr * addr, socklen_t * addrLen );
typedef int (* FType_setsockopt) ( int fd, int level, int optName, const void * optVal, socklen_t optLen );

/// @brief A struct with pointers to custom socket API calls.
/// Not all of them need to be set. If a call type is not configured, a native version will be used.
/// Those calls look like regular socket API calls.
struct DnsSocketApiCalls
{
    FType_socket f_socket;           ///< Used instead of native socket() function.
    FType_close f_close;             ///< Used instead of native close() function.
    FType_send f_send;               ///< Used instead of native send() function.
    FType_recv f_recv;               ///< Used instead of native recv() function.
    FType_bind f_bind;               ///< Used instead of native bind() function.
    FType_connect f_connect;         ///< Used instead of native connect() function.
    FType_getpeername f_getpeername; ///< Used instead of native getpeername() function.
    FType_setsockopt f_setsockopt;   ///< Used instead of native setsockopt() function.
};

typedef int (* FType_socket_ext) (
        int family, int type, int protocol, union DnsApiUserData * userData );
typedef int (* FType_close_ext) (
        int fd, union DnsApiUserData * userData );
typedef ssize_t (* FType_send_ext) (
        int sockfd, const void * buf, size_t len, int flags, union DnsApiUserData * userData );
typedef ssize_t (* FType_recv_ext) (
        int sockfd, void * buf, size_t len, int flags, union DnsApiUserData * userData );
typedef int (* FType_bind_ext) (
        int fd, const struct sockaddr * addr, socklen_t addrLen, union DnsApiUserData * userData );
typedef int (* FType_connect_ext) (
        int fd, const struct sockaddr * addr, socklen_t addrLen, union DnsApiUserData * userData );
typedef int (* FType_getpeername_ext) (
        int fd, struct sockaddr * addr, socklen_t * addrLen, union DnsApiUserData * userData );
typedef int (* FType_setsockopt_ext) (
        int fd, int level, int optName, const void * optVal, socklen_t optLen, union DnsApiUserData * userData );

/// @brief A struct with pointers to custom socket API calls in extended versions.
/// Not all of them need to be set. If a call type is not configured, a native version will be used.
/// It's similar to DnsSocketApiCalls, but each call takes additional argument - user data parameter.
/// @note If a specific call is configured in both extended and basic version, the extended version is used.
struct DnsSocketApiExtCalls
{
    FType_socket_ext f_socket;           ///< Used instead of native socket() function.
    FType_close_ext f_close;             ///< Used instead of native close() function.
    FType_send_ext f_send;               ///< Used instead of native send() function.
    FType_recv_ext f_recv;               ///< Used instead of native recv() function.
    FType_bind_ext f_bind;               ///< Used instead of native bind() function.
    FType_connect_ext f_connect;         ///< Used instead of native connect() function.
    FType_getpeername_ext f_getpeername; ///< Used instead of native getpeername() function.
    FType_setsockopt_ext f_setsockopt;   ///< Used instead of native setsockopt() function.
};

/// @brief Per-thread configuration for DNS.
struct DnsThreadConfig
{
    /// @brief The socket API calls to use instead of native ones.
    const struct DnsSocketApiCalls * sockApiCalls;

    /// @brief The extended socket API calls to use instead of native ones.
    /// These calls, in addition to regular arguments, will also receive sockApiUserData pointer configured.
    /// If a specific extended call is configured it takes precedence over the value in sockApiCalls.
    /// @note Both API sets can be used at the same time, some calls configured in this field,
    ///       while the others configured in the basic set - if they don't require user pointer.
    const struct DnsSocketApiExtCalls * sockApiExtCalls;

    /// @brief User data to pass to extended socket API calls.
    union DnsApiUserData * sockApiUserData;
};

/// @brief Configures current thread to use specific sockets API.
/// It will overwrite existing configuration (if it already exists).
/// @warning This functions creates a shallow copy of DnsThreadConfig structure.
///          Only the copies of internal pointers will be created, and not copies of the values pointed to.
///          This means that those pointers MUST remain valid until dnsThreadClear() is called,
///          or until dnsThreadSetup() is called again.
/// @param [in] threadConfig Configuration for current thread.
///                          Can be 0 (which sets 'empty' configuration, but does not remove this thread's entry).
PRAV_PUBLIC_API void dnsThreadSetup ( const struct DnsThreadConfig * threadConfig );

/// @brief Clears this thread's settings.
/// It will cause this thread to use custom socket API.
PRAV_PUBLIC_API void dnsThreadClear ( void );

/// @brief Checks if the resolver just used a cache to get DNS answer.
/// @param [in] r The resolver to inspect.
/// @return 1 if this resolver just used cache to obtain the DNS answer; 0 otherwise.
PRAV_PUBLIC_API int dnsResolverUsedCache ( struct dns_resolver * r );

/// @brief Returns an answer field from dns_addrinfo structure.
/// @param [in] ai The addrinfo whose answer should be returned.
/// @return An answer stored in dns_addrinfo, or 0 if there is none or dns_addrinfo is invalid.
PRAV_PUBLIC_API struct dns_packet * dnsGetAnswer ( struct dns_addrinfo * ai );

/// @brief Configures given dns_resolv_conf object to use cache.
/// @param [in] rConf dns_resolv_conf to configure. Nothing happens if it's null.
/// @return 1 if the cache was just enabled; 0 otherwise (if null was passed, or if the cache was already enabled)
PRAV_PUBLIC_API int dnsEnableCache ( struct dns_resolv_conf * rConf );

/// @brief A helper function that generates dns_hints using given DNS servers and resolv-conf.
/// @param [in] dnsServers The array with addresses of DNS servers to put in dns_hints. Cannot be NULL.
/// @param [in] numServers The number of servers. Should be at least 1.
/// @param [in] resConf The resolv-conf object to use for dns_hints. Cannot be NULL.
/// @return The new dns_hints object to use, or NULL on error.
///         If a valid pointer is returned, it should be (eventually) released by the caller using dns_hints_close().
PRAV_PUBLIC_API struct dns_hints * dnsGenHints (
        const struct sockaddr_in6 * dnsServers, size_t numServers, struct dns_resolv_conf * resConf );

/// @brief A wrapper around native 'socket' used by ndns_resolve when socket bound to specific interface is required.
/// @param [in] family Family of the socket to create.
/// @param [in] type Type of the socket to create.
/// @param [in] protocol Protocol to be used by the socket.
/// @param [in] userData Pointer to user data configured for that socket.
/// @return Result of socket().
PRAV_PUBLIC_API int dnsSocketBoundToIface ( int family, int type, int protocol, union DnsApiUserData * userData );

#ifdef __cplusplus
}
#endif
