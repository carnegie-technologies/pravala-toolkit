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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <net/if.h>
#include <unistd.h>

#include "DnsWrapper.h"
#include "DnsCore.h"

// This should be before including SimpleLog.h
#define SIMPLE_LOG_TAG    "DNS/Native"

#include "simplelog/SimpleLog.h"

// The maximum size of the packet that will be sent to the DNS server.
#define MAX_PAYLOAD_SIZE    512

/// @brief When set in QueryData::flags, a query that results in truncated response will be re-tried using TCP.
static const unsigned int QueryDataFlagTryTcp = 1;

/// @brief All the data associated with a query.
struct QueryData
{
    /// @brief The address the query will be sent to.
    /// Used for logging. Could be valid even when the query is not active.
    /// This should not be freed when clearing QueryData.
    const union DnsSockAddr * addr;

    /// @brief The socket to send the query over.
    /// It is set to null when the query is not active (yet/anymore).
    struct dns_socket * socket;

    /// @brief The query to send.
    /// It is set to null when the query is not active (yet/anymore).
    struct dns_packet * query;

    /// @brief A pointer to the original query.
    /// It is used if another query needs to be generated to replace existing one.
    /// In that case we need a copy of the original query, we cannot reuse 'query' above.
    const struct dns_packet * orgQuery;

    /// @brief The user data to set before creating sockets.
    /// It is used if another query needs to be generated to replace existing one.
    /// If that happens, a new socket is generated and the user data needs to be configured again.
    union DnsApiUserData sockUserData;

    /// @brief Additional flags.
    /// A bitmask of QueryDataFlag* values.
    unsigned int flags;
};

/// @brief A helper wrapper around simpleLogAddrDesc() that just takes our DnsSockAddr union.
/// @param [in] addr The address whose description to generate.
/// @param [in,out] buf The buffer to store the description in.
/// @return The description of the address.
static const char * simpleLogDnsAddrDesc ( const union DnsSockAddr * addr, struct SimpleLogAddrDescBuf * buf )
{
    return ( addr != 0 )
           ? simpleLogAddrDesc ( &addr->s, sizeof ( union DnsSockAddr ), buf )
           : simpleLogAddrDesc ( 0, 0, buf );
}

/// @brief Cleanup the query.
/// It removes and unsets DNS resources associated with the query.
/// It does NOT unset the address pointer.
/// @param [in] query The query to cleanup.
static void cleanupQuery ( struct QueryData * query )
{
    if ( !query )
        return;

    if ( query->socket != 0 )
    {
        dns_so_close ( query->socket );
        query->socket = 0;
    }

    if ( query->query != 0 )
    {
        dnsPacketFree ( query->query );
        query->query = 0;
    }
}

/// @brief Configures the query.
/// Then it will create a copy of the query's orgQuery, and create a socket.
/// @param [in] name The name being queried (for logging).
/// @param [in] dnsType The type of the query (for logging).
/// @param [in] query The query to setup. It will be cleaned first, using cleanupQuery().
/// @param [in] sockType The type of the socket to generate.
/// @param [in] userData The pointer to user data object to configure before creating the socket.
/// @return 1 on success; 0 on failure.
static int setupQuery (
        const char * name, enum dns_type dnsType,
        struct QueryData * query, int sockType,
        union DnsApiUserData * userData )
{
    if ( !query )
        return 0;

    cleanupQuery ( query );

    // This is the local socket address to bind the socket to.
    // The family must match the family of the remote address of the DNS server.
    // The IP address and port are set to 0 to specify they can be bound to any address.
    union DnsSockAddr localAddr;

    memset ( &localAddr, 0, sizeof ( localAddr ) );
    localAddr.s.sa_family = query->addr->s.sa_family;

    // dns_so_open calls bind().
    // If socketFunc is used, we want it to get userData associated with this server!
    *userData = query->sockUserData;

    int error = 0;

    query->socket = dns_so_open ( &localAddr.s, sockType, dns_opts(), &error );

    struct SimpleLogAddrDescBuf addrDesc;

