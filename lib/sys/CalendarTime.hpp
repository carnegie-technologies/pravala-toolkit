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

extern "C"
{
#include <stdint.h>
}

#include "Date.hpp"

namespace Pravala
{
/// @brief Contains time and date related functions.
class CalendarTime
{
    public:
        /// @brief Gets the current UTC epoch time in milliseconds
        /// @return The current UTC epoch time in milliseconds
        static uint64_t getUTCEpochTimeMs();

        /// @brief Returns the current UTC time in seconds since epoch (1970-01-01)
        /// @return the current UTC time in seconds since epoch
        static int64_t getUTCEpochTime();

        /// @brief Converts the UTC epoch time (as returned by getUTCEpochTime) to tm structure describing UTC time.
        /// @param [in] epochTime UTC epoch time (as returned by getUTCEpochTime)
        /// @param [out] time tm structure describing UTC time.
        static void epochToUTCTime ( int64_t epochTime, struct tm & time );

        /// @brief Converts the UTC epoch time (as returned by getUTCEpochTime) to tm structure describing local time.
        /// @param [in] epochTime UTC epoch time (as returned by getUTCEpochTime)
        /// @param [out] time tm structure describing local time.
        static void epochToLocalTime ( int64_t epochTime, struct tm & time );

        /// @brief Returns the current UTC time and date
        /// @param [out] time tm structure describing current UTC time.
        static void getUTCTime ( struct tm & time );

        /// @brief Returns the current UTC time and date
        /// @param [out] time tm structure describing current UTC time.
        static void getLocalTime ( struct tm & time );

        /// @brief Returns the current local date
        /// @return The current local date.
        static Date getLocalDate();

        /// @brief Returns the current local time as an integer in "YYYYMMDD" format
        /// @return the current local time as an integer in "YYYYMMDD" format
        static int32_t getLocalDateStamp();

        /// @brief Increment the datestamp's month by 1
        /// @param [in] dateStamp DateStamp to increment month
        /// @return DateStamp with month incremented by 1
        static int32_t incrDateStampMonth ( const int32_t & dateStamp );

        /// @brief Decrement the datestamp's month by 1
        /// @param [in] dateStamp DateStamp to decrement month
        /// @return DateStamp with month decremented by 1
        static int32_t decrDateStampMonth ( const int32_t & dateStamp );

        /// @brief Generates a description of the time.
        /// @param [in] utcEpochTimeMs UTC epoch time in milliseconds.
        /// @return A description of the time passed.
        static String getTimeDesc ( uint64_t utcEpochTimeMs );
};
}
