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

#include "PrometheusChild.hpp"
#include "PrometheusMetric.hpp"

using namespace Pravala;

PrometheusChild::PrometheusChild ( PrometheusMetric * metric, const String & labelValues ):
    LabelStr ( metric->generateLabelStr ( labelValues ) ),
    _metric ( metric ),
    _autoDeleteMetric ( true )
{
    assert ( _metric != 0 );

    _metric->addChild ( *this );
}

PrometheusChild::PrometheusChild ( PrometheusMetric & metric, const String & labelValues ):
    LabelStr ( metric.generateLabelStr ( labelValues ) ),
    _metric ( &metric ),
    _autoDeleteMetric ( false )
{
    assert ( _metric != 0 );

    _metric->addChild ( *this );
}

uint64_t PrometheusChild::getTimestamp()
{
    return 0;
}

PrometheusChild::~PrometheusChild()
{
    _metric->removeChild ( *this );

    if ( _autoDeleteMetric )
    {
        delete _metric;
    }
}
