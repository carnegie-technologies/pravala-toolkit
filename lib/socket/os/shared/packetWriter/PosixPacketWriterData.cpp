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

#include <cerrno>
#include <cstdio>

#ifdef USE_UDP_IMPL_MMSG
extern "C"
{
#include "syscall/mmsghdr.h"

#ifndef HAVE_SENDMMSG
#include "syscall/sendmmsg.h"
#endif
}
#elif !defined( HAVE_RECVMMSG ) && !defined( HAVE_SENDMMSG ) && !defined( HAVE_MMSGHDR )
// Unused
struct mmsghdr
{
    uint32_t foo;
};
#endif

#include "PosixPacketWriterData.hpp"

#define MAX_QUEUE_SIZE    1024

using namespace Pravala;

#ifdef USE_UDP_IMPL_MMSG
// We want _data in threaded mode or in socket mode with multi-write:
#define WANT_DATA \
    ( ( flags & CorePacketWriter::FlagThreaded ) || ( IsSocketWriter && ( flags & CorePacketWriter::FlagMultiWrite ) ) )

// We want _dest only in socket mode, when we use threads or multi-write:
#define WANT_DEST \
    ( IsSocketWriter && ( ( flags & CorePacketWriter::FlagThreaded ) || ( flags & CorePacketWriter::FlagMultiWrite ) ) )

// We want _msgs only in socket mode when we use multi-write:
#define WANT_MSGS    ( IsSocketWriter && ( flags & CorePacketWriter::FlagMultiWrite ) )

#else
// We want _data only in threaded mode:
#define WANT_DATA    ( ( flags & CorePacketWriter::FlagThreaded ) )

// We want _dest only in threaded mode and only if we are writing over sockets:
#define WANT_DEST    ( ( flags & CorePacketWriter::FlagThreaded ) && ( IsSocketWriter ) )

// We never want _msgs:
#define WANT_MSGS    ( false )
#endif

PosixPacketWriterData::PosixPacketWriterData ( CorePacketWriter::WriterType wType, uint16_t flags, uint16_t queueSize ):
    IsSocketWriter ( wType == CorePacketWriter::SocketWriter ),
    QueueSize ( limit<uint16_t> ( queueSize, 1, MAX_QUEUE_SIZE ) ),
    _data ( WANT_DATA ? ( new MemVector[ QueueSize ] ) : 0 ),
    _dest ( WANT_DEST ? ( new SockAddr[ QueueSize ] ) : 0 ),
    _msgs ( WANT_MSGS ? ( new struct mmsghdr[ QueueSize ] ) : 0 )
{
}

PosixPacketWriterData::~PosixPacketWriterData()
{
    delete[] _data;
    delete[] _dest;
    delete[] _msgs;
}

ERRCODE PosixPacketWriterData::dataWritePacket ( int fd, const SockAddr & addr, MemVector & data )
{
    if ( fd < 0 )
    {
        return Error::Closed;
    }

    if ( data.isEmpty() )
    {
        return Error::Success;
    }

    ssize_t ret = 0;

    if ( !IsSocketWriter )
    {
        ret = ::writev ( fd, data.getChunks(), data.getNumChunks() );
    }
    else
    {
        struct msghdr msg;

        memset ( &msg, 0, sizeof ( msg ) );

        if ( addr.hasIpAddr() )
        {
            // We only set it if it is valid.
            msg.msg_name = const_cast<SockAddr *> ( &addr );
            msg.msg_namelen = addr.getSocklen();
        }

        // It's not actually modified...
        msg.msg_iov = const_cast<struct iovec *> ( data.getChunks() );
        msg.msg_iovlen = data.getNumChunks();

        ret = ::sendmsg ( fd, &msg, 0 );
    }

    // We need to write entire packet at a time.
    // If not, this write has failed...

    if ( ret < 0 )
    {
        if ( SocketApi::isErrnoSoft() )
        {
            return Error::SoftFail;
        }

#ifdef EMSGSIZE
        if ( errno == EMSGSIZE )
        {
            return Error::TooMuchData;
        }
#endif

#ifdef EISCONN
        if ( errno == EISCONN )
        {
            return Error::AlreadyConnected;
        }
#endif

#ifdef EINVAL
        if ( errno == EINVAL )
        {
            // See comment about EINVAL in CorePacketWriter::doWrite().
            return Error::Closed;
        }
#endif

        if ( SocketApi::isErrnoNonFatal() )
        {
            return Error::WriteFailed;
        }

        return Error::Closed;
    }

    if ( ( size_t ) ret != data.getDataSize() )
    {
        return Error::WriteFailed;
    }

    return Error::Success;
}

