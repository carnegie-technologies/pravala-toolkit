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

#include "PrometheusGauge.hpp"

namespace Pravala
{
class PrometheusGaugeMetric;

/// @brief A basic Prometheus gauge.
/// A gauge is a counter that can go up and down.
class BasicPrometheusGauge: public PrometheusGauge
{
    public:
        /// @brief A constructor for a gauge without labels.
        /// It uses internal gauge metric object.
        /// @param [in] timestampMode The timestamp mode the internal metric should use.
        /// @param [in] name The name of the metric.
        /// @param [in] help A description of the metric.
        BasicPrometheusGauge (
            PrometheusMetric::TimeMode timestampMode, const String & name, const String & help = String::EmptyString );

        /// @brief A constructor for a gauge with labels
        /// @param [in] parent The parent gauge metric to which to add this gauge
        /// @param [in] labelValues A comma separated list of label values
        BasicPrometheusGauge ( PrometheusGaugeMetric & parent, const String & labelValues );

        /// @brief Destructor
        virtual ~BasicPrometheusGauge();

        /// @brief Increments the gauge by 'value' and updates the timestamp.
        /// @param [in] value The value by which to increment the gauge.
        void increment ( uint64_t value = 1 );

        /// @brief Decrements the gauge by 'value' and updates the timestamp.
        /// @param [in] value The value by which to decrement the gauge.
        void decrement ( uint64_t value = 1 );

        /// @brief Increments the gauge by 'value' (which may be negative) and updates the timestamp.
        /// @param [in] value The value by which to adjust the gauge.
        void adjust ( int64_t value );

        /// @brief Resets the gauge to zero and updates the timestamp.
        inline void reset()
        {
            set ( 0 );
        }

        /// @brief Sets the gauge to the specified value
        /// @param [in] value The value to set the gauge to
        inline void set ( int64_t value )
        {
            _value = value;
            updateTimestamp();
        }

    protected:
        virtual int64_t getValue();
        virtual uint64_t getTimestamp();

    private:
        /// @brief The value of the gauge
        /// @note The Prometheus specifications use non-integral (double) values which is a bit weird for a counter.
        /// As we don't (currently) have floating-point use-cases, we are using integral counters to improve
        /// performance
        int64_t _value;

        uint64_t _timestamp; ///< The timestamp in milliseconds from the UTC epoch

        /// @brief Updates the timestamp
        void updateTimestamp();
};
}
