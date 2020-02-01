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
#include "TimerManager.hpp"
#include "basic/Math.hpp"

#include <cstring>

// #define DEBUG_TIMERS 1

#ifdef DEBUG_TIMERS
#include <cstdio>
#endif

using namespace Pravala;

ConfigLimitedNumber<uint16_t> TimerManager::optResolution (
        ConfigOpt::FlagInitializeOnly,
        "os.timers.resolution",
        "The resolution of timers (in milliseconds)",
        1, 1000, 1
);

ConfigLimitedNumber<uint8_t> TimerManager::optBaseLevelBits (
        ConfigOpt::FlagInitializeOnly,
        "os.timers.base_level_bits",
        "The number of bits of the timer tick counter to be represented "
        "by the first (base) level of timer wheels. Between 8 and 30 bits. "
        "Higher values should offer better performance at the cost of higher memory usage.\n\n"
        "8 bits results in an array with 256 entries (0-255) and a number of levels = 4, "
        "each with 256 entries. On 32bit architecture (4b pointers) the total memory "
        "used is 4*256*4b = 4kb. On 64bit architecture (8b pointers) the total memory used is 8kb.\n\n"
        "16 bits results in two levels, each with ~65K entries and 2*512K of memory (on 64bit)\n\n"
        "24 bits results in an array with ~16M entries and 64mb of memory on 32bit architecture "
        "or 128mb on 64bit architecture. If 24 is used, there will be two levels - one with 24 bits, "
        "and the other with 8 bits.\n\n"
        "Other sizes (for 64bit architecture, on 32bit it's half of that):\n"
        "  25 - 256MB\n"
        "  26 - 512MB\n"
        "  27 - 1GB\n"
        "  28 - 2GB\n"
        "  29 - 4GB\n"
        "  30 - 8GB\n",
        8, 30, 8
);

ConfigLimitedNumber<uint16_t> TimerManager::optReadAheadSlots (
        0,
        "os.timers.read_ahead_slots",
        "The number of slots ahead to check for existing timers when calculating the next timeout value; "
        "It can be modified while the program is running",
        1, 0xFFFF, 10
);

TimerManager::TimerVector::TimerVector ( uint8_t bitsOffset, uint8_t descBits ):
    offset ( bitsOffset ),
    size ( ( descBits > 0 ) ? ( 1 << descBits ) : 0 ),
    mask ( ( descBits > 0 ) ? ( ( ~( 0xFFFFFFFF << descBits ) ) << bitsOffset ) : 0 ),
    index ( 0 ),
    values ( 0 )
{
#ifdef DEBUG_TIMERS
    printf ( "Creating TimerVector; Offset: %d; Bits: %d; Size: %u; Mask: %u\n",
             offset, descBits, size, mask );
#endif

    assert ( descBits <= 30 );
    assert ( bitsOffset <= 32 );
    assert ( bitsOffset < 32 || size == 0 );
    assert ( descBits == 0 || size > 0 );
    assert ( descBits == 0 || mask > 0 );
    assert ( descBits > 0 || size == 0 );
    assert ( descBits > 0 || mask == 0 );
    assert ( ( size == 0 && mask == 0 ) || ( size > 0 && mask > 0 ) );

    if ( size > 0 )
    {
        values = new TimerPtr[ size ];
        memset ( values, 0, size * sizeof ( TimerPtr ) );
    }
}

TimerManager::TimerVector::~TimerVector()
{
    removeAllTimers();

    delete[] values;
    values = 0;
}

void TimerManager::TimerVector::removeAllTimers()
{
    if ( size < 1 )
        return;

    for ( uint32_t i = 0; i < size; ++i )
    {
        while ( values[ i ] != 0 )
        {
            values[ i ]->listRemove();
        }
    }
}

