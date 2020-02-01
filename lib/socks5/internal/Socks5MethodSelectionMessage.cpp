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

#include "basic/String.hpp"
#include "socket/PacketDataStore.hpp"

#include "Socks5MethodSelectionMessage.hpp"

using namespace Pravala;

#pragma pack(push, 1)

/// @brief Struct used to define the fields for SOCKS5 method selection messages
struct Socks5Method
{
    uint8_t ver;        ///< The SOCKS protocol version number
    uint8_t method;     ///< The chosen authentication method
};

#pragma pack(pop)

Socks5MethodSelectionMessage::Socks5MethodSelectionMessage()
{
}

Socks5MethodSelectionMessage::Socks5MethodSelectionMessage ( Socks5Message::AuthenticationMethod a )
{
    assert ( a >= 0 );
    assert ( a <= 0xFF );

    if ( a < 0 || a > 0xFF )
    {
        return;
    }

    const size_t totalSize = sizeof ( Socks5Method );

    _data = PacketDataStore::getPacket ( totalSize );

    // NOTE: getMessage() casts memory in _data:
    struct Socks5Method * const msg = getMessage<Socks5Method>();

    if ( _data.size() < totalSize || !msg )
    {
        LOG ( L_ERROR, getLogId()
              << ": PacketDataStore generated less memory than required; Generated: " << _data.size()
              << "; Required: " << totalSize << "; Not generating the message" );
        _data.clear();
        return;
    }

    msg->ver = Socks5Version;
    msg->method = ( uint8_t ) a;

    _data.truncate ( totalSize );

    assert ( _data.size() == totalSize );
}

ERRCODE Socks5MethodSelectionMessage::parseAndConsume ( MemHandle & data, size_t & bytesNeeded )
{
    const struct Socks5Method * const msg = getMessage<Socks5Method> ( data, bytesNeeded );

    if ( !msg )
    {
        return Error::IncompleteData;
    }
    else if ( msg->ver != Socks5Version )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid version: " << msg->ver );
        return Error::InvalidData;
    }

    return setAndConsume<Socks5Method> ( data, bytesNeeded );
}

uint8_t Socks5MethodSelectionMessage::getAuthenticationMethod() const
{
    const struct Socks5Method * const msg = getMessage<Socks5Method>();

    return ( ( msg != 0 ) ? ( msg->method ) : 0 );
}

String Socks5MethodSelectionMessage::getLogId() const
{
    return "Method-Selection";
}

void Socks5MethodSelectionMessage::describe ( Buffer & toBuffer ) const
{
    const struct Socks5Method * const msg = getMessage<Socks5Method>();

    if ( !isValid() || !msg )
    {
        toBuffer.append ( "Invalid message" );
        return;
    }

    assert ( getSize() >= sizeof ( Socks5Method ) );

    toBuffer.append ( "SOCKS Version: " );
    toBuffer.append ( String::number ( msg->ver ) );
    toBuffer.append ( "; Chosen auth method: " );
    toBuffer.append ( String::number ( getAuthenticationMethod() ) );
}
