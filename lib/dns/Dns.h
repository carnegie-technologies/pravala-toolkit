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

#include <sys/socket.h>
#include <netinet/in.h>

/// @brief Different types of DNS records.
enum DnsRecordType
{
    DnsRTypeInvalid = 0, ///< Invalid type.
    DnsRTypeA       = 1, ///< A type.
    DnsRTypeAAAA    = 2, ///< AAAA type.
    DnsRTypeSRV     = 3  ///< SRV type.
};

/// @brief Contains a single DNS result record.
struct DnsRecord
{
    uint32_t recordType; ///< The type of this record (and data type).
    uint32_t ttl;        ///< TTL of the result, in seconds.

    union
    {
        struct
        {
            uint16_t priority;   ///< Priority of the record.
            uint16_t weight;     ///< Weight of the record.
            uint16_t port;       ///< Service port.
            const char * target; ///< Target name.
        } srv; ///< Data used by SRV records.

        struct
        {
            struct in_addr addr; ///< IPv4 address of the host.
        } a; ///< Data used by A records.

        struct
        {
            struct in6_addr addr; ///< IPv6 address of the host.
        } aaaa; ///< Data used by AAAA records.
    } data; ///< This record's data.
};

/// @brief A union that makes it easier to pass different socket address types.
union DnsSockAddr
{
    struct sockaddr s;      ///< The address as a generic socket address.
    struct sockaddr_in v4;  ///< The address as a IPv4 socket address.
    struct sockaddr_in6 v6; ///< The address as a IPv6 socket address.
};

/// @brief A union used to pass and receive additional user data in callbacks.
/// Instead of simply passing 'void*' and casting back and forth,
/// we use the union to simplify using different data types.
union DnsApiUserData
{
    void * vPtr;       ///< User data as void pointer.
    const char * cPtr; ///< User data as a const char void pointer.
    int iVal;          ///< User data as an int value.
    long lVal;         ///< User data as a long value.
};

/// @brief Pointer to a function creating sockets.
/// It can be passed to dns_resolve_ext function, to customize the process of creating network sockets.
/// This way, for instance, new sockets can be bound to specific interfaces or networks.
/// @param [in] family Family of the socket to create.
/// @param [in] type Type of the socket to create.
/// @param [in] protocol Protocol to be used by the socket.
/// @param [in] userData Pointer to user data object configured in DnsServerConfig associated with the socket.
///                      It will point to the copy of the data passed in the config.
/// @return A file descriptor of created socket on success, or -1 on error.
typedef int (* DnsSocketFuncType) ( int family, int type, int protocol, union DnsApiUserData * userData );

/// @def DNS_SERVER_FLAG_USE_TCP If set, specified server will be connected to using TCP protocol (instead of UDP).
#define DNS_SERVER_FLAG_USE_TCP    1

/// @def DNS_SERVER_FLAG_DONT_USE_TCP If set, specified server will NOT be connected to using TCP protocol.
/// By default we use UDP protocol, and if that results in truncated DNS answer, we retry the same query using TCP.
/// Passing that flag disables that behaviour.
/// This flag has no effect if DNS_SERVER_FLAG_USE_TCP is also passed.
#define DNS_SERVER_FLAG_DONT_USE_TCP    2

/// @brief A structure describing a configuration of a single DNS server.
struct DnsServerConfig
{
    /// @brief The address of the DNS server.
    union DnsSockAddr address;

    /// @brief A bitsum of DNS_SERVER_FLAG_* values.
    unsigned flags;

    /// @brief User data to pass to DNS 'socketFunc' callback from dns_resolve_ext().
    /// If socketFunc is not set, this field will be ignored.
    /// @note socketFunc will receive a pointer to a COPY of this field.
    union DnsApiUserData userData;
};