    if ( !query->socket || error != 0 )
    {
        SIMPLE_LOG_ERR ( "Error opening a DNS socket; Addr: %s; Family: %d; Type: %d: %s",
                         simpleLogDnsAddrDesc ( query->addr, &addrDesc ),
                         localAddr.s.sa_family, sockType, dns_strerror ( error ) );

        cleanupQuery ( query );
        return 0;
    }

    // However, each query will have separate state, so each one needs its own copy:
    error = 0;

    query->query = dns_p_copy ( dns_p_make ( query->orgQuery->end, &error ), query->orgQuery );

    if ( !query->query || error != 0 )
    {
        SIMPLE_LOG_ERR ( "Error copying a DNS query: Addr: %s; Name: %s; DNS Type: %d; Error: %s",
                         simpleLogDnsAddrDesc ( query->addr, &addrDesc ),
                         name, dnsType, dns_strerror ( error ) );

        cleanupQuery ( query );
        return 0;
    }

    return 1;
}

/// @brief Wait for any socket to be ready for reading.
/// @param [in] queries The queries containing the sockets to wait on.
/// @param [in] numQueries The size of 'queries' array.
/// @param [in] timeout The maximum amount of time to wait, in seconds.
/// @return The number of sockets ready for reading, or -1 on error.
static int waitForSocket ( struct QueryData * queries, size_t numQueries, int timeout )
{
    int maxFd = -1; // The highest valued FD in rset.
    size_t numSockets = 0;

    // Build the sets.
    // For UDP sockets we would only care about read events.
    // For TCP sockets, we need both read and write events.

    fd_set rset;
    fd_set wset;
    FD_ZERO ( &rset );
    FD_ZERO ( &wset );

    if ( !queries )
    {
        errno = EINVAL;
        return -1;
    }

    for ( size_t i = 0; i < numQueries; ++i )
    {
        if ( !queries[ i ].socket )
        {
            // This socket is not active.
            continue;
        }

        const int events = dns_so_events ( queries[ i ].socket );

        if ( ( events & ( DNS_POLLIN | DNS_POLLOUT ) ) == 0 )
        {
            // This socket is not waiting for any events. We should skip it.
            continue;
        }

        const int fd = dns_so_pollfd ( queries[ i ].socket );

        if ( fd < 0 || fd >= FD_SETSIZE )
        {
            SIMPLE_LOG_DEBUG ( "Socket FD %d is not valid or too large; Skipping", fd );
            continue;
        }

        if ( events & DNS_POLLIN )
        {
            FD_SET ( fd, &rset );
        }

        if ( events & DNS_POLLOUT )
        {
            FD_SET ( fd, &wset );
        }

        if ( maxFd < fd )
        {
            maxFd = fd;
        }

        ++numSockets;
    }

    if ( numSockets < 1 )
    {
        // No sockets that we should wait on!
        // This shouldn't really happen...

        SIMPLE_LOG_ERR ( "There are no sockets left to wait on" );

        errno = EINVAL;
        return -1;
    }

    assert ( maxFd >= 0 );

    // The length of time to wait for an event.
    struct timeval tv = { timeout, 0 };

    SIMPLE_LOG_DEBUG2 ( "Select on %u sockets", ( unsigned ) numSockets );

    errno = 0;
    const int ret = select ( maxFd + 1, &rset, &wset, 0, ( timeout >= 0 ) ? &tv : 0 );

    SIMPLE_LOG_DEBUG2 ( "Select returned %d: %s", ret, ( errno == 0 ) ? "No error" : strerror ( errno ) );

    return ret;
}

