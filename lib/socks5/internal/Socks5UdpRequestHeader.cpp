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

#include "Socks5UdpRequestHeader.hpp"

using namespace Pravala;

// First three fields in Socks5AddrMessage are:
// - Reserved. Must be equal to 0x00.
// - Reserved. Must be equal to 0x00.
// - The packet fragment number.

Socks5UdpRequestHeader::Socks5UdpRequestHeader ( const SockAddr & destAddr ):
    Socks5AddrMessage ( Socks5ReservedVal, Socks5ReservedVal, 0x00, destAddr )
{
}

Socks5UdpRequestHeader::Socks5UdpRequestHeader()
{
}

bool Socks5UdpRequestHeader::isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const
{
    if ( !msg )
    {
        return false;
    }
    else if ( msg->u.udpReq.rsv[ 0 ] != Socks5ReservedVal || msg->u.udpReq.rsv[ 1 ] != Socks5ReservedVal )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid reserved field(s): "
              << msg->u.udpReq.rsv[ 0 ] << "," << msg->u.udpReq.rsv[ 1 ] );
        return false;
    }

    return true;
}

uint8_t Socks5UdpRequestHeader::getFragment() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    return ( ( msg != 0 ) ? ( msg->u.udpReq.frag ) : 0 );
}

String Socks5UdpRequestHeader::getLogId() const
{
    return "UDP-Request";
}

void Socks5UdpRequestHeader::describe ( Buffer & toBuffer ) const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    if ( !isValid() || !msg )
    {
        toBuffer.append ( "Invalid message" );
        return;
    }

    assert ( getSize() >= sizeof ( struct Socks5AddrMsgBase ) );

    toBuffer.append ( "Reserved: " );
    toBuffer.append ( String::number ( msg->u.udpReq.rsv[ 0 ] ) );
    toBuffer.append ( "," );
    toBuffer.append ( String::number ( msg->u.udpReq.rsv[ 1 ] ) );
    toBuffer.append ( "; Fragment number: " );
    toBuffer.append ( String::number ( getFragment() ) );
    toBuffer.append ( "; Address type: " );
    toBuffer.append ( String::number ( getAddressType() ) );

    switch ( ( Socks5Message::AddressType ) getAddressType() )
    {
        case AddrIPv4:
        case AddrIPv6:
            toBuffer.append ( "; Destination address: " );
            toBuffer.append ( getAddress().toString ( true ) );
            toBuffer.append ( ":" );
            toBuffer.append ( String::number ( getPort() ) );
            break;

        case AddrDomainName:
            /// @TODO domain names
            toBuffer.append ( "; Domain name: not yet implemented" );
            break;
    }
}
