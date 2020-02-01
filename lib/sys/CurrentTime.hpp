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

#include <ctime>

#include "Time.hpp"

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
struct timespec
{
    time_t tv_sec;
    uint32_t tv_nsec;
};
#endif

namespace Pravala
{
class CurrentTimePriv;

/// @brief A wrapper around Time class, that is used for keeping track of current time.
/// It's normally only used by the TimerManager and whichever code wants to read current time independently of it.
/// @warning This class is not thread safe, a single CurrentTime object should only be used by a single thread.
class CurrentTime: public Time
{
    public:
        /// @brief Default constructor.
        /// When it's created, it updates itself to the current time.
        CurrentTime();

        /// @brief Destructor.
        ~CurrentTime();

        /// @brief Updates current time using readTime().
        /// @note It does NOT update the current time used by the EventManager, so the time read could be greater
        ///       than what EventManager::getCurrentTime() returns after calling this function.
        ///       EventManager::getCurrentTime ( true ) should be used instead of this function.
        ///       This function should only be used when EventManager is not available.
        void update();

        /// @brief Reads current time using a syscall.
        /// It can be used to read time with higher resolution than what Time class supports.
        /// @note This does NOT update the time stored in this object, or in the EventManager.
        /// @warning It should be used only when necessary, as it may be slow!
        /// @param [out] time The structure to store the time read.
        void readTime ( struct timespec & time ) const;

    private:
        CurrentTimePriv * _priv; ///< Platform/implementation specific data.
};
}