/// @brief Parses a DNS answer and generates results.
/// @note ALL parameters must be set and valid!
/// @param [in] queryType The type of the query. Only records matching this type will be used.
/// @param [in] dnsAnswer Received DNS packet with an answer. It is NOT deallocated inside.
/// @param [out] resultsPtr The location for the results. It will only be modified when this function returns
///                         a value greater than 0. The memory stored there will be allocated using malloc,
///                         and it's caller's responsibility to deallocate it.
/// @return The number of matching records in the answer; 0 if there were no matching records; <0 on error.
static int parseDnsAnswer ( enum dns_type queryType, struct dns_packet * dnsAnswer, struct DnsRecord ** resultsPtr )
{
    struct dns_rr rr;
    union dns_any dnsData;

    int rCount = 0;
    size_t strLen = 0;

    dns_rr_foreach ( &rr, dnsAnswer, .section = DNS_S_AN )
    {
        if ( rr.type == queryType )
        {
            ++rCount;

            if ( rr.type == DNS_T_SRV
                 && dns_any_parse ( dns_any_init ( &dnsData, sizeof ( dnsData ) ), &rr, dnsAnswer ) == 0 )
            {
                // +1 for \0:
                strLen += strlen ( dnsData.srv.target ) + 1;
            }
        }
    }

    if ( rCount < 1 )
    {
        return 0;
    }

    const size_t resultsSize = rCount * sizeof ( struct DnsRecord ) + strLen;
    struct DnsRecord * results = ( struct DnsRecord * ) malloc ( resultsSize );

    if ( !results )
    {
        errno = ENOMEM;
        return -1;
    }

    memset ( results, 0, resultsSize );

    int rIdx = 0;

    // We want to put all the strings right after the actual records.
    // We will also decrease strLen to represent the number of bytes left.

    char * strResults = ( char * ) &( results[ rCount ] );

    dns_rr_foreach ( &rr, dnsAnswer, .section = DNS_S_AN )
    {
        if ( rIdx < rCount
             && rr.type == queryType
             && dns_any_parse ( dns_any_init ( &dnsData, sizeof ( dnsData ) ), &rr, dnsAnswer ) == 0 )
        {
            if ( rr.type == DNS_T_A )
            {
                results[ rIdx ].ttl = rr.ttl;
                results[ rIdx ].recordType = DnsRTypeA;
                results[ rIdx ].data.a.addr = dnsData.a.addr;

                ++rIdx;
            }
            else if ( rr.type == DNS_T_AAAA )
            {
                results[ rIdx ].ttl = rr.ttl;
                results[ rIdx ].recordType = DnsRTypeAAAA;
                results[ rIdx ].data.aaaa.addr = dnsData.aaaa.addr;

                ++rIdx;
            }
            else if ( rr.type == DNS_T_SRV )
            {
                results[ rIdx ].ttl = rr.ttl;
                results[ rIdx ].recordType = DnsRTypeSRV;
                results[ rIdx ].data.srv.port = dnsData.srv.port;
                results[ rIdx ].data.srv.priority = dnsData.srv.priority;
                results[ rIdx ].data.srv.weight = dnsData.srv.weight;

                const size_t l = strlen ( dnsData.srv.target ) + 1;

                if ( l <= strLen )
                {
                    memcpy ( strResults, dnsData.srv.target, l - 1 );
                    strResults[ l - 1 ] = 0;

                    results[ rIdx ].data.srv.target = strResults;
                    strResults += l;
                }

                ++rIdx;
            }
        }
    }

    if ( rIdx > 0 )
    {
        *resultsPtr = results;
    }
    else if ( results != 0 )
    {
        free ( results );
    }

    results = 0;
    return rIdx;
}

/// @brief Runs DNS resolution.
/// @note It assumes ALL the arguments are valid!
/// @param [in] name The name being queried. For logging.
/// @param [in] queryType The type of the query (for filtering the results).
/// @param [in] sockUserData The pointer to userData to configure before creating new sockets.
/// @param [in] queries And array with all queries. Not all of the queries need to be valid.
/// @param [in] numQueries Size of 'queries' array.
/// @param [in] timeout The timeout for the operation (in seconds). Not enforced if < 1.
/// @param [out] resultsPtr The location for the results. It will only be modified when this function returns
///                         a value greater than 0. The memory stored there will be allocated using malloc,
///                         and it's caller's responsibility to deallocate it.
/// @return The number of matching records in the answer; 0 if there were no matching records; <0 on error.
static int resolveQueries (
        const char * name,
        enum dns_type queryType,
        union DnsApiUserData * sockUserData,
        struct QueryData * queries,
        size_t numQueries,
        unsigned int timeout,
        struct DnsRecord ** resultsPtr )
{
    int error = 0;
    struct SimpleLogAddrDescBuf addrDesc;

