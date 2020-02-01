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

#include "internal/PrometheusMetric.hpp"

namespace Pravala
{
/// @brief A Prometheus gauge metric which contains 1 or more child gauges
class PrometheusGaugeMetric: public PrometheusMetric
{
    public:
        /// @brief A constructor for a gauge metric with labels
        /// @param [in] timestampMode The timestamp mode the metric should use.
        /// @param [in] name The name of the metric
        /// @param [in] labelNames Comma separated list of label names
        /// @param [in] help A description of the metric
        PrometheusGaugeMetric (
            PrometheusMetric::TimeMode timestampMode,
            const String & name, const String & labelNames, const String & help = String::EmptyString ):
            PrometheusMetric ( PrometheusMetric::PromGauge, timestampMode, name, labelNames, help )
        {
        }
};
}
