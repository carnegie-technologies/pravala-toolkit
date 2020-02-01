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

#include <cassert>
#include <cstdio>

#include "String.hpp"
#include "IpSubnet.hpp"

using namespace Pravala;

IpSubnet::IpSubnet(): _prefixLength ( 0 )
{
}

IpSubnet::IpSubnet ( const IpSubnet & other ): _address ( other._address ), _prefixLength ( other._prefixLength )
{
}

IpSubnet::IpSubnet ( const IpAddress & addr, uint8_t prefixLength ): _address ( addr ), _prefixLength ( prefixLength )
{
    switch ( _address.getAddrType() )
    {
        case IpAddress::V4Address:
            if ( _prefixLength > 32 )
                _prefixLength = 32;
            break;

        case IpAddress::V6Address:
            if ( _prefixLength > 128 )
                _prefixLength = 128;
            break;

        default:
            _prefixLength = 0;
            break;
    }
}

IpSubnet::IpSubnet ( const IpAddress & addr ):
    _address ( addr ),
    _prefixLength ( addr.isIPv4() ? ( 32 ) : ( addr.isIPv6() ? ( 128 ) : ( 0 ) ) )
{
}

IpSubnet::IpSubnet ( const String & str ): _prefixLength ( 0 )
{
    if ( !setFromString ( str ) )
    {
        // Shouldn't be needed, but let's make sure we are invalid:
        clear();
    }
}

IpSubnet & IpSubnet::operator= ( const IpSubnet & other )
{
    if ( &other != this )
    {
        _address = other._address;
        _prefixLength = other._prefixLength;
    }

    return *this;
}

IpSubnet & IpSubnet::operator= ( const IpAddress & addr )
{
    _address = addr;
    _prefixLength = ( addr.isIPv4() ? ( 32 ) : ( addr.isIPv6() ? ( 128 ) : ( 0 ) ) );

    return *this;
}

bool IpSubnet::setFromString ( const String & str )
{
    const StringList vals = str.split ( "/", true );

    if ( vals.size() != 2 || vals.at ( 0 ).isEmpty() || vals.at ( 1 ).isEmpty() )
    {
        return false;
    }

    const IpAddress addr ( vals.at ( 0 ) );

    if ( !addr.isValid() || ( !addr.isIPv4() && !addr.isIPv6() ) )
    {
        return false;
    }

    bool ok = false;
    const uint8_t prefLen = vals.at ( 1 ).toUInt8 ( &ok );

    if ( !ok || ( addr.isIPv4() && prefLen > 32 ) || ( addr.isIPv6() && prefLen > 128 ) )
    {
        return false;
    }

    if ( addr != addr.getNetworkAddress ( prefLen ) )
    {
        return false;
    }

    _address = addr;
    _prefixLength = prefLen;

    return true;
}

String IpSubnet::toString ( bool includeIPv6Brackets ) const
{
    switch ( _address.getAddrType() )
    {
        case IpAddress::V4Address:
        case IpAddress::V6Address:
            return _address.toString ( includeIPv6Brackets ).append ( "/" ).append ( String::number ( _prefixLength ) );
            break;

        default:
            return "Invalid Subnet";
            break;
    }
}

String Pravala::toString ( const List<IpSubnet> & ipSubnetList )
{
    String ret ( "[" );

    for ( size_t i = 0; i < ipSubnetList.size(); ++i )
    {
        if ( i > 0 )
        {
            ret.append ( ", " );
        }

        ret.append ( ipSubnetList[ i ].toString() );
    }

    ret.append ( "]" );

    return ret;
}

size_t Pravala::getHash ( const IpSubnet & key )
{
    return ( key.getAddress().isValid()
             ? ( getHash ( key.getAddress() ) ^ key.getPrefixLength() )
             : 0 );
}