    // To simplify things, we use the wall clock.
    // However, that is potentially unsafe if it changes (it's possible).
    // To make things slightly safer, we store start time, and if we ever see we are before that, we bail.

    const time_t startTime = time ( 0 );
    time_t curTime = startTime;
    time_t endTime = startTime + timeout;

    // We mark this whenever we get an empty answer.
    // We have seen, that some (misconfigured?) DNS servers were giving us an empty answer while others
    // were returning valid results. So we treat an empty answer differently.
    // If all we see are empty answers we keep running for up to a second (resolution of our time source)
    // to check if anything else gives us a different result (only if there are other servers running).
    int receivedEmptyAnswer = 0;

    // We mark this when we want to run the loop again, regardless of the timeout.
    // We want at least one run, so we mark this at the beginning.
    int runAgain = 1;

    while ( 1 )
    {
        // This counts the number of queries still active.
        size_t activeQueries = 0;

        // We set this when we restart a query using TCP protocol.
        // It causes us to retry right away - without waiting for sockets.
        // This is needed, because that TCP query has not been started yet, so the socket is not waiting
        // for any events yet.
        int tcpRestarted = 0;

        for ( size_t i = 0; i < numQueries; ++i )
        {
            if ( !queries[ i ].socket || !queries[ i ].query )
            {
                // Not an active query anymore.
                continue;
            }

            SIMPLE_LOG_DEBUG2 ( "queryName=%s; queryType=%d; resultsPtr=%p; Querying DNS server %s",
                                name, queryType, resultsPtr,
                                simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ) );

            // Let's run the query.
            // dns_so_query will start a new query, try to write/read data from the socket,
            // and do whatever DNS needs to do get an answer. It should be called repeatedly,
            // until it returns an answer or fails (except for EAGAIN error, which means we need to
            // wait for an event on a socket, and then run this again).
            // Also, it takes non-const address pointer, despite not actually modifying it, so we need a cast.

            error = 0;
            struct dns_packet * dnsAnswer = dns_so_query (
                queries[ i ].socket,
                queries[ i ].query,
                ( struct sockaddr * ) &queries[ i ].addr->s,
                &error );

            if ( !dnsAnswer )
            {
                if ( error == EAGAIN )
                {
                    // This query waits for data to arrive, and it has not failed yet.
                    // activeQueries is set to 0 before each 'for' loop over the queries.
                    // It simply counts the number of queries that have not failed yet during
                    // a single run of the main 'do-while' loop. It will include all the queries
                    // that report EAGAIN error (meaning they haven't failed wait, just waiting for
                    // data to be read or written).
                    ++activeQueries;
                    continue;
                }

                SIMPLE_LOG_DEBUG ( "queryName=%s; queryType=%d; resultsPtr=%p; Error while querying %s: %s; ",
                                   name, queryType, resultsPtr,
                                   simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ),
                                   dns_strerror ( error ) );

                // Error on this socket. Remove it from the list of active queries.
                cleanupQuery ( &queries[ i ] );
                continue;
            }

            // Let's see if the answer is truncated ('tc' flag is set).
            // If it is, and the QueryDataFlagTryTcp flag is set, we want to repeat this query using TCP protocol.
            if ( dns_header ( dnsAnswer )->tc && ( queries[ i ].flags & QueryDataFlagTryTcp ) )
            {
                // We received a truncated answer; Let's restart this query using TCP.
                if ( setupQuery ( name, queryType, &queries[ i ], SOCK_STREAM, sockUserData ) != 1 )
                {
                    SIMPLE_LOG_ERR (
                        "queryName=%s; queryType=%d; resultsPtr=%p; "
                        "Error re-configuring a DNS query using TCP against %s",
                        name, queryType, resultsPtr, simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ) );

                    // Remove this query from the list of active queries.
                    cleanupQuery ( &queries[ i ] );
                    continue;
                }

