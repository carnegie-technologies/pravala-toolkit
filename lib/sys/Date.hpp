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

#include "basic/String.hpp"

namespace Pravala
{
/// @brief Contains a single date.
class Date
{
    public:
        /// @brief Default constructor.
        Date();

        /// @brief Constructor.
        /// @param [in] cal The tm structure with the time description to use.
        Date ( const struct tm & cal );

        /// @brief Gets the year portion of the date in this object
        /// @return The year portion of the date in this object
        inline uint16_t getYear() const
        {
            return _year;
        }

        /// @brief Gets the month portion of the date in this object
        /// @return The month portion of the date in this object
        inline uint8_t getMonth() const
        {
            return _month;
        }

        /// @brief Gets the day portion of the date in this object
        /// @return The day portion of the date in this object
        inline uint8_t getDay() const
        {
            return _day;
        }

        /// @brief Equality comparison
        /// @param [in] other The Date object to compare to.
        /// @return True if both Date objects are the same; False otherwise.
        inline bool operator== ( const Date & other ) const
        {
            return ( _year == other._year && _month == other._month && _day == other._day );
        }

        /// @brief Inequality comparison
        /// @param [in] other The Date object to compare to
        /// @return True if Date objects are different; false otherwise.
        inline bool operator!= ( const Date & other ) const
        {
            return !( operator== ( other ) );
        }

        /// @brief LessThanOrEqual comparison operator.
        /// @param [in] other The other Date object to compare against.
        /// @return True if the Date in this object is earlier or the same as in the other Date object.
        inline bool operator<= ( const Date & other ) const
        {
            return ( operator== ( other ) || operator< ( other ) );
        }

        /// @brief GreaterThanOrEqual comparison operator.
        /// @param [in] other The other Date object to compare against.
        /// @return True if the Date in this object is later or the same as in the other Date object.
        bool operator>= ( const Date & other ) const
        {
            return ( operator== ( other ) || operator> ( other ) );
        }

        /// @brief LessThan comparison operator.
        /// @param [in] other The other Date object to compare against.
        /// @return True if the Date in this object is earlier than in the other Date object.
        bool operator< ( const Date & other ) const;

        /// @brief GreaterThan comparison operator.
        /// @param [in] other The other Date object to compare against.
        /// @return True if the Date in this object is later than in the other Date object.
        bool operator> ( const Date & other ) const;

        /// @brief Sets the date based on the string given.
        /// @param [in] strDate The date as a string in "YYYY-[M]M-[D]D" format.
        /// @return True if the conversion was successful; False otherwise.
        bool set ( const String & strDate );

        /// @brief Returns the string description of this date.
        /// It uses "YYYY-MM-DD" format.
        /// @return A string description of the date in YYYY-MM-DD format.
        String toString() const;

    private:
        uint16_t _year; ///< The year
        uint8_t _month; ///< The month
        uint8_t _day; ///< The day
};
}
