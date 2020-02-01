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

#include "sys/OsUtils.hpp"
#include "EventManager.hpp"

#ifdef SYSTEM_WINDOWS
extern "C"
{
// For closesocket:
#include <winsock2.h>
}
#endif

using namespace Pravala;

Mutex EventManager::_mutex ( "event_manager" );
size_t EventManager::_numManagers ( 0 );
bool EventManager::_primaryManagerExists ( false );

THREAD_LOCAL EventManager * EventManager::_instance ( 0 );

EventManager::EventManager():
    _isPrimaryManager ( newManagerCreated() ),
    _working ( false ),
    _currentEndOfLoopId ( 1 )
{
    assert ( !_instance );

    _instance = this;
}

EventManager::~EventManager()
{
    assert ( _instance == this );

    _instance = 0;

    _mutex.lock();

    assert ( _numManagers > 0 );

    --_numManagers;

    if ( _isPrimaryManager )
    {
        _primaryManagerExists = false;
    }

    _mutex.unlock();
}

bool EventManager::newManagerCreated()
{
    bool newHandler = false;

    _mutex.lock();

    if ( !_primaryManagerExists )
    {
        _primaryManagerExists = true;
        newHandler = true;
    }

    ++_numManagers;

    _mutex.unlock();

    return newHandler;
}

size_t EventManager::getNumManagers()
{
    size_t ret;

    _mutex.lock();

    ret = _numManagers;

    _mutex.unlock();

    return ret;
}

ERRCODE EventManager::shutdown ( bool force )
{
    if ( !_instance )
    {
        return Error::NotInitialized;
    }

    const ERRCODE eCode = _instance->implShutdown ( force );

    if ( IS_OK ( eCode ) )
    {
        delete _instance;
        _instance = 0;
    }

    return eCode;
}

ERRCODE EventManager::implShutdown ( bool force )
{
    if ( _working )
    {
        // We always want to make sure that this manager is not running anymore.

        fprintf ( stderr, "Error shutting down EventManager: it is marked as 'working'\n" );

        return Error::WrongState;
    }

    ShutdownHandler * lastHandler = 0;

    // We can't simply iterate over the list and call the receiveShutdownEvent on each element.
    // Calling it in one of the objects could cause also other objects subscribed to this list to be removed.
    // This means that we would be using invalid pointer.
    // Instead we always call the callback in the first element until there are no more elements.
    // Objects upon receiving that callback should, at the very least, unsubscribe from those events.
    // In case that doesn't happen, we will check whether the first object is the same the next time
    // and instead of calling the callback again, we just remove that pointer from the list.

    while ( !_shutdownHandlers.isEmpty() )
    {
        if ( _shutdownHandlers.first() == lastHandler )
        {
            _shutdownHandlers.removeFirst();
        }
        else
        {
            lastHandler = _shutdownHandlers.first();
            lastHandler->receiveShutdownEvent();
        }
    }

    if ( !force )
    {
        if ( getNumTimers() > 0
             || !_loopEndQueue.isEmpty()
             || !_processedLoopEndQueue.isEmpty()
             || !_signalHandlers.isEmpty()
             || !_childHandlers.isEmpty() )
        {
            fprintf ( stderr, "Error shutting down EventManager; "
                      "Timers: %lu; Loop-End-Queue: %lu; Processed-LEQ: %lu; Signal: %lu; Child: %lu\n",
                      ( long unsigned ) getNumTimers(),
                      ( long unsigned ) _loopEndQueue.size(),
                      ( long unsigned ) _processedLoopEndQueue.size(),
                      ( long unsigned ) _signalHandlers.size(),
                      ( long unsigned ) _childHandlers.size() );

            return Error::WrongState;
        }

        for ( size_t i = 0; i < _events.size(); ++i )
        {
            if ( _events[ i ].handler != 0 || _events[ i ].events != 0 )
            {
                fprintf ( stderr, "Error shutting down EventManager; Existing FD handler; FD: %lu\n",
                          ( long unsigned ) i );

                return Error::WrongState;
            }
        }
    }
    else
    {
        TimerManager::removeAllTimers();

        for ( size_t i = 0; i < _events.size(); ++i )
        {
            if ( _events[ i ].handler != 0 || _events[ i ].events != 0 )
            {
                _events[ i ].handler = 0;
                _events[ i ].events = 0;

                ::close ( i );
            }
        }
    }

    return Error::Success;
}

void EventManager::stop()
{
    if ( _instance != 0 )
    {
        _instance->_working = false;
    }
}