                SIMPLE_LOG_DEBUG (
                    "queryName=%s; queryType=%d; resultsPtr=%p; Regenerated a query against %s using TCP protocol",
                    name, queryType, resultsPtr, simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ) );

                // Let's keep running!
                ++activeQueries;

                tcpRestarted = 1;
                continue;
            }

            // We got a valid answer (which may be empty)! Let's generate the list of results.
            // If there are no records in it, we are OK returning it right away.
            // Otherwise we would wait until the timeout if there was at least one non-reachable DNS server,
            // even if all others returned 0 records.
            // However, since there might be additional DNS servers with better results,
            // we want to continue running 'for' loop and check other servers too.
            // If they don't have any results yet, or they are all empty, we return 0 records.

            const int numRecords = parseDnsAnswer ( queryType, dnsAnswer, resultsPtr );

            dnsPacketFree ( dnsAnswer );
            dnsAnswer = 0;

            SIMPLE_LOG_DEBUG (
                "queryName=%s; queryType=%d; resultsPtr=%p; Server %s returned an answer with %d records",
                name, queryType, resultsPtr,
                simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ),
                numRecords );

            // Regardless of the number of records, this query is not active anymore!
            cleanupQuery ( &queries[ i ] );

            if ( numRecords > 0 )
            {
                // We have a non-empty answer!
                // Let's return it!
                SIMPLE_LOG_DEBUG ( "queryName=%s; queryType=%d; resultsPtr=%p; Query %d succeeded"
                                   "; Returning %d records",
                                   name, queryType, resultsPtr, ( int ) i, numRecords );
                return numRecords;
            }

            // An empty answer...

            if ( !receivedEmptyAnswer )
            {
                // This is our first empty answer.
                receivedEmptyAnswer = 1;

                // We want to finish within one second (note that curTime is not updated yet):
                const time_t newEnd = time ( 0 ) + 1;

                // We only set endTime if the new end time is shorter, or if timeout is infinite.
                if ( timeout < 1 || newEnd < endTime )
                {
                    endTime = newEnd;

                    // We want to start respecting the endTime even if timeout was 0 (infinite) - so make it > 0:
                    timeout = 1;

                    // We also want to make sure that we try at least once more:
                    runAgain = 1;
                }

                // NOTE: All of this doesn't matter if this was our last active query.
                //       If that's the case, we will return an empty result below.
            }
        }

        if ( tcpRestarted )
        {
            continue;
        }

        if ( activeQueries < 1 )
        {
            // There are no more queries left.
            // If anything came back with valid records, we would have already returned.
            // So we are here if everything either failed or completed with empty results.

            if ( receivedEmptyAnswer )
            {
                // We had at least one empty answer, let's return that.
                SIMPLE_LOG_ERR ( "queryName=%s; queryType=%d; resultsPtr=%p"
                                 "; All queries have completed; We saw some empty answers, so we report that",
                                 name, queryType, resultsPtr );
                return 0;
            }

            // All queries have failed...
            SIMPLE_LOG_DEBUG ( "queryName=%s; queryType=%d; resultsPtr=%p; All queries have failed",
                               name, queryType, resultsPtr );

            errno = EINVAL;
            return -1;
        }

        // Some queries are still not completed. Let's wait for sockets, unless we should time-out.

        // We only do anything time-related if timeout is > 0 and runAgain doesn't force us to run anyway.
        if ( timeout > 0 && !runAgain )
        {
            // Update the time.
            curTime = time ( 0 );

            if ( curTime >= endTime )
            {
                // time-out!
                if ( receivedEmptyAnswer )
                {
                    // We had at least one empty answer, let's return that.
                    SIMPLE_LOG_ERR ( "queryName=%s; queryType=%d; resultsPtr=%p"
                                     "; DNS resolution timed out; We saw some empty answers, so we report that",
                                     name, queryType, resultsPtr );
                    return 0;
                }

                SIMPLE_LOG_DEBUG ( "queryName=%s; queryType=%d; resultsPtr=%p; DNS resolution timed out",
                                   name, queryType, resultsPtr );

                errno = ETIMEDOUT;
                return -1;
            }
            else if ( curTime < startTime )
            {
                // Time went backwards...

                if ( receivedEmptyAnswer )
                {
                    // We had some empty answers, let's return that.
                    SIMPLE_LOG_ERR ( "queryName=%s; queryType=%d; resultsPtr=%p"
                                     "; Current time went below start time - wall clock has been changed"
                                     "; We saw some empty answers, so we report that",
                                     name, queryType, resultsPtr );

                    return 0;
                }

                SIMPLE_LOG_ERR ( "queryName=%s; queryType=%d; resultsPtr=%p"
                                 "; Current time went below start time - wall clock has been changed"
                                 "; Timing out all requests",
                                 name, queryType, resultsPtr );

                errno = ETIMEDOUT;
                return -1;
            }
        }

        runAgain = 0;

        SIMPLE_LOG_DEBUG ( "queryName=%s; queryType=%d; resultsPtr=%p; Active queries: %u; Waiting for sockets...",
                           name, queryType, resultsPtr, ( unsigned ) activeQueries );

        if ( waitForSocket ( queries, numQueries, 1 ) < 0 )
        {
            // There was an error!
            return -1;
        }
    }
}

