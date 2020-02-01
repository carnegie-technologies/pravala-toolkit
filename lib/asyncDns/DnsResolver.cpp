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

extern "C"
{
#include <net/if.h>
#include <pthread.h>

#include "dns/DnsInternal.h"
}

#include <cerrno>

#ifdef PLATFORM_ANDROID
#include "sys/os/Android/AndroidSocketApi.hpp"
#endif

#include "basic/Random.hpp"
#include "sys/SocketApi.hpp"

#include "DnsResolver.hpp"

// This should be before including SimpleLog.h
#define SIMPLE_LOG_TAG    "DNS/AsyncResolver"

#include "simplelog/SimpleLog.h"

using namespace Pravala;

TextLog DnsResolver::_log ( "dns_resolver" );

String DnsResolver::SrvRecord::toString() const
{
    return String ( "priority: %1; weight: %2; port: %3; target: %4" )
           .arg ( priority )
           .arg ( weight )
           .arg ( port )
           .arg ( target );
}

DnsResolver::LookupCompleteTask::LookupCompleteTask (
        DnsResolver * resolver, uint32_t lookupId, enum DnsRecordType qType,
        struct DnsRecord * results, int numResults ):
    Task ( resolver ),
    _resolver ( resolver ),
    _results ( results ),
    _lookupId ( lookupId ),
    _numResults ( numResults ),
    _type ( qType )
{
}

DnsResolver::LookupCompleteTask::~LookupCompleteTask()
{
    if ( _results != 0 )
    {
        free ( _results );
    }
}

void DnsResolver::LookupCompleteTask::runTask()
{
    if ( _resolver != 0 )
    {
        LOG ( L_DEBUG, "Lookup " << _lookupId << " completed; Type: " << _type << "; NumResults: " << _numResults );

        _resolver->lookupComplete ( _lookupId, _type, _results, _numResults );
    }
}

DnsResolver::DnsResolver ( Owner & owner ): _owner ( owner ), _currentId ( 0 ), _lastId ( 0 )
{
    Random::init();

    AsyncQueue::get().registerReceiver ( this );
}

DnsResolver::~DnsResolver()
{
    AsyncQueue::get().unregisterReceiver ( this );
}

/// @brief Contains configuration for the DNS lookup thread.
struct ThreadConfig: public NoCopy
{
    /// @brief The resolver running the query.
    DnsResolver * resolver;

    /// @brief Contains the list of DNS servers.
    /// config->dnsServers points to this.
    /// We use a SimpleArray, because it will create a memory copy of the original array, to avoid threading issues.
    SimpleArray<struct sockaddr_in6> servers;

    /// @brief The name to query.
    String name;

    /// @brief Interface configuration.
    DnsResolver::IfaceConfig ifaceConfig;

    /// @brief The ID of the lookup.
    uint32_t id;

    /// @brief The type of the query to perform.
    enum DnsRecordType queryType;

    /// @brief The timeout for the operation (in seconds).
    uint16_t timeout;

    /// @brief A bitmask of ReqFlag* values.
    uint8_t flags;

    /// @brief Default constructor.
    ThreadConfig(): resolver ( 0 ), id ( 0 ), queryType ( DnsRTypeInvalid ), timeout ( 0 ), flags ( 0 )
    {
    }
};

