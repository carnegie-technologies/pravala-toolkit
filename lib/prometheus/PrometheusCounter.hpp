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

#include "internal/PrometheusChild.hpp"
#include "PrometheusCounterMetric.hpp"

namespace Pravala
{
/// @brief A base class for Prometheus counters.
/// A counter has a monotonically increasing value.
class PrometheusCounter: public PrometheusChild
{
    protected:
        /// @brief A constructor for a counter without labels.
        /// It uses internal counter metric object.
        /// @param [in] timestampMode The timestamp mode the internal metric should use.
        /// @param [in] name The name of the metric.
        /// @param [in] help A description of the metric.
        PrometheusCounter ( PrometheusMetric::TimeMode timestampMode, const String & name, const String & help );

        /// @brief A constructor for a counter with labels
        /// @param [in] parent The parent counter metric to which to add this counter
        /// @param [in] labelValues A comma separated list of label values
        PrometheusCounter ( PrometheusCounterMetric & parent, const String & labelValues );

        /// @brief Gets the counter value
        /// @return The counter value.
        virtual uint64_t getValue() = 0;

        virtual void appendData ( Buffer & buf, const String & name, uint64_t timestamp );
};
}
