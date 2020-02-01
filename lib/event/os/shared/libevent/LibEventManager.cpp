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

#include <cstring>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <csignal>

extern "C"
{
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <event.h>
}

#include "LibEventManager.hpp"

#include "../PosixEventManager.cpp"

using namespace Pravala;

const int EventManager::EventRead ( EV_READ );
const int EventManager::EventWrite ( EV_WRITE );

void LibEventManager::evSigHandler ( int sigNum, short event, void * arg )
{
    assert ( event == EV_SIGNAL );

    ( void ) event;
    ( void ) arg;

    posixSignalHandler ( sigNum );
}

ERRCODE EventManager::init()
{
    assert ( !_instance );

    if ( _instance != 0 )
        return Error::AlreadyInitialized;

    struct event_base * const eventBase = event_init();

    if ( !eventBase )
    {
        perror ( "LibEventManager: Error calling event_init\n" );
        return Error::SyscallError;
    }

    LibEventManager * const mgr = new LibEventManager ( eventBase );

    if ( !mgr )
    {
        return Error::MemoryError;
    }

    // We don't need to store 'mgr', EventManager's constructor should set the instance pointer:
    assert ( _instance == ( EventManager * ) mgr );

    return Error::Success;
}

LibEventManager::LibEventManager ( struct event_base * eventBase ): _eventBase ( eventBase )
{
    assert ( _eventBase != 0 );

    memset ( &_sigEvents, 0, sizeof ( _sigEvents ) );

    signal_set ( &_sigEvents.sigChld, SIGCHLD, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigInt, SIGINT, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigTerm, SIGTERM, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigPipe, SIGPIPE, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigHup, SIGHUP, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigUsr1, SIGUSR1, evSigHandler, 0 );
    signal_set ( &_sigEvents.sigUsr2, SIGUSR2, evSigHandler, 0 );

    assert ( event_initialized ( &_sigEvents.sigChld ) );
    assert ( event_initialized ( &_sigEvents.sigInt ) );
    assert ( event_initialized ( &_sigEvents.sigTerm ) );
    assert ( event_initialized ( &_sigEvents.sigPipe ) );
    assert ( event_initialized ( &_sigEvents.sigHup ) );
    assert ( event_initialized ( &_sigEvents.sigUsr1 ) );
    assert ( event_initialized ( &_sigEvents.sigUsr2 ) );

    signal_add ( &_sigEvents.sigChld, 0 );
    signal_add ( &_sigEvents.sigInt, 0 );
    signal_add ( &_sigEvents.sigTerm, 0 );
    signal_add ( &_sigEvents.sigPipe, 0 );
    signal_add ( &_sigEvents.sigHup, 0 );
    signal_add ( &_sigEvents.sigUsr1, 0 );
    signal_add ( &_sigEvents.sigUsr2, 0 );
}

LibEventManager::~LibEventManager()
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] ~EventManager(), events.size() = %lu\n", getpid(), _events.size() );
#endif

    if ( _eventBase != 0 )
    {
        event_base_free ( _eventBase );
        _eventBase = 0;
    }

    const size_t size = _events.size();

    for ( size_t i = 0; i < size; i++ )
    {
        if ( _events[ i ].libEventState )
        {
            free ( _events[ i ].libEventState );
            _events[ i ].libEventState = 0;
        }
    }
}

void LibEventManager::implSetFdHandler ( int fd, EventManager::FdEventHandler * handler, int events )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [before]\n",
              getpid(), fd, ( long unsigned ) handler, _events.size() );
#endif

    assert ( fd >= 0 );
    assert ( handler != 0 );

    _events.getOrCreate ( fd ).handler = handler;

    initFd ( fd );

    assert ( ( size_t ) fd < _events.size() );

    if ( !_events[ fd ].libEventState )
    {
        _events[ fd ].libEventState = static_cast<event *> ( malloc ( sizeof ( struct event ) ) );
    }

    assert ( _events[ fd ].libEventState != 0 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) handler, _events.size() );
#endif

    if ( events != 0 )
        setFdEvents ( fd, events );
}

