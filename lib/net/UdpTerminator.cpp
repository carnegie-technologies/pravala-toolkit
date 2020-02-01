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

#include "net/UdpPacket.hpp"

#include "UdpTerminator.hpp"

using namespace Pravala;

// Max is one month. Just so there is a limit.
// Otherwise it would be possible to configure a value that overflows when converted to 'ms' in uint32.
ConfigLimitedNumber<uint32_t> UdpTerminator::optMaxInactivityTime (
        0,
        "mas.udp_terminator.max_inactivity_time",
        "The time of inactivity (in seconds) after the UDP socket terminator will be closed; 0 to disable",
        0, 31 * 24 * 60 * 60, 60
);

TextLogLimited UdpTerminator::_log ( "udp_terminator" );

UdpTerminator::UdpTerminator ( const FlowDesc & flowDesc ):
    IpFlow ( flowDesc ),
    ClientAddr ( ( flowDesc.common.type == 4 )
            ? IpAddress ( flowDesc.v4.clientAddr )
            : IpAddress ( flowDesc.v6.clientAddr ) ),
    ServerAddr ( ( flowDesc.common.type == 4 )
            ? IpAddress ( flowDesc.v4.serverAddr )
            : IpAddress ( flowDesc.v6.serverAddr ) ),
    ClientPort ( ntohs ( flowDesc.common.u.port.client ) ),
    ServerPort ( ntohs ( flowDesc.common.u.port.server ) ),
    _timer ( *this, optMaxInactivityTime.value() * 1000 )
{
    assert ( flowDesc.common.type == 4 || flowDesc.common.type == 6 );
    assert ( flowDesc.common.heProto == UdpPacket::ProtoNumber );

    restartTimer();

    LOG ( L_DEBUG, getLogId() << ": New UDP terminator created" );
}

UdpTerminator::~UdpTerminator()
{
}

String UdpTerminator::getLogId() const
{
    return String ( "[%1:%2-%3:%4]" )
           .arg ( ClientAddr.toString() )
           .arg ( ClientPort )
           .arg ( ServerAddr.toString() )
           .arg ( ServerPort );
}

void UdpTerminator::timerExpired ( Timer * timer )
{
    assert ( timer == &_timer );
    ( void ) timer;

    LOG ( L_DEBUG, getLogId() << ": UDP terminator removed due to inactivity" );

    // It is safe to self-remove UDP terminator.
    // Its destructor will remove it from the flow map, and it should only ever be stored in that map
    // and accessed through it.

    delete this;
}

void UdpTerminator::restartTimer()
{
    if ( _timer.FixedTimeout > 0 )
    {
        _timer.start();
    }
}

void UdpTerminator::flowRemoved()
{
    // This is some cleanup, or a shutdown.

    LOG ( L_DEBUG2, getLogId() << ": UDP terminator removed" );

    // It is safe to self-remove UDP terminator.
    // Its destructor will remove it from the flow map, and it should only ever be stored in that map
    // and accessed through it.

    delete this;
}

ERRCODE UdpTerminator::sendData ( MemHandle & data )
{
    if ( data.isEmpty() )
    {
        LOG ( L_ERROR, getLogId() << ": Not sending empty data packet" );
        return Error::EmptyWrite;
    }

    restartTimer();

    const UdpPacket packet ( ServerAddr, ServerPort, ClientAddr, ClientPort, data );
    data.clear();

    const ERRCODE eCode = sendPacket ( packet );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending UDP packet [" << packet << "] over the tunnel interface" );
    }
    else
    {
        LOG ( L_DEBUG2, getLogId() << ": Successfully sent UDP packet: " << packet );
    }

    return eCode;
}

ERRCODE UdpTerminator::packetReceived ( IpPacket & ipPacket, int32_t userData, void * /*userPtr*/ )
{
    if ( userData != DefaultDescType )
    {
        LOG ( L_WARN, getLogId() << ": Received an IP packet in the wrong direction ("
              << userData << ", expected " << DefaultDescType << "): " << ipPacket << "; Dropping" );

        return Error::InvalidParameter;
    }

    MemVector udpPayload;

    if ( !ipPacket.getProtoPayload<UdpPacket> ( udpPayload ) )
    {
        LOG ( L_WARN, getLogId() << ": Could not extract UDP payload from packet: " << ipPacket << "; Dropping" );
        return Error::InvalidData;
    }

    restartTimer();

    return receiveData ( udpPayload );
}
