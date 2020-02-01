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

#include "TunIpPacket.hpp"

using namespace Pravala;

TunIpPacketData::TunIpPacketData(): tag ( 0 )
{
}

void TunIpPacketData::clear()
{
    tag = 0;
}

TunIpPacket::TunIpPacket()
{
}

TunIpPacket::TunIpPacket ( const MemHandle & data ):
    IpPacket ( data )
{
}

TunIpPacket::TunIpPacket ( const MemHandle & data, const TunIpPacketData & tunData ):
    IpPacket ( data ), _tunData ( tunData )
{
}

TunIpPacket::TunIpPacket ( const IpPacket & ipPacket ): IpPacket ( ipPacket )
{
}

TunIpPacket & TunIpPacket::operator= ( const IpPacket & ipPacket )
{
    if ( &ipPacket == ( const IpPacket * ) this )
        return *this;

    _tunData.clear();
    IpPacket::operator= ( ipPacket );

    return *this;
}

void TunIpPacket::clear()
{
    _tunData.clear();
    IpPacket::clear();
}

TextMessage & Pravala::operator<< ( TextMessage & textMessage, const TunIpPacket & value )
{
    operator<< ( textMessage, ( const IpPacket & ) value );

    textMessage.getInternalBuf().append ( String ( "; Tag: %1" ).arg ( value.getTag() ) );

    return textMessage;
}
