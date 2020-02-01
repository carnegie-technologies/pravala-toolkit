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

#pragma once

extern "C"
{
#include <stdint.h>
}

namespace Pravala
{
/// @brief A base timer class.
/// Different Timer implementation can determine specific delay/interval policies.
/// This is the common part that gets triggered after some delay, and calls timerExpired() in its owner.
/// Also, when that callback is called, the timer is stopped.
class Timer
{
    public:
        /// @brief To be inherited by classes that wish to receive timer events.
        class Receiver
        {
            protected:

                /// @brief Timer callback in the object.
                /// When it is called, the timer is already stopped.
                /// @param [in] timer The pointer to the timer that expired
                virtual void timerExpired ( Timer * timer ) = 0;

                /// @brief Virtual destructor
                virtual ~Receiver()
                {
                }

                friend class Timer;
        };

        /// @brief Returns true if the timer is running
        /// @return true if the timer is running; false otherwise
        inline bool isActive() const
        {
            return ( _previousNext != 0 );
        }

    protected:
        /// @brief Constructor
        /// @param [in] receiver The receiver to be notified by this timer's expiry events
        Timer ( Receiver & receiver );

        /// @brief Destructor
        ~Timer();

        /// @brief Starts the timer
        ///
        /// If the timer is already running, it is stopped first.
        ///
        /// @param [in] timeout The time (in milliseconds) after which this timer will expire. The timer accuracy
        ///              depends on the resolution (EventManager::TimerResolutionMs). Two timers with a timeout
        ///              difference of less than that resolution are indistinguishable. They can also run
        ///              in ANY order! So if the resolution is 10ms and we start two timers at the same time,
        ///              on of them to expire in 11ms and the other in 19ms from now, they will expire at the
        ///              same time. Also, the callback for the 19ms timer may be called BEFORE the 11ms one!
        ///
        ///              If the timeout used is smaller than the resolution, the timer should, in theory, run
        ///              right away. Instead, it is scheduled to run at the next timer tick.
        ///
        /// @param [in] useTimerTime If true, instead of using current time as the base for
        ///              calculating the timer's delay, the theoretical time at which timers
        ///              should be running at this point will be used. It is false by default.
        ///              It should be used when we need to have timers running at exact intervals.
        ///              It is possible for timers to be late, depending on the system load, amount
        ///              of work to be done, number of timers, etc. Whenever we set the timer in X ms
        ///              in the future, it is set to run in X seconds from the real current time.
        ///              By passing true as the second parameter of this function, we set the timer to
        ///              be run X ms from the time current timer should be running (which could be
        ///              in the past). If this function is called not inside of timer callback,
        ///              the "timer time" of the previous timer tick is used.
        void start ( uint32_t timeout, bool useTimerTime = false );

        /// @brief Stops the timer
        /// @return True if the timer was running, false otherwise
        inline bool stop()
        {
            return listRemove();
        }

        /// @brief Expires the timer
        /// It will stop this timer if necessary (remove it from the list) and notify this timer's owner
        void expire();

        /// @brief Removes this timer from the list
        /// @return True if the timer was part of a list before, false otherwise.
        bool listRemove();

        /// @brief Inserts this timer to the list
        /// If this timer is already a part of a list it is removed first.
        /// @param [in] listHead The pointer to the head of the list to insert this timer to
        void listInsert ( Timer ** listHead );

    private:
        Receiver & _myReceiver; ///< Receiver to be notified about expiry of this timer

        Timer * _next; ///< The next timer in the list

        /// @brief The pointer to the 'next' pointer in the previous Timer in the list (or the head)
        /// Storing pointer-to-pointer instead of simple pointer makes things (especially dealing with
        /// the first element of the list) easier.
        Timer ** _previousNext;

        /// @brief The 'tick' this timer should expire at
        /// Each tick is equivalent to a single 'resolution period' (defined in EventManager).
        /// For instance, if the resolution is 10ms, one tick represents a period of 10ms.
        uint32_t _expireTick;

        friend class TimerManager;
};

/// @brief A simple single-shot timer implementation.
class SimpleTimer: public Timer
{
    public:
        /// @brief Constructor
        /// @param [in] receiver The receiver to be notified by this timer's expiry events
        inline SimpleTimer ( Timer::Receiver & receiver ): Timer ( receiver )
        {
        }

        using Timer::start;
        using Timer::stop;
};

/// @brief A simple fixed-interval timer implementation.
/// It has a fixed interval and useTimerTime value (see base Timer).
class FixedTimer: public Timer
{
    public:
        const uint32_t FixedTimeout; ///< The fixed time value (in milliseconds). See base Timer class for details.
        const bool UseTimerTime; ///< Whether to use 'timer time'. See base Timer class for details.

        /// @brief Constructor.
        /// @param [in] receiver The receiver to be notified by this timer's expiry events.
        /// @param [in] timeout The fixed time value (in milliseconds). See base Timer class for details.
        /// @param [in] useTimerTime Whether to use 'timer time'. See base Timer class for details.
        inline FixedTimer ( Timer::Receiver & receiver, uint32_t timeout, bool useTimerTime = false ):
            Timer ( receiver ),
            FixedTimeout ( timeout ),
            UseTimerTime ( useTimerTime )
        {
        }

        /// @brief Starts the timer using fixed parameters.
        /// If the timer is already running, it is stopped first.
        inline void start()
        {
            Timer::start ( FixedTimeout, UseTimerTime );
        }

        using Timer::stop;
};
}
