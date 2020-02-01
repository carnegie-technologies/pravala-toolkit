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
#include "asyncDns/DnsResolver.hpp"

using namespace Pravala;

static ConfigAddrSpecList optDnsServer ( "dns-servers", 'd', "DNS servers to use", ",", "8.8.8.8:53" );
static ConfigString optQueryType (
        "query-type", 't', "Query type to run; One of: A, AAAA, IP (for both A and AAAA), SRV", "IP" );

class Reporter: public DnsResolver::Owner
{
    public:
        void dnsLookupComplete (
            DnsResolver *, const String & name, const List<DnsResolver::SrvRecord> & results )
        {
            printf ( "Results for '%s' SRV query:\n", name.c_str() );

            for ( size_t i = 0; i < results.size(); ++i )
            {
                const DnsResolver::SrvRecord & r = results.at ( i );

                printf ( "%d: Target: %s; Port: %u, Priority: %u; Weight: %u\n",
                         ( int ) i, r.target.c_str(), r.port, r.priority, r.weight );
            }

            EventManager::stop();
        }

        void dnsLookupComplete (
            DnsResolver *, const String & name, const List<IpAddress> & results )
        {
            printf ( "Results for '%s' IP query:\n", name.c_str() );

            for ( size_t i = 0; i < results.size(); ++i )
            {
                printf ( "%d: '%s'\n", ( int ) i, results.at ( i ).toString().c_str() );
            }

            EventManager::stop();
        }
};

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs );
    app.init();

    uint8_t reqType = 0;

    if ( optQueryType.value().compare ( "a", false ) == 0 )
    {
        reqType = DnsResolver::ReqTypeA;
    }
    else if ( optQueryType.value().compare ( "aaaa", false ) == 0 )
    {
        reqType = DnsResolver::ReqTypeAAAA;
    }
    else if ( optQueryType.value().compare ( "ip", false ) == 0 )
    {
        reqType = DnsResolver::ReqTypeA | DnsResolver::ReqTypeAAAA;
    }
    else if ( optQueryType.value().compare ( "srv", false ) == 0 )
    {
        reqType = DnsResolver::ReqTypeSRV;
    }
    else
    {
        fprintf ( stderr, "Invalid query type: '%s'\n", optQueryType.value().c_str() );
        return EXIT_FAILURE;
    }

    const StringList & names = app.getExtraCmdLineArgs();

    if ( names.size() != 1 )
    {
        fprintf ( stderr, "Provide exactly one name to resolve\n" );
        return EXIT_FAILURE;
    }

    HashSet<SockAddr> dnsAddrs;

    for ( size_t idx = 0; idx < optDnsServer.value().size(); ++idx )
    {
        const SockAddr & dns = optDnsServer.value().at ( idx );

        if ( !dns.isIPv4() && !dns.isIPv6() )
        {
            fprintf ( stderr, "Invalid DNS server used: %s\n", dns.toString().c_str() );
            return EXIT_FAILURE;
        }

        dnsAddrs.insert ( dns );
    }

    if ( dnsAddrs.size() < 1 )
    {
        fprintf ( stderr, "Provide at least one DNS server\n" );
        return EXIT_FAILURE;
    }

    Reporter rep;
    DnsResolver resolver ( rep );

    DnsResolver::IfaceConfig ifCfg;

    // ifCfg.bindToIfaceV4 = "eth0";
    // ifCfg.bindToIfaceV6 = "eth1";

    const ERRCODE eCode = resolver.start ( dnsAddrs, reqType, names.first(), 0, &ifCfg, 5 );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error starting the resolver: %s\n", eCode.toString() );
        return EXIT_FAILURE;
    }

    EventManager::run();

    return EXIT_SUCCESS;
}
