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

#include "../../CalendarTime.hpp"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

using namespace Pravala;

int64_t CalendarTime::getUTCEpochTime()
{
    return ( int64_t ) time ( 0 );
}

void CalendarTime::epochToUTCTime ( int64_t epochTime, struct tm & cal )
{
    const time_t t = ( time_t ) epochTime;
    memset ( &cal, 0, sizeof ( cal ) );

    gmtime_s ( &cal, &t );
}

void CalendarTime::epochToLocalTime ( int64_t epochTime, struct tm & cal )
{
    const time_t t = ( time_t ) epochTime;
    memset ( &cal, 0, sizeof ( cal ) );

    localtime_s ( &cal, &t );
}

uint64_t CalendarTime::getUTCEpochTimeMs()
{
    // From boost -- constant to subtract from Windows "file time" to convert it to UNIX epoch time
    static const uint64_t epoch = ( ( uint64_t ) 116444736000000000ULL );

    SYSTEMTIME system_time;
    FILETIME file_time;

    GetSystemTime ( &system_time );
    SystemTimeToFileTime ( &system_time, &file_time );

    // File time is a 64 bit value representing the number of 100-nanosecond intervals since Windows epoch
    // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284%28v=vs.85%29.aspx
    const uint64_t time = ( ( uint64_t ) file_time.dwLowDateTime )
                          | ( ( ( uint64_t ) file_time.dwHighDateTime ) << 32 );

    return ( ( time - epoch ) / 10000L );
}
