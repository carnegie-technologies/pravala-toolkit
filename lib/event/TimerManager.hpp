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

#include "basic/NoCopy.hpp"
#include "config/ConfigNumber.hpp"
#include "sys/CurrentTime.hpp"

namespace Pravala
{
class Timer;
typedef Timer * TimerPtr; ///< Pointer to the Timer class

/// @brief Timer Manager.
/// To be inherited by Event Manager
class TimerManager: public NoCopy
{
    public:
        static ConfigLimitedNumber<uint16_t> optResolution; ///< Timer resolution in milliseconds
        static ConfigLimitedNumber<uint16_t> optReadAheadSlots; ///< The number of slots ahead to check for timers
        static ConfigLimitedNumber<uint8_t> optBaseLevelBits; ///< Number of bits for the base level

        const uint16_t TimerResolutionMs; ///< Timer resolution in milliseconds
        const uint8_t TimerBaseLevelBits; ///< The number of bits of the timer tick counter at the first level

        /// @brief Returns the number of timers scheduled.
        /// @return The number of timers scheduled.
        inline size_t getNumTimers() const
        {
            return _numTimers;
        }

    protected:
        /// @brief Constructor.
        TimerManager();

        /// @brief Destructor.
        ~TimerManager();

        /// @brief Schedules given timer to expire after the timeout given
        /// @param [in] timer The timer to scheduler
        /// @param [in] timeout The timeout (in ms) after which this timer should expire
        /// @param [in] useTimerTime Whether the 'timer time' (=true) or real time (=false) should be used.
        /// @note Have a look at a comment of startTimer() function in Timer class.
        void startTimer ( Timer & timer, uint32_t timeout, bool useTimerTime );

        /// @brief Processes and expires timers.
        void runTimers();

        /// @brief Returns the timeout to be used by epoll_wait or equivalent
        ///
        /// The timeout is the time since now till the next time a timer slot should be run.
        /// If it is 0 it means that we should run the timer right away.
        /// The number of slots inspected is configured using 'os.timers.read_ahead_slots' config option.
        /// It could also be -1, which means there are no timers scheduled.
        ///
        /// @return The number of milliseconds after which the runTimers should be run.
        ///         0 for "right away", -1 for "no timers scheduled"
        int getTimeout();

        /// @brief A helper function that removes all timers.
        void removeAllTimers();

        /// @brief Exposes current time used by this TimerManager.
        /// @note This is not named "getCurrentTime()" to avoid ambiguity with the static version in EventManager.
        /// @param [in] refresh If true, current time will be refreshed before it's returned.
        ///                     This is somewhat heavier operation, so the use should be limited to cases that
        ///                     require most up-to-date time.
        /// @return Current time from the moment it was last refreshed.
        const Time & currentTime ( bool refresh );

    private:

        /// @brief A class representing a single Timer Vector
        struct TimerVector: public NoCopy
        {
            const uint8_t offset; ///< Bit offset in 32bit tick counter at bits represented by this vector start
            const uint32_t size; ///< Size of the internal array
            const uint32_t mask; ///< The mask to use for comparing current tick and 'tick to expire'

            uint32_t index; ///< Current index in the internal array
            TimerPtr * values; ///< The array with Timer pointers

            /// @brief Constructor
            /// @param [in] bitsOffset The offset (in bits) at which bits of tick counter
            ///                         represented by this vector start
            /// @param [in] descBits The number of bits this vector describes (0, or 8-24)
            TimerVector ( uint8_t bitsOffset, uint8_t descBits );

            /// @brief Destructor
            ~TimerVector();

            /// @brief A helper function that removes all timers.
            void removeAllTimers();
        };

        /// @brief The first timer vector
        ///
        /// The least significant bits, the 'base level'
        TimerVector _tv1;

        TimerVector _tv2; ///< The second timer vector
        TimerVector _tv3; ///< The third timer vector

        /// @brief The fourth timer vector
        ///
        /// The most significant bits,
        /// unless it represents 0 bits -  in which case the TV3 (or even TV2) is used instead
        TimerVector _tv4;

        CurrentTime _currentTime; ///< The current time (when it was last refreshed).

        Time _nextTickTime; ///< The time at which the next tick should be run
        Time _currentTickTime; ///< The 'time' at which current (or the last one) tick should be running
        uint32_t _currentTick; ///< The current tick counter

        size_t _numTimers; ///< The number of timers scheduled

        /// @brief (Re)schedules the timer to run at a specific time slot.
        /// @param [in] timer The timer to schedule. If it already is a member of the list it is removed
        ///              from that list first. It should have the proper _expiryTick set.
        void scheduleTimer ( Timer & timer );

        friend class Timer;
};
}
