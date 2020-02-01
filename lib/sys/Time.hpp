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
#include <sys/types.h>
}

#include <cstddef>
#include <cassert>

#define ONE_SEC_IN_MSEC     1000U
#define ONE_MSEC_IN_NSEC    1000000U

namespace Pravala
{
/// @brief Class representing time expressed in seconds and nanoseconds
class Time
{
    public:
        /// @brief Default constructor.
        /// Constructed time has seconds and nanoseconds set to 0.
        Time();

        /// @brief Constructor.
        /// @param [in] s The number of seconds to set.
        /// @param [in] ms The number of milliseconds to set. Could be greater than 1000, in which case seconds
        ///                will be adjusted accordingly.
        explicit Time ( uint32_t s, uint32_t ms = 0 );

        /// @brief Copy constructor
        /// @param [in] other The other time object to copy.
        Time ( const Time & other );

        /// @brief Assignment operator.
        /// @param [in] other The Time object to make a copy of.
        /// @return Reference to this Time object.
        Time & operator= ( const Time & other );

        /// @brief Destructor
        ~Time()
        {
        }

        /// @brief Sets the time to 0:0
        void clear();

        /// @brief Tests if time is set to 0:0
        /// @return True if both seconds and nanoseconds are set to 0; false otherwise.
        inline bool isZero() const
        {
            return ( _myTime.sec == 0 && _myTime.msec == 0 );
        }

        /// @brief Equality comparison
        /// @param [in] other The time object to compare to.
        /// @return True if both Time objects are the same; False otherwise.
        inline bool operator== ( const Time & other ) const
        {
            return ( _myTime.sec == other._myTime.sec && _myTime.msec == other._myTime.msec );
        }

        /// @brief Inequality comparison
        /// @param [in] other The time object to compare to
        /// @return True if Time objects are different; false otherwise.
        inline bool operator!= ( const Time & other ) const
        {
            return ( _myTime.sec != other._myTime.sec || _myTime.msec != other._myTime.msec );
        }

        /// @brief Increases the time value by the given number of seconds
        /// @param [in] s The number of seconds to increase the time value by
        inline void increaseSeconds ( uint32_t s )
        {
            _myTime.sec += s;
        }

        /// @brief Increases the time value by the given number of milliseconds
        /// @param [in] ms The number of milliseconds to increase the time value by
        inline void increaseMilliseconds ( uint32_t ms )
        {
            _myTime.sec += ( ms / ONE_SEC_IN_MSEC );
            _myTime.msec += ms % ONE_SEC_IN_MSEC;

            _myTime.sec += ( _myTime.msec / ONE_SEC_IN_MSEC );
            _myTime.msec %= ONE_SEC_IN_MSEC;
        }

        /// @brief Decreases the time value by the given number of seconds
        /// @param [in] s The number of seconds to decrease the time value by
        /// @return True on success;
        ///         False if the time's value was too low to decrease it by the given number of seconds.
        ///         If this returns false, the time will be set to 0.
        bool decreaseSeconds ( uint32_t s );

        /// @brief Decreases the time value by the given number of milliseconds
        /// @param [in] ms The number of milliseconds to decrease the time value by
        /// @return True on success;
        ///         False if the time's value was too low to decrease it by the given number of milliseconds.
        ///         If this returns false, the time will be set to 0.
        bool decreaseMilliseconds ( uint32_t ms );

        /// @brief LessThan comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @return True if the time in this object is earlier than in the other time object.
        bool operator< ( const Time & other ) const;

        /// @brief LessThanOrEqual comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @return True if the time in this object is earlier or the same as in the other time object.
        bool operator<= ( const Time & other ) const;

        /// @brief GreaterThan comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @return True if the time in this object is later than in the other time object.
        bool operator> ( const Time & other ) const;

        /// @brief GreaterThanOrEqual comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @return True if the time in this object is later or the same as in the other time object.
        bool operator>= ( const Time & other ) const;

        /// @brief GreaterThan by more than comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @param [in] moreThanSeconds The minimal difference between those two objects (in seconds).
        /// @return True if this object has later time than the sum of the other time and given number of seconds.
        bool isGreaterThan ( const Time & other, uint32_t moreThanSeconds ) const;

        /// @brief GreaterThanOrEqual by more than comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @param [in] moreThanSeconds The minimal difference between those two objects (in seconds).
        /// @return True if this object has later or equal time as the sum of the other time
        ///         and given number of seconds.
        bool isGreaterEqualThan ( const Time & other, uint32_t moreThanSeconds ) const;

