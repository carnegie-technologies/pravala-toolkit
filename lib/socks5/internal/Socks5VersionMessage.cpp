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

#include "Socks5VersionMessage.hpp"

using namespace Pravala;

#pragma pack(push, 1)

/// @brief Struct used to define the fields for SOCKS5 version identifier/method select messages
/// @note This struct is followed by an nmethods number of single byte authentication methods
struct Socks5VersionHeader
{
    uint8_t ver;               ///< The SOCKS protocol version number
    uint8_t nmethods;          ///< The number of authentication methods offered
};

#pragma pack(pop)

/// @brief Returns the total size of a version message that includes given number of methods.
/// @param [in] numMethods The number of methods included with the message.
/// @return The total size of a version message that incldues given number of methods.
inline static size_t getTotalSize ( uint8_t numMethods )
{
    return ( sizeof ( Socks5VersionHeader ) + numMethods );
}

Socks5VersionMessage::Socks5VersionMessage()
{
}

Socks5VersionMessage::Socks5VersionMessage ( const List<uint8_t> & authMethods )
{
    const size_t numMethods = authMethods.size();

    if ( numMethods > 255 )
    {
        LOG ( L_ERROR, getLogId() << ": " << numMethods << " authentication methods provided, max is 255" );
        return;
    }

    const size_t totalSize = getTotalSize ( numMethods );

    _data = PacketDataStore::getPacket ( totalSize );

    // NOTE: getMessage() casts memory in _data:
    struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader>();

    if ( _data.size() < totalSize || !msg )
    {
        LOG ( L_ERROR, getLogId()
              << ": PacketDataStore generated less memory than required; Generated: " << _data.size()
              << "; Required: " << totalSize << "; Not generating the message" );

        _data.clear();
        return;
    }

    // SOCKS version 5
    msg->ver = Socks5Version;
    msg->nmethods = numMethods;

    uint8_t * methods = ( uint8_t * ) msg + sizeof ( Socks5VersionHeader );

    for ( size_t i = 0; i < numMethods; ++i )
    {
        methods[ i ] = authMethods[ i ];
    }

    _data.truncate ( totalSize );

    assert ( _data.size() == totalSize );
}

ERRCODE Socks5VersionMessage::parseAndConsume ( MemHandle & data, size_t & bytesNeeded )
{
    const struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader> ( data, bytesNeeded );

    if ( !msg )
    {
        return Error::IncompleteData;
    }
    else if ( Socks5Version != msg->ver )
    {
        LOG ( L_ERROR, getLogId() << ": Can't parse data, invalid SOCKS version: " << msg->ver );
        return Error::InvalidData;
    }

    LOG ( L_DEBUG4, getLogId() << ": Version: " << msg->ver << "; Num methods: " << msg->nmethods
          << "; Total size: " << getTotalSize ( msg->nmethods ) );

    return Socks5Message::setAndConsume ( data, getTotalSize ( msg->nmethods ), bytesNeeded );
}

uint8_t Socks5VersionMessage::getNumMethods() const
{
    const struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader>();

    return ( ( msg != 0 ) ? ( msg->nmethods ) : 0 );
}

bool Socks5VersionMessage::containsAuthMethod ( uint8_t authMethod ) const
{
    const struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader>();

    if ( !msg || msg->nmethods < 1 )
        return false;

    const uint8_t * methods = ( ( const uint8_t * ) msg ) + sizeof ( Socks5VersionHeader );

    for ( uint8_t i = 0; i < msg->nmethods; ++i )
    {
        if ( authMethod == methods[ i ] )
            return true;
    }

    return false;
}

uint8_t Socks5VersionMessage::getMethod ( uint8_t methodNumber ) const
{
    const struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader>();

    if ( !msg || methodNumber >= msg->nmethods )
        return 0;

    const uint8_t * methods = ( ( const uint8_t * ) msg ) + sizeof ( Socks5VersionHeader );

    return methods[ methodNumber ];
}

String Socks5VersionMessage::getLogId() const
{
    return "Version";
}

void Socks5VersionMessage::describe ( Buffer & toBuffer ) const
{
    const struct Socks5VersionHeader * const msg = getMessage<Socks5VersionHeader>();

    if ( !isValid() || !msg )
    {
        toBuffer.append ( "Invalid message" );
        return;
    }

    assert ( getSize() >= sizeof ( Socks5VersionHeader ) );

    toBuffer.append ( "SOCKS Version: " );
    toBuffer.append ( String::number ( msg->ver ) );
    toBuffer.append ( "; Auth methods: " );

    assert ( getSize() >= getTotalSize ( getNumMethods() ) );

    for ( uint8_t i = 0; i < msg->nmethods; ++i )
    {
        toBuffer.append ( String::number ( getMethod ( i ) ) );
        toBuffer.append ( ", " );
    }
}
