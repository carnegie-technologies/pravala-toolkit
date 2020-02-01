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
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
}

#include <cassert>

#include "basic/Random.hpp"
#include "sys/Time.hpp"
#include "event/EventManager.hpp"

#include "LwipEventPoller.hpp"

using namespace Pravala;

/// @brief Returns a timestamp in milliseconds, this is only used by lwIP for time diffs
/// @return The current timestamp in milliseconds, this is only used by lwIP for time diffs
u32_t sys_now ( void )
{
    static Time startTime;

    if ( startTime.isZero() )
        startTime = EventManager::getCurrentTime();

    return ( uint32_t ) EventManager::getCurrentTime().getDiffInMilliSeconds ( startTime );
}

LwipEventPoller & LwipEventPoller::get()
{
    static LwipEventPoller * global = 0;

    if ( !global )
    {
        // lwip_init will randomize port numbers (using rand()). To make sure they are actually random,
        // let's initialize the random number generator. Note that if it has already been initialized
        // by something else - nothing will happen.

        Random::init();

        // This is only safe to call once. There is no deinit function.
        lwip_init();

        global = new LwipEventPoller();
    }

    return *global;
}

LwipEventPoller::LwipEventPoller(): _refCount ( 0 ), _timer ( *this ), _nextEvent ( 0 ), _running ( false )
{
}

void LwipEventPoller::start()
{
    _running = true;
    _nextEvent = 0;
    checkLwipTimer();
}

void LwipEventPoller::stop()
{
    _running = false;
    _nextEvent = 0;
    _timer.stop();
}

void LwipEventPoller::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer == &_timer );

    if ( !_running )
        return;

    const uint32_t delay = sys_timeouts_sleeptime();

    if ( delay < 1 )
    {
        // delay = 0 means that timers should run right away.
        sys_check_timeouts();
    }

    checkLwipTimer();
}

void LwipEventPoller::checkLwipTimer()
{
    if ( !_running )
        return;

    const uint32_t delay = sys_timeouts_sleeptime();

    if ( delay == 0xFFFFFFFFU )
    {
        // There are no timers. Nothing to do!
        return;
    }

    const uint32_t nextEvent = sys_now() + delay;

    if ( nextEvent == _nextEvent && _timer.isActive() )
        return;

    _nextEvent = nextEvent;
    _timer.start ( delay );
}
