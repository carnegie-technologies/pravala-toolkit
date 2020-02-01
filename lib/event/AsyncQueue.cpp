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

extern "C"
{
#include <unistd.h>
}

// This should be before including SimpleLog.h
#define SIMPLE_LOG_TAG    "Event/AsyncQueue"

#include "basic/Math.hpp"
#include "simplelog/SimpleLog.h"
#include "sys/SocketApi.hpp"

#include "AsyncQueue.hpp"

/// @brief Number of microseconds in one millisecond
#define USEC_IN_MSEC                     1000

/// @brief Number of milliseconds to wait between runTask attempts for blockingRunTask
#define BLOCKING_RUN_TASK_INTERVAL_MS    100

using namespace Pravala;

AsyncQueue::Task::Task ( NoCopy * receiver ): _receiver ( receiver )
{
}

AsyncQueue::Task::~Task()
{
}

NoCopy * AsyncQueue::Task::getReceiver() const
{
    return _receiver;
}

void AsyncQueue::Task::deleteOnError ( DeletePolicy deletePolicy )
{
    if ( deletePolicy == DeleteOnError )
    {
        delete this;
    }
}

AsyncQueue & AsyncQueue::get()
{
    static AsyncQueue * global = 0;

    if ( !global )
    {
        global = new AsyncQueue();
    }

    return *global;
}

AsyncQueue::AsyncQueue(): _mutex ( "AsyncQueue" ), _offset ( 0 ), _isBroken ( false )
{
    assert ( EventManager::isInitialized() );

    ERRCODE eCode = _socks.init();

    if ( NOT_OK ( eCode ) )
    {
        SIMPLE_LOG_DEBUG ( "Error initializing the socket pair: %s", eCode.toString() );
        return;
    }

    EventManager::setFdHandler ( _socks.getSockA(), this, EventManager::EventRead );

    // We use a non-blocking socket to send the task so AsyncQueue can treat EAGAIN/EWOULDBLOCK as soft-fails.
    SocketApi::setNonBlocking ( _socks.getSockB(), true );
}

void AsyncQueue::registerReceiver ( NoCopy * receiver )
{
    // This will lock the mutex and unlock it when this function exits
    MutexLock m ( _mutex );

    _receivers.insert ( receiver );
}

void AsyncQueue::unregisterReceiver ( NoCopy * receiver )
{
    // This will lock the mutex and unlock it when this function exits
    MutexLock m ( _mutex );

    _receivers.remove ( receiver );
}

ERRCODE AsyncQueue::runTask ( AsyncQueue::Task * task, DeletePolicy deletePolicy )
{
    if ( !task )
    {
        return Error::InvalidParameter;
    }

    if ( deletePolicy != DeleteOnError && deletePolicy != DontDeleteOnError )
    {
        // We can't use log streams in a different thread :(
        SIMPLE_LOG_DEBUG ( "Invalid DeletePolicy: %d; Not deleting or scheduling the task!", deletePolicy );

        return Error::InvalidParameter;
    }

    char buf[ sizeof ( Task * ) ];

    memcpy ( buf, &task, sizeof ( buf ) );

    // This will lock the mutex and unlock it when this function exits
    MutexLock m ( _mutex );

    if ( _isBroken )
    {
        // We can't use log streams in a different thread :(
        SIMPLE_LOG_DEBUG ( "AsyncQueue is broken; DeletePolicy: %d; Not scheduling the task!", deletePolicy );

        task->deleteOnError ( deletePolicy );

        return Error::Closed;
    }

    const int fd = _socks.getSockB();

    if ( fd < 0 )
    {
        // We can't use log streams in a different thread :(
        SIMPLE_LOG_DEBUG ( "Writing socket is missing; DeletePolicy: %d; Not scheduling the task!", deletePolicy );

        task->deleteOnError ( deletePolicy );

        return Error::NotInitialized;
    }

    errno = 0;
    const int ret = send ( fd, buf, sizeof ( buf ), 0 );

    if ( ret < 0 && (
             errno == EAGAIN
#ifdef EWOULDBLOCK
             || errno == EWOULDBLOCK
#endif
         ) )
    {
        // We can't use log streams in a different thread :(
        SIMPLE_LOG_DEBUG ( "Send would block; DeletePolicy: %d; Not scheduling the task!", deletePolicy );

        task->deleteOnError ( deletePolicy );

        return Error::SoftFail;
    }

    if ( ret == sizeof ( buf ) )
    {
        return Error::Success;
    }

    // We can't use log streams in a different thread :(
    SIMPLE_LOG_DEBUG ( "Error writing to the socket, or a partial write (%d vs %d): %s [%d]"
                       "; DeletePolicy: %d; Closing the queue!",
                       ret, ( int ) sizeof ( buf ), strerror ( errno ), errno, deletePolicy );

    _isBroken = true;

    task->deleteOnError ( deletePolicy );

    return Error::Closed;
}

