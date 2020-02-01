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
#include <mach/clock.h>
#include <mach/mach.h>
}

#include <cassert>

#include "../../CurrentTime.hpp"

using namespace Pravala;

namespace Pravala
{
/// @brief Internal data used by CurrentTime.
// From: http://stackoverflow.com/a/11681069
class CurrentTimePriv
{
    public:
        /// @brief Default constructor.
        CurrentTimePriv()
        {
            // Options:
            // SYSTEM_CLOCK - returns the time since boot time, monotonically increasing
            // CALENDAR_CLOCK - returns the UTC time since 1970-01-01, can go backwards

            host_get_clock_service ( mach_host_self(), SYSTEM_CLOCK, &_cclock );
        }

        /// @brief Destructor.
        ~CurrentTimePriv()
        {
            mach_port_deallocate ( mach_task_self(), _cclock );
        }

        /// @brief Reads the current time.
        /// @param [out] mts Pointer to mach_timespec_t to store the time in. Must be valid.
        inline void readTime ( mach_timespec_t * mts )
        {
            assert ( mts != 0 );

            const int cRet = clock_get_time ( _cclock, mts );

            ( void ) cRet;
            assert ( cRet == 0 );
        }

    private:
        clock_serv_t _cclock; ///< The name port for a kernel clock object.
};
}

CurrentTime::CurrentTime(): _priv ( new CurrentTimePriv )
{
    update();
}

CurrentTime::~CurrentTime()
{
    delete _priv;
}

void CurrentTime::readTime ( struct timespec & tspec ) const
{
    assert ( _priv != 0 );

    mach_timespec_t mts;

    _priv->readTime ( &mts );

    tspec.tv_sec = mts.tv_sec;
    tspec.tv_nsec = mts.tv_nsec;
}
