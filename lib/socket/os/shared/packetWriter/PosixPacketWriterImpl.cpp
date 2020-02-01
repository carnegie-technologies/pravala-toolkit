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
#include "socket/PacketWriter.hpp"

#include "PosixPacketWriter.cpp"

using namespace Pravala;

PacketWriter::PacketWriter ( WriterType wType, uint16_t flags, uint16_t queueSize, uint16_t speedLimit ):
    PosixPacketWriter ( wType, flags, queueSize, speedLimit )
{
}

PacketWriter::~PacketWriter()
{
}

void PacketWriter::setupFd ( int fileDesc )
{
    PosixPacketWriter::configureFd ( fileDesc );
}

void PacketWriter::clearFd()
{
    PosixPacketWriter::configureFd ( -1 );
}

ERRCODE PacketWriter::write ( const char * data, size_t dataSize )
{
    // To use any of the advanced write features we would need to copy the memory.
    // We really don't want to do that, so let's just write the data to socket directly.
    // It may result in some reordering, but this is UDP, so it's not a big deal.
    // Also, this is an API to support "foreign" data sources (like data encrypted using DTLS),
    // and the same type of data will be sent using this API, so there will be no reordering with this data "stream".
    return doWrite ( EmptySockAddress, data, dataSize );
}

ERRCODE PacketWriter::write ( const SockAddr & addr, const char * data, size_t dataSize )
{
    // To use any of the advanced write features we would need to copy the memory.
    // We really don't want to do that, so let's just write the data to socket directly.
    // It may result in some reordering, but this is UDP, so it's not a big deal.
    // Also, this is an API to support "foreign" data sources (like data encrypted using DTLS),
    // and the same type of data will be sent using this API, so there will be no reordering with this data "stream".

    if ( Type != SocketWriter )
    {
        return Error::Unsupported;
    }

    if ( !addr.hasPort() || !addr.hasIpAddr() || addr.hasZeroIpAddr() )
    {
        return Error::InvalidAddress;
    }

    return doWrite ( addr, data, dataSize );
}

ERRCODE PacketWriter::write ( MemHandle & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( data.isEmpty() )
    {
        // Nothing to send
        return Error::Success;
    }

    MemVector vec;

    if ( !vec.append ( data ) )
    {
        return Error::MemoryError;
    }

    const ERRCODE eCode = writePacket ( EmptySockAddress, vec );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( const SockAddr & addr, MemHandle & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( Type != SocketWriter )
    {
        return Error::Unsupported;
    }

    if ( !addr.hasPort() || !addr.hasIpAddr() || addr.hasZeroIpAddr() )
    {
        return Error::InvalidAddress;
    }

    MemVector vec;

    if ( !vec.append ( data ) )
    {
        return Error::MemoryError;
    }

    const ERRCODE eCode = writePacket ( addr, vec );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( MemVector & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    const ERRCODE eCode = writePacket ( EmptySockAddress, data );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( const SockAddr & addr, MemVector & data )
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( Type != SocketWriter )
    {
        return Error::Unsupported;
    }

    if ( !addr.hasPort() || !addr.hasIpAddr() || addr.hasZeroIpAddr() )
    {
        return Error::InvalidAddress;
    }

    const ERRCODE eCode = writePacket ( addr, data );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}
