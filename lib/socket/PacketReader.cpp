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

#ifdef USE_UDP_IMPL_MMSG
extern "C"
{
#include "syscall/mmsghdr.h"

#ifndef HAVE_RECVMMSG
#include "syscall/recvmmsg.h"
#endif
}
#elif !defined( HAVE_RECVMMSG ) && !defined( HAVE_SENDMMSG ) && !defined( HAVE_MMSGHDR )
extern "C"
{
// Unused
struct mmsghdr
{
    int foo;
};
}
#endif

#include "basic/Math.hpp"
#include "sys/SocketApi.hpp"

#include "Socket.hpp"
#include "PacketDataStore.hpp"
#include "PacketReader.hpp"

using namespace Pravala;

#define MAX_PACKETS    1000

TextLogLimited PacketReader::_log ( "packet_reader" );

PacketReader::PacketReader ( uint16_t maxPackets ):
#ifdef USE_UDP_IMPL_MMSG
    MaxPackets ( limit<uint16_t> ( maxPackets, 1, MAX_PACKETS ) ),
    _recvMsgs ( new struct mmsghdr[ MaxPackets ] ),
    _recvIovecs ( new struct iovec[ MaxPackets ] ),
    _recvAddrs ( new SockAddr[ MaxPackets ] ),
    _recvData ( new MemHandle[ MaxPackets ] ),
#else
    MaxPackets ( 1 ),
    _recvMsgs ( 0 ),
    _recvIovecs ( 0 ),
    _recvAddrs ( new SockAddr[ MaxPackets ] ),
    _recvData ( new MemHandle[ MaxPackets ] ),
#endif
    _lastReadCount ( 0 )
{
    ( void ) maxPackets;

    assert ( _recvData != 0 );
    assert ( _recvAddrs != 0 );

#ifdef USE_UDP_IMPL_MMSG
    assert ( _recvMsgs != 0 );
    assert ( _recvIovecs != 0 );

    memset ( _recvMsgs, 0, MaxPackets * sizeof ( _recvMsgs[ 0 ] ) );
    memset ( _recvIovecs, 0, MaxPackets * sizeof ( _recvIovecs[ 0 ] ) );

    // Set up receive parameters that don't change, and the initial receive buffers
    for ( uint16_t i = 0; i < MaxPackets; ++i )
    {
        _recvData[ i ] = PacketDataStore::getPacket();

        _recvIovecs[ i ].iov_base = _recvData[ i ].getWritable();
        _recvIovecs[ i ].iov_len = _recvData[ i ].size();

        _recvMsgs[ i ].msg_hdr.msg_iov = &_recvIovecs[ i ];
        _recvMsgs[ i ].msg_hdr.msg_iovlen = 1;

        // msg_name expects a sockaddr struct to put the remote IP/port.
        // SockAddr is our union around sockaddr with some nice functions, so we can directly point it to that!
        _recvMsgs[ i ].msg_hdr.msg_name = &_recvAddrs[ i ];
        _recvMsgs[ i ].msg_hdr.msg_namelen = sizeof ( SockAddr );
    }
#else
    assert ( !_recvMsgs );
    assert ( !_recvIovecs );
    assert ( MaxPackets == 1 );

    _recvData[ 0 ] = PacketDataStore::getPacket();
#endif
}

PacketReader::~PacketReader()
{
    delete[] _recvMsgs;
    delete[] _recvIovecs;
    delete[] _recvAddrs;
    delete[] _recvData;
}

bool PacketReader::getPacket ( uint16_t idx, MemHandle & data, SockAddr & addr )
{
    if ( idx >= _lastReadCount || idx >= MaxPackets )
    {
        return false;
    }

    assert ( _recvData != 0 );
    assert ( _recvAddrs != 0 );

    if ( _recvData[ idx ].isEmpty() )
    {
        return false;
    }

    data = _recvData[ idx ];
    addr = _recvAddrs[ idx ];

    if ( addr.isIPv6MappedIPv4() )
    {
        addr.convertToV4();
    }

    _recvData[ idx ].clear();
    _recvAddrs[ idx ].clear();
    return true;
}

