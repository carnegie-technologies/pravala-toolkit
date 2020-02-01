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

#include "PrometheusHistogramMetric.hpp"

using namespace Pravala;

PrometheusHistogramMetric::PrometheusHistogramMetric (
        const String & name,
        const String & labelNames,
        size_t numBuckets,
        const int64_t bucketUpperBounds[],
        const String & help ):
    PrometheusMetric ( PrometheusMetric::PromHistogram, PrometheusMetric::TimeSkip, name, labelNames, help ),
    _numBuckets ( numBuckets ),
    _bucketUpperBounds ( new int64_t[ _numBuckets ] )
{
#ifndef NDEBUG
    assert ( _numBuckets > 0 );
    assert ( bucketUpperBounds != 0 );

    // validates that there are no "le" labelNames

    for ( size_t i = 0; i < _labelNames.size(); ++i )
    {
        assert ( _labelNames[ i ] != "le" );
    }

    for ( size_t i = 1; i < _numBuckets; ++i )
    {
        // ensure that the buckets are in increasing order
        assert ( bucketUpperBounds[ i - 1 ] < bucketUpperBounds[ i ] );
    }
#endif

    for ( size_t i = 0; i < _numBuckets; ++i )
    {
        _bucketUpperBounds[ i ] = bucketUpperBounds[ i ];
    }
}

PrometheusHistogramMetric::~PrometheusHistogramMetric()
{
    delete[] _bucketUpperBounds;
}
