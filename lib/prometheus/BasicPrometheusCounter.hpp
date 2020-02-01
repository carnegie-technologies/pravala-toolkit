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

#include "PrometheusCounter.hpp"

namespace Pravala
{
/// @brief A basic Prometheus counter.
/// A counter has a monotonically increasing value.
class BasicPrometheusCounter: public PrometheusCounter
{
    public:
        /// @brief A constructor for a counter without labels.
        /// It uses internal counter metric object.
        /// @param [in] timestampMode The timestamp mode the internal metric should use.
        /// @param [in] name The name of the metric.
        /// @param [in] help A description of the metric.
        BasicPrometheusCounter (
            PrometheusMetric::TimeMode timestampMode, const String & name, const String & help = String::EmptyString );

        /// @brief A constructor for a counter with labels
        /// @param [in] parent The parent counter metric to which to add this counter
        /// @param [in] labelValues A comma separated list of label values
        BasicPrometheusCounter ( PrometheusCounterMetric & parent, const String & labelValues );

        /// @brief Destructor
        ~BasicPrometheusCounter();

        /// @brief Increments the counter by 'value' and updates the timestamp.
        /// @param [in] value The value by which to increment the counter.
        void increment ( uint64_t value = 1 );

        /// @brief Resets the counter to zero and updates the timestamp.
        inline void reset()
        {
            _value = 0;
            updateTimestamp();
        }

    protected:
        virtual uint64_t getValue();
        virtual uint64_t getTimestamp();

    private:
        /// @brief The value of the counter
        /// @note The Prometheus specifications use non-integral (double) values which is a bit weird for a counter.
        /// As we don't (currently) have floating-point use-cases, we are using integral counters to improve
        /// performance
        uint64_t _value;

        uint64_t _timestamp; ///< The timestamp in milliseconds from the UTC epoch

        /// @brief Updates the timestamp
        void updateTimestamp();
};
}