int dns_resolve_ext (
        const char * name, enum DnsRecordType qType,
        const struct DnsServerConfig * serverConfigs, size_t numServers, DnsSocketFuncType socketFunc,
        unsigned int timeout,
        struct DnsRecord ** resultsPtr )
{
    if ( !name || !serverConfigs || numServers < 1 || !resultsPtr )
    {
        SIMPLE_LOG_ERR ( "dns_resolve(): Invalid parameter(s)" );

        errno = EINVAL;
        return -1;
    }

    *resultsPtr = 0;

    enum dns_type dnsType;

    switch ( qType )
    {
        case DnsRTypeA:
            dnsType = DNS_T_A;
            break;

        case DnsRTypeAAAA:
            dnsType = DNS_T_AAAA;
            break;

        case DnsRTypeSRV:
            dnsType = DNS_T_SRV;
            break;

        default:
            SIMPLE_LOG_ERR ( "dns_resolve(%s,%d): Invalid record type requested", name, qType );

            errno = EINVAL;
            return -1;
    }

    int error = 0;

    // This actually allocates queryPacket on stack:
    struct dns_packet * const queryPacket = dns_p_new ( MAX_PAYLOAD_SIZE );

    // dns.c has no comments, but it looks like dns_p_push() adds a new section to a "packet".
    // Here we add a 'question' section:
    if ( ( error = dns_p_push ( queryPacket, DNS_S_QUESTION, name, strlen ( name ), dnsType, DNS_C_IN, 0, 0 ) ) != 0 )
    {
        SIMPLE_LOG_ERR ( "dns_resolve(%s,%d): Error generating query: %s",
                         name, qType, dns_strerror ( error ) );

        errno = EINVAL;
        return -1;
    }

    // Recursion Desired
    // Request that the server receiving the query attempt to answer the query recursively.
    dns_header ( queryPacket )->rd = 1;

    // If socketFunc is set, we will be using custom bind.
    // We will configure the thread once, and just replace a content of this field for each socket:
    union DnsApiUserData userData;
    const struct DnsSocketApiExtCalls sockCalls = { socketFunc, 0, 0, 0, 0, 0, 0, 0 };

    if ( socketFunc != 0 )
    {
        // We can create this on stack in this block, because it's copied.
        // sockCalls, however, need to remain valid while we are running!
        const struct DnsThreadConfig threadCfg = { 0, &sockCalls, &userData };

        dnsThreadSetup ( &threadCfg );
    }

    size_t validQueries = 0;
    struct QueryData * queries = calloc ( numServers, sizeof ( struct QueryData ) );

    {
        struct SimpleLogAddrDescBuf addrDesc;

