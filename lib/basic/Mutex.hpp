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

// Define MUTEX_DEBUGGING to enable logging before and after each mutex operation:
// #define MUTEX_DEBUGGING

namespace Pravala
{
/// @brief A wrapper around system-specific mutex
class Mutex: public NoCopy
{
    public:
        /// @brief Default constructor
        /// Unless the fast mode is set to true, this creates a recursive mutex, which can be locked multiple
        /// times by the same thread. It then has to be unlocked the same number of times.
        /// This helps avoiding deadlocks, but comes at some performance penalty when using pthreads.
        /// By default recursive mode is used. If the owner of the mutex can ensure that this mutex won't be locked
        /// recursively, then they can pass 'true' and disable recursive mode achieving better performance.
        /// This flag doesn't do anything on Windows, since it always uses mutexes in recursive mode.
        /// @param [in] name The name of the mutex. Only used when mutex debugging is enabled.
        /// @param [in] fastMode Whether the faster, non-recursive mode should be used (only applies to pthreads)
        Mutex ( const char * name, bool fastMode = false );

        /// @brief Destructor
        ~Mutex();

        /// @brief Locks the mutex
        /// If the mutex is locked by someone else, this function will wait for it to be unlocked first.
        /// @return True on success; False on failure
        bool lock();

        /// @brief Tries to lock the mutex
        /// If the mutex is locked by someone else, this function returns false immediately.
        /// @return True on success; False on failure
        bool tryLock();

        /// @brief Unlocks the mutex
        /// @return True on success; False on failure
        bool unlock();

    private:
        void * _mutexPtr; ///< Pointer to the internal mutex object

#ifdef MUTEX_DEBUGGING
        char * _name; ///< The name of this mutex.
#endif
};

/// @brief A class that simplifies the used of mutexes.
/// @warning Using this class should NOT be mixed with calling lock()/unlock() directly in the same block.
class MutexLock
{
    public:
        /// @brief Default constructor.
        /// Locks the mutex.
        inline MutexLock ( Mutex & mutex ): _mutex ( mutex )
        {
            _mutex.lock();
        }

        /// @brief Destructor.
        /// Unlocks the mutex.
        inline ~MutexLock()
        {
            _mutex.unlock();
        }

    private:
        Mutex & _mutex; ///< The mutex we are locking
};
}
