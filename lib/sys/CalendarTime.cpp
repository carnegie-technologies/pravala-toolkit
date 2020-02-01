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

#include <cstring>

#include "Date.hpp"
#include "CalendarTime.hpp"

using namespace Pravala;

void CalendarTime::getUTCTime ( struct tm & cal )
{
    epochToUTCTime ( getUTCEpochTime(), cal );
}

void CalendarTime::getLocalTime ( struct tm & cal )
{
    epochToLocalTime ( getUTCEpochTime(), cal );
}

Date CalendarTime::getLocalDate()
{
    struct tm cal;
    getLocalTime ( cal );

    return Date ( cal );
}

int32_t CalendarTime::getLocalDateStamp()
{
    const Date d ( getLocalDate() );

    // YYYYMMDD

    int32_t ret = d.getYear();
    ret *= 100;

    ret += d.getMonth();
    ret *= 100;

    ret += d.getDay();

    return ret;
}

int32_t CalendarTime::incrDateStampMonth ( const int32_t & dateStamp )
{
    int32_t ret = dateStamp;

    ret += 100;

    const int32_t month = ( ret / 100 ) % 100;
    if ( month > 12 )
    {
        assert ( month == 13 );

        // Subtract 1200 to set the month to 1
        ret -= 1200;

        // Increment the year by 1
        ret += 10000;
    }

    return ret;
}

int32_t CalendarTime::decrDateStampMonth ( const int32_t & dateStamp )
{
    int32_t ret = dateStamp;

    ret -= 100;

    const int32_t month = ( ret / 100 ) % 100;
    if ( month < 1 )
    {
        assert ( month == 0 );

        // Add 1200 to set the month to 12
        ret += 1200;

        // Decrement the year by 1
        ret -= 10000;
    }

    return ret;
}

// For some reason GCC 7.1.1 doesn't like passing "%z" to strftime.
// It says that %z is not a part of C++98. Changing C++ dialect doesn't help.
// This disables that warning:
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#endif

String CalendarTime::getTimeDesc ( uint64_t utcEpochTimeMs )
{
    struct tm localTime;
    CalendarTime::epochToLocalTime ( utcEpochTimeMs / 1000, localTime );

    // The string we generate will include '%1', which we will later replace with milliseconds using .arg().
    // We also reserve some extra space (using spaces) in case the time is invalid.
    // For example if the time passed was not in ms, but in us, resulting in much bigger year.
    // The result will not make sense, but at least we will print something instead of an empty buffer...
    static const char fmt[] = "YYYY-MM-DD hh:mm:ss.%1+ZZZZ    ";
    char buf[ sizeof ( fmt ) ];

    const size_t len = ::strftime ( buf, sizeof ( fmt ), "%Y-%m-%d %H:%M:%S.%%1%z", &localTime );

    return String ( buf, len ).arg ( String::number ( utcEpochTimeMs % 1000, String::Int_Dec, 3, true ) );
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
