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

#include "Timer.hpp"

namespace Pravala
{
/// @brief A timer that expires at exponentially longer intervals.
class ExponentialTimer: public Timer
{
    public:
        /// @brief The constant that the current expiry interval is multiplied by each time the timer is started.
        const double BackoffMultiplier;

        /// @brief The interval (in milliseconds) before the timer expires on the first start (or restart).
        const uint32_t StartingInterval;

        /// @brief The max value (in milliseconds) that the current expiry interval is allowed to increase to.
        /// Once the current expiry interval reaches the max value, it will not increase any more.
        const uint32_t MaxInterval;

        /// @brief Whether to use 'timer time'. See base Timer class for details.
        const bool UseTimerTime;

        /// @brief Constructor
        /// @param [in] receiver The receiver to be notified by this timer's expiry events.
        /// @param [in] startingInterval The interval (in milliseconds) before the timer expires on
        ///                              the first start (or restart).
        /// @param [in] backoffMultiplier The constant that the current expiry interval is multiplied by
        ///                               each time the timer is started. This is always initialized to at least 1.0.
        /// @param [in] maxInterval The max value (in milliseconds) that the current expiry interval is
        ///                         allowed to increase to. This is always initialized to at least StartingInterval.
        /// @param [in] useTimerTime Whether to use 'timer time'. See base Timer class for details.
        ExponentialTimer (
            Timer::Receiver & receiver, uint32_t startingInterval, double backoffMultiplier,
            uint32_t maxInterval, bool useTimerTime = false );

        /// @brief Starts the exponential timer.
        /// If the timer is already running, it will be restarted.
        /// @return The delay after which this timer will fire (in milliseconds).
        uint32_t start();

        /// @brief Stops the exponential timer.
        /// This always resets the current expiry interval to the starting value.
        void stop();

    private:
        /// @brief The interval to be used next time the timer is started (in milliseconds).
        uint32_t _nextInterval;
};
}
