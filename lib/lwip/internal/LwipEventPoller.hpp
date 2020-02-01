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
#include "event/Timer.hpp"

namespace Pravala
{
/// @brief Singleton class that manages lwIP initialization and event polling.
/// @note This class is not thread-safe.
class LwipEventPoller: public NoCopy, protected Timer::Receiver
{
    protected:
        /// @brief Returns the global instance of LwipEventPoller.
        /// @return the global instance of the LwipEventPoller.
        static LwipEventPoller & get();

        /// @brief Increments reference counter. The first reference will start event polling.
        inline void ref()
        {
            if ( ++_refCount == 1 )
            {
                start();
            }
            else
            {
                checkLwipTimer();
            }
        }

        /// @brief Decrements reference counter. The last unreference will stop event polling.
        inline void unref()
        {
            if ( --_refCount < 1 )
            {
                stop();
            }
            else
            {
                checkLwipTimer();
            }
        }

        /// @brief Configures timer for handling internal lwip events.
        /// If it needs to be run or changed, it will get scheduled.
        /// It should be called after calling lwip functions that could potentially
        /// change the internal lwIP state resulting in scheduling new events.
        void checkLwipTimer();

        virtual void timerExpired ( Timer * timer );

    private:
        /// @brief The number of references to LwipEventPoller.
        /// If there are 0 references, this class will stop event polling.
        size_t _refCount;

        SimpleTimer _timer; ///< Timer for lwIP event polling

        /// @brief The time of the next event we scheduled our timer to run at.
        uint32_t _nextEvent;

        bool _running; ///< Whether we should be running or not.

        /// @brief Constructor
        LwipEventPoller();

        /// @brief Starts polling for lwIP events
        void start();

        /// @brief Stops polling for lwIP events
        void stop();

        friend class LwipInterface;
        friend class LwipSocket;
};
}
