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

#include <cstdio>
#include <csignal>

#include "basic/MsvcSupport.hpp"
#include "SelectEventManager.hpp"

extern "C"
{
#include <io.h>
#include <ws2spi.h>
}

#if EVENT_MANAGER_DEBUG_FD_OPS
// For _getpid
extern "C"
{
#include <process.h>
}
#endif

using namespace Pravala;

const int EventManager::EventRead ( 1 << 0 );
const int EventManager::EventWrite ( 1 << 1 );

const int EventManager::SignalHUP ( 1 );
const int EventManager::SignalUsr1 ( 10 );
const int EventManager::SignalUsr2 ( 12 );

bool SelectEventManager::_globalExit ( false );

void SelectEventManager::signalHandler ( int sigNum )
{
    if ( sigNum == SIGINT || sigNum == SIGTERM )
    {
        _globalExit = true;
    }
}

ERRCODE EventManager::init()
{
    assert ( !_instance );

    if ( _instance != 0 )
        return Error::AlreadyInitialized;

    SelectEventManager * const mgr = new SelectEventManager;

    if ( !mgr )
    {
        return Error::MemoryError;
    }

    // We don't need to store 'mgr', EventManager's constructor should set the instance pointer:
    assert ( _instance == ( EventManager * ) mgr );

    return Error::Success;
}

SelectEventManager::SelectEventManager()
{
}

SelectEventManager::~SelectEventManager()
{
}

void SelectEventManager::initFd ( int fd )
{
    if ( fd < 0 )
        return;

    unsigned long arg = 1;

    if ( ioctlsocket ( fd, FIONBIO, &arg ) != 0 )
    {
        fprintf ( stderr, "initFd: ioctlsocket(FIONBIO) failed: %d\n", WSAGetLastError() );
    }
}

void SelectEventManager::implSetFdHandler ( int fd, EventManager::FdEventHandler * handler, int events )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [before]\n",
              _getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );
    assert ( handler != 0 );

    _events.getOrCreate ( fd ).handler = handler;

    initFd ( fd );

    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, %lx), events.size() = %lu [after]\n",
              _getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    if ( events != 0 )
        setFdEvents ( fd, events );
}

void SelectEventManager::implSetFdEvents ( int fd, int events )
{
    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdEvents(%d, %d), events.size() = %lu\n",
              _getpid(), fd, events, ( long unsigned ) _events.size() );
#endif

    if ( fd < 0 || ( size_t ) fd >= _events.size() ) return;

    _events[ fd ].events = events;
}

void SelectEventManager::implRemoveFdHandler ( int fd )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [before]\n",
              _getpid(), fd, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );

    if ( fd >= 0 && ( size_t ) fd < _events.size() )
    {
        _events.memsetZero ( fd );
    }

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [after]\n",
              _getpid(), fd, ( long unsigned ) _events.size() );
#endif
}

void SelectEventManager::implRun()
{
    if ( _working )
        return;

    if ( _isPrimaryManager )
    {
        signal ( SIGTERM, signalHandler );
        signal ( SIGINT, signalHandler );
    }

    _working = true;

    int count;
    size_t idx;
    int maxFd = -1;

    fd_set readfds, writefds, exceptfds;

    struct timeval loop_timeout =
    {
        0, 10000
    };

    while ( _working )
    {
        FD_ZERO ( &readfds );
        FD_ZERO ( &writefds );
        FD_ZERO ( &exceptfds );

        maxFd = -1;

        for ( idx = 0; idx < _events.size(); ++idx )
        {
            if ( !_events[ idx ].handler )
                continue;

            maxFd = ( int ) idx;

            FD_SET ( idx, &exceptfds );

            if ( ( _events[ idx ].events & EventRead ) != 0 )
            {
                FD_SET ( idx, &readfds );
            }

            if ( ( _events[ idx ].events & EventWrite ) != 0 )
            {
                FD_SET ( idx, &writefds );
            }
        }

        // If we have end-of-loop entries, we should timeout right away
        const int msTimeout = ( _loopEndQueue.isEmpty() ) ? getTimeout() : 0;

        if ( msTimeout >= 0 )
        {
            loop_timeout.tv_usec = msTimeout * 1000;

            if ( loop_timeout.tv_usec < 1 )
            {
                loop_timeout.tv_usec = 100;
            }
        }

        if ( maxFd < 0 )
        {
            if ( msTimeout < 0 )
            {
                Sleep ( 1000 );
            }
            else if ( msTimeout > 0 )
            {
                Sleep ( msTimeout );
            }

            if ( _globalExit )
            {
                printf ( "Signal received. Exiting Event Manager.\n" );
                _working = false;
                return;
            }
        }
        else
        {
            errno = 0;
            WSASetLastError ( 0 );
            count = select ( maxFd + 1, &readfds, &writefds, &exceptfds, ( msTimeout >= 0 ) ? &loop_timeout : 0 );

            if ( _globalExit )
            {
                printf ( "Signal received. Exiting Event Manager.\n" );
                _working = false;
                return;
            }

            // This refreshes current time - so the callbacks have fresh time info.
            // Timers, that are run at the end, will refresh it too.
            currentTime ( true );

            if ( count < 0 )
            {
                fprintf ( stderr, "Error running select; Timeout: %d ms; Errno: %s [%d]; WSA error: %d\n",
                          msTimeout, strerror ( errno ), errno, WSAGetLastError() );
            }
            else if ( count > 0 )
            {
                for ( idx = 0; idx < _events.size(); idx++ )
                {
                    if ( !_events[ idx ].handler )
                        continue;

                    short events = 0;

                    if ( FD_ISSET ( ( int ) idx, &readfds ) )
                    {
                        events |= EventRead;
                    }

                    if ( FD_ISSET ( ( int ) idx, &writefds ) )
                    {
                        events |= EventWrite;
                    }

                    if ( FD_ISSET ( ( int ) idx, &exceptfds ) )
                    {
                        // Unless the handler is only interested in receiving 'write' events
                        // (in which case we set 'write' event), on error we set 'read' event.

                        if ( ( _events[ idx ].events & EventWrite ) != 0
                             && ( _events[ idx ].events & EventRead ) == 0 )
                        {
                            events = EventWrite;
                        }
                        else
                        {
                            events = EventRead;
                        }

#if EVENT_MANAGER_DEBUG_FD_OPS
                        fprintf ( stderr, "[%6d] Received error event on a file desriptor %d; Unsetting events\n",
                                  _getpid(), idx );
#endif

                        setFdEvents ( idx, 0 );
                    }

                    if ( events != 0 )
                    {
                        _events[ idx ].handler->receiveFdEvent ( idx, events );
                    }
                }
            }
        }

        runEndOfLoop();
    }

    _working = false;
}