static uint8_t getBits ( const int level, uint8_t baseLevelBits )
{
    uint8_t bits = baseLevelBits;

    if ( bits < 8 )
        bits = 8;

    if ( bits > 30 )
        bits = 30;

    // If this is the first (base) level, we simply use the value provided.
    if ( level == 1 )
        return bits;

    // remBits is the number of bits of the tick counter that need to be represented
    // by levels 2-4 (total number of bits is 32, minus 'bits' represented by the first level)
    int remBits = 32 - bits;

    // If level > 2, we already have 'bits' bits represented at level 2
    if ( level > 2 )
        remBits -= bits;

    // If level > 3, we already have 'bits' bits represented at level 3 (on top of what is represented by level 2)
    if ( level > 3 )
        remBits -= bits;

    // There is still more than 'bits' bits to represent, levels 2-4 represent at most the same number of bits
    // as the base level. If, at this level, we still need to represent more than that,
    // we can return that value right away.
    if ( remBits >= bits )
        return bits;

    // There is nothing else to represent
    if ( remBits <= 0 )
        return 0;

    assert ( remBits > 0 );
    assert ( remBits < bits );

    // whatever was left
    return ( uint8_t ) remBits;
}

TimerManager::TimerManager():

// Make sure that the resolution is > 0 and <= 1s
    TimerResolutionMs ( min ( ( uint16_t ) 1000, max ( ( uint16_t ) 1, optResolution.value() ) ) ),

// Make sure that bits are >= 8 and <= 30
    TimerBaseLevelBits ( min ( ( uint8_t ) 30, max ( ( uint8_t ) 8, optBaseLevelBits.value() ) ) ),

    _tv1 ( 0, getBits ( 1, TimerBaseLevelBits ) ),
    _tv2 ( getBits ( 1, TimerBaseLevelBits ), getBits ( 2, TimerBaseLevelBits ) ),
    _tv3 ( _tv2.offset + getBits ( 2, TimerBaseLevelBits ), getBits ( 3, TimerBaseLevelBits ) ),
    _tv4 ( _tv3.offset + getBits ( 3, TimerBaseLevelBits ), getBits ( 4, TimerBaseLevelBits ) ),
    _nextTickTime ( _currentTime ), // _currentTime updates itself when it's created.
    _currentTickTime ( _currentTime ), // _currentTime updates itself when it's created.
    _currentTick ( 0 ),
    _numTimers ( 0 )
{
#ifdef DEBUG_TIMERS
    printf ( "Creating TimerManager; Resolution: %d, BaseLevelBits: %d\n",
             optResolution.value(), TimerBaseLevelBits );
#endif

    assert ( getBits ( 1, TimerBaseLevelBits ) + getBits ( 2, TimerBaseLevelBits )
             + getBits ( 3, TimerBaseLevelBits ) + getBits ( 4, TimerBaseLevelBits ) == 32 );

    _nextTickTime.increaseMilliseconds ( TimerResolutionMs );
}

TimerManager::~TimerManager()
{
}

void TimerManager::removeAllTimers()
{
    _tv1.removeAllTimers();
    _tv2.removeAllTimers();
    _tv3.removeAllTimers();
    _tv4.removeAllTimers();
}

const Time & TimerManager::currentTime ( bool refresh )
{
    if ( refresh )
    {
        _currentTime.update();
    }

    return _currentTime;
}

