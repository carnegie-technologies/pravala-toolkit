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

#include "basic/Math.hpp"
#include "basic/Random.hpp"
#include "config/ConfigNumber.hpp"

#include "PosixPacketWriter.hpp"

// Include implementation of the 'data' part:
#include "PosixPacketWriterData.cpp"

#define MAX_WRITE_BYTES    0xFFFFFFFFU

using namespace Pravala;

/// @brief The length of a bucket for limiting sending speed, in microseconds.
ConfigLimitedNumber<uint32_t> optBucketSize (
        0,
        "os.packet_writer.speed_bucket_length",
        "The length of a bucket for limiting sending speed, in microseconds",
        10, 1000 * 1000, 500 );

PosixPacketWriter::PosixPacketWriter (
        WriterType wType, uint16_t flags, uint16_t queueSize, uint16_t speedLimit ):
    CorePacketWriter ( wType ),
    PosixPacketWriterData ( wType, flags, queueSize ),
    SpeedLimit ( speedLimit ),
    _mutex ( "PosixPacketWriter" ),
    _thread ( 0 ),
    _semaphore ( 0 ),
    _maxQueuedPackets ( 0 ),
    _writesSinceEol ( 0 ),
    _flags ( flags & CoreFlags ),
    _sendIndex ( 0 ),
    _appendIndex ( 0 ),
    _clearIndex ( 0 )
{
    assert ( ( ( FlagEolSubscribed | FlagThreadRunning ) & CoreFlags ) == 0 );

    if ( _flags & FlagThreaded )
    {
#ifdef SYSTEM_APPLE
        const String name ( String ( "packet_writer_%1_%2" ).arg ( getpid() ).arg ( Random::rand() ) );

        if ( ( _semaphore = sem_open ( name.c_str(), O_CREAT | O_EXCL, 0600, 0 ) ) == SEM_FAILED )
        {
            _semaphore = 0;
        }
        else if ( sem_unlink ( name.c_str() ) != 0 )
        {
            // We don't share it between processes, so no need to keep the name.

            sem_close ( _semaphore );
            _semaphore = 0;
        }
#else
        _semaphore = new sem_t;

        if ( sem_init ( _semaphore, 0, 0 ) != 0 )
        {
            LOG ( L_ERROR, ( ( Type == SocketWriter ) ? "Socket" : "Basic" )
                  << " writer failed to initialize a semaphore: " << strerror ( errno ) << " [" << errno << "]" );

            delete _semaphore;
            _semaphore = 0;
        }
#endif

        if ( !_semaphore )
        {
            _flags &= ~FlagThreaded;
        }
    }
}

PosixPacketWriter::~PosixPacketWriter()
{
    configureFd ( -1 );

    if ( _semaphore != 0 )
    {
#ifdef SYSTEM_APPLE
        // This also deallocates the memory:
        sem_close ( _semaphore );
#else
        sem_destroy ( _semaphore );
        delete _semaphore;
#endif

        _semaphore = 0;
    }
}

void PosixPacketWriter::configureFd ( int fDesc )
{
    if ( _flags & FlagThreadRunning )
    {
        void * ret = 0;

        _mutex.lock();

        // This will cause the other thread to exit:
        _fd = -1;

        _mutex.unlock();

        sem_post ( _semaphore );
        pthread_join ( _thread, &ret );

        _flags &= ~FlagThreadRunning;
    }

    // Here there should be no other thread running, so we can check and set things freely:

    if ( ( _fd < 0 && fDesc < 0 ) || ( _fd == fDesc ) )
    {
        // Nothing to do...
        return;
    }

    _fd = fDesc;

    // If the thread was running, we stopped it first, at the beginning of this function.
    // If we get a valid FD we want to start a thread, even if it was previously running (using a different FD).

    if ( _fd >= 0 && ( _flags & FlagThreaded ) )
    {
        if ( pthread_create ( &_thread, 0, staticThreadFunc, ( void * ) this ) == 0 )
        {
            _flags |= FlagThreadRunning;
        }
        else
        {
            LOG ( L_ERROR, ( ( Type == SocketWriter ) ? "Socket" : "Basic" )
                  << " writer failed to create a thread: " << strerror ( errno ) << " [" << errno << "]" );

            _flags &= ~FlagThreaded;
        }
    }
}

