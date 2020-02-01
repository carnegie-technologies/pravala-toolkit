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

using namespace Pravala;

PacketWriter::PacketWriter ( WriterType wType, uint16_t, uint16_t, uint16_t ): CorePacketWriter ( wType )
{
}

PacketWriter::~PacketWriter()
{
    clearFd();
}

void PacketWriter::setupFd ( int fileDesc )
{
    _fd = fileDesc;
}

void PacketWriter::clearFd()
{
    _fd = -1;
}

ERRCODE PacketWriter::write ( MemHandle & data )
{
    const ERRCODE eCode = doWrite ( EmptySockAddress, data.get(), data.size() );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( const SockAddr & addr, MemHandle & data )
{
    if ( Type != SocketWriter )
    {
        return Error::Unsupported;
    }

    if ( !addr.hasPort() || !addr.hasIpAddr() || addr.hasZeroIpAddr() )
    {
        return Error::InvalidAddress;
    }

    const ERRCODE eCode = doWrite ( addr, data.get(), data.size() );

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

    MemHandle contMem;

    if ( data.getNumChunks() > 1 )
    {
        // We only want to pre-allocate memory if there are multiple chunks.
        contMem = PacketDataStore::getPacket ( data.getDataSize() );
    }

    if ( !data.storeContinuous ( contMem ) )
    {
        return Error::MemoryError;
    }

    assert ( contMem.size() == data.getDataSize() );

    const ERRCODE eCode = doWrite ( EmptySockAddress, contMem.get(), contMem.size() );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( const SockAddr & addr, MemVector & data )
{
    if ( Type != SocketWriter )
    {
        return Error::Unsupported;
    }

    if ( !addr.hasPort() || !addr.hasIpAddr() || addr.hasZeroIpAddr() )
    {
        return Error::InvalidAddress;
    }

    if ( !isValid() )
    {
        // This will be checked later too, but generating continuous memory is expensive,
        // so we don't want to do that if we are about to fail anyway!
        return Error::Closed;
    }

    MemHandle contMem;

    if ( data.getNumChunks() > 1 )
    {
        // We only want to pre-allocate memory if there are multiple chunks.
        contMem = PacketDataStore::getPacket ( data.getDataSize() );
    }

    if ( !data.storeContinuous ( contMem ) )
    {
        return Error::MemoryError;
    }

    assert ( contMem.size() == data.getDataSize() );

    const ERRCODE eCode = doWrite ( addr, contMem.get(), contMem.size() );

    if ( IS_OK ( eCode ) )
    {
        data.clear();
    }

    return eCode;
}

ERRCODE PacketWriter::write ( const char * data, size_t dataSize )
{
    return doWrite ( EmptySockAddress, data, dataSize );
}

ERRCODE PacketWriter::write ( const SockAddr & addr, const char * data, size_t dataSize )
{
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
