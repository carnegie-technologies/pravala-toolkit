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

#include "basic/String.hpp"
#include "basic/Buffer.hpp"

namespace Pravala
{
class PrometheusMetric;

/// @brief This is the Prometheus child base class which exposes metrics for a given set of label values
/// in the Prometheus text exposition format to a single PrometheusMetric.
class PrometheusChild
{
    public:
        /// @brief The combined string of label pairs
        /// The string has the form: labelName1="labelValue1",labelName2="labelValue2",...
        const String LabelStr;

    protected:
        /// @brief Constructor to be used with internal metric.
        /// Creates a child that uses internal metric, that will be deleted when this object is destroyed.
        /// @param [in] metric The internal metric to use. Must be valid. The metric will be deleted with this object.
        /// @param [in] labelValues The comma separated list of label values
        ///                         The number of label values must match the number of parent metric label names
        PrometheusChild ( PrometheusMetric * metric, const String & labelValues );

        /// @brief Constructor to be used with external metric.
        /// Creates a child that uses external metric.
        /// @param [in] metric The external metric to use.
        /// @param [in] labelValues The comma separated list of label values
        ///                         The number of label values must match the number of parent metric label names
        PrometheusChild ( PrometheusMetric & metric, const String & labelValues );

        /// @brief Destructor
        virtual ~PrometheusChild();

        /// @brief Gets the timestamp the data was last current (set, modified, or read from somewhere).
        /// A timestamp of 0 indicates the timestamp is not present,
        /// and should be omitted in the Prometheus text format.
        /// The default implementation returns 0.
        /// @return The timestamp in milliseconds from the UTC epoch.
        virtual uint64_t getTimestamp();

        /// @brief Appends the Prometheus text exposition of the child to a buffer
        /// @note The child metric string should be terminated with a '\n'.
        ///       For more information, refer to:
        ///       https://prometheus.io/docs/instrumenting/exposition_formats/#text-format-details
        /// @param [out] buf The buffer to which to append the data.
        /// @param [in] name The metric name to use.
        /// @param [in] timestamp The timestamp to use. 0 if the timestamp should NOT be included.
        virtual void appendData ( Buffer & buf, const String & name, uint64_t timestamp ) = 0;

        friend class PrometheusMetric;

    private:
        PrometheusMetric * const _metric; ///< Internal metric.
        const bool _autoDeleteMetric; ///< If true, the metric will be deleted when this object is destructed.
};
}
