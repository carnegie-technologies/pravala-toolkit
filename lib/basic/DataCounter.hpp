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

namespace Pravala
{
/// @brief Class for storing amount of data
/// Can be used for keeping track of large amounts of data sent, received, stored, etc.
class DataCounter
{
    public:
        /// @brief Default constructor
        DataCounter():
            _dataCounter ( 0 ), _overflowCounter ( 0 )
        {
        }

        /// @brief Returns the base amount of data stored by the counter
        /// @return The amount of data in the counter
        inline uint32_t getData() const
        {
            return _dataCounter;
        }

        /// @brief Returns the value of the overflow counter
        /// This counter is increased by one every time the base counter overflows
        /// @return The overflow counter.
        inline uint16_t getOverflow() const
        {
            return _overflowCounter;
        }

        /// @brief Resets the counter by setting both base and overflow counters to 0
        inline void reset()
        {
            _overflowCounter = 0;
            _dataCounter = 0;
        }

        /// @brief Adds data to the counter
        /// @param [in] size The amount of data to add
        void addData ( uint32_t size );

        /// @brief Compares this and the other data counter
        ///
        /// If the counters are too far apart, the 0x7FFFFFFF is returned
        /// if the "other" counter is bigger, and -0x7FFFFFFF is returned
        /// when "this" counter is bigger. Current implementation
        /// should not return negative 0xFFFFFFFF value.
        ///
        /// @param [in] other The other data counter to compare to
        /// @return Difference between the counters. The positive value means
        ///  that "other" data counter contained larger data count.
        int32_t getDiff ( const DataCounter & other ) const;

        /// @brief Compares counters and sets this to the values in the other one
        ///
        /// Comparison is done with getDiff function.
        ///
        /// Takes data counts from the other data counter,
        /// sets its current values to the same counts as in
        /// the other data counter, and returns a difference.
        ///
        /// @param [in] other The other data counter to compare to
        /// @return Difference between the counters. The positive value means
        ///  that "other" data counter contained larger data count.
        int32_t getDiffAndSet ( const DataCounter & other );

    private:
        /// @brief The amount of data stored in the counter
        uint32_t _dataCounter;

        /// @brief The overflow counter. It is increased by one every time the _dataCounter overflows.
        uint16_t _overflowCounter;
};
}