ERRCODE PosixPacketWriter::writePacket ( const SockAddr & addr, MemVector & data )
{
    // We don't synchronize this, even if it's not updated yet, it doesn't matter...
    if ( _fd < 0 )
    {
        return Error::Closed;
    }

    if ( data.isEmpty() )
    {
        return Error::Success;
    }

    if ( !_data )
    {
        // We don't have the write queue. This means that it doesn't make sense to use it.
        // Let's just perform a regular write.
        return dataWritePacket ( _fd, addr, data );
    }

    // We use a copy of _sendIndex, in case there is another thread that may change it.
    // The only other thing we need to synchronize is updating _appendIndex.
    uint16_t sendIndex = 0;

    if ( _flags & FlagThreaded )
    {
        _mutex.lock();
        sendIndex = _sendIndex;
        _mutex.unlock();
    }
    else
    {
        sendIndex = _sendIndex;
    }

    while ( _clearIndex != sendIndex )
    {
        _data[ _clearIndex ].clear();
        _clearIndex = ( _clearIndex + 1 ) % QueueSize;
    }

    ++_writesSinceEol;

    const size_t qSize = ( ( sendIndex > _appendIndex ) ? QueueSize : 0 ) + _appendIndex - sendIndex;

    if ( qSize > _maxQueuedPackets )
    {
        LOG ( L_DEBUG, ( ( Type == SocketWriter ) ? "Socket" : "Basic" ) << " writer's queue max size increase "
              << _maxQueuedPackets << " -> " << qSize << "; WritesSinceEol: " << _writesSinceEol );

        _maxQueuedPackets = qSize;
    }

    // We want to "catch up" with the sender, by clearing everything that has been dealt with.
    // After that we are allowed to produce up to _clearIndex/sendIndex and NOT _sendIndex (it may be different now).
    // Otherwise the new packet could get cleared next time this gets called.

    const uint16_t nextAppendIndex = ( _appendIndex + 1 ) % QueueSize;

    if ( nextAppendIndex == sendIndex )
    {
        // The queue is full...
        LOG_LIM ( L_DEBUG, ( ( Type == SocketWriter ) ? "Socket" : "Basic" ) << " writer's queue is full; ClearIdx: "
                  << _clearIndex << "; SendIdx: " << sendIndex << "; AppendIdx: " << _appendIndex << "; Size: "
                  << QueueSize << "; WritesSinceEol: " << _writesSinceEol );

        if ( ( _flags & FlagThreaded ) == 0 )
        {
            // We should try flushing the queue again. Maybe this time it works and the next packet will succeed.
            flushQueue();
        }
        else if ( ( _flags & FlagEolSubscribed ) == 0 )
        {
            // If the queue is full, we should be subscribed to end-of-loop events...
            _flags |= FlagEolSubscribed;
            EventManager::loopEndSubscribe ( this );
        }

        return Error::SoftFail;
    }

    _data[ _appendIndex ].stealFrom ( data );

    assert ( !_data[ _appendIndex ].isEmpty() );

    if ( _dest != 0 )
    {
        _dest[ _appendIndex ] = addr;
    }

    if ( _flags & FlagThreaded )
    {
        _mutex.lock();
        _appendIndex = nextAppendIndex;
        _mutex.unlock();
    }
    else
    {
        _appendIndex = nextAppendIndex;
    }

    if ( ( _flags & FlagThreaded ) == 0 && ( ( _appendIndex + 1 ) % QueueSize ) == _clearIndex )
    {
        // We are NOT in threaded mode and the queue is now full.
        // We should try writing right away!
        flushQueue();
    }
    else if ( ( _flags & FlagEolSubscribed ) == 0 )
    {
        _flags |= FlagEolSubscribed;
        EventManager::loopEndSubscribe ( this );
    }

    return Error::Success;
}

void PosixPacketWriter::receiveLoopEndEvent()
{
    assert ( _data != 0 );

    _flags &= ~FlagEolSubscribed;
    _writesSinceEol = 0;

    if ( _flags & FlagThreaded )
    {
        sem_post ( _semaphore );
        return;
    }

    flushQueue();
}

void PosixPacketWriter::flushQueue()
{
    // This is only used in non-threaded mode.
    // There is no need to synchronize anything...
    assert ( ( _flags & FlagThreaded ) == 0 );

    // Send index is "chasing" append index. The number of packets to write is the distance between them.
    const uint16_t qSize = ( ( _sendIndex > _appendIndex ) ? QueueSize : 0 ) + _appendIndex - _sendIndex;

    if ( qSize < 1 )
    {
        return;
    }

    uint16_t pWritten = 0;
    uint32_t bWritten = 0;

    const ERRCODE eCode = dataWritePackets ( _fd, _sendIndex, qSize, MAX_WRITE_BYTES, pWritten, bWritten );

    _sendIndex = ( _sendIndex + pWritten ) % QueueSize;

    if ( eCode == Error::Closed )
    {
        _fd = -1;

        for ( size_t i = 0; i < QueueSize; ++i )
        {
            _data[ i ].clear();
        }

        _clearIndex = _sendIndex = _appendIndex = 0;
        return;
    }

    while ( _clearIndex != _sendIndex )
    {
        _data[ _clearIndex ].clear();
        _clearIndex = ( _clearIndex + 1 ) % QueueSize;
    }

    if ( _sendIndex != _appendIndex )
    {
        // There is something left in the queue...
        _flags |= FlagEolSubscribed;
        EventManager::loopEndSubscribe ( this );
    }
}

void * PosixPacketWriter::staticThreadFunc ( void * arg )
{
    assert ( arg != 0 );

    return ( ( PosixPacketWriter * ) arg )->threadFunc();
}

