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
#include <cstdio>

#include "basic/SimpleArray.hpp"
#include "basic/IpAddress.hpp"
#include "config/ConfigString.hpp"
#include "config/ConfigAddrSpecList.hpp"
#include "app/StdApp.hpp"
#include "dns/DnsInternal.h"

using namespace Pravala;

static ConfigAddrSpecList optDnsServer ( "dns-servers", 'd', "DNS servers to use", ",", "8.8.8.8:53" );
static ConfigString optQueryType ( "query-type", 't', "Query type to run; One of: A, AAAA, SRV", "A" );
static ConfigSwitch optEnableLogs ( "enable-logs", 'l', "Enable API logs" );
static ConfigSwitch optUseOldApi ( "old-api", 'o', "Use old DNS API" );
static ConfigSwitch optUseTcp ( "use-tcp", 0, "Use TCP (only valid with the new API)" );

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs );
    app.initConfig();

    enum DnsRecordType qType = DnsRTypeInvalid;

    if ( optQueryType.value().compare ( "a", false ) == 0 )
    {
        qType = DnsRTypeA;
    }
    else if ( optQueryType.value().compare ( "aaaa", false ) == 0 )
    {
        qType = DnsRTypeAAAA;
    }
    else if ( optQueryType.value().compare ( "srv", false ) == 0 )
    {
        qType = DnsRTypeSRV;
    }
    else
    {
        fprintf ( stderr, "Invalid query type: '%s'\n", optQueryType.value().c_str() );
        return EXIT_FAILURE;
    }

    const StringList & names = app.getExtraCmdLineArgs();

    if ( names.size() < 1 )
    {
        fprintf ( stderr, "Provide at least one name to resolve\n" );
        return EXIT_FAILURE;
    }

    SimpleArray<struct sockaddr_in6> dnsAddrs;
    String serverStr; // A comma-separated list of all the DNS servers that will be used.

    for ( size_t idx = 0; idx < optDnsServer.value().size(); ++idx )
    {
        const SockAddr & dns = optDnsServer.value().at ( idx );

        if ( !dns.isIPv4() && !dns.isIPv6() )
        {
            fprintf ( stderr, "Invalid DNS server used: %s\n", dns.toString().c_str() );
            return EXIT_FAILURE;
        }

        dnsAddrs.append ( dns.sa_in6 );

        if ( !serverStr.isEmpty() )
        {
            serverStr.append ( ',' );
        }

        serverStr.append ( dns.toString() );
    }

    if ( dnsAddrs.size() < 1 )
    {
        fprintf ( stderr, "Provide at least one DNS server\n" );
        return EXIT_FAILURE;
    }

    SimpleArray<struct DnsServerConfig> dnsServers;

    for ( size_t idx = 0; idx < dnsAddrs.size(); ++idx )
    {
        struct DnsServerConfig cfg;
        memset ( &cfg, 0, sizeof ( cfg ) );

        cfg.address.v6 = dnsAddrs[ idx ];

/*
        // Configuring interface binds:
        if (idx == 0)
        {
            cfg.userData.cPtr = "lo";
        }
        else if (idx == 1)
        {
            cfg.userData.cPtr = "eth9";
        }
        else if (idx == 2)
        {
            cfg.userData.cPtr = "enp0s25";
        }
 */

        if ( optUseTcp.value() )
        {
            cfg.flags = DNS_SERVER_FLAG_USE_TCP;
        }

        dnsServers.append ( cfg );
    }

    struct DnsConfig config;

    memset ( &config, 0, sizeof ( config ) );

    config.dnsServers = dnsAddrs.getMemory();
    config.numDnsServers = dnsAddrs.size();

    for ( size_t i = 0; i < names.size(); ++i )
    {
        const char * n = names.at ( i ).c_str();

        // To bind DNS sockets to specific interface:
        // config.bindToIface = "eth0";

        struct DnsRecord * records = 0;
        int numRecords;

        if ( optUseOldApi.value() )
        {
            if ( optUseTcp.value() )
            {
                fprintf ( stderr, "Old API does not support TCP connections\n" );
                return EXIT_FAILURE;
            }

            printf ( "\nRunning ndns_resolve(%s,%s,[%s])...\n",
                     optQueryType.value().c_str(), n, serverStr.c_str() );

            numRecords = ndns_resolve ( qType, n, &config, 5, &records );
        }
        else
        {
            printf ( "\nRunning dns_resolve_ext(%s,%s,[%s],%s)...\n",
                     optQueryType.value().c_str(), n, serverStr.c_str(), optUseTcp.value() ? "TCP" : "UDP" );

            numRecords = dns_resolve_ext (
                n, qType, dnsServers.getMemory(), dnsServers.size(), dnsSocketBoundToIface, 5, &records );
        }

        if ( numRecords < 0 )
        {
            printf ( "ndns_resolve(%s,%s) failed: %d\n", optQueryType.value().c_str(), n, numRecords );
        }
        else
        {
            printf ( "ndns_resolve(%s,%s) generated %d results(s)\n",
                     optQueryType.value().c_str(), n, numRecords );

            for ( int j = 0; j < numRecords; ++j )
            {
                struct DnsRecord & r = records[ j ];

                printf ( "%2d: ", j );

                switch ( r.recordType )
                {
                    case DnsRTypeA:
                        printf ( "Type: A; TTL: %lu; Address: %s",
                                 ( long unsigned ) r.ttl,
                                 IpAddress ( r.data.a.addr ).toString().c_str() );
                        break;

                    case DnsRTypeAAAA:
                        printf ( "Type: AAAA; TTL: %lu; Address: %s",
                                 ( long unsigned ) r.ttl,
                                 IpAddress ( r.data.aaaa.addr ).toString().c_str() );
                        break;

                    case DnsRTypeSRV:
                        printf ( "Type: SRV; TTL: %lu; Target: %s; Port: %u, Priority: %u; Weight: %u",
                                 ( long unsigned ) r.ttl,
                                 r.data.srv.target,
                                 ( unsigned ) r.data.srv.port,
                                 ( unsigned ) r.data.srv.priority,
                                 ( unsigned ) r.data.srv.weight );
                        break;

                    default:
                        printf ( "Invalid type (%u)", ( unsigned ) r.recordType );
                        break;
                }

                printf ( "\n" );
            }
        }

        if ( records != 0 )
        {
            free ( records );
        }
    }

    return EXIT_SUCCESS;
}
