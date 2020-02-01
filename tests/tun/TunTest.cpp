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

#include <cerrno>

#include "TunTest.hpp"

#include "net/UdpPacket.hpp"
#include "netmgr/NetManager.hpp"
#include "socket/PacketDataStore.hpp"
#include "tun/TunIfaceDev.hpp"

using namespace Pravala;

ConfigIpAddress TunTest::optTunIpAddr (
        ConfigOpt::FlagReqNonEmpty, "ipaddr", 'i', "tun_test.tun_ip_addr", "IP address for tunnel", "10.0.0.1" );

ConfigAddrSpec TunTest::optDestIpPort (
        "dest", 'd', "IP:port to send junk traffic" );

ConfigNumber<uint16_t> TunTest::optSendDelay (
        "wait", 'w', "Time to wait between sending packets (ms)", 1000 );

ConfigLimitedNumber<uint16_t> TunTest::optNumIfaces (
        "num-ifaces", 'n', "The number of tunnel interfaces", 1, 0xFFFF, 1 );

TextLog TunTest::_log ( "tun_test" );

TunTest::TunTest(): _timer ( *this )
{
}

TunTest::~TunTest()
{
    while ( !_tuns.isEmpty() )
    {
        LOG ( L_INFO, "Removing TunIface[" << getTunDesc ( _tuns.last() ) << "]" );

        _tuns.last()->unrefOwner ( this );
        _tuns.removeLast();
    }
}

String TunTest::getTunDesc ( TunIface * iface )
{
    if ( !iface )
        return "invalid";

    for ( HashSet<IpAddress>::Iterator it ( iface->getAddresses() ); it.isValid(); it.next() )
    {
        if ( it.value().isValid() )
        {
            return String ( "%1:%2" ).arg ( iface->getIfaceName(), it.value().toString() );
        }
    }

    return iface->getIfaceName();
}

bool TunTest::start()
{
    if ( !_tuns.isEmpty() )
    {
        LOG ( L_ERROR, "Already started" );
        return false;
    }

    IpAddress tAddr = optTunIpAddr.value();

    for ( size_t tIdx = 0; tIdx < optNumIfaces.value(); ++tIdx )
    {
        TunIface * tun = TunIfaceDev::generate ( this );

        if ( !tun )
        {
            LOG ( L_ERROR, "Failed to generate TunIface[" << tIdx << "]" );
            return false;
        }

        _tuns.append ( tun );

        ERRCODE eCode = tun->startManaged();

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_ERROR, eCode, "Failed to start TunIface[" << tIdx << "]: " << strerror ( errno ) );
            return false;
        }

        if ( NOT_OK ( eCode = tun->addAddress ( tAddr ) ) )
        {
            LOG_ERR ( L_ERROR, eCode,
                      "Failed to add '" << tAddr << "' address to TunIface[" << getTunDesc ( tun ) << "]" );
            return false;
        }

        LOG ( L_INFO, "TunIface[" << getTunDesc ( tun ) << "] started" );

        tAddr.incrementBy ( 1 );
    }

    if ( _tuns.isEmpty() || !_tuns[ 0 ] )
    {
        LOG ( L_ERROR, "No interfaces created" );
        return false;
    }

    IpAddress subnet = optTunIpAddr.value().getNetworkAddress ( 24 );

    NetManager::get().addRoute ( subnet, 24, IpAddress(), _tuns[ 0 ]->getIfaceId() );

    if ( optDestIpPort.address().isValid() && optDestIpPort.port() > 0 )
    {
        const char * payload = "abcdef";

        MemHandle mh ( strlen ( payload ) );
        memcpy ( mh.getWritable(), payload, mh.size() );

        subnet.incrementBy ( 10 );

        _pkt = UdpPacket ( subnet, 9999, optDestIpPort.address(), optDestIpPort.port(), mh );

        _timer.start ( optSendDelay.value() );
    }

    return true;
}

void TunTest::tunIfaceClosed ( TunIface * iface )
{
    ( void ) iface;

    LOG ( L_INFO, "TunIface[" << getTunDesc ( iface ) << "] closed" );
}

void TunTest::tunIfaceRateUpdate ( TunIface * iface, uint32_t sendRate, uint32_t recvRate, const Time & sinceTime )
{
    ( void ) iface;
    ( void ) sendRate;
    ( void ) recvRate;
    ( void ) sinceTime;

    LOG ( L_DEBUG2, "TunIface[" << getTunDesc ( iface )
          << "] rate update; Last reported time point (sec): " << sinceTime.getSeconds()
          << " sec; Send rate: " << sendRate
          << " bytes/sec; Recv rate: " << recvRate << " bytes/sec" );
}

void TunTest::tunIfaceRead ( TunIface * iface, TunIpPacket & pkt )
{
    ( void ) iface;
    ( void ) pkt;

    LOG ( L_DEBUG, "TunIface[" << getTunDesc ( iface ) << "] received packet: " << pkt );
}

void TunTest::timerExpired ( Timer * )
{
    // When using the vhost implementation, the tunnel normally only tells the system that there's data
    // for it to send at the end of loop.
    //
    // So send a few packets to verify that that multiple packets are actually sent when we tell the system (only once)
    // that there's data to send.

    TunIface * tun = 0;

    if ( _tuns.isEmpty() || !( tun = _tuns[ 0 ] ) )
    {
        return;
    }

    tun->sendPacket ( _pkt );
    tun->sendPacket ( _pkt );
    tun->sendPacket ( _pkt );

    _timer.start ( optSendDelay.value() );
}