/// @brief Performs a DNS query.
/// @param [in] name The name to query.
/// @param [in] qType The type of the query (A/AAAA/SRV).
/// @param [in] serverConfigs An array with DNS server configurations.
///                           It can point to data on stack as it is not freed by the DNS system.
///                           All servers will be queried in parallel, and whichever answer arrives first
///                           (even if that answer is empty) will be returned by this function.
/// @param [in] numServers The number of entries in serverConfigs array.
/// @param [in] socketFunc Function to be called by the DNS system when creating new sockets.
///                        If this function is set, when DNS tries to create new sockets for sending DNS requests,
///                        it will be called instead of native socket().
///                        A pointer to userData stored in serverConfigs will be passed in that call.
///                        This function pointer MUST remain valid until this function returns.
/// @param [in] timeout Timeout for the operation (in seconds); 0 means infinite timeout.
/// @param [out] results A pointer to the memory where results should be stored. Cannot be NULL.
///                       Records generated will use a continuous memory segment, that should be deallocated
///                       by the caller using free(). If it includes strings (in SRV records), those strings
///                       are stored in the same memory segment and don't need to be individually deallocated.
/// @return The number of results generated (could be 0 if the query didn't generate any results); -1 on errors.
PRAV_PUBLIC_API int dns_resolve_ext (
        const char * name, enum DnsRecordType qType,
        const struct DnsServerConfig * serverConfigs,
        size_t numServers,
        DnsSocketFuncType socketFunc,
        unsigned int timeout,
        struct DnsRecord ** results );

/// @brief Performs a DNS query.
/// This is a convenience wrapper around dns_resolve_ext.
/// @param [in] name The name to query.
/// @param [in] qType The type of the query (A/AAAA/SRV).
/// @param [in] dnsServer A pointer to DNS server's address.
///                       It can point to data on stack as it is not freed by the DNS system.
/// @param [in] timeout Timeout for the operation (in seconds); 0 means infinite timeout.
/// @param [out] results A pointer to the memory where results should be stored. Cannot be NULL.
///                       Records generated will use a continuous memory segment, that should be deallocated
///                       by the caller using free(). If it includes strings (in SRV records), those strings
///                       are stored in the same memory segment and don't need to be individually deallocated.
/// @return The number of results generated (could be 0 if the query didn't generate any results); -1 on errors.
PRAV_PUBLIC_API int dns_resolve (
        const char * name, enum DnsRecordType qType,
        const union DnsSockAddr * dnsServer,
        unsigned int timeout,
        struct DnsRecord ** results );

/// @brief Contains configuration for the DNS resolver.
struct DnsConfig
{
    /// @brief The array with addresses of DNS servers to use.
    /// Cannot be NULL.
    /// This is not freed by the DNS system, so it can point to memory on the stack or anything externally managed.
    const struct sockaddr_in6 * dnsServers;

    size_t numDnsServers; ///< The number of entries in dnsServers array. At least one entry is required.

    /// @brief If non-null and non-empty, DNS sockets will be bound to the physical interface with this name.
    /// This is not freed by the DNS system, so it can point to memory on the stack or anything externally managed.
    const char * bindToIface;
};

/// @brief Performs a native DNS query.
/// @param [in] qType The type of the query (A/AAAA/SRV).
/// @param [in] name The name to query.
/// @param [in] config The configuration for the DNS resolver. Cannot be NULL.
///                    It can point to data on stack as it is not freed by the DNS system.
/// @param [in] timeout Timeout for the operation (in seconds); 0 means infinite timeout.
/// @param [out] results A pointer to the memory where results should be stored. Cannot be NULL.
///                       Records generated will use a continuous memory segment, that should be deallocated
///                       by the caller using free(). If it includes strings (in SRV records), those strings
///                       are stored in the same memory segment and don't need to be individually deallocated.
/// @return The number of results generated (could be 0 if the query didn't generate any results); -1 on errors.
PRAV_PUBLIC_API int ndns_resolve (
        enum DnsRecordType qType, const char * name,
        const struct DnsConfig * config,
        unsigned int timeout,
        struct DnsRecord ** results );

#ifdef __cplusplus
}
#endif
