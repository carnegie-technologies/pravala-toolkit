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
#include <sys/event.h>
#include <sys/select.h>
#include <sys/time.h>
}

#include "KqueueEventManager.hpp"

#define MAX_EVENTS    64

#include "../PosixEventManager.cpp"

using namespace Pravala;

const int EventManager::EventRead ( 1 << 0 );
const int EventManager::EventWrite ( 1 << 1 );

ERRCODE EventManager::init()
{
    assert ( !_instance );

    if ( _instance != 0 )
        return Error::AlreadyInitialized;

    const int kqueueFd = kqueue();

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] KqueueEventManager: Created kqueueFd: %d\n", getpid(), kqueueFd );
#endif

    if ( kqueueFd < 0 )
    {
        perror ( "KqueueEventManager: Error calling kqueue_create\n" );

        return Error::SyscallError;
    }

    // Set epoll fd to close on exec/fork
    // This doesn't seem to work on some kernel versions
    int flags = fcntl ( kqueueFd, F_GETFD, 0 );

    if ( flags < 0 )
        flags = 0;

    if ( fcntl ( kqueueFd, F_SETFD, flags | FD_CLOEXEC ) < 0 )
    {
        perror ( "KqueueEventManager: Unable to fcntl kqueue fd with FD_CLOEXEC\n" );
    }

    KqueueEventManager * const mgr = new KqueueEventManager ( kqueueFd );

    if ( !mgr )
    {
        return Error::MemoryError;
    }

    // We don't need to store 'mgr', EventManager's constructor should set the instance pointer:
    assert ( _instance == ( EventManager * ) mgr );

    return Error::Success;
}

KqueueEventManager::KqueueEventManager ( int kqueueFd ): _kqueueFd ( kqueueFd )
{
    assert ( _kqueueFd >= 0 );
}

KqueueEventManager::~KqueueEventManager()
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] ~KqueueEventManager(), events.size() = %lu\n",
              getpid(), ( long unsigned ) _events.size() );
#endif

    if ( _kqueueFd >= 0 )
    {
        ::close ( _kqueueFd );
        _kqueueFd = -1;
    }
}

void KqueueEventManager::implSetFdHandler ( int fd, FdEventHandler * handler, int events )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [before]\n",
              getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );
    assert ( handler != 0 );

    _events.getOrCreate ( fd ).handler = handler;

    initFd ( fd );

    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdHandler(%d, 0x%lx), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    if ( events != 0 )
    {
        implSetFdEvents ( fd, events );
    }
}

void KqueueEventManager::implSetFdEvents ( int fd, int events )
{
    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdEvents(%d, %d), events.size() = %lu\n",
              getpid(), fd, events, ( long unsigned ) _events.size() );
#endif

    if ( fd < 0 || ( size_t ) fd >= _events.size() ) return;

    FdEventInfo & eInfo = _events[ fd ];

    struct kevent kev;

    memset ( &kev, 0, sizeof ( kev ) );

    const int kqueueFd = KqueueEventManager::_kqueueFd;

    if ( !events )
    {
        if ( !eInfo.events ) return;

        if ( eInfo.events & EventRead )
        {
            EV_SET ( &kev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL );
            int ret = kevent ( kqueueFd, &kev, 1, NULL, 0, NULL );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: kevent(EV_DELETE READ) for fd %d failed: %s\n",
                          fd, strerror ( errno ) );
            }
        }

        if ( eInfo.events & EventWrite )
        {
            EV_SET ( &kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL );
            int ret = kevent ( kqueueFd, &kev, 1, NULL, 0, NULL );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: kevent(EV_DELETE WRITE) for fd %d failed: %s\n",
                          fd, strerror ( errno ) );
            }
        }

        eInfo.events = 0;
    }
    else if ( eInfo.events != events )
    {
        if ( ( eInfo.events & EventRead ) != ( events & EventRead ) )
        {
            EV_SET ( &kev, fd, EVFILT_READ, ( ( events & EventRead ) ? EV_ADD : EV_DELETE ), 0, 0, NULL );

            int ret = kevent ( kqueueFd, &kev, 1, NULL, 0, NULL );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: kevent(EV_%s READ) for fd %d and events %d failed: %s\n",
                          ( events & EventRead ) ? "ADD" : "DEL", fd, events, strerror ( errno ) );
            }
        }

        if ( ( eInfo.events & EventWrite ) != ( events & EventWrite ) )
        {
            EV_SET ( &kev, fd, EVFILT_WRITE, ( ( events & EventWrite ) ? EV_ADD : EV_DELETE ), 0, 0, NULL );

            int ret = kevent ( kqueueFd, &kev, 1, NULL, 0, NULL );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: kevent(EV_%s WRITE) for fd %d and events %d failed: %s\n",
                          ( events & EventWrite ) ? "ADD" : "DEL", fd, events, strerror ( errno ) );
            }
        }

        eInfo.events = events;
    }
}

void KqueueEventManager::implRemoveFdHandler ( int fd )
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [before]\n",
              getpid(), fd, ( long unsigned ) _events.size() );
#endif

    assert ( fd >= 0 );

    if ( fd >= 0 && ( size_t ) fd < _events.size() )
    {
        FdEventInfo & eInfo = _events[ fd ];

        if ( eInfo.events != 0 )
        {
            setFdEvents ( fd, 0 );

            assert ( eInfo.events == 0 );
        }

        eInfo.handler = 0;
    }

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) _events.size() );
#endif
}

void KqueueEventManager::implRun()
{
    if ( _working )
        return;

    initSignals();

    _working = true;

    struct kevent eResult[ MAX_EVENTS ];

    memset ( eResult, 0, sizeof ( struct kevent ) * MAX_EVENTS );

    int count;
    int idx;

    while ( _working )
    {
        const int msTimeout = getSafeTimeout();

        if ( msTimeout < 0 )
        {
            count = kevent ( KqueueEventManager::_kqueueFd, NULL, 0, eResult, MAX_EVENTS, 0 );
        }
        else
        {
            Time timeout;
            timeout.increaseMilliseconds ( msTimeout );

            timespec tspec;

            tspec.tv_sec = timeout.getSeconds();
            tspec.tv_nsec = timeout.getMilliSeconds() * ONE_MSEC_IN_NSEC;

            count = kevent ( KqueueEventManager::_kqueueFd, NULL, 0, eResult, MAX_EVENTS, &tspec );
        }

        // This refreshes current time - so the callbacks have fresh time info.
        // Timers, that are run at the end, will refresh it too.
        currentTime ( true );

        if ( runProcessSignals() && count < 0 )
        {
            if ( count < 0 ) count = 0;

            if ( !_working )
                return;
        }

        if ( count < 0 )
        {
            // It wasn't one of the signals we care about, so kevent_wait had some error
            perror ( "kevent_wait" );
        }
        else
        {
            for ( idx = 0; idx < count; idx++ )
            {
                const int fd = eResult[ idx ].ident;

                assert ( fd >= 0 );

                if ( fd < 0 )
                    continue;

                assert ( ( size_t ) fd < _events.size() );

                if ( _events[ fd ].handler != 0 )
                {
                    assert ( _events[ fd ].handler != 0 );

                    short events = 0;

                    if ( eResult[ idx ].filter == EVFILT_READ )
                    {
                        events = EventRead;
                    }
                    else if ( eResult[ idx ].filter == EVFILT_WRITE )
                    {
                        events = EventWrite;
                    }

                    if ( ( eResult[ idx ].flags & EV_ERROR ) != 0 )
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
}