void * PosixPacketWriter::threadFunc()
{
    _mutex.lock();
    uint16_t sendIndex = _sendIndex;
    _mutex.unlock();

    // Between 10us and 1s:
    const int32_t bucketSizeUs = limit<int32_t> ( optBucketSize.value(), 10, 1000 * 1000 );

    assert ( bucketSizeUs > 0 );

    // If speed limit is > 0, we want to make maxBucketBytes at least 1 byte.
    // Limits are not enforced strictly, a packet will be sent if at least a single byte is allowed.
    // So maxBucketBytes=1 means one packet per bucket.
    // SpeedLimit is in Mbps.
    // So the Mbps to bytes/per bucket:
    // limit_mbps * 1000 * 1000 / 8 * bucket_size_us / 1000 / 1000.
    // We can skip * 1000 / 1000:
    const uint32_t maxBucketBytes
        = ( SpeedLimit > 0 )
          ? limit<uint32_t> ( static_cast<uint32_t> ( SpeedLimit ) * bucketSizeUs / 8, 1, MAX_WRITE_BYTES )
          : 0;

    // This is only done once, so we can do this whether we need it or not...

    CurrentTime cTime;

    struct timespec curTime = {
        0, 0
    };

    cTime.readTime ( curTime );

    struct timespec bucketTime = curTime;

    uint32_t bucketAllowedBytes = 0;
    bool justSlept = false;

    while ( true )
    {
        _mutex.lock();

        // Let's read the current values:
        const int fd = _fd;
        const uint16_t appendIndex = _appendIndex;

        if ( fd < 0 )
        {
            // The main thread wants us to quit. Let's do it...
            _mutex.unlock();
            break;
        }

        // We can keep running, let's store updated 'send index':
        _sendIndex = sendIndex;

        _mutex.unlock();

        // Send index is "chasing" append index. The number of packets to write is the distance between them.
        // If they are equal, it means that the queue is empty.
        const uint16_t qSize = ( ( sendIndex > appendIndex ) ? QueueSize : 0 ) + appendIndex - sendIndex;

        if ( qSize < 1 )
        {
            // Let's wait on the semaphore!
            sem_wait ( _semaphore );

            justSlept = false;
            continue;
        }

        if ( maxBucketBytes < 1 )
        {
            bucketAllowedBytes = MAX_WRITE_BYTES;
        }
        else
        {
            cTime.readTime ( curTime );

            // In us:
            const int32_t diff
                = ( curTime.tv_sec - bucketTime.tv_sec ) * 1000 * 1000
                  + ( static_cast<int32_t> ( curTime.tv_nsec ) - static_cast<int32_t> ( bucketTime.tv_nsec ) ) / 1000;

            if ( diff >= bucketSizeUs )
            {
                // New bucket!

                bucketTime = curTime;
                bucketAllowedBytes = maxBucketBytes;

                if ( justSlept )
                {
                    // We may have slept too long.
                    // It is possible that we slept for more than a single bucket's length.
                    // To account for that, we want to make the allowed number of bytes larger.
                    // This only happens right after we sleep!
                    const uint32_t extraBytes
                        = maxBucketBytes * ( ( diff - bucketSizeUs ) / static_cast<double> ( bucketSizeUs ) );

                    // And just to be safe, we don't add more than extra 10 buckets worth of bytes...
                    bucketAllowedBytes += min<uint32_t> ( extraBytes, maxBucketBytes * 10 );
                }
            }
            else if ( bucketAllowedBytes < 1 )
            {
                // We hit the limit.
                // usleep ( 1 ) takes much longer than 1us, on AWS it is typically around 200us and it changes,
                // depending on the system load.
                // Ideally, we would wait till the end of the current bucket, but it is very imprecise.
                // So we wait the shortest possible amount of time. If it's too short, we will sleep again.
                // If it's too long, we add extra bytes to the new bucket.

                usleep ( 1 );

                justSlept = true;
                continue;
            }

            justSlept = false;
        }

        uint16_t pWritten = 0;
        uint32_t bWritten = 0;
        const ERRCODE eCode = dataWritePackets ( fd, sendIndex, qSize, bucketAllowedBytes, pWritten, bWritten );

        sendIndex = ( sendIndex + pWritten ) % QueueSize;

        if ( eCode == Error::Closed )
        {
            // This FD is closed, we can set it to -1.
            // This way the main thread will know that the socket was closed.
            // The main thread may set it to -1 to tell us to exit,
            // before setting a new one, but in that case it will wait for us to finish first.
            // Either way, it should be safe to set it to -1, even if it already is set to -1.

            _mutex.lock();
            _fd = -1;
            _mutex.unlock();

            break;
        }

        if ( bWritten < bucketAllowedBytes )
        {
            bucketAllowedBytes -= bWritten;
        }
        else
        {
            bucketAllowedBytes = 0;
        }
    }

    return 0;
}