ERRCODE PosixPacketWriterData::dataWritePackets (
        int fd, uint16_t index,
        uint16_t maxPackets, uint32_t maxBytes,
        uint16_t & packetsWritten, uint32_t & bytesWritten )
{
    assert ( maxPackets <= QueueSize );
    assert ( index < QueueSize );

    packetsWritten = 0;
    bytesWritten = 0;

    if ( fd < 0 )
    {
        return Error::Closed;
    }

    while ( packetsWritten < maxPackets && bytesWritten < maxBytes )
    {
#ifdef USE_UDP_IMPL_MMSG
        // Can we use sendmmsg?
        if ( _msgs != 0 )
        {
            assert ( _dest != 0 );

            uint16_t numMsg = 0;
            uint32_t numBytes = 0;

            for ( uint16_t idx = index;
                  packetsWritten + numMsg < maxPackets && bytesWritten + numBytes < maxBytes;
                  idx = ( idx + 1 ) % QueueSize )
            {
                assert ( idx < QueueSize );

                memset ( &_msgs[ numMsg ], 0, sizeof ( _msgs[ 0 ] ) );

                struct msghdr & mHdr = _msgs[ numMsg ].msg_hdr;

                mHdr.msg_iov = const_cast<struct iovec *> ( _data[ idx ].getChunks() );
                mHdr.msg_iovlen = _data[ idx ].getNumChunks();

                if ( _dest[ idx ].hasIpAddr() )
                {
                    mHdr.msg_name = &_dest[ idx ];
                    mHdr.msg_namelen = _dest[ idx ].getSocklen();
                }

                ++numMsg;
                numBytes += _data[ idx ].getDataSize();
            }

            const int ret = ::sendmmsg ( fd, _msgs, numMsg, 0 );

            // This should be consistent with dataWritePacket() codes:
            if ( ret < 0 )
            {
#ifdef EINVAL
                if ( errno == EINVAL )
                {
                    // See comment about EINVAL in CorePacketWriter::doWrite().
                    return Error::Closed;
                }
#endif

                if ( SocketApi::isErrnoSoft() )
                {
                    return Error::SoftFail;
                }

                if ( SocketApi::isErrnoNonFatal() )
                {
                    // Something is wrong, let's skip a single message.
                    // Otherwise we could get stuck...
                    // We still need to pretend it was written (but we don't include its size).

                    ++packetsWritten;
                    index = ( index + 1 ) % QueueSize;
                    continue;
                }

                return Error::Closed;
            }

            if ( ret < 1 )
            {
                return Error::Closed;
            }

            if ( ret < numMsg )
            {
                for ( int i = 0; i < ret; ++i )
                {
                    bytesWritten += _data[ ( index + i ) % QueueSize ].getDataSize();
                }
            }
            else
            {
                bytesWritten += numBytes;
            }

            packetsWritten += ret;
            index = ( index + ret ) % QueueSize;
            continue;
        }
#endif

        assert ( index < QueueSize );

        ERRCODE eCode = dataWritePacket (
            fd, ( _dest != 0 ) ? _dest[ index ] : EmptySockAddress, _data[ index ] );

        if ( eCode == Error::SoftFail || eCode == Error::Closed )
        {
            return eCode;
        }

        // It succeeded, or failed to write a specific packet.

        if ( IS_OK ( eCode ) )
        {
            // If the packet failed to be written, we want to skip it
            // and pretend we wrote it (increment packetsWritten).
            // There is no point pretending we wrote the data...

            bytesWritten += _data[ index ].getDataSize();
        }

        // We still need to pretend we wrote it...
        ++packetsWritten;

        // Now let's move to the next one:
        index = ( index + 1 ) % QueueSize;
    }

    return Error::Success;
}
