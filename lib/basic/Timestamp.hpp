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

#include "String.hpp"

namespace Pravala
{
/// @brief Simple class that represents an RFC 3339 timestamp, with millisecond resolution.
/// @note This allows seconds to be 60 (the leap second), but doesn't verify if it actually should be a leap second.
class Timestamp
{
    public:
        /// @brief A helper structure describing a moment in time.
        struct TimeDesc
        {
            uint16_t year;       ///< The year.
            uint8_t month;       ///< The month.
            uint8_t day;         ///< The day.
            uint8_t hour;        ///< The hour.
            uint8_t minute;      ///< The minute.
            uint8_t second;      ///< The second.
            uint16_t millisecond; ///< The millisecond.

            /// @brief Default constructor.
            TimeDesc();

            /// @brief Constructor.
            /// @param [in] year The year to set.
            /// @param [in] month The month to set.
            /// @param [in] day The day to set.
            /// @param [in] h The hour to set, 0 by default.
            /// @param [in] m The number of minute to set, 0 by default.
            /// @param [in] s The number of seconds to set, 0 by default.
            /// @param [in] ms The number of milliseconds to set, 0 by default.
            TimeDesc (
                    uint16_t year, uint8_t month, uint8_t day,
                    uint8_t h = 0, uint8_t m = 0, uint8_t s = 0, uint16_t ms = 0 );

            /// @brief Constructor.
            /// @param [in] time tm structure describing the UTC time to set.
            /// @param [in] ms The milliseconds to set (not part of the tm structure).
            TimeDesc ( const struct tm & time, uint16_t ms = 0 );

            /// @brief Checks if the TimeDesc describes a valid time.
            /// @note It allows seconds=60, which is allowed by RFC-3339,
            ///       but it does not check if that time is actually correct.
            /// @return True if the TimeDesc describes a valid time; False otherwise.
            bool isValid() const;
        };

        /// @brief The minimum legal value for the internal timestamp's representation.
        /// It represents 1st of January, year 0, at 00:00:00.000.
        static const uint64_t MinBinValue = 0x108000000;

        /// @brief Returns true if the given date is valid.
        /// @param [in] y The year.
        /// @param [in] m The month.
        /// @param [in] d The day.
        static bool isValidDate ( int y, int m, int d );

        /// @brief Returns true if the given time is valid.
        /// @note It allows seconds=60, which is allowed by RFC-3339,
        ///       but it does not check if that time is actually correct.
        /// @param [in] h The hour.
        /// @param [in] m The minute.
        /// @param [in] s The second.
        /// @param [in] ms The millisecond.
        static bool isValidTime ( int h, int m, int s, int ms = 0 );

        /// @brief Default constructor.
        /// It sets the timestamp to MinBinValue.
        Timestamp();

        /// @brief Equality operator.
        /// @param [in] other The object to compare against.
        /// @return True if both timestamps are the same; False otherwise.
        inline bool operator== ( const Timestamp & other ) const
        {
            return ( _value == other._value );
        }

        /// @brief Inequality operator.
        /// @param [in] other The object to compare against.
        /// @return True if timestamps are different; False otherwise.
        inline bool operator!= ( const Timestamp & other ) const
        {
            return ( _value != other._value );
        }

        /// @brief Clears the timestamp.
        /// It sets the timestamp to MinBinValue.
        void clear();

        /// @brief Sets the timestamp to given UTC time.
        /// @note This class does NOT support timezones, UTC time must be set.
        /// @param [in] tDesc The structure describing the time. It MUST use UTC time!
        /// @return True if the value provided was correct (and was set); False otherwise.
        ///         This checks if the month and the day makes sense, including leap years.
        ///         It allows seconds=60, which is allowed by RFC-3339, but it does not check
        ///         if that time is actually correct.
        bool setUtcTime ( const TimeDesc & tDesc );

        /// @brief Sets the timestamp to given UTC time.
        /// @note This class does NOT support timezones, UTC time must be set.
        /// @param [in] time tm structure describing the UTC time to set.
        /// @param [in] ms The milliseconds to set (not part of the tm structure).
        /// @return True if the value provided was correct (and was set); False otherwise.
        ///         This checks if the month and the day makes sense, including leap years.
        ///         It allows seconds=60, which is allowed by RFC-3339, but it does not check
        ///         if that time is actually correct.
        bool setUtcTime ( struct tm & time, uint16_t ms = 0 );

        /// @brief Sets internal value.
        /// @return True if the value provided was correct (and was set); False otherwise.
        bool setBinValue ( uint64_t value );

        /// @brief Exposes the internal binary value.
        /// @return The internal binary value.
        inline uint64_t getBinValue() const
        {
            return _value;
        }

        /// @brief Returns description of the timestamp in format specified by RFC-3339.
        /// @note The string generated always describes the UTC time.
        /// @return The description of the timestamp in RFC-3339 format.
        const String toString() const;

    private:
        /// @brief Internal timestamp's value.
        /// We specific number of bits for each part of the timestamp (from LSB to MSB):
        /// 10b - milliseconds
        ///  6b - seconds
        ///  6b - minutes
        ///  5b - hours
        ///  5b - days
        ///  4b - months
        /// 14b - years
        /// Total: 50bits.
        uint64_t _value;

        /// @brief Converts binary value to a TimeDesc structure.
        /// @note It does not check if the result is correct.
        /// @param [in] value The value to convert.
        /// @param [out] tDesc The TimeDesc structure to set.
        /// @return True if the value seemed to have the right format.
        ///         It only checks if there are any additional bits set, that shouldn't be.
        ///         It does NOT check if the converted date actually makes sense!
        static bool convertBinValue ( uint64_t value, TimeDesc & tDesc );
};
}