        /// @brief GreaterThan by more than comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @param [in] moreThanMilliseconds The minimal difference between those two objects (in milliseconds).
        /// @return True if this object has later time than the sum of the other time and given number of milliseconds.
        bool isGreaterThanMilliseconds ( const Time & other, uint32_t moreThanMilliseconds ) const;

        /// @brief GreaterThanOrEqual by more than comparison operator.
        /// @param [in] other The other Time object to compare against.
        /// @param [in] moreThanMilliseconds The minimal difference between those two objects (in milliseconds).
        /// @return True if this object has later or equal time as the sum of the other time
        ///         and given number of milliseconds.
        bool isGreaterEqualThanMilliseconds ( const Time & other, uint32_t moreThanMilliseconds ) const;

        /// @brief Sets the number of seconds in this Time object.
        /// @param [in] secs Number of seconds to set.
        inline void setSeconds ( uint32_t secs )
        {
            _myTime.sec = secs;
        }

        /// @brief Sets the number of milliseconds in this Time object.
        /// If provided value is greater than one second (expressed in milliseconds)
        /// it is decremented to the legal value, and the number of seconds in
        /// this object is incremented accordingly.
        /// @param [in] ms Number of milliseconds to set.
        inline void setMilliseconds ( uint32_t ms )
        {
            _myTime.sec += ( ms / ONE_SEC_IN_MSEC );
            _myTime.msec = ms % ONE_SEC_IN_MSEC;
        }

        /// @brief Returns the number of seconds in this Time object.
        /// @return Number of seconds in this Time object.
        inline uint32_t getSeconds() const
        {
            return _myTime.sec;
        }

        /// @brief Returns the number of milliseconds in this Time object.
        /// It is just milliseconds part of the time, not the entire time expressed in milliseconds.
        /// @return Number of milliseconds in this Time object.
        inline uint32_t getMilliSeconds() const
        {
            return _myTime.msec;
        }

        /// @brief Returns this time value expressed in milliseconds.
        /// This includes the seconds converted to milliseconds.
        /// @return This time value expressed in milliseconds.
        inline uint64_t asMilliSeconds() const
        {
            return ( ( uint64_t ) _myTime.sec ) * 1000 + ( uint64_t ) _myTime.msec;
        }

        /// @brief Returns difference in seconds between this object and the other time object.
        ///
        /// If positive, it means that "this" object stores larger time value (in seconds) than the "other" object.
        /// @param [in] other The other time object.
        /// @returns The difference in seconds (rounded down)
        inline long getDiffInSeconds ( const Time & other ) const
        {
            return getDiffInMilliSeconds ( other ) / 1000;
        }

        /// @brief Returns difference in milliseconds between this object and the other time object.
        ///
        /// If positive, it means that "this" object stores larger time value (in milliseconds) than the "other" object.
        ///
        /// @param [in] other The other time object.
        /// @returns The difference in milliseconds (rounded down)
        inline long getDiffInMilliSeconds ( const Time & other ) const
        {
            return ( ( long ) ( _myTime.sec ) - ( ( long ) other._myTime.sec ) ) * 1000
                   + ( ( long ) _myTime.msec ) - ( ( long ) other._myTime.msec );
        }

        /// @brief Calculates number of bytes per second from given values and time period.
        /// The period of time used is the time from fromTime to the current value of this time object.
        /// @note If the type used is not large enough to contain calculated rate,
        ///       a max value for that type will be returned.
        /// @param [in] bytes Total number of bytes sent (received) over a period of time.
        /// @param [in] fromTime Starting time.
        /// @return Number of bytes per second in the specified time period.
        /// @tparam T The data type used for the byte counter and for the return value.
        ///           It MUST be an unsigned type!
        template<typename T> T calcBytesPerSecond ( T bytes, const Time & fromTime ) const
        {
            assert ( ( *this ) >= fromTime );

            if ( ( *this ) <= fromTime )
                return 0;

            const uint64_t msecs = ( _myTime.sec - fromTime._myTime.sec ) * ONE_SEC_IN_MSEC
                                   + _myTime.msec - fromTime._myTime.msec;

            if ( msecs < 1 ) return 0;

            const uint64_t val = ( ( uint64_t ) bytes ) * ( ( uint64_t ) ONE_SEC_IN_MSEC ) / msecs;

            if ( val > ( T ) -1 )
            {
                return ( T ) -1;
            }

            return ( T ) val;
        }

    protected:
        /// @brief Contains internal time value
        struct
        {
            /// @brief The 'seconds' of the time value stored in this object
            /// We assume that the values don't get too big. For time differences it should be OK.
            /// If not, we may need to increase the size of this.
            uint32_t sec;

            uint32_t msec; ///< The 'milliseconds' of the time value stored in this object
        } _myTime;
};
}
