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

#include "Timer.hpp"
#include "EventManager.hpp"

#include <cstdio>

using namespace Pravala;

Timer::Timer ( Receiver & receiver ):
    _myReceiver ( receiver ), _next ( 0 ), _previousNext ( 0 ), _expireTick ( 0 )
{
}

Timer::~Timer()
{
    listRemove();
}

void Timer::expire()
{
    if ( _previousNext != 0 )
        listRemove();

    _myReceiver.timerExpired ( this );
}

void Timer::start ( uint32_t timeout, bool useTimerTime )
{
    assert ( EventManager::getInstance() != 0 );

    EventManager::getInstance()->startTimer ( *this, timeout, useTimerTime );
}

bool Timer::listRemove()
{
    if ( _previousNext != 0 )
    {
        *_previousNext = _next;

        if ( _next != 0 )
        {
            _next->_previousNext = _previousNext;
            _next = 0;
        }

        _previousNext = 0;

        assert ( EventManager::getInstance() != 0 );
        assert ( EventManager::getInstance()->_numTimers > 0 );

        --EventManager::getInstance()->_numTimers;

        return true;
    }

    return false;
}

void Timer::listInsert ( Timer ** listHead )
{
    assert ( listHead != 0 );

    if ( _previousNext != 0 )
        listRemove();

    assert ( !_next );
    assert ( !_previousNext );
    assert ( EventManager::getInstance() != 0 );

    if ( EventManager::getInstance()->_numTimers + 1 == 0 )
    {
        fprintf ( stderr, "Too many timers - not starting the next one!\n" );

        // This could happen if we had A LOT of timers...
        assert ( false );

        return;
    }

    // This is a bit tricky. listHead is a pointer to the "head pointer" itself.
    // so we are setting our _previousNext to the variable that points to the first element of the list.
    // When we remove this element, we will set the CONTENT of _previousNext to 0 and, effectively,
    // make the list empty.

    _previousNext = listHead;
    _next = *listHead;
    *listHead = this;

    if ( _next != 0 )
    {
        _next->_previousNext = &_next;
    }

    ++EventManager::getInstance()->_numTimers;
}