ERRCODE DnsResolver::start (
        const HashSet<SockAddr> & dnsServers,
        uint8_t reqType, const String & name, uint8_t flags,
        const IfaceConfig * ifaceConfig, uint16_t timeout )
{
    // We need a simple array of servers.
    // We store both v4 and v6 addresses as sockaddr_in6, because it's bigger.
    SimpleArray<sockaddr_in6> servers;

    for ( HashSet<SockAddr>::Iterator it ( dnsServers ); it.isValid(); it.next() )
    {
        if ( it.value().hasIpAddr() && it.value().hasPort() )
        {
            servers.append ( it.value().sa_in6 );
        }
    }

    if ( name.isEmpty()
         || servers.size() < 1
         || ( reqType & ( ReqTypeA | ReqTypeAAAA | ReqTypeSRV ) ) == 0
         || ( ( reqType & ReqTypeSRV ) && reqType != ReqTypeSRV ) )
    {
        // One of those is wrong:
        // - an empty name
        // - an empty list of servers (or all invalid)
        // - no request
        // - SRV request together with another request type (which we don't support).
        return Error::InvalidParameter;
    }

    stop();

    _reqType = reqType;
    _currentName = name;

    // In case the ID wraps around.
    do
    {
        _currentId = ++_lastId;
    }
    while ( _currentId == 0 );

    // Now we treat 'reqType' as 'requests left to do, in case there are multiple (A + AAAA).
    // Every time we create a request type, we unset it in reqType.
    while ( reqType != 0 )
    {
        ThreadConfig * const tCfg = new ThreadConfig();

        tCfg->resolver = this;
        tCfg->id = _currentId;
        tCfg->timeout = timeout;
        tCfg->flags = flags;

        // We use c_str() to perform a deep copy, to avoid threading issues.
        tCfg->name = _currentName.c_str();

        // This also creates a memory copy:
        tCfg->servers = servers;

        if ( ifaceConfig != 0 && ifaceConfig->isUsed() )
        {
            tCfg->ifaceConfig.bindToNetwork = ifaceConfig->bindToNetwork;

            // We use c_str() to perform a deep copy, to avoid threading issues.
            tCfg->ifaceConfig.bindToIfaceV4 = ifaceConfig->bindToIfaceV4.c_str();
            tCfg->ifaceConfig.bindToIfaceV6 = ifaceConfig->bindToIfaceV6.c_str();
        }

        if ( reqType & ReqTypeA )
        {
            reqType &= ~ReqTypeA;
            tCfg->queryType = DnsRTypeA;
        }
        else if ( reqType & ReqTypeAAAA )
        {
            reqType &= ~ReqTypeAAAA;
            tCfg->queryType = DnsRTypeAAAA;
        }
        else if ( reqType & ReqTypeSRV )
        {
            reqType &= ~ReqTypeSRV;
            tCfg->queryType = DnsRTypeSRV;
        }
        else
        {
            assert ( false );

            delete tCfg;

            stop();
            return Error::InternalError;
        }

        pthread_attr_t attrs;

        pthread_attr_init ( &attrs );

        // This way we won't need to join it:
        pthread_attr_setdetachstate ( &attrs, PTHREAD_CREATE_DETACHED );

        pthread_t t;
        const int ret = pthread_create ( &t, &attrs, threadMain, tCfg );

        if ( ret != 0 )
        {
            LOG ( L_WARN, "Error starting a thread: " << strerror ( errno ) );

            delete tCfg;

            stop();
            return Error::InternalError;
        }
    }

    return Error::Success;
}