ERRCODE AsyncQueue::blockingRunTask ( AsyncQueue::Task * task, uint32_t timeoutMs, DeletePolicy deletePolicy )
{
    if ( !task )
    {
        return Error::InvalidParameter;
    }

    if ( deletePolicy != DeleteOnError && deletePolicy != DontDeleteOnError )
    {
        // We can't use log streams in a different thread :(
        SIMPLE_LOG_DEBUG ( "Invalid DeletePolicy: %d; Not deleting or scheduling the task!", deletePolicy );

        return Error::InvalidParameter;
    }

    ERRCODE eCode;
    uint32_t intervalMs;
    uint32_t timeLeftMs = timeoutMs;

    while ( true )
    {
        eCode = runTask ( task, DontDeleteOnError );

        if ( IS_OK ( eCode ) )
        {
            return eCode;
        }
        else if ( eCode != Error::SoftFail )
        {
            // We can't use log streams in a different thread :(
            SIMPLE_LOG_DEBUG ( "runTask returned %s; DeletePolicy: %d; Not scheduling the task!",
                               eCode.toString(), deletePolicy );

            task->deleteOnError ( deletePolicy );

            // This is a fatal error, so we can return immediately.
            return eCode;
        }

        if ( timeoutMs < 1 )
        {
            intervalMs = BLOCKING_RUN_TASK_INTERVAL_MS;
        }
        else if ( timeLeftMs > 0 )
        {
            intervalMs = min ( timeLeftMs, ( uint32_t ) BLOCKING_RUN_TASK_INTERVAL_MS );

            assert ( intervalMs <= timeLeftMs );

            timeLeftMs -= intervalMs;
        }
        else
        {
            // We can't use log streams in a different thread :(
            SIMPLE_LOG_DEBUG ( "Timeout (%u ms) reached; DeletePolicy: %d; Not scheduling the task!",
                               timeoutMs, deletePolicy );

            task->deleteOnError ( deletePolicy );

            // We timed out, so just abort.
            return Error::Timeout;
        }

        usleep ( USEC_IN_MSEC * intervalMs );
    }
}

void AsyncQueue::receiveFdEvent ( int fd, short int events )
{
    assert ( _socks.getSockA() == fd );
    assert ( _offset < sizeof ( _readBuf ) );

    if ( ( events & EventManager::EventRead ) == 0 )
        return;

    int ret = recv ( fd, _readBuf + _offset, sizeof ( _readBuf ) - _offset, 0 );

    if ( ret < 1 )
    {
        SIMPLE_LOG_DEBUG ( "Error reading from the socket: %s [%d]; recv returned: %ld",
                           strerror ( errno ), errno, ( long int ) ret );

        return;
    }

    _offset += ret;

    assert ( _offset <= sizeof ( _readBuf ) );

    if ( _offset < sizeof ( _readBuf ) )
    {
        return;
    }

    _offset = 0;

    Task * task = 0;

    memcpy ( &task, _readBuf, sizeof ( _readBuf ) );

    if ( !task )
    {
        SIMPLE_LOG_DEBUG_NP ( "Received an empty task pointer; Ignoring" );
        return;
    }

    // Memory barrier - to make sure that the task's memory, if it was created on a different cpu, is updated:
    // We also protect the '_receivers' set:

    _mutex.lock();

    NoCopy * receiver = task->getReceiver();

    if ( !receiver || _receivers.contains ( receiver ) )
    {
        _mutex.unlock();

        // LOG ( L_DEBUG3, "Running a task: " << ( ( size_t ) task ) );

        task->runTask();
    }
    else
    {
        _mutex.unlock();

        SIMPLE_LOG_DEBUG ( "Task's receiver is not registered; Not running the task (%lx); Receiver: %lx",
                           ( long unsigned ) task, ( long unsigned ) receiver );
    }

    delete task;
    task = 0;

    return;
}