void LibEventManager::implSetFdEvents ( int fd, int events )
{
    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdEvents(%d, %d), events.size() = %lu\n", getpid(), fd, events, _events.size() );
#endif

    if ( fd < 0 || ( size_t ) fd >= _events.size() ) return;

    FdEventInfo & eInfo = _events[ fd ];

    assert ( eInfo.libEventState != 0 );

    // has something changed?
    if ( eInfo.events != events )
    {
        // libevent doesn't provide way to "modify" events, so...
        // if event is active, remove event first
        if ( eInfo.events != 0 )
        {
            assert ( event_initialized ( eInfo.libEventState ) );

            eInfo.events = 0;

            int ret = event_del ( eInfo.libEventState );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: event_del for file descriptor %d and events %d failed: %s\n",
                          fd, events, strerror ( errno ) );
            }
        }

        // if need to set new events, add it back
        if ( events != 0 )
        {
            assert ( eInfo.events == 0 );

            event_set ( eInfo.libEventState, fd, events | EV_PERSIST,
                        LibEventManager::fdEventHandlerCallback, 0 );

            assert ( event_initialized ( eInfo.libEventState ) );

            eInfo.events = events;

            int ret = event_add ( eInfo.libEventState, 0 );

            assert ( event_initialized ( eInfo.libEventState ) );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: event_add for file descriptor %d and events %d failed: %s\n",
                          fd, events, strerror ( errno ) );
            }
        }
    }
}

void LibEventManager::implRemoveFdHandler ( int fd )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [before]\n", getpid(), fd, _events.size() );
#endif

    assert ( fd >= 0 );

    if ( fd >= 0 && ( size_t ) fd < _events.size() )
    {
        FdEventInfo & eInfo = _events[ fd ];

        if ( eInfo.events != 0 )
        {
            assert ( event_initialized ( eInfo.libEventState ) );

            eInfo.events = 0;

            int ret = event_del ( eInfo.libEventState );

            if ( ret != 0 )
            {
                fprintf ( stderr, "removeFdHandler: event_del for file descriptor %d failed: %s\n",
                          fd, strerror ( errno ) );
            }
        }

        eInfo.handler = 0;
    }

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [after]\n", getpid(), fd, _events.size() );
#endif
}

void LibEventManager::fdEventHandlerCallback ( int fd, short event, void * arg )
{
    ( void ) arg;
    assert ( !arg );
    assert ( fd >= 0 );

    LibEventManager & evManager = static_cast<LibEventManager &> ( *getInstance() );

    assert ( ( size_t ) fd < evManager._events.size() );

    if ( fd < 0 || ( size_t ) fd >= evManager._events.size() ) return;

    FdEventInfo & eInfo = evManager._events[ fd ];

    if ( eInfo.events != 0 && eInfo.handler != 0 )
    {
        // This refreshes current time - so the callbacks have fresh time info.
        // Timers, that are run at the end, will refresh it too.
        currentTime ( true );

        eInfo.handler->receiveFdEvent ( fd, event );
    }
}

void LibEventManager::implRun()
{
    if ( _working )
        return;

    if ( _isPrimaryManager )
    {
        // We can't use PosixEventManager's initSignals(), because libevent uses its own signal handling.
        // But we have to enable them to be able to receive them!

        sigset_t sigMask;
        sigemptyset ( &sigMask );
        sigaddset ( &sigMask, SIGCHLD );
        sigaddset ( &sigMask, SIGINT );
        sigaddset ( &sigMask, SIGTERM );
        sigaddset ( &sigMask, SIGPIPE );
        sigaddset ( &sigMask, SIGHUP );
        sigaddset ( &sigMask, SIGUSR1 );
        sigaddset ( &sigMask, SIGUSR2 );

        if ( sigprocmask ( SIG_UNBLOCK, &sigMask, 0 ) < 0 )
        {
            perror ( "LibEventManager::run(): Error calling sigprocmask\n" );
        }
    }

    _working = true;

    Time lastExtraWaitPid;

    struct timeval loop_timeout =
    {
        0, 10000
    };

    while ( _working )
    {
        const int msTimeout = getSafeTimeout();

        if ( msTimeout >= 0 )
        {
            loop_timeout.tv_usec = msTimeout * 1000;

            if ( loop_timeout.tv_usec < 1 )
            {
                loop_timeout.tv_usec = 100;
            }

            event_loopexit ( &loop_timeout );
        }

        int ret = event_loop ( EVLOOP_ONCE );

        if ( ret < 0 )
        {
            perror ( "event_loop" );
        }

        if ( runProcessSignals() && !_working )
        {
            printf ( "Signal received. Exiting Event Manager.\n" );

            return;
        }

        runEndOfLoop();
    }
}
