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

#include "sys/CalendarTime.hpp"
#include "BasicPrometheusCounter.hpp"

using namespace Pravala;

BasicPrometheusCounter::BasicPrometheusCounter (
        PrometheusMetric::TimeMode timestampType, const String & name, const String & help ):
    PrometheusCounter ( timestampType, name, help ),
    _value ( 0 ),
    _timestamp ( 0 )
{
}

BasicPrometheusCounter::BasicPrometheusCounter ( PrometheusCounterMetric & parent, const String & labelValues ):
    PrometheusCounter ( parent, labelValues ),
    _value ( 0 ),
    _timestamp ( 0 )
{
}

BasicPrometheusCounter::~BasicPrometheusCounter()
{
}

void BasicPrometheusCounter::increment ( uint64_t value )
{
    _value += value;
    updateTimestamp();
}

uint64_t BasicPrometheusCounter::getValue()
{
    return _value;
}

uint64_t BasicPrometheusCounter::getTimestamp()
{
    return _timestamp;
}

void BasicPrometheusCounter::updateTimestamp()
{
    _timestamp = CalendarTime::getUTCEpochTimeMs();
}
