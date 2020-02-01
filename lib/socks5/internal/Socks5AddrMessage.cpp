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

#include "socket/PacketDataStore.hpp"

#include "Socks5AddrMessage.hpp"

using namespace Pravala;

Socks5AddrMessage::Socks5AddrMessage()
{
}

Socks5AddrMessage::Socks5AddrMessage ( uint8_t fieldA, uint8_t fieldB, uint8_t fieldC, const SockAddr & addr )
{
    if ( !addr.hasIpAddr() )
    {
        // We cannot use getLogId() here (it is purely abstract at this point)

        LOG ( L_ERROR, "SockAddr has no IP address; Not generating the message" );
        return;
    }

    _data = PacketDataStore::getPacket();

    // PacketDataStore should really generate enough memory for both types of requests.
    // To make things easier, we check both first, and then we assume everything is correct.

    struct Socks5AddrMsgBase * const msg = getMessage<Socks5AddrMsgBase>();
    struct Socks5AddrMsgV4 * const v4Msg = getMessage<Socks5AddrMsgV4>();
    struct Socks5AddrMsgV6 * const v6Msg = getMessage<Socks5AddrMsgV6>();

    if ( !msg || !v4Msg || !v6Msg )
    {
        // We cannot use getLogId() here (it is purely abstract at this point)

        // (v6 should be bigger)
        LOG ( L_ERROR, "PacketDataStore generated less memory than required; Generated: " << _data.size()
              << "; Required: " << sizeof ( Socks5AddrMsgV6 ) << "; Not generating the message" );

        _data.clear();
        return;
    }

    assert ( msg != 0 );

    msg->u.base.fields[ 0 ] = fieldA;
    msg->u.base.fields[ 1 ] = fieldB;
    msg->u.base.fields[ 2 ] = fieldC;

    if ( addr.isIPv4() )
    {
        assert ( v4Msg != 0 );

        v4Msg->u.base.atyp = ( uint8_t ) Socks5Message::AddrIPv4;
        v4Msg->addr = addr.getAddr().getV4();
        v4Msg->port = htons ( addr.getPort() );

        _data.truncate ( sizeof ( *v4Msg ) );
    }
    else if ( addr.isIPv6() )
    {
        assert ( v6Msg != 0 );

        v6Msg->u.base.atyp = ( uint8_t ) Socks5Message::AddrIPv6;
        v6Msg->addr = addr.getAddr().getV6();
        v6Msg->port = htons ( addr.getPort() );

        _data.truncate ( sizeof ( *v6Msg ) );
    }
    else
    {
        // We cannot use getLogId() here (it is purely abstract at this point)

        LOG ( L_ERROR, "The address (" << addr << ") is invalid; Not generating the message" );

        _data.clear();
    }
}

ERRCODE Socks5AddrMessage::parseAndConsume ( MemHandle & data, size_t & bytesNeeded )
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase> ( data, bytesNeeded );

    if ( !msg )
    {
        return Error::IncompleteData;
    }
    else if ( !isAddrMsgDataValid ( msg ) )
    {
        return Error::InvalidData;
    }
    else if ( !isValidAddressType ( msg->u.base.atyp ) )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid address type: " << msg->u.base.atyp );
        return Error::InvalidData;
    }

    switch ( ( Socks5Message::AddressType ) msg->u.base.atyp )
    {
        case AddrIPv4:
            return setAndConsume<Socks5AddrMsgV4> ( data, bytesNeeded );
            break;

        case AddrIPv6:
            return setAndConsume<Socks5AddrMsgV6> ( data, bytesNeeded );
            break;

        case AddrDomainName:
            /// @TODO domain names
            LOG ( L_ERROR, getLogId() << ": Domain names are not implemented yet" );
            break;
    }

    return Error::NotImplemented;
}

uint8_t Socks5AddrMessage::getAddressType() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    return ( ( msg != 0 ) ? ( msg->u.base.atyp ) : 0 );
}

IpAddress Socks5AddrMessage::getAddress() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    if ( !msg )
        return IpAddress::IpEmptyAddress;

    switch ( ( Socks5Message::AddressType ) msg->u.base.atyp )
    {
        case AddrIPv4:
            if ( getSize() >= sizeof ( struct Socks5AddrMsgV4 ) )
            {
                return IpAddress ( ( ( const struct Socks5AddrMsgV4 * ) msg )->addr );
            }
            break;

        case AddrIPv6:
            if ( getSize() >= sizeof ( struct Socks5AddrMsgV6 ) )
            {
                return IpAddress ( ( ( const struct Socks5AddrMsgV6 * ) msg )->addr );
            }
            break;

        case AddrDomainName:
            /// @TODO domain names
            LOG ( L_ERROR, getLogId() << ": Asked for IP address of message that uses a domain name" );
            break;
    }

    return IpAddress::IpEmptyAddress;
}

uint16_t Socks5AddrMessage::getPort() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    if ( !msg )
        return 0;

    switch ( ( Socks5Message::AddressType ) msg->u.base.atyp )
    {
        case AddrIPv4:
            if ( getSize() >= sizeof ( struct Socks5AddrMsgV4 ) )
            {
                return ntohs ( ( ( const struct Socks5AddrMsgV4 * ) msg )->port );
            }
            break;

        case AddrIPv6:
            if ( getSize() >= sizeof ( struct Socks5AddrMsgV6 ) )
            {
                return ntohs ( ( ( const struct Socks5AddrMsgV6 * ) msg )->port );
            }
            break;

        case AddrDomainName:
            /// @TODO domain names
            LOG ( L_ERROR, getLogId() << ": Domain names are not implemented yet" );
            break;
    }

    return 0;
}