void EventManager::run()
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    if ( _instance != 0 )
    {
        _instance->implRun();
    }
}

const Time & EventManager::getCurrentTime ( bool refresh )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    if ( !_instance )
    {
        // We do have to return something...
        static Time zeroTime;

        return zeroTime;
    }

    return _instance->currentTime ( refresh );
}

bool EventManager::closeFd ( int fd )
{
    if ( fd < 0 )
        return false;

    if ( _instance != 0 )
    {
        _instance->implRemoveFdHandler ( fd );
    }

#ifdef SYSTEM_WINDOWS
    // On windows calling regular close() on socket crashes.
    // Calling closesocket on regular descriptor just causes an error.
    // Also, most (all on windows?) of the descriptors are sockets anyway.
    // So let's try to treat it like a socket first:
    if ( ::closesocket ( fd ) == 0 )
        return true;
#endif

    return ( ::close ( fd ) == 0 );
}

int EventManager::getFdEvents ( int fd )
{
    if ( _instance != 0 && fd >= 0 && ( size_t ) fd < _instance->_events.size() )
    {
        return _instance->_events[ fd ].events;
    }

    return 0;
}

void EventManager::setFdHandler ( int fd, EventManager::FdEventHandler * handler, int events )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( fd >= 0 );
    assert ( handler != 0 );

    if ( _instance != 0 && fd >= 0 && handler != 0 )
    {
        _instance->implSetFdHandler ( fd, handler, events );
    }
}

void EventManager::setFdEvents ( int fd, int events )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _instance->_events.size() );
    assert ( _instance->_events[ fd ].handler != 0 );

    if ( _instance != 0
         && fd >= 0
         && ( size_t ) fd < _instance->_events.size()
         && _instance->_events[ fd ].handler != 0
         && events != _instance->_events[ fd ].events )
    {
        _instance->implSetFdEvents ( fd, events );
    }
}

void EventManager::removeFdHandler ( int fd )
{
    if ( _instance != 0 && fd >= 0 && ( size_t ) fd < _instance->_events.size() )
    {
        _instance->implRemoveFdHandler ( fd );
    }
}

void EventManager::enableWriteEvents ( int fd )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _instance->_events.size() );
    assert ( _instance->_events[ fd ].handler != 0 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] enableWriteEvents(%d), events.size() = %ld\n",
              getpid(), fd, ( !_instance ) ? ( -1 ) : ( ( long ) _instance->_events.size() ) );
#endif

    int events;

    if ( _instance != 0
         && fd >= 0
         && ( size_t ) fd < _instance->_events.size()
         && _instance->_events[ fd ].handler != 0
         && ( events = ( _instance->_events[ fd ].events | EventWrite ) ) != _instance->_events[ fd ].events )
    {
        _instance->implSetFdEvents ( fd, events );
    }
}

void EventManager::disableWriteEvents ( int fd )
{
    assert ( fd >= 0 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] disableWriteEvents(%d), events.size() = %ld\n",
              getpid(), fd, ( !_instance ) ? ( -1 ) : ( ( long ) _instance->_events.size() ) );
#endif

    int events;

    if ( _instance != 0
         && fd >= 0
         && ( size_t ) fd < _instance->_events.size()
         && _instance->_events[ fd ].handler != 0
         && ( events = ( _instance->_events[ fd ].events & ( ~EventWrite ) ) ) != _instance->_events[ fd ].events )
    {
        _instance->implSetFdEvents ( fd, events );
    }
}

void EventManager::enableReadEvents ( int fd )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( fd >= 0 );
    assert ( ( size_t ) fd < _instance->_events.size() );
    assert ( _instance->_events[ fd ].handler != 0 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] enableReadEvents(%d), events.size() = %ld\n",
              getpid(), fd, ( !_instance ) ? ( -1 ) : ( ( long ) _instance->_events.size() ) );
#endif

    int events;

    if ( _instance != 0
         && fd >= 0
         && ( size_t ) fd < _instance->_events.size()
         && _instance->_events[ fd ].handler != 0
         && ( events = ( _instance->_events[ fd ].events | EventRead ) ) != _instance->_events[ fd ].events )
    {
        _instance->implSetFdEvents ( fd, events );
    }
}