        for ( size_t i = 0; i < numServers; ++i )
        {
            if ( serverConfigs[ i ].address.s.sa_family != AF_INET
                 && serverConfigs[ i ].address.s.sa_family != AF_INET6 )
            {
                SIMPLE_LOG_ERR ( "Invalid DNS server address provided; Skipping" );
                continue;
            }

            queries[ i ].addr = &serverConfigs[ i ].address;
            queries[ i ].orgQuery = queryPacket;
            queries[ i ].sockUserData = serverConfigs[ i ].userData;

            // By default we use UDP.
            int sockType = SOCK_DGRAM;

            if ( serverConfigs[ i ].flags & DNS_SERVER_FLAG_USE_TCP )
            {
                // We need to use TCP mode. In that mode there is no point retrying using TCP.
                sockType = SOCK_STREAM;
            }
            else if ( !( serverConfigs[ i ].flags & DNS_SERVER_FLAG_DONT_USE_TCP ) )
            {
                // In the default UDP mode, we want to enable retrying using TCP if the answer is truncated.
                // This flag enables that behaviour, but we don't set it if DNS_SERVER_FLAG_DONT_USE_TCP is passed.
                queries[ i ].flags |= QueryDataFlagTryTcp;
            }

            if ( setupQuery ( name, dnsType, &queries[ i ], sockType, &userData ) != 1 )
            {
                SIMPLE_LOG_ERR ( "Error configuring a DNS query: Addr: %s; Name: %s; DNS Type: %d",
                                 simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ), name, dnsType );

                cleanupQuery ( &queries[ i ] );
                continue;
            }

            SIMPLE_LOG_DEBUG ( "Generated a query for %s using %s",
                               name, simpleLogDnsAddrDesc ( queries[ i ].addr, &addrDesc ) );
            ++validQueries;
        }
    }

    int ret = -1;
    int retErrno = EINVAL;

    if ( validQueries < 1 )
    {
        // No valid queries generated...
        SIMPLE_LOG_ERR ( "Could not generate any queries for %s", name );
    }
    else
    {
        errno = 0;
        ret = resolveQueries ( name, dnsType, &userData, queries, numServers, timeout, resultsPtr );
        retErrno = errno;

        SIMPLE_LOG_DEBUG ( "dns_resolve(%s,%d) returns %d record(s)", name, qType, ret );
    }

    if ( socketFunc != 0 )
    {
        // We only need this when running dns_so_open.
        // It is called inside setupQuery(), which is called above in this function,
        // but can also be called inside resolveQueries().
        dnsThreadClear();
    }

    for ( size_t i = 0; i < numServers; ++i )
    {
        // If there was a failure, there is no need to check individual queries,
        // but just in case (or if something changes in the code):
        cleanupQuery ( &queries[ i ] );
    }

    free ( queries );
    queries = 0;

    errno = retErrno;
    return ret;
}

int dns_resolve (
        const char * name, enum DnsRecordType qType,
        const union DnsSockAddr * dnsServer,
        unsigned int timeout,
        struct DnsRecord ** results )
{
    struct DnsServerConfig cfg;
    memset ( &cfg, 0, sizeof ( cfg ) );

    if ( dnsServer != 0 )
    {
        cfg.address = *dnsServer;
    }

    return dns_resolve_ext ( name, qType, &cfg, 1, 0, timeout, results );
}

