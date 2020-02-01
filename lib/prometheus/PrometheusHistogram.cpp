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

#include "PrometheusHistogram.hpp"

using namespace Pravala;

PrometheusHistogram::PrometheusHistogram (
        const String & name, size_t numBuckets, const int64_t bucketUpperBounds[], const String & help ):
    PrometheusChild ( new PrometheusHistogramMetric ( name, "", numBuckets, bucketUpperBounds, help ), "" ),
    _numBuckets ( numBuckets ),
    _buckets ( new Bucket[ numBuckets ] ),
    _totalCount ( 0 ),
    _sum ( 0 )
{
    for ( size_t i = 0; i < _numBuckets; ++i )
    {
        _buckets[ i ].upperBound = bucketUpperBounds[ i ];
        _buckets[ i ].count = 0;
    }
}

PrometheusHistogram::PrometheusHistogram ( PrometheusHistogramMetric & parent, const String & labelValues ):
    PrometheusChild ( parent, labelValues ),
    _numBuckets ( parent.getNumBuckets() ),
    _buckets ( new Bucket[ _numBuckets ] ),
    _totalCount ( 0 ),
    _sum ( 0 )
{
    assert ( parent.getBucketUpperBoundsArray() != 0 );
    assert ( _numBuckets > 0 );

    const int64_t * bucketArray = parent.getBucketUpperBoundsArray();

    for ( size_t i = 0; i < _numBuckets; ++i )
    {
        _buckets[ i ].upperBound = bucketArray[ i ];
        _buckets[ i ].count = 0;
    }
}

PrometheusHistogram::~PrometheusHistogram()
{
    delete[] _buckets;
}

void PrometheusHistogram::observe ( int64_t value )
{
    for ( size_t i = 0; i < _numBuckets; ++i )
    {
        if ( value <= _buckets[ i ].upperBound )
        {
            _buckets[ i ].count++;
        }
    }

    ++_totalCount;

    _sum += value;
}

void PrometheusHistogram::appendData ( Buffer & buf, const String & name, uint64_t )
{
    // Histograms always skip the timestamp.

    const String labelStr = LabelStr.isEmpty() ? "" : String ( "{%1}" ).arg ( LabelStr );
    const String labelsBucketPrefix = LabelStr.isEmpty() ? "" : String ( "%1," ).arg ( LabelStr );

    for ( size_t i = 0; i < _numBuckets; ++i )
    {
        buf.append ( String ( "%1_bucket{%2le=\"%3\"} %4\n" )
                     .arg ( name, labelsBucketPrefix )
                     .arg ( _buckets[ i ].upperBound )
                     .arg ( _buckets[ i ].count ) );
    }

    // +Inf bucket
    buf.append ( String ( "%1_bucket{%2le=\"+Inf\"} %3\n" )
                 .arg ( name, labelsBucketPrefix )
                 .arg ( _totalCount ) );

    // sum
    buf.append ( String ( "%1_sum%2 %3\n" ).arg ( name, labelStr ).arg ( _sum ) );

    // totalCount
    buf.append ( String ( "%1_count%2 %3\n" ).arg ( name, labelStr ).arg ( _totalCount ) );
}
