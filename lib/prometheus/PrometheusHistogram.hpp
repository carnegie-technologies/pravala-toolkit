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
#include "PrometheusHistogramMetric.hpp"

namespace Pravala
{
/// @brief A Prometheus histogram.
/// The histogram is cumulative. A histogram has a distribution of values across specified buckets where each bucket
/// is a counter. The total sum and total count of observances are also recorded. The number of buckets and their upper
/// bounds have the same configuration as the parent histogram metric.
class PrometheusHistogram: public PrometheusChild
{
    public:
        /// @brief A constructor for a histogram without labels
        /// @param [in] name The name of the metric
        /// @param [in] numBuckets The number of buckets in the histogram which must be greater than zero.
        /// @param [in] bucketUpperBounds An array of upper bounds of the buckets of length numBuckets
        ///             The bucket upper bounds must be listed in increasing order.
        /// @param [in] help A description of the metric
        PrometheusHistogram (
            const String & name,
            size_t numBuckets,
            const int64_t bucketUpperBounds[],
            const String & help = String::EmptyString );

        /// @brief A constructor for a histogram with labels
        /// @param [in] parent The parent histogram metric to which to add this histogram
        /// @param [in] labelValues A comma separated list of label values
        PrometheusHistogram ( PrometheusHistogramMetric & parent, const String & labelValues );

        /// @brief Destructor
        virtual ~PrometheusHistogram();

        /// @brief Observes a value for the histogram
        /// Each bucket count where v is <= the bucket upper bound will be incremented.
        /// The total count will be incremented, along with the sum of the observances.
        /// @param [in] v The value to observe
        void observe ( int64_t v );

        /// @brief Gets the sum of all the observances
        /// @return The sum of all the observances
        inline int64_t getSum() const
        {
            return _sum;
        }

        /// @brief Gets the total count of the observances
        /// @return The count of all the observances
        inline uint64_t getTotalCount() const
        {
            return _totalCount;
        }

    protected:
        /// @brief Appends the Prometheus text exposition of the histogram to a buffer
        /// @note The Prometheus text exposition of the histogram is somewhat complex.
        /// It has the form:
        /// <metric_name>_bucket{[<labels>,]le="<least bucket upper bound>"} <bucket1_value>
        /// <metric_name>_bucket{[<labels>,]le="<2nd least bucket upper bound>"} <bucket2_value>
        /// ...
        /// <metric_name>_bucket{[<labels>,]le="<Greatest bucket upper bound>"} <bucketN_value>
        /// <metric_name>_bucket{[<labels>,]le="+Inf"} <total_count_of_all_observances>
        /// <metric_name>_sum <total_sum_of_all_observances>
        /// <metric_name>_count <total_count_of_all_observances>
        /// @param [out] buf The buffer to which to append the data.
        virtual void appendData ( Buffer & buf, const String & name, uint64_t timestamp );

    private:
        /// @brief Struct to define each bucket of the histogram
        struct Bucket
        {
            int64_t upperBound;     ///< The upper bound of the bucket
            uint64_t count;         ///< The total number of observances for this bucket
        };

        const size_t _numBuckets;   ///< The number of buckets in the histogram

        Bucket * _buckets;          ///< The array of buckets

        uint64_t _totalCount;       ///< The total number of observances for all buckets

        int64_t _sum;               ///< The total sum of all the observances
};
}
