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

#include "PrometheusCounter.hpp"

using namespace Pravala;

PrometheusCounter::PrometheusCounter (
        PrometheusMetric::TimeMode timestampMode, const String & name, const String & help ):
    PrometheusChild ( new PrometheusCounterMetric ( timestampMode, name, "", help ), "" )
{
}

PrometheusCounter::PrometheusCounter ( PrometheusCounterMetric & parent, const String & labelValues ):
    PrometheusChild ( parent, labelValues )
{
}

void PrometheusCounter::appendData ( Buffer & buf, const String & name, uint64_t timestamp )
{
    buf.append ( name );

    if ( !LabelStr.isEmpty() )
    {
        buf.append ( "{" );
        buf.append ( LabelStr );
        buf.append ( "}" );
    }

    buf.append ( " " );
    buf.append ( String::number ( getValue() ) );

    if ( timestamp > 0 )
    {
        buf.append ( " " );
        buf.append ( String::number ( timestamp ) );
    }

    buf.append ( "\n" );
}