void TimerManager::startTimer ( Timer & timer, const uint32_t timeout, bool useTimerTime )
{
    if ( _numTimers < 1 )
    {
        // We don't have any timers at the moment.
        // Let's clear the state:

        _currentTime.update();

        _currentTick = 0;

        _nextTickTime = _currentTickTime = _currentTime;
        _nextTickTime.increaseMilliseconds ( TimerResolutionMs );

        _tv1.index = 0;
        _tv2.index = 0;
        _tv3.index = 0;
        _tv4.index = 0;
    }

    timer._expireTick = 0;

    if ( useTimerTime )
    {
        // We don't care about overflows, it should work fine even if we overflow here
        timer._expireTick = _currentTick + ( timeout / TimerResolutionMs );
    }
    else
    {
        // Our "timer time" is the same as or behind the real time, it can never be ahead of it!
        // Timer time is the "real time at which we should be processing current tick'.
        // If we are running late (too much load), the real time will be ahead of us.
        assert ( _currentTime >= _currentTickTime );

        long int timeDiff = _currentTime.getDiffInMilliSeconds ( _currentTickTime );

        assert ( timeDiff >= 0 );

        // Instead of using 'timeout' we need to use 'timeout + timeDiff' as our timeout. When setting
        // the timer, the caller want something to happen 'timeout' ms after the real time, not
        // 'timeout' ms after the theoretical time at which we should have had.

        if ( ( uint32_t ) timeDiff <= ( 0xFFFFFFFF - timeout ) )
        {
            // This means, that 'timeout + timeDiff' is not larger than the max possible timeout to be
            // expressed in uint32_t. So we can use exact values.

            timer._expireTick = _currentTick + ( ( timeout + ( ( uint32_t ) timeDiff ) ) / TimerResolutionMs );
        }
        else
        {
            // This means that 'timeout + timeDiff' is too large to be expressed as a single uint32_t value.
            // But we may be able to add them using 'tick number' arithmetic ;)

            timeDiff /= TimerResolutionMs;
            uint32_t timeoutTicks = timeout / TimerResolutionMs;

            if ( ( uint32_t ) timeDiff <= ( 0xFFFFFFFF - timeout ) )
            {
                // We can represent the 'tick difference' in uint32_t

                timer._expireTick = _currentTick + timeoutTicks + ( ( uint32_t ) timeDiff );
            }
            else
            {
                // We can't represent the difference even using the tick count.
                // The only thing we can do is to use the max time difference (in tick counts)
                timer._expireTick = _currentTick + 0xFFFFFFFF;

                assert ( timer._expireTick + 1 == _currentTick );
            }
        }
    }

    if ( timer._expireTick == _currentTick )
    {
        // The timeout was too small and we should expire right away.
        // Instead of doing this (it could cause problems in timer processing)
        // we schedule this timer to expire at the next tick.
        //
        // The same would happen if expireTick = _currentTick + max_uint32_t + 1,
        // but because we use uint32_t for 'timeout' value (and make sure this
        // is not the case if useTimerTime == false), this should not happen!

        timer._expireTick = _currentTick + 1;
    }

    scheduleTimer ( timer );
}

void TimerManager::scheduleTimer ( Timer & timer )
{
#ifdef DEBUG_TIMERS
    printf ( "Scheduling timer with tick = %u [current: %u]\n", timer._expireTick, _currentTick );
    printf ( "TV4 index: %u\n", _tv4.index );
    printf ( "TV3 index: %u\n", _tv3.index );
    printf ( "TV2 index: %u\n", _tv2.index );
    printf ( "TV1 index: %u\n", _tv1.index );
#endif

    if ( _tv4.size > 0 )
    {
        if ( timer._expireTick < _currentTick
             || ( timer._expireTick & _tv4.mask ) != ( _currentTick & _tv4.mask ) )
        {
            // Either expireTick is in the past (due to tick counter overflow),
            // or the MSBs of expireTick and currentTick are different.
            // This timer should be placed in the right slot at this level.
            //
            // If the MSBs are equal, but expireTick is < currentTick,
            // the expire tick is slightly lower than currentTick. This means
            // that the delay was huge, and close to the size of the uint32_t.
            // We need to put this timer in the current slot at this level.
            // The 'current' slot at this level was just processed, and will be processed
            // again after all the lower wheels come back to this position. This is what we want!

            uint32_t idx = ( timer._expireTick & _tv4.mask ) >> _tv4.offset;

            assert ( idx < _tv4.size );

            timer.listInsert ( _tv4.values + idx );

#ifdef DEBUG_TIMERS
            printf ( "Scheduling in TV4, index: %u\n", idx );
#endif

            return;
        }
    }

    if ( _tv3.size > 0 )
    {
        // See comment in the block above

        if ( timer._expireTick < _currentTick
             || ( timer._expireTick & _tv3.mask ) != ( _currentTick & _tv3.mask ) )
        {
            uint32_t idx = ( timer._expireTick & _tv3.mask ) >> _tv3.offset;

            assert ( idx < _tv3.size );

            timer.listInsert ( _tv3.values + idx );

#ifdef DEBUG_TIMERS
            printf ( "Scheduling in TV3, index: %u\n", idx );
#endif

            return;
        }
    }

    assert ( _tv2.size > 0 );
    assert ( _tv1.size > 0 );

    // See comment in the block above

    if ( timer._expireTick < _currentTick
         || ( timer._expireTick & _tv2.mask ) != ( _currentTick & _tv2.mask ) )
    {
        uint32_t idx = ( timer._expireTick & _tv2.mask ) >> _tv2.offset;

        assert ( idx < _tv2.size );

        timer.listInsert ( _tv2.values + idx );

#ifdef DEBUG_TIMERS
        printf ( "Scheduling in TV2, index: %u\n", idx );
#endif

        return;
    }

    assert ( _tv1.offset == 0 );

    uint32_t idx = timer._expireTick & _tv1.mask;

    assert ( idx < _tv1.size );

    // We don't want timers to be scheduled at current index!
    // startTimer() should make sure that it doesn't happen...
    // Also, when we cascade timers down, the _tv1.index should be equal to _tv1.size
    assert ( idx != _tv1.index );

    timer.listInsert ( _tv1.values + idx );

#ifdef DEBUG_TIMERS
    printf ( "Scheduling in TV1, index: %u\n", idx );
#endif

    return;
}

