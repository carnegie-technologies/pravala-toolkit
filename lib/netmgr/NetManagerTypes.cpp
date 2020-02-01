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

#include "NetManagerTypes.hpp"

using namespace Pravala;
using namespace Pravala::NetManagerTypes;

Address::Address(): ifaceId ( 0 ), prefixLen ( 0 )
{
}

bool Address::operator== ( const Address & other ) const
{
    return (
        ifaceId == other.ifaceId
        && localAddress == other.localAddress
        && broadcastAddress == other.broadcastAddress
    );
}

size_t Pravala::NetManagerTypes::getHash ( const Address & key )
{
    return (
        Pravala::getHash ( key.ifaceId )
        ^ Pravala::getHash ( key.localAddress )
        ^ Pravala::getHash ( key.broadcastAddress )
    );
}

Route::Route():
    ifaceIdIn ( 0 ),
    ifaceIdOut ( 0 ),
    metric ( 0 ),
    dstPrefixLen ( 0 ),
    srcPrefixLen ( 0 ),
    table ( 0 ),
    routingProtocol ( 0 )
{
}

bool Route::operator== ( const Route & other ) const
{
    return (
        ifaceIdIn == other.ifaceIdIn
        && ifaceIdOut == other.ifaceIdOut
        && metric == other.metric
        && dstPrefixLen == other.dstPrefixLen
        && srcPrefixLen == other.srcPrefixLen
        && table == other.table
        && routingProtocol == other.routingProtocol
        && src == other.src
        && dst == other.dst
        && gw == other.gw
    );
}

size_t Pravala::NetManagerTypes::getHash ( const Route & key )
{
    return (
        Pravala::getHash ( key.ifaceIdIn )
        ^ Pravala::getHash ( key.ifaceIdOut )
        ^ Pravala::getHash ( key.metric )
        ^ Pravala::getHash ( key.dstPrefixLen )
        ^ Pravala::getHash ( key.srcPrefixLen )
        ^ Pravala::getHash ( key.table )
        ^ Pravala::getHash ( key.routingProtocol )
        ^ Pravala::getHash ( key.src )
        ^ Pravala::getHash ( key.dst )
        ^ Pravala::getHash ( key.gw )
    );
}

Interface::Interface():
    type ( 0 ),
    id ( 0 ),
    realId ( 0 ),
    hwAddrLen ( 0 ),
    hwBroadcastAddrLen ( 0 ),
    mtu ( 0 ),
    flags ( 0 )
{
    memset ( hwAddr, 0, sizeof ( hwAddr ) );
    memset ( hwBroadcastAddr, 0, sizeof ( hwBroadcastAddr ) );
}
