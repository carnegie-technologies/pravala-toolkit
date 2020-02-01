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

// To be included by OS-specific implementations

// The number of bytes that should be ignored in each read from the tunnel.
// TUN_PREFIX_SIZE % 4 should be 0!
#ifndef TUN_PREFIX_SIZE
#define TUN_PREFIX_SIZE    4
#endif

#include "basic/Math.hpp"
#include "../../TunIfaceDev.hpp"

using namespace Pravala;

/// @brief A helper function that generates tunnel header data.
/// @param [in] addrFamily The family to include in the prefix (AF_INET, AF_INET6)
/// @return Tunnel prefix data for specified family.
static MemHandle getPrefix ( uint8_t addrFamily )
{
    MemHandle mh ( TUN_PREFIX_SIZE );
    char * const mem = mh.getWritable();

    if ( !mem || mh.size() != TUN_PREFIX_SIZE )
    {
        return MemHandle();
    }

    mem[ TUN_PREFIX_SIZE - 1 ] = addrFamily;
    return mh;
}

/// @brief The write prefix used for IPv4 packets.
static const MemHandle v4WritePrefix ( getPrefix ( AF_INET ) );

/// @brief The write prefix used for IPv6 packets.
static const MemHandle v6WritePrefix ( getPrefix ( AF_INET6 ) );

bool TunIfaceDev::osGetWriteData ( const IpPacket & ipPacket, MemVector & vec )
{
    switch ( ipPacket.getIpVersion() )
    {
        case 4:
            return ( vec.append ( v4WritePrefix ) && vec.append ( ipPacket.getPacketData() ) );
            break;

        case 6:
            return ( vec.append ( v6WritePrefix ) && vec.append ( ipPacket.getPacketData() ) );
            break;
    }

    return false;
}

bool TunIfaceDev::osRead ( MemHandle & data )
{
    assert ( TUN_PREFIX_SIZE % 4 == 0 );

    char * const w = data.getWritable();

    if ( !w || data.size() <= TUN_PREFIX_SIZE )
    {
        LOG ( L_ERROR, "Not enough memory provided" );

        data.clear();
        return false;
    }

    // And here we read the max packet size plus the number of bytes we will be ignoring:

    const ssize_t ret = ::read ( _fd, w, data.size() );

    if ( ret == 0 )
    {
        LOG ( L_ERROR, "Tunnel interface has been closed" );

        data.clear();
        return false;
    }

    if ( ret > 0 )
    {
        assert ( ( size_t ) ret <= data.size() );

        data.truncate ( ret );
        data.consume ( TUN_PREFIX_SIZE );

        assert ( ( ( size_t ) data.get() ) % 4U == 0 );

        return true;
    }

    data.clear();

    if ( errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
         && errno != EWOULDBLOCK
#endif
    )
    {
        LOG ( L_ERROR, "Error reading from the tunnel device: " << strerror ( errno ) << "; Closing the tunnel" );

        return false;
    }

    // read returned an error, but it is EAGAIN, so not really critical...
    return true;
}
