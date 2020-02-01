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
#include "log/TextLog.hpp"

namespace Pravala
{
class PrometheusChild;

/// @brief A class representing a Prometheus metric.
/// Exposes metrics data in the Prometheus text exposition format. The PrometheusMetric does not store any metrics
/// data itself. It instead maintains a set of Prometheus children, and aggregates their metrics data.
/// @note A PrometheusMetric shall have one PrometheusChild for every unique combination of label values. A
/// PrometheusMetric without any labels shall have a single PrometheusChild.
class PrometheusMetric
{
    public:
        /// @brief An enum of the metric type
        /// @note Currently only counter, gauge and histogram are supported
        enum MetricType
        {
            PromCounter,    ///< A Prometheus counter metric
            PromGauge,      ///< A Prometheus gauge metric
            PromHistogram,  ///< A Prometheus histogram metric
            PromSummary,    ///< A Prometheus summary metric
            PromUntyped     ///< A Prometheus untyped metric
        };

        /// @brief The timestamp mode a metric is using.
        enum TimeMode
        {
            /// @brief The metric will NOT include timestamps in the output.
            TimeSkip,

            /// @brief The metric will include the time of the last data set.
            /// The timestamp will be updated whenever the data is set.
            TimeSet,

            /// @brief The metric will always include the current time.
            /// The code needs to ensure the data is ALWAYS up to date.
            TimeCurrent
        };

        const String Name; ///< Name of the metric

        /// @brief A description of the metric
        /// @note "help" is somewhat misleading but it's a term from the Prometheus documentation, it's more
        /// a description of the metric
        const String Help;

        const MetricType Type;        ///< The metric type
        const TimeMode TimestampMode; ///< The timestamp mode used by this metric.

    protected:
        static TextLog _log; ///< Log stream
        const StringList _labelNames; ///< List of label names used by the metric

        /// @brief Constructor for a metric with labels
        /// @param [in] type The type of the metric
        /// @param [in] timestampMode The timestamp mode this metric uses
        /// @param [in] name A valid Prometheus metric name
        ///                  A valid metric name matches the regex [a-zA-Z_:][a-zA-Z0-9_:]*
        ///                  A metric name must not match any existing metric names
        /// @param [in] labelNames A comma separated list of distinct, valid Prometheus label names
        /// @param [in] help A description of the metric
        /// A valid label name matches the regex [a-zA-Z_][a-zA-Z0-9_]*
        PrometheusMetric (
            MetricType type, TimeMode timestampMode,
            const String & name, const String & labelNames, const String & help );

        /// @brief Destructor
        virtual ~PrometheusMetric();

        /// @brief Adds a child to the metric.
        /// @param [in] child Reference to the child to be added.
        /// @note The child must not have label values that match an existing child as each child must have a
        /// unique set of label values.
        void addChild ( PrometheusChild & child );

        /// @brief Removes a child from the metric.
        /// @param [in] child Reference to the child to be removed.
        void removeChild ( PrometheusChild & child );

        /// @brief Appends the Prometheus text exposition of the metric to a buffer
        /// @param [out] buf The buffer to append the data to.
        /// @param [in] currentTimestamp The value to be used as the "current" timestamp.
        ///                              If 0, current timestamps will not be generated.
        void appendData ( Buffer & buf, uint64_t currentTimestamp );

        /// @brief Converts a comma separated list of label names to a StringList, trimming whitespace
        /// @param [in] str The comma separated string (which may contain whitespace)
        /// @return The whitespace trimmed StringList of label names
        static StringList getWSTrimmedList ( const String & str );

        friend class PrometheusManager;
        friend class PrometheusChild;

    private:
        List<PrometheusChild *> _children;  ///< List of child metrics

        /// @brief Gets the string representation of the type of the metric
        /// @return The string with the Prometheus metric type
        const char * getTypeStr() const;

        /// @brief Helper function that combines the lists of label names and values into a string
        /// name1=value1,name2=value2,...
        /// @note More details here: https://prometheus.io/docs/instrumenting/exposition_formats/#text-format-details
        /// @param [in] labelValues A comma separated list of label values
        /// @return A string of the combined metric label names and values
        String generateLabelStr ( const String & labelValues );

        /// @brief Checks whether or not a string is a valid metric name, as per the Prometheus specification.
        /// A valid metric name matches the regex [a-zA-Z_:][a-zA-Z0-9_:]*
        /// @param [in] metricName The string to check
        /// @return true if the string is a valid metric name; false otherwise
        static bool validateMetricName ( const String & metricName );

        /// @brief Checks whether or not a string is a valid label name, as per the Prometheus specification
        /// A valid label name matches the regex [a-zA-Z_][a-zA-Z0-9_]*
        /// @param [in] labelName The string to check
        /// @return true if the string is a valid label name; false otherwise
        static bool validateLabelName ( const String & labelName );
};
}
