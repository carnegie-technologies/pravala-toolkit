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
#include <sys/time.h>
}

#include <cassert>
#include <ctime>

#include "../../CurrentTime.hpp"

using namespace Pravala;

CurrentTime::CurrentTime(): _priv ( 0 )
{
    ( void ) _priv;

    update();
}

CurrentTime::~CurrentTime()
{
}

void CurrentTime::readTime ( struct timespec & tspec ) const
{
    // Options:
    // CLOCK_MONOTONIC_RAW - not affected by NTP, raw hardware-based time
    // CLOCK_MONOTONIC - possibly affected by NTP, will not jump backwards, but can slew
    // CLOCK_MONOTONIC_COARSE - like MONOTONIC, just faster and with less precision (that depends on kernel ticks)
    // Since we expose high precision time now, we cannot use CLOCK_MONOTONIC_COARSE...
    const int cRet = clock_gettime ( CLOCK_MONOTONIC, &tspec );

    // This is based on the assumption that CLOCK_MONOTONIC(_COARSE) gives us a relatively small value.
    // Not seconds since epoch, but rather some point in the past that is not too far.
    // For example boot time. These values will be much smaller than seconds since epoch
    // and give us a lot of room for expressing time differences.
    // If we were getting larger values, like seconds since epoch (which is returned by CLOCK_REALTIME),
    // we could store the initial value when we create _globalCurrentTime for the first time,
    // and have all Time objects represent only the offset from that initial value to achieve something similar.

    ( void ) cRet;
    assert ( cRet == 0 );
}
