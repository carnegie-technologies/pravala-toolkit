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
#include "basic/HashMap.hpp"
#include "basic/NoCopy.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
class PrometheusMetric;

/// @brief Prometheus manager.
class PrometheusManager: public NoCopy
{
    public:
        /// @brief Gets the singleton instance of the Prometheus manager
        /// @return The instance of the Prometheus manager
        static PrometheusManager & get();

        /// @brief Gets the collection of Prometheus metric expositions for all registered metrics.
        /// @return The text exposition of all collected metrics as per the Prometheus text format
        MemHandle getData();

    protected:
        /// @brief Registers the specified metric for data collection
        /// All registered metrics must have unique names. Metrics must not be registered with names that are
        /// duplicates of previously registered metric names.
        /// @param [in] metric A reference to the metric to register
        void registerMetric ( PrometheusMetric & metric );

        /// @brief Unregisters the specified metric from data collection
        /// @param [in] metric A reference to the metric to unregister
        void unregisterMetric ( PrometheusMetric & metric );

    private:
        static TextLog _log;    ///< log stream

        /// @brief HashMap of registered metrics, keyed by the metric's name
        HashMap<String, PrometheusMetric *> _metrics;

        size_t _maxAllocatedBufSize; ///< the maximum previously allocated size of the buffer for the text exposition

        /// @brief Default constructor.
        PrometheusManager();

        friend class PrometheusMetric;
};
}
