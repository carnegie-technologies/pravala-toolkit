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

#include "basic/IpAddress.hpp"

#include "FlowDesc.hpp"
#include "IcmpPacket.hpp"
#include "UdpPacket.hpp"
#include "TcpPacket.hpp"

#ifdef __SSE4_2__

extern "C"
{
#include <cpuid.h>
}

bool Pravala::FlowDesc::detectSse42()
{
    assert ( sizeof ( V4Data ) == 16 );
    assert ( sizeof ( V6Data ) == 40 );
    assert ( sizeof ( FlowDesc ) == 40 );

#ifndef bit_SSE4_2
    //
    // TODO/FIXME
    //
    // If bit_SSE4_2 is not defined, we can't really run this test even though __SSE4_2__ is supported by the compiler.
    // Maybe we could use a different define or just use specific bit, but this hasn't been examined.
    // It is defined on Linux, and this is what we care about at this point.

    return false;
#else
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

    __get_cpuid ( 1, &eax, &ebx, &ecx, &edx );

    return ( ( ecx & bit_SSE4_2 ) != 0 );
#endif
}
#endif

using namespace Pravala;

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const FlowDesc & value )
{
    Buffer & buf = textMessage.getInternalBuf();

    buf.append ( ( value.common.type == 4 ) ? ( "IPv4" ) : ( ( value.common.type == 6 ) ? "IPv6" : "Ipv?" ) );
    buf.append ( " flow; Proto: " );
    buf.append ( String::number ( value.common.heProto ) );

    if ( value.common.heProto == UdpPacket::ProtoNumber || value.common.heProto == TcpPacket::ProtoNumber )
    {
        buf.append ( ( value.common.heProto == UdpPacket::ProtoNumber )
                     ? " (UDP); ClientPort: " : " (TCP); ClientPort: " );

        buf.append ( String::number ( ntohs ( value.common.u.port.client ) ) );
        buf.append ( "; ServerPort: " );
        buf.append ( String::number ( ntohs ( value.common.u.port.server ) ) );
    }
    else
    {
        buf.append ( " (" );
        buf.append ( IpPacket::getProtoName ( value.common.heProto ) );
        buf.append ( ")" );
    }

    if ( value.common.type == 4 )
    {
        buf.append ( "; ClientAddr: " );
        buf.append ( IpAddress::toString ( value.v4.clientAddr ) );
        buf.append ( "; ServerAddr: " );
        buf.append ( IpAddress::toString ( value.v4.serverAddr ) );
    }
    else if ( value.common.type == 6 )
    {
        buf.append ( "; ClientAddr: " );
        buf.append ( IpAddress::toString ( value.v6.clientAddr ) );
        buf.append ( "; ServerAddr: " );
        buf.append ( IpAddress::toString ( value.v6.serverAddr ) );
    }

    buf.append ( "; Hash: 0x" );
    buf.append ( String::number ( value.getHash(), String::Int_HEX ) );

    return textMessage;
}