ERRCODE PacketReader::readPackets ( int fd, const LogId & logId, uint16_t & packetsRead )
{
    ( void ) logId;

    packetsRead = 0;

    if ( fd < 0 )
    {
        return Error::InvalidParameter;
    }

    // Let's regenerate the entries that were previously used:
    for ( uint16_t i = 0; i < _lastReadCount; ++i )
    {
        _recvData[ i ] = PacketDataStore::getPacket();
        _recvAddrs[ i ].clear();

#ifdef USE_UDP_IMPL_MMSG
        assert ( _recvMsgs != 0 );
        assert ( _recvIovecs != 0 );
        assert ( _recvMsgs[ i ].msg_hdr.msg_name == &_recvAddrs[ i ] );
        assert ( _recvMsgs[ i ].msg_hdr.msg_iov == &_recvIovecs[ i ] );
        assert ( _recvMsgs[ i ].msg_hdr.msg_iovlen == 1 );

        // It may have been set to something else in the previous read:
        _recvMsgs[ i ].msg_hdr.msg_namelen = sizeof ( SockAddr );

        _recvIovecs[ i ].iov_base = _recvData[ i ].getWritable();
        _recvIovecs[ i ].iov_len = _recvData[ i ].size();
#endif
    }

    _lastReadCount = 0;

    ssize_t ret = 0;

#ifndef USE_UDP_IMPL_MMSG
    MemHandle & recvData = _recvData[ 0 ];
    SockAddr & recvAddr = _recvAddrs[ 0 ];

    char * w = recvData.getWritable();

    if ( !w || recvData.size() < 1 )
    {
        return Error::InternalError;
    }

    socklen_t sockLen = sizeof ( recvAddr );

    if ( ( ret = ::recvfrom ( fd, w, recvData.size(), 0, &recvAddr.sa, &sockLen ) ) > 0 )
    {
        if ( ret < 1 || ( size_t ) ret >= recvData.size() )
        {
            // If we fill the entire buffer, it is likely that the message was truncated.
            // In general messages should be smaller than buffers generated by PacketDataStore,
            // so we should usually see less data generated. If we see a full buffer,
            // it's likely the message was too big.

            LOG_LIM ( L_WARN, logId.getLogId() << ": recvfrom() filled the entire data buffer (" << ret
                      << " bytes were generated); Most likely the message was truncated; Discarding packet: "
                      << recvData.getHexDump() );

            recvData.clear();
            return Error::SoftFail;
        }

        packetsRead = _lastReadCount = 1;
        recvData.truncate ( ( size_t ) ret );

        return Error::Success;
    }
#else
    if ( ( ret = ::recvmmsg ( fd, _recvMsgs, MaxPackets, 0, 0 ) ) > 0 )
    {
        if ( ret > 0xFFFF || ( uint16_t ) ret > MaxPackets )
        {
            return Error::InternalError;
        }

        for ( int i = 0; i < ret; ++i )
        {
            if ( _recvMsgs[ i ].msg_len >= _recvIovecs[ i ].iov_len )
            {
                // If we fill the entire buffer, it is likely that the message was truncated.
                // In general messages should be smaller than buffers generated by PacketDataStore,
                // so we should usually see less data generated. If we see a full buffer,
                // it's likely the message was too big.

                LOG_LIM ( L_WARN, logId.getLogId() << ": recvmmsg() filled the entire data buffer ("
                          << _recvMsgs[ i ].msg_len
                          << " bytes were generated); Most likely the message was truncated; Discarding packet: "
                          << _recvData[ i ].getHexDump() );

                _recvData[ i ].clear();
            }
            else
            {
                _recvData[ i ].truncate ( _recvMsgs[ i ].msg_len );
            }
        }

        packetsRead = _lastReadCount = ( uint16_t ) ret;
        return Error::Success;
    }
#endif

    if ( ret < 0 )
    {
        if ( SocketApi::isErrnoSoft() )
        {
            LOG_LIM ( L_WARN, logId.getLogId() << ": Received soft read error: " << SocketApi::getLastErrorDesc() );
            return Error::SoftFail;
        }
        else if ( SocketApi::isErrnoNonFatal() )
        {
            LOG_LIM ( L_ERROR, logId.getLogId() << ": Non-fatal error receiving data; Error: "
                      << SocketApi::getLastErrorDesc() );

            return Error::ReadFailed;
        }

        LOG_LIM ( L_ERROR, logId.getLogId() << ": Fatal error receiving data; Closing the socket; Error: "
                  << SocketApi::getLastErrorDesc() );
    }

    return Error::Closed;
}
