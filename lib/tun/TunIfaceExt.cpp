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

#include "TunIfaceExt.hpp"

using namespace Pravala;

TunIfaceExt::TunIfaceExt ( TunIfaceOwner * owner ):
    TunIface ( owner )
{
}

TunIfaceExt::~TunIfaceExt()
{
    // We HAVE to call it here.
    // TunIface's destructor would call only its own version!
    stop();
}

ERRCODE TunIfaceExt::startUnmanaged ( int /*fd*/, const HashSet<IpAddress> & /*ipAddresses*/, int /*ifaceMtu*/ )
{
    return Error::Unsupported;
}

ERRCODE TunIfaceExt::startManaged ( int /*ifaceMtu*/ )
{
    return Error::Unsupported;
}

bool TunIfaceExt::isManaged()
{
    return false;
}

bool TunIfaceExt::isInitialized() const
{
    return false;
}

int TunIfaceExt::getIfaceId() const
{
    return -1;
}

const String & TunIfaceExt::getIfaceName() const
{
    return String::EmptyString;
}

ERRCODE TunIfaceExt::sendPacket ( const IpPacket & packet )
{
    assert ( packet.isValid() );

    if ( !packet.isValid() )
        return Error::InvalidParameter;

    // We'll count all packets, even if we decide to drop them later
    updateSendDataCount ( packet.getPacketSize() );

    IpAddress srcAddr, dstAddr;

    if ( !packet.getAddr ( srcAddr, dstAddr ) )
        return Error::InvalidAddress;

    assert ( srcAddr.getAddrType() == dstAddr.getAddrType() );

    LOG ( L_DEBUG4, "Packet to external tunnel: " << packet );

    return doSendPacket ( packet );
}