int dnsSocketBoundToIface ( int family, int type, int protocol, union DnsApiUserData * userData )
{
    int sockFd = socket ( family, type, protocol );

    if ( sockFd < 0 )
        return sockFd;

    const char * ifaceName = ( userData != 0 ) ? userData->cPtr : 0;

    if ( !ifaceName || ifaceName[ 0 ] == 0 )
    {
        return sockFd;
    }

#if defined( IP_BOUND_IF ) || defined( IPV6_BOUND_IF )
    int ifIndex = -1;

    if ( family != AF_INET && family != AF_INET6 )
    {
        SIMPLE_LOG_ERR ( "Unsupported socket family for binding: %d; IfaceName: '%s'", family, ifaceName );
    }
    else if ( ( ifIndex = if_nametoindex ( ifaceName ) ) < 1 )
    {
        SIMPLE_LOG_ERR ( "Failed to find interface index for IfaceName: '%s'; Error: [%d] %s",
                         ifaceName, errno, strerror ( errno ) );
    }
#if defined( IP_BOUND_IF )
    else if ( family == AF_INET
              && 0 != setsockopt ( sockFd, IPPROTO_IP, IP_BOUND_IF, &ifIndex, sizeof ( ifIndex ) ) )
    {
        SIMPLE_LOG_ERR ( "Error binding socket with FD %d; Family: %d; IfaceName: '%s'; IfaceIndex: %d; Error: [%d] %s",
                         sockFd, family, ifaceName, ifIndex, errno, strerror ( errno ) );
    }
#else
    else if ( family == AF_INET )
    {
        SIMPLE_LOG_ERR ( "IP_BOUND_IF not defined; Unsupported socket family for binding: %d; IfaceName: '%s'",
                         family, ifaceName );
    }
#endif
#if defined( IPV6_BOUND_IF )
    else if ( family == AF_INET6
              && 0 != setsockopt ( sockFd, IPPROTO_IPV6, IPV6_BOUND_IF, &ifIndex, sizeof ( ifIndex ) ) )
    {
        SIMPLE_LOG_ERR ( "Error binding socket with FD %d; Family: %d; IfaceName: '%s'; IfaceIndex: %d; Error: [%d] %s",
                         sockFd, family, ifaceName, ifIndex, errno, strerror ( errno ) );
    }
#else
    else if ( family == AF_INET6 )
    {
        SIMPLE_LOG_ERR ( "IPV6_BOUND_IF not defined; Unsupported socket family for binding: %d; IfaceName: '%s'",
                         family, ifaceName );
    }
#endif
    else
    {
        return sockFd;
    }
#elif defined( SO_BINDTODEVICE )
    const socklen_t ifaceNameLen = strlen ( ifaceName );

    if ( ifaceNameLen + 1 > IFNAMSIZ )
    {
        // +1, because NULL needs to fit as well.

        SIMPLE_LOG_ERR ( "Interface name '%s' is too long; Max length is %u characters",
                         ifaceName, IFNAMSIZ - 1 );
    }
    else if ( 0 != setsockopt ( sockFd, SOL_SOCKET, SO_BINDTODEVICE, ifaceName, ifaceNameLen + 1 ) )
    {
        SIMPLE_LOG_ERR ( "Error setting socket option SO_BINDTODEVICE for socket with FD %d"
                         " using IfaceName: '%s'; Error: [%d] %s",
                         sockFd, ifaceName, errno, strerror ( errno ) );
    }
    else
    {
        return sockFd;
    }
#else
    SIMPLE_LOG_ERR ( "Could not bind to iface '%s': Binding to interfaces is not supported on this platform",
                     bindToIface );
#endif

    close ( sockFd );

    errno = EINVAL;
    return -1;
}

int ndns_resolve (
        enum DnsRecordType qType, const char * name,
        const struct DnsConfig * config,
        unsigned int timeout,
        struct DnsRecord ** results )
{
    if ( !config || !config->dnsServers || config->numDnsServers < 1 )
    {
        errno = EINVAL;
        return -1;
    }

    DnsSocketFuncType socketFunc = 0;
    struct DnsServerConfig * cfg = calloc ( config->numDnsServers, sizeof ( struct DnsServerConfig ) );

    for ( size_t i = 0; i < config->numDnsServers; ++i )
    {
        cfg[ i ].address.v6 = config->dnsServers[ i ];

        if ( config->bindToIface != 0 && config->bindToIface[ 0 ] != 0 )
        {
            socketFunc = dnsSocketBoundToIface;
            cfg[ i ].userData.cPtr = config->bindToIface;
        }
    }

    const int ret = dns_resolve_ext ( name, qType, cfg, config->numDnsServers, socketFunc, timeout, results );

    free ( cfg );
    return ret;
}