void TimerManager::runTimers()
{
    if ( _numTimers < 1 )
        return;

    _currentTime.update();

    while ( _nextTickTime <= _currentTime )
    {
        _currentTickTime = _nextTickTime;
        _nextTickTime.increaseMilliseconds ( TimerResolutionMs );

        ++_currentTick;
        ++_tv1.index;

        if ( _tv1.index == _tv1.size )
        {
            // We just hit the end of TV1's array. We need to propagate events from TV2 down.
            ++_tv2.index;

            if ( _tv2.index == _tv2.size )
            {
                _tv2.index = 0;

                // We just hit the end of TV2's array. We need to propagate events from TV3 down.
                if ( _tv3.size > 0 )
                {
                    ++_tv3.index;

                    if ( _tv3.index == _tv3.size )
                    {
                        _tv3.index = 0;

                        // We just hit the end of TV3's array. We need to propagate events from TV4 down.
                        if ( _tv4.size > 0 )
                        {
                            ++_tv4.index;

                            if ( _tv4.index == _tv4.size )
                            {
                                // TV4 is the last level, so we just go back to the beginning
                                _tv4.index = 0;
                            }

#ifdef DEBUG_TIMERS
                            if ( _tv4.values[ _tv4.index ] != 0 )
                                printf ( "Rescheduling timers from TV4.%u (current: %u)\n", _tv4.index, _currentTick );
#endif

                            while ( _tv4.values[ _tv4.index ] != 0 )
                            {
                                assert ( _tv4.values[ _tv4.index ]->_expireTick >= _currentTick
                                         && ( _tv4.values[ _tv4.index ]->_expireTick & _tv4.mask )
                                         == ( _currentTick & _tv4.mask ) );

                                scheduleTimer ( *_tv4.values[ _tv4.index ] );
                            }
                        }
                    }

#ifdef DEBUG_TIMERS
                    if ( _tv3.values[ _tv3.index ] != 0 )
                        printf ( "Rescheduling timers from TV3.%u (current: %u)\n", _tv3.index, _currentTick );
#endif

                    while ( _tv3.values[ _tv3.index ] != 0 )
                    {
                        assert ( _tv3.values[ _tv3.index ]->_expireTick >= _currentTick
                                 && ( _tv3.values[ _tv3.index ]->_expireTick & _tv3.mask )
                                 == ( _currentTick & _tv3.mask ) );

                        scheduleTimer ( *_tv3.values[ _tv3.index ] );
                    }
                }
            }

#ifdef DEBUG_TIMERS
            if ( _tv2.values[ _tv2.index ] != 0 )
                printf ( "Rescheduling timers from TV2.%u (current: %u)\n", _tv2.index, _currentTick );
#endif

            while ( _tv2.values[ _tv2.index ] != 0 )
            {
                assert ( _tv2.values[ _tv2.index ]->_expireTick >= _currentTick
                         && ( _tv2.values[ _tv2.index ]->_expireTick & _tv2.mask )
                         == ( _currentTick & _tv2.mask ) );

                scheduleTimer ( *_tv2.values[ _tv2.index ] );
            }

            // Let's make sure nothing modified it:
            assert ( _tv1.index == _tv1.size );

            // We have to set this to 0 at the end, otherwise scheduleTimer would
            // crash when assigning timers from TV2 in TV1[0] (this crash is just for debugging, but we want
            // to make sure nobody schedules timer from outside at current index!):
            _tv1.index = 0;

            // This is the end of:
            // if ( _tv1.index == _tv1.size )
        }

        assert ( _tv1.index < _tv1.size );

#ifdef DEBUG_TIMERS
        if ( _tv1.values[ _tv1.index ] != 0 )
            printf ( "Expiring timer(s) from TV1.%u (current: %u)\n", _tv1.index, _currentTick );
#endif

        while ( _tv1.values[ _tv1.index ] != 0 )
        {
            assert ( _tv1.values[ _tv1.index ]->_expireTick == _currentTick );

            _tv1.values[ _tv1.index ]->expire();
        }
    }
}

