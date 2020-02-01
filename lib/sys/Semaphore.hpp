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

#if defined( SYSTEM_LINUX ) || defined( SYSTEM_APPLE )
extern "C"
{
#include <semaphore.h>
}
#endif

#include "basic/NoCopy.hpp"
#include "basic/String.hpp"

namespace Pravala
{
/// @brief A semaphore wrapper.
/// This will destroy the semaphore when it goes out of scope.
/// @note This class only has Linux and Apple implementations at the moment.
class Semaphore: public NoCopy
{
    public:
        /// @brief Constructor.
        /// @param [in] name The name of the semaphore.
        Semaphore ( const char * name );

        /// @brief Destructor.
        ~Semaphore();

        /// @brief Initialize the semaphore.
        /// @param [in] value The initial value of the semaphore.
        /// @return 0 on success, -1 on error, and errno is set.
        int init ( unsigned int value = 0 );

        /// @brief Increment the semaphore.
        /// @return 0 on success. On error, the value of the semaphore is unchanged, -1 is returned, and errno is set.
        int post();

        /// @brief Decrement the semaphore.
        /// If the semaphore's value is 0, the call blocks until it rises above 0.
        /// @return 0 on success. On error, the value of the semaphore is unchanged, -1 is returned, and errno is set.
        int wait();

        /// @brief Decrement the semaphore with a timeout.
        /// @param [in] timeout The maximum amount of time (in milliseconds) to wait before returning.
        /// @return 0 on success. On error, the value of the semaphore is unchanged, -1 is returned, and errno is set.
        ///         If the wait times out, errno will be set to ETIMEDOUT.
        int timedWait ( uint32_t timeout );

    private:
        sem_t * _semaphore; ///< A pointer to the semaphore being wrapped. Unset if the semaphore isn't initialized.
        const String _name; ///< The name of the semaphore.
};
}
