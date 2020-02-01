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

#include <mach/clock.h>
#include <mach/mach.h>

#include "../../CalendarTime.hpp"

#define ONE_SEC_IN_MSEC     1000U
#define ONE_MSEC_IN_NSEC    1000000U

using namespace Pravala;

int64_t CalendarTime::getUTCEpochTime()
{
    return ( int64_t ) time ( 0 );
}

void CalendarTime::epochToUTCTime ( int64_t epochTime, struct tm & cal )
{
    const time_t t = ( time_t ) epochTime;
    memset ( &cal, 0, sizeof ( cal ) );

    gmtime_r ( &t, &cal );
}

void CalendarTime::epochToLocalTime ( int64_t epochTime, struct tm & cal )
{
    const time_t t = ( time_t ) epochTime;
    memset ( &cal, 0, sizeof ( cal ) );

    localtime_r ( &t, &cal );
}

uint64_t CalendarTime::getUTCEpochTimeMs()
{
    // From: http://stackoverflow.com/a/11681069
    clock_serv_t cclock;
    mach_timespec_t mts;

    // Options:
    // SYSTEM_CLOCK - returns the time since boot time, monotonically increasing
    // CALENDAR_CLOCK - returns the UTC time since 1970-01-01, can go backwards

    host_get_clock_service ( mach_host_self(), CALENDAR_CLOCK, &cclock );
    clock_get_time ( cclock, &mts );
    mach_port_deallocate ( mach_task_self(), cclock );

    return ( ( uint64_t ) mts.tv_sec ) * ONE_SEC_IN_MSEC + ( mts.tv_nsec / ONE_MSEC_IN_NSEC );
}
