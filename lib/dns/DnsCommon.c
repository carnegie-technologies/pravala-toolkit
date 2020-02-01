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

// This should be before including SimpleLog.h
#define SIMPLE_LOG_TAG    "DNS/Common"

#include "simplelog/SimpleLog.h"
#include "DnsWrapper.h"

struct dns_hints * dnsGenHints (
        const struct sockaddr_in6 * dnsServers, size_t numServers, struct dns_resolv_conf * resConf )
{
    if ( !dnsServers || numServers < 1 )
    {
        SIMPLE_LOG_ERR ( "No DNS servers" );
        return 0;
    }

    if ( !resConf )
    {
        SIMPLE_LOG_ERR ( "No dns_resolv_conf" );
        return 0;
    }

    SIMPLE_LOG_DEBUG2 ( "Generating dns_hints using %d addresses", ( int ) numServers );

    int error;
    struct dns_hints * hints = dns_hints_open ( resConf, &error );

    if ( !hints )
    {
        SIMPLE_LOG_ERR ( "dns_hints_open() failed" );
        return 0;
    }

    size_t i;
    size_t added = 0;

    for ( i = 0; i < numServers; ++i )
    {
        const struct sockaddr * addr = ( const struct sockaddr * ) ( &dnsServers[ i ] );

        if ( addr->sa_family == AF_INET || addr->sa_family == AF_INET6 )
        {
            struct SimpleLogAddrDescBuf hDesc;

            SIMPLE_LOG_DEBUG2 ( "Inserting hint[%d]: %s (family: %d)",
                                ( int ) i,
                                simpleLogAddrDesc ( addr, sizeof ( struct sockaddr_in6 ), &hDesc ),
                                addr->sa_family );

            if ( dns_hints_insert ( hints, ".", addr, 1 ) == 0 )
            {
                ++added;
            }
        }
    }

    SIMPLE_LOG_DEBUG2 ( "Inserted %d hints", ( int ) added );

    if ( added > 0 )
    {
        SIMPLE_LOG_DEBUG ( "Configured DNS hints with %lu (of %lu) DNS servers",
                           ( long unsigned ) added, ( long unsigned ) numServers );
        return hints;
    }

    SIMPLE_LOG_DEBUG ( "Failed to configure DNS hints using any of %lu DNS servers",
                       ( long unsigned ) numServers );

    dns_hints_close ( hints );
    hints = 0;

    return 0;
}