/// @brief A socket() function to be used by the underlying dns.c resolver.
/// It expects a DnsApiUserData object with vPtr pointed at IfaceConfig object.
/// That object's configuration is used for binding sockets to interfaces and/or networks.
/// @note This function is copied from NativeDns.c.
///       The difference is, this version supports binding to Android networks.
/// @param [in] family family for socket() call.
/// @param [in] type type for socket() call.
/// @param [in] protocol protocol for socket() call.
/// @param [in] userData DNS user data pointer. If invalid, or with invalid vPtr, no binding will be performed.
/// @return Result of socket() call.
static int dnsBoundSocket ( int family, int type, int protocol, union DnsApiUserData * userData )
{
    int sockFd = socket ( family, type, protocol );

    if ( sockFd < 0 || !userData )
    {
        return sockFd;
    }

    const DnsResolver::IfaceConfig * ifCfg = ( const DnsResolver::IfaceConfig * ) userData->vPtr;

    if ( !ifCfg )
    {
        return sockFd;
    }

#ifdef PLATFORM_ANDROID
    if ( ifCfg->bindToNetwork >= 0
         && NOT_OK ( AndroidSocketApi::bindSocketToNetwork ( sockFd, ifCfg->bindToNetwork ) ) )
    {
        SocketApi::close ( sockFd );
        return -1;
    }
#endif

    const char * ifaceName = 0;

    if ( family == AF_INET )
    {
        ifaceName = ifCfg->bindToIfaceV4.c_str();
    }
    else if ( family == AF_INET6 )
    {
        ifaceName = ifCfg->bindToIfaceV6.c_str();
    }

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

void * DnsResolver::threadMain ( void * ptr )
{
    ThreadConfig * const cfg = ( ThreadConfig * ) ptr;

    if ( !cfg )
    {
        return 0;
    }

    if ( !cfg->resolver || cfg->servers.size() < 1 )
    {
        delete cfg;
        return 0;
    }

    unsigned int tout = cfg->timeout;

    if ( tout > MaxTimeout || tout < 1 )
    {
        tout = MaxTimeout;
    }

    DnsSocketFuncType socketFunc = 0;
    DnsServerConfig * srvCfg = new DnsServerConfig[ cfg->servers.size() ];

    for ( size_t i = 0; i < cfg->servers.size(); ++i )
    {
        // We store both v4 and v6 addresses as sockaddr_in6, because it's bigger.
        srvCfg[ i ].address.v6 = cfg->servers[ i ];

        if ( cfg->ifaceConfig.isUsed() )
        {
            socketFunc = dnsBoundSocket;
            srvCfg[ i ].userData.vPtr = &( cfg->ifaceConfig );
        }

        if ( cfg->flags & ReqFlagUseTcp )
        {
            srvCfg[ i ].flags |= DNS_SERVER_FLAG_USE_TCP;
        }

        if ( cfg->flags & ReqFlagDontUseTcp )
        {
            srvCfg[ i ].flags |= DNS_SERVER_FLAG_DONT_USE_TCP;
        }
    }

    struct DnsRecord * results = 0;

    const int numResults
        = dns_resolve_ext (
        cfg->name.c_str(), cfg->queryType, srvCfg, cfg->servers.size(), socketFunc, tout, &results );

    AsyncQueue::get().blockingRunTask (
        new LookupCompleteTask ( cfg->resolver, cfg->id, cfg->queryType, results, numResults ) );

    delete cfg;
    delete[] srvCfg;

    return 0;
}

void DnsResolver::stop()
{
    _reqType = 0;
    _currentId = 0;
    _currentName.clear();
    _pendingResults.clear();
}

void DnsResolver::lookupComplete ( uint32_t id, enum DnsRecordType qType, struct DnsRecord * results, int numResults )
{
    if ( id != _currentId )
    {
        LOG ( L_DEBUG, "Ignoring old request; ID: " << id << "; Current ID: " << _currentId );

        return;
    }

    if ( numResults > 0 && !results )
    {
        assert ( false );

        numResults = -1;
    }

    const String name = _currentName;

    if ( qType == DnsRTypeSRV )
    {
        // We got 'SRV' response, we no longer wait for it.
        _reqType &= ~ReqTypeSRV;
        _currentId = 0;

        List<SrvRecord> list;

        for ( int i = 0; i < numResults; ++i )
        {
            if ( results[ i ].recordType != DnsRTypeSRV )
            {
                LOG ( L_WARN, "Got result " << getRecordDesc ( results[ i ] ) << " for SRV lookup"
                      "; name: '" << name << "'; Ignoring" );
                continue;
            }

            LOG ( L_DEBUG3, "Resolved '" << name << "' to " << getRecordDesc ( results[ i ] ) );

            SrvRecord srv;
            srv.ttl = results[ i ].ttl;
            srv.priority = results[ i ].data.srv.priority;
            srv.weight = results[ i ].data.srv.weight;
            srv.port = results[ i ].data.srv.port;
            srv.target = results[ i ].data.srv.target;

            list.append ( srv );
        }

        LOG ( L_DEBUG, "DNS SRV lookup completed for name: '" << name << "'; NumResults: " << list.size() );

        _owner.dnsLookupComplete ( this, name, list );

        // We MUST return after each callback.
        return;
    }

    if ( qType == DnsRTypeA || qType == DnsRTypeAAAA )
    {
        if ( qType == DnsRTypeA )
        {
            // We got 'A' response, we no longer wait for it.
            _reqType &= ~ReqTypeA;
        }
        else
        {
            // We got 'AAAA' response, we no longer wait for it.
            _reqType &= ~ReqTypeAAAA;
        }

        for ( int i = 0; i < numResults; ++i )
        {
            if ( qType == DnsRTypeA && results[ i ].recordType != DnsRTypeA )
            {
                LOG ( L_WARN, "Got result " << getRecordDesc ( results[ i ] ) << " for IPv4 lookup"
                      "; Name: '" << name << "'; Ignoring" );
                continue;
            }

            if ( qType == DnsRTypeAAAA && results[ i ].recordType != DnsRTypeAAAA )
            {
                LOG ( L_WARN, "Got result " << getRecordDesc ( results[ i ] ) << " for IPv6 lookup"
                      "; Name: '" << name << "'; Ignoring" );
                continue;
            }

            IpAddress addr;

            if ( results[ i ].recordType == DnsRTypeA )
            {
                addr = results[ i ].data.a.addr;
            }
            else
            {
                addr = results[ i ].data.aaaa.addr;
            }

            if ( !addr.isIPv4() && !addr.isIPv6() )
            {
                LOG ( L_WARN, "Got non-IPv4 and non-IPv6 address " << addr << " for result "
                      << getRecordDesc ( results[ i ] ) << "; Name: '" << name << "'; Ignoring" );
                continue;
            }

            LOG ( L_DEBUG3, "Resolved '" << name << "' to " << getRecordDesc ( results[ i ] ) );

            _pendingResults.insert ( addr );
        }

        if ( ( _reqType & ( ReqTypeA | ReqTypeAAAA ) ) == 0 )
        {
            // We no longer wait for results.
            _currentId = 0;

            List<IpAddress> list;

            for ( HashSet<IpAddress>::Iterator it ( _pendingResults ); it.isValid(); it.next() )
            {
                list.append ( it.value() );
            }

            _pendingResults.clear();

            _owner.dnsLookupComplete ( this, name, list );

            // We MUST return after each callback.
            return;
        }

        return;
    }
}

String DnsResolver::getRecordDesc ( const struct DnsRecord & record )
{
    String type = "Unknown";
    String data = "";

    switch ( record.recordType )
    {
        case DnsRTypeInvalid:
            type = "Invalid";
            break;

        case DnsRTypeA:
            type = "A";
            data = IpAddress ( record.data.a.addr ).toString();
            break;

        case DnsRTypeAAAA:
            type = "AAAA";
            data = IpAddress ( record.data.aaaa.addr ).toString();
            break;

        case DnsRTypeSRV:
            type = "SRV";
            data = String ( "priority: %1; weight: %2; port: %3; target: %4" )
                   .arg ( record.data.srv.priority )
                   .arg ( record.data.srv.weight )
                   .arg ( record.data.srv.port )
                   .arg ( record.data.srv.target );
            break;
    }

    return String ( "[%1] ttl: %2; %3" ).arg ( type ).arg ( record.ttl ).arg ( data );
}

bool DnsResolver::compareRecords ( const DnsResolver::SrvRecord & a, const DnsResolver::SrvRecord & b )
{
    if ( a.priority != b.priority )
    {
        // If priorities are different, return 'true' to put 'a' in front of 'b' if a's priority value is lower
        // (meaning higher priority). Otherwise 'b' has higher priority and goes in front of 'a'.
        return ( a.priority < b.priority );
    }

    // Their priorities are the same - we want to randomize things.
    // We take into account their weights.
    // The higher the weight of 'a' the more likely we are to put 'a' first.

    return ( Random::rand ( a.weight + b.weight ) < a.weight );
}

String Pravala::toString ( const List<DnsResolver::SrvRecord> & list )
{
    String ret ( "[" );

    for ( size_t i = 0; i < list.size(); ++i )
    {
        if ( i > 0 )
        {
            ret.append ( ", " );
        }

        ret.append ( String ( "(%1)" ).arg ( list[ i ].toString() ) );
    }

    ret.append ( "]" );

    return ret;
}