void EventManager::disableReadEvents ( int fd )
{
    assert ( fd >= 0 );

#if EVENT_MANAGER_DEBUG_FD_OPS
    fprintf ( stderr, "[%6d] disableReadEvents(%d), events.size() = %ld\n",
              getpid(), fd, ( !_instance ) ? ( -1 ) : ( ( long ) _instance->_events.size() ) );
#endif

    int events;

    if ( _instance != 0
         && fd >= 0
         && ( size_t ) fd < _instance->_events.size()
         && _instance->_events[ fd ].handler != 0
         && ( events = ( _instance->_events[ fd ].events & ( ~EventRead ) ) ) != _instance->_events[ fd ].events )
    {
        _instance->implSetFdEvents ( fd, events );
    }
}

void EventManager::setChildHandler ( int pid, ChildEventHandler * handler )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( handler != 0 );

    if ( _instance != 0 )
    {
        _instance->_childHandlers.insert ( pid, handler );
    }
}

void EventManager::removeChildHandler ( int pid )
{
    if ( _instance != 0 )
    {
        _instance->_childHandlers.remove ( pid );
    }
}

void EventManager::loopEndSubscribe ( LoopEndEventHandler * handler )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    assert ( handler != 0 );
    assert ( _instance->_currentEndOfLoopId != 0 );

    if ( handler != 0 && _instance != 0 && handler->_endOfLoopId != _instance->_currentEndOfLoopId )
    {
        // Valid handler, EventManager exists and this handler is not subscribed
        // to the currently running end-of-loop queue.

        handler->_endOfLoopId = _instance->_currentEndOfLoopId;

        _instance->_loopEndQueue.append ( handler );
    }
}

void EventManager::loopEndUnsubscribe ( LoopEndEventHandler * handler )
{
    assert ( handler != 0 );

    if ( handler != 0 && handler->_endOfLoopId != 0 && _instance != 0 )
    {
        // Valid handler, it is subscribed and we have an EventManager

        handler->_endOfLoopId = 0;

        _instance->_loopEndQueue.removeValue ( handler );
        _instance->_processedLoopEndQueue.removeValue ( handler );
    }
}

void EventManager::signalSubscribe ( EventManager::SignalHandler * handler )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    if ( handler != 0 && _instance != 0 )
    {
        for ( size_t i = 0; i < _instance->_signalHandlers.size(); ++i )
        {
            if ( _instance->_signalHandlers[ i ] == handler )
            {
                return;
            }
        }

        _instance->_signalHandlers.append ( handler );
    }
}

void EventManager::signalUnsubscribe ( EventManager::SignalHandler * handler )
{
    if ( handler != 0 && _instance != 0 )
    {
        _instance->_signalHandlers.removeValue ( handler );
    }
}

void EventManager::shutdownSubscribe ( ShutdownHandler * handler )
{
    // This function should only be used once EventManager is initialized:
    assert ( _instance != 0 );

    if ( handler != 0 && _instance != 0 )
    {
        for ( size_t i = 0; i < _instance->_shutdownHandlers.size(); ++i )
        {
            if ( _instance->_shutdownHandlers[ i ] == handler )
            {
                return;
            }
        }

        _instance->_shutdownHandlers.append ( handler );
    }
}

void EventManager::shutdownUnsubscribe ( ShutdownHandler * handler )
{
    if ( handler != 0 && _instance != 0 )
    {
        _instance->_shutdownHandlers.removeValue ( handler );
    }
}

void EventManager::runEndOfLoop()
{
    runTimers();

    _processedLoopEndQueue = _loopEndQueue;
    _loopEndQueue.clear();

    if ( ( ++_currentEndOfLoopId ) == 0 )
    {
        _currentEndOfLoopId = 1;
    }

    while ( !_processedLoopEndQueue.isEmpty() )
    {
        LoopEndEventHandler * handler = _processedLoopEndQueue.first();

        assert ( handler != 0 );

        _processedLoopEndQueue.removeFirst();

        if ( handler->_endOfLoopId != _currentEndOfLoopId )
        {
            // We don't want to set it to 0 if it already is subscribed to the next version of the EOL queue
            // It is possible if some earlier callback inside this loop already re-subscribed this handler
            // (different than the one that received that earlier callback).

            handler->_endOfLoopId = 0;
        }

        handler->receiveLoopEndEvent();
    }
}

void EventManager::notifySignalHandlers ( int sigRcvd )
{
    for ( size_t i = 0; i < _signalHandlers.size(); ++i )
    {
        if ( _signalHandlers[ i ] != 0 )
        {
            _signalHandlers[ i ]->receiveSignalEvent ( sigRcvd );
        }
    }
}
