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
/// @brief Calculates smoothed round-trip time and round-trip time variation.
/// Based on RFC-6298.
class RttStat
{
    public:
        /// @brief Default constructor.
        RttStat();

        /// @brief Clears the values.
        void clear();

        /// @brief Adds a new RTT measurement.
        /// @param [in] r The new RTT measurement (in milliseconds).
        ///               NOTE: If it's 0, an RTT = 1ms will be used instead.
        void addRtt ( uint32_t r );

        /// @brief Returns current smoothed round-trip time (in milliseconds).
        /// @return Current smoothed round-trip time (in milliseconds).
        inline uint32_t getSRtt() const
        {
            return _sRtt;
        }

        /// @brief Returns lowest RTT value seen (in milliseconds).
        /// @return Lowest RTT value seen (in milliseconds).
        inline uint32_t getMinRtt() const
        {
            return _minRtt;
        }

        /// @brief Returns current round-trip time variation.
        /// @return Current round-trip time variation.
        inline uint32_t getRttVar() const
        {
            return _rttVar;
        }

        /// @brief Calculates the retransmission timeout (in milliseconds).
        /// It makes sure that it's at least 500 (more aggressive than RFC-6298's 1s)
        /// @return Retransmission timeout (in milliseconds) given current RTT data.
        uint32_t getRto() const;

    private:
        uint32_t _sRtt; ///< Smoothed round-trip time (in milliseconds).
        uint32_t _rttVar; ///< Round-trip time variation.
        uint32_t _minRtt; ///< The lowest single RTT seen (in milliseconds).
};
}