int TimerManager::getTimeout()
{
    if ( _numTimers < 1 )
        return -1;

    Time nextTick = _nextTickTime;

    // The next slot that will be run is _tv1.index + 1.
    uint32_t idx = _tv1.index + 1;

    // By taking the _nextTickTime we already "inspected" the next slot.
    // So we only want to do anything extra if optReadAheadSlots > 1.
    // Also, we only want to go up to _tv1.size - when we reach the max size, we will
    // propagate timers from TV2. This may affect the content of TV1's slots.
    // Alternatively we could inspect TV2's slots as well, but we would have to figure out where
    // the timers would go after they're propagated to TV1.
    // This becomes more complex, so let's not do that.

    for ( uint32_t i = 1; i < optReadAheadSlots.value() && idx < _tv1.size; ++idx, ++i )
    {
#ifdef DEBUG_TIMERS
        fprintf ( stderr, "{%d,%d}", i, idx );
#endif

        // If the next slot to run is NOT empty, this means that we want to run that slot
        // at the current nextTick time!
        if ( _tv1.values[ idx ] != 0 )
            break;

        // Otherwise we can increase the nextTick time by one period:
        nextTick.increaseMilliseconds ( TimerResolutionMs );
    }

    // This updates _currentTime as well:
    const int msDiff = nextTick.getDiffInMilliSeconds ( currentTime ( true ) );

#ifdef DEBUG_TIMERS
    fprintf ( stderr, "\n[NT:%u.%u, _NT:%u.%u, T:%u.%u, %d]\n",
              nextTick.getSeconds(), nextTick.getMilliSeconds(),
              _nextTickTime.getSeconds(), _nextTickTime.getMilliSeconds(),
              _currentTime.getSeconds(), _currentTime.getMilliSeconds(),
              msDiff );
#endif

    if ( msDiff < 0 )
    {
        return 0;
    }
    else if ( msDiff == 0 )
    {
        // The milliseconds difference is the same. If the actual difference (including nanoseconds)
        // says that the next tick is in the future, then return 1. Otherwise, the actual time
        // has already past, so return 0 so that the timer will get run as soon as possible.
        const int ret = ( nextTick > _currentTime ) ? 1 : 0;

#ifdef DEBUG_TIMERS
        fprintf ( stderr, "Ret: %d\n", ret );
#endif

        return ret;
    }
    else
    {
        return msDiff;
    }
}
