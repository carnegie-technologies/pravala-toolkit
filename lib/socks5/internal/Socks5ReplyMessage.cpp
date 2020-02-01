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

#include "Socks5ReplyMessage.hpp"

using namespace Pravala;

// First three fields in Socks5AddrMessage are:
// - The SOCKS protocol version number
// - The proxy server's reply code
// - Empty reserved field

Socks5ReplyMessage::Socks5ReplyMessage ( Socks5ReplyMessage::Reply r, const SockAddr & boundAddr ):
    Socks5AddrMessage ( Socks5Version, ( uint8_t ) r, Socks5ReservedVal, boundAddr )
{
    assert ( r >= 0 );
    assert ( ( ( int ) r ) <= 0xFF );

    if ( r < 0 || ( ( int ) r ) > 0xFF )
    {
        LOG ( L_ERROR, getLogId() << ": Reply code (" << r << ") is invalid; Not generating the message" );

        _data.clear();
        return;
    }
}

Socks5ReplyMessage::Socks5ReplyMessage()
{
}

bool Socks5ReplyMessage::isAddrMsgDataValid ( const struct Socks5AddrMsgBase * msg ) const
{
    if ( !msg )
    {
        return false;
    }
    else if ( msg->u.reply.ver != Socks5Version )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid version: " << msg->u.reply.ver );
        return false;
    }
    else if ( msg->u.reply.rsv != Socks5ReservedVal )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid reserved field: " << msg->u.reply.rsv );
        return false;
    }

    return true;
}

uint8_t Socks5ReplyMessage::getReply() const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    return ( ( msg != 0 ) ? ( msg->u.reply.rep ) : 0 );
}

String Socks5ReplyMessage::getLogId() const
{
    return "Reply";
}

void Socks5ReplyMessage::describe ( Buffer & toBuffer ) const
{
    const struct Socks5AddrMsgBase * const msg = getMessage<struct Socks5AddrMsgBase>();

    if ( !isValid() || !msg )
    {
        toBuffer.append ( "Invalid message" );
        return;
    }

    assert ( getSize() >= sizeof ( struct Socks5AddrMsgBase ) );

    toBuffer.append ( "SOCKS Version: " );
    toBuffer.append ( String::number ( msg->u.reply.ver ) );
    toBuffer.append ( "; Reply: " );
    toBuffer.append ( String::number ( getReply() ) );
    toBuffer.append ( "; Reserved: " );
    toBuffer.append ( String::number ( msg->u.reply.rsv ) );
    toBuffer.append ( "; Address type: " );
    toBuffer.append ( String::number ( getAddressType() ) );

    switch ( ( Socks5Message::AddressType ) getAddressType() )
    {
        case AddrIPv4:
        case AddrIPv6:
            toBuffer.append ( "; Bound address: " );
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
