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
#include "BasicPrometheusGauge.hpp"

using namespace Pravala;

BasicPrometheusGauge::BasicPrometheusGauge (
        PrometheusMetric::TimeMode timestampMode, const String & name, const String & help ):
    PrometheusGauge ( timestampMode, name, help ),
    _value ( 0 ),
    _timestamp ( 0 )
{
}

BasicPrometheusGauge::BasicPrometheusGauge ( PrometheusGaugeMetric & parent, const String & labelValues ):
    PrometheusGauge ( parent, labelValues ),
    _value ( 0 ),
    _timestamp ( 0 )
{
}

BasicPrometheusGauge::~BasicPrometheusGauge()
{
}

void BasicPrometheusGauge::increment ( uint64_t value )
{
    adjust ( value );
}

void BasicPrometheusGauge::decrement ( uint64_t value )
{
    adjust ( -static_cast<int64_t> ( value ) );
}

void BasicPrometheusGauge::adjust ( int64_t value )
{
    _value += value;
    updateTimestamp();
}

int64_t BasicPrometheusGauge::getValue()
{
    return _value;
}

uint64_t BasicPrometheusGauge::getTimestamp()
{
    return _timestamp;
}

void BasicPrometheusGauge::updateTimestamp()
{
    _timestamp = CalendarTime::getUTCEpochTimeMs();
}
