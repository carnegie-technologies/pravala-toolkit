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

extern "C"
{
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdio>

#include "EpollEventManager.hpp"

#define MAX_EVENTS    64

#include "../PosixEventManager.cpp"

using namespace Pravala;

const int EventManager::EventRead ( EPOLLIN );
const int EventManager::EventWrite ( EPOLLOUT );

ERRCODE EventManager::init()
{
    assert ( !_instance );

    if ( _instance != 0 )
        return Error::AlreadyInitialized;

    const int epollFd = epoll_create ( 10 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] EpollEventManager: Created epollFd: %d\n", getpid(), epollFd );
#endif

    if ( epollFd < 0 )
    {
        perror ( "EpollEventManager: Error calling epoll_create\n" );

        return Error::SyscallError;
    }

    // Set epoll fd to close on exec/fork
    // This doesn't seem to work on some kernel versions
    int flags = fcntl ( epollFd, F_GETFD, 0 );

    if ( flags < 0 )
        flags = 0;

    if ( fcntl ( epollFd, F_SETFD, flags | FD_CLOEXEC ) < 0 )
    {
        perror ( "EpollEventManager: Unable to fcntl epoll fd with FD_CLOEXEC\n" );
    }

    EpollEventManager * const mgr = new EpollEventManager ( epollFd );

    if ( !mgr )
    {
        return Error::MemoryError;
    }

    // We don't need to store 'mgr', EventManager's constructor should set the instance pointer:
    assert ( _instance == ( EventManager * ) mgr );

    return Error::Success;
}

EpollEventManager::EpollEventManager ( int epollFd ): _epollFd ( epollFd )
{
    assert ( _epollFd >= 0 );
}

EpollEventManager::~EpollEventManager()
{
#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] ~EpollEventManager(), events.size() = %lu\n",
              getpid(), ( long unsigned ) _events.size() );
#endif

    if ( _epollFd >= 0 )
    {
        ::close ( _epollFd );
        _epollFd = -1;
    }
}

void EpollEventManager::implSetFdHandler ( int fd, FdEventHandler * handler, int events )
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
    fprintf ( stderr, "[%6d] setFdHandler(%d, %lx), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) handler, ( long unsigned ) _events.size() );
#endif

    if ( events != 0 )
    {
        implSetFdEvents ( fd, events );
    }
}

void EpollEventManager::implSetFdEvents ( int fd, int events )
{
    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _events.size() );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] setFdEvents(%d, %d), events.size() = %lu\n",
              getpid(), fd, events, ( long unsigned ) _events.size() );
#endif

    if ( fd < 0 || ( size_t ) fd >= _events.size() ) return;

    FdEventInfo & eInfo = _events[ fd ];

    struct epoll_event ev;

    memset ( &ev, 0, sizeof ( ev ) );

    if ( !events )
    {
        if ( !eInfo.events ) return;

        int ret = epoll_ctl ( _epollFd, EPOLL_CTL_DEL, fd, &ev );

        if ( ret != 0 )
        {
            fprintf ( stderr, "setFdEvents: epoll_ctl(EPOLL_CTL_DEL) for file descriptor %d failed: %s\n",
                      fd, strerror ( errno ) );
        }

        // update eInfo.events with current events state
        eInfo.events = events;
    }
    else if ( eInfo.events != events )
    {
        ev.data.fd = fd;
        ev.events = events;

        if ( !eInfo.events )
        {
            int ret = epoll_ctl ( _epollFd, EPOLL_CTL_ADD, fd, &ev );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: epoll_ctl(EPOLL_CTL_ADD) for "
                          "file descriptor %d and events %d failed: %s\n",
                          fd, events, strerror ( errno ) );
            }
        }
        else
        {
            int ret = epoll_ctl ( _epollFd, EPOLL_CTL_MOD, fd, &ev );

            if ( ret != 0 )
            {
                fprintf ( stderr, "setFdEvents: epoll_ctl(EPOLL_CTL_MOD) for "
                          "file descriptor %d and events %d failed: %s\n",
                          fd, events, strerror ( errno ) );
            }
        }

        // update eInfo.events with current events state
        eInfo.events = events;
    }
}

void EpollEventManager::implRemoveFdHandler ( int fd )
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
            eInfo.events = 0;

            // We don't need it, but kernel versions before 2.6.9 require
            // not-null epoll event parameter, even though it's not used
            struct epoll_event foo;

            memset ( &foo, 0, sizeof ( foo ) );

            int ret = epoll_ctl ( _epollFd, EPOLL_CTL_DEL, fd, &foo );

            if ( ret != 0 )
            {
                // this is normal if something closed it already
#if EVENT_MANAGER_DEBUG_FD_OPS
                fprintf ( stderr, "removeFdHandler: epoll_ctl(EPOLL_CTL_DEL) for file descriptor %d failed: %s\n",
                          fd, strerror ( errno ) );
#endif
            }
        }

        eInfo.handler = 0;
    }

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] removeFdHandler(%d), events.size() = %lu [after]\n",
              getpid(), fd, ( long unsigned ) _events.size() );
#endif
}

void EpollEventManager::implRun()
{
    if ( _working )
        return;

    if ( initSignals() )
    {
#ifdef USE_SIGNALFD
        struct epoll_event ev;

        memset ( &ev, 0, sizeof ( ev ) );

        ev.data.fd = _signalFd;
        ev.events = EventRead;

        int ret = epoll_ctl ( _epollFd, EPOLL_CTL_ADD, _signalFd, &ev );

        if ( ret != 0 )
        {
            fprintf ( stderr, "EpollEventManager:run(): epoll_ctl(EPOLL_CTL_ADD) for "
                      "signalfd (%d) and read event (%d) failed: %s\n", _signalFd,
                      EventRead, strerror ( errno ) );
            return;
        }
#endif
    }

    _working = true;

    struct epoll_event eResult[ MAX_EVENTS ];

    memset ( eResult, 0, sizeof ( struct epoll_event ) * MAX_EVENTS );

    int count;
    int idx;

    while ( _working && !_globalExit )
    {
        count = epoll_wait ( _epollFd, eResult, MAX_EVENTS, getSafeTimeout() );

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
            // It wasn't one of the signals we care about, so epoll_wait had some error
            perror ( "epoll_wait" );
        }
        else
        {
            for ( idx = 0; idx < count; idx++ )
            {
                const int fd = eResult[ idx ].data.fd;

                assert ( fd >= 0 );

                if ( fd < 0 )
                    continue;

#ifdef USE_SIGNALFD
                if ( fd == _signalFd )
                {
                    if ( runProcessSignals() && !_working )
                        return;

                    continue;
                }
#endif

                assert ( ( size_t ) fd < _events.size() );

                if ( _events[ fd ].handler != 0 )
                {
                    assert ( _events[ fd ].handler != 0 );

                    short events = eResult[ idx ].events;

                    if ( ( events & ( EPOLLERR | EPOLLHUP ) ) != 0 )
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
