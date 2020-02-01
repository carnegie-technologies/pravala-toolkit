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

#include "Socks5RequestMessage.hpp"

using namespace Pravala;

// First three fields in Socks5AddrMessage are:
// - The SOCKS protocol version number
// - The proxy command requested
// - Empty reserved field

Socks5RequestMessage::Socks5RequestMessage ( Socks5RequestMessage::Command command, const SockAddr & destAddr ):
    Socks5AddrMessage ( Socks5Version, ( uint8_t ) command, Socks5ReservedVal, destAddr )
{
    assert ( command >= 0 );
    assert ( ( ( int ) command ) <= 0xFF );

    if ( command < 0 || ( ( int ) command ) > 0xFF )
    {
        LOG ( L_ERROR, getLogId() << ": Command (" << command << ") is invalid; Not generating the message" );

        _data.clear();
        return;
    }
}

Socks5RequestMessage::Socks5RequestMessage()
{
}

bool Socks5RequestMessage::isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const
{
    if ( !msg )
    {
        return false;
    }
    else if ( msg->u.req.ver != Socks5Version )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid version: " << msg->u.req.ver );
        return false;
    }
    else if ( !isValidCommand ( msg->u.req.cmd ) )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid command: " << msg->u.req.cmd );
        return false;
    }

    return true;
}

uint8_t Socks5RequestMessage::getCommand() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    return ( ( msg != 0 ) ? ( msg->u.req.cmd ) : 0 );
}

String Socks5RequestMessage::getLogId() const
{
    return "Request";
}

void Socks5RequestMessage::describe ( Buffer & toBuffer ) const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    if ( !isValid() || !msg )
    {
        toBuffer.append ( "Invalid message" );
        return;
    }

    assert ( getSize() >= sizeof ( struct Socks5AddrMsgBase ) );

    toBuffer.append ( "SOCKS Version: " );
    toBuffer.append ( String::number ( msg->u.req.ver ) );
    toBuffer.append ( "; Command: " );
    toBuffer.append ( String::number ( getCommand() ) );
    toBuffer.append ( "; Reserved: " );
    toBuffer.append ( String::number ( msg->u.req.rsv ) );
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
