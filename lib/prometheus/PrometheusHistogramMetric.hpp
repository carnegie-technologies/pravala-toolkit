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
/// @brief A histogram metric which has 1 or more child histograms
/// The histogram's buckets are defined by their upper bounds.  A mandatory "le" label
/// is created which differentiates the different buckets in the (cumulative) histogram.
/// A special "+Inf" bucket is required to be output in the text format, which corresponds
/// to the totalCount of observations in the histogram. This bucket is not included
/// in the number of buckets specified by the user (it is always appended to the text output).
/// More details here: https://prometheus.io/docs/concepts/metric_types/#histogram
class PrometheusHistogramMetric: public PrometheusMetric
{
    public:
        /// @brief A constructor for a histogram metric with labels
        /// @param [in] name The name of the metric
        /// @param [in] labelNames Comma separated list of label names
        ///             The list of label names must not include "le" as this is a special label reserved for
        ///             histogram buckets
        /// @param [in] numBuckets The number of buckets in the histogram which must be greater than zero.
        /// @param [in] bucketUpperBounds An array of upper bounds of the buckets of length numBuckets
        ///             The bucket upper bounds must be listed in increasing order.
        /// @param [in] help A description of the metric
        PrometheusHistogramMetric (
            const String & name,
            const String & labelNames,
            size_t numBuckets,
            const int64_t bucketUpperBounds[],
            const String & help = String::EmptyString );

        /// @brief Destructor
        ~PrometheusHistogramMetric();

    protected:
        /// @brief Gets the number of buckets in the histogram
        /// @return The number of buckets in the histogram (excluding the +Inf bucket)
        inline size_t getNumBuckets() const
        {
            return _numBuckets;
        }

        /// @brief Gets a pointer to the bucket upper bounds array
        /// @return A pointer to the array of bucket upper bounds
        inline const int64_t * getBucketUpperBoundsArray()
        {
            return _bucketUpperBounds;
        }

        friend class PrometheusHistogram;

    private:
        const size_t _numBuckets;   ///< Number of buckets

        int64_t * _bucketUpperBounds;    ///< Array of upper bounds of the buckets
};
}
