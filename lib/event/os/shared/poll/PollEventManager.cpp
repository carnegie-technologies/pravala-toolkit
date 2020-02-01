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

#include "PollEventManager.hpp"

#include "../PosixEventManager.cpp"

using namespace Pravala;

const int EventManager::EventRead ( POLLIN );
const int EventManager::EventWrite ( POLLOUT );

ERRCODE EventManager::init()
{
    assert ( !_instance );

    if ( _instance != 0 )
        return Error::AlreadyInitialized;

    PollEventManager * const mgr = new PollEventManager;

    if ( !mgr )
    {
        return Error::MemoryError;
    }

    // We don't need to store 'mgr', EventManager's constructor should set the instance pointer:
    assert ( _instance == ( EventManager * ) mgr );

    return Error::Success;
}

PollEventManager::PollEventManager()
{
}

PollEventManager::~PollEventManager()
{
}

void PollEventManager::implSetFdHandler ( int fd, EventManager::FdEventHandler * handler, int events )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [before]\n",
              getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );
    assert ( handler != 0 );

    _events.getOrCreate ( fd ).handler = handler;
    _pollData.getOrCreate ( fd );

    _pollData[ fd ].fd = ( _pollData[ fd ].events != 0 ) ? ( fd ) : ( -1 );

    initFd ( fd );

    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, %lx), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    if ( events != 0 )
        setFdEvents ( fd, events );
}

void PollEventManager::implSetFdEvents ( int fd, int events )
{
    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _events.size() );
    assert ( ( size_t ) fd < _pollData.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdEvents(%d, %d), events.size() = %lu\n",
              getpid(), fd, events, ( long unsigned ) _events.size() );
#endif

    if ( fd < 0 || ( size_t ) fd >= _events.size() || ( size_t ) fd >= _pollData.size() ) return;

    _events[ fd ].events = events;

    _pollData[ fd ].fd = ( events != 0 ) ? ( fd ) : ( -1 );
    _pollData[ fd ].events = events;
    _pollData[ fd ].revents = 0;
}

void PollEventManager::implRemoveFdHandler ( int fd )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [before]\n",
              getpid(), fd, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );

    if ( fd >= 0 && ( size_t ) fd < _events.size() && ( size_t ) fd < _pollData.size() )
    {
        _events.memsetZero ( fd );

        _pollData.memsetZero ( fd );
        _pollData[ fd ].fd = -1;
    }

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) _events.size() );
#endif
}

void PollEventManager::implRun()
{
    if ( _working )
        return;

    if ( initSignals() )
    {
#ifdef USE_SIGNALFD
        if ( _signalFd >= 0 )
        {
            // We don't really need to set the handler...

            _events.getOrCreate ( _signalFd );

            _pollData.getOrCreate ( _signalFd ).fd = _signalFd;
            _pollData[ _signalFd ].events = POLLIN;
        }
#endif
    }

    _working = true;

    int count;
    size_t idx;

    while ( _working && !_globalExit )
    {
        count = poll ( _pollData.getWritableMemory(), _pollData.size(), getSafeTimeout() );

        // This refreshes current time - so the callbacks have fresh time info.
        // Timers, that are run at the end, will refresh it too.
        currentTime ( true );

#ifndef USE_SIGNALFD
        if ( runProcessSignals() && count < 0 )
        {
            if ( count < 0 ) count = 0;

            if ( !_working )
                return;
        }
#endif

        if ( count < 0 )
        {
            // It wasn't one of the signals we care about, so poll had some error
            perror ( "poll" );
        }
        else
        {
            for ( idx = 0; idx < _pollData.size(); idx++ )
            {
                if ( count < 1 )
                    break;

                if ( _pollData[ idx ].revents == 0 )
                    continue;

                const int fd = _pollData[ idx ].fd;

                assert ( fd >= 0 );

                if ( fd < 0 )
                    continue;

                // count is the number of entries that have non-zero revevents field
                --count;

#ifdef USE_SIGNALFD
                if ( fd == _signalFd )
                {
                    if ( runProcessSignals() && !_working )
                        return;

                    continue;
                }
#endif

                assert ( ( size_t ) fd < _events.size() );
                assert ( ( size_t ) fd < _pollData.size() );

                if ( _events[ fd ].handler != 0 )
                {
                    assert ( _events[ fd ].handler != 0 );

                    short events = _pollData[ idx ].revents;

                    if ( ( events & ( POLLERR | POLLHUP ) ) != 0 )
                    {
                        // Unless the handler is only interested in receiving 'write' events
                        // (in which case we set 'write' event), on error we set 'read' event.

                        if ( ( _events[ fd ].events & EventWrite ) != 0
                             && ( _events[ fd ].events & EventRead ) == 0 )
                        {
                            events = EventWrite;
                        }
                        else
                        {
                            events = EventRead;
                        }

#if EVENT_MANAGER_DEBUG_FD_OPS
                        fprintf ( stderr, "[%6d] Received error event on a file desriptor %d; Unsetting events\n",
                                  getpid(), fd );
#endif

                        setFdEvents ( fd, 0 );
                    }

                    _events[ fd ].handler->receiveFdEvent ( fd, events );
                }
                else
                {
                    // If this happens it's because code inside the callback removed
                    // a handler. It should be safe :)
                    removeFdHandler ( fd );
                }
            }
        }

        runEndOfLoop();
    }

    _working = false;
}
