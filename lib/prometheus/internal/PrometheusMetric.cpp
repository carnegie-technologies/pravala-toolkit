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

#include "basic/HashSet.hpp"
#include "PrometheusMetric.hpp"
#include "PrometheusManager.hpp"
#include "PrometheusChild.hpp"

using namespace Pravala;

TextLog PrometheusMetric::_log ( "prometheus_metric" );

PrometheusMetric::PrometheusMetric (
        MetricType type, TimeMode timestampMode, const String & name, const String & labelNames, const String & help ):
    Name ( name ),
    Help ( help ),
    Type ( type ),
    TimestampMode ( timestampMode ),
    _labelNames ( getWSTrimmedList ( labelNames ) )
{
    assert ( name != 0 );
    assert ( validateMetricName ( String ( name ) ) );

#ifndef NDEBUG
    // check each string in list against regex
    for ( size_t i = 0; i < _labelNames.size(); ++i )
    {
        assert ( validateLabelName ( _labelNames[ i ] ) );
    }

    // ensure no duplicate label names
    HashSet<String> set;

    for ( size_t i = 0; i < _labelNames.size(); ++i )
    {
        assert ( !set.contains ( _labelNames[ i ] ) );
        set.insert ( _labelNames[ i ] );
    }
#endif

    PrometheusManager::get().registerMetric ( *this );
}

PrometheusMetric::~PrometheusMetric()
{
    PrometheusManager::get().unregisterMetric ( *this );
    _children.clear();
}

bool PrometheusMetric::validateMetricName ( const String & name )
{
    for ( int i = 0; i < name.length(); ++i )
    {
        char c = name[ i ];

        if ( !( ( 'A' <= c && c <= 'Z' )
                || ( 'a' <= c && c <= 'z' )
                || c == '_'
                || c == ':'
                || ( i > 0 && '0' <= c && c <= '9' ) ) )
        {
            return false;
        }
    }

    return true;
}

bool PrometheusMetric::validateLabelName ( const String & name )
{
    for ( int i = 0; i < name.length(); ++i )
    {
        char c = name[ i ];

        if ( !( ( 'A' <= c && c <= 'Z' )
                || ( 'a' <= c && c <= 'z' )
                || c == '_'
                || ( i > 0 && '0' <= c && c <= '9' ) ) )
        {
            return false;
        }
    }

    return true;
}

const char * PrometheusMetric::getTypeStr() const
{
    switch ( Type )
    {
        case PromCounter:
            return "counter";

        case PromGauge:
            return "gauge";

        case PromHistogram:
            return "histogram";

        case PromSummary:
            return "summary";

        case PromUntyped:
            return "untyped";
    }

    return "invalid";
}

StringList PrometheusMetric::getWSTrimmedList ( const String & str )
{
    return str.split ( ", \t\n\v\f\r" );
}

void PrometheusMetric::addChild ( PrometheusChild & child )
{
    if ( _children.findValue ( &child ) )
    {
        LOG ( L_FATAL_ERROR, "The same child registered more than once: '" << child.LabelStr << "'" );

        assert ( false );
        return;
    }

#ifndef NDEBUG
    for ( size_t i = 0; i < _children.size(); ++i )
    {
        if ( child.LabelStr == _children[ i ]->LabelStr )
        {
            LOG ( L_FATAL_ERROR, "Children with duplicate label values found: '" << child.LabelStr << "'" );
            assert ( false );
        }
    }
#endif

    _children.append ( &child );
}

void Pravala::PrometheusMetric::removeChild ( Pravala::PrometheusChild & child )
{
    _children.removeValue ( &child );

    // No child should be registered more than once:
    assert ( !_children.removeValue ( &child ) );
}

void PrometheusMetric::appendData ( Buffer & buf, uint64_t timestamp )
{
    // 'timestamp' passed is the current timestamp to be used.
    // Depending on TimestampMode we may use it, or use something else.

    // help and type headers
    buf.append ( "# HELP " );
    buf.append ( Name );
    buf.append ( " " );
    buf.append ( Help );
    buf.append ( "\n# TYPE " );
    buf.append ( Name );
    buf.append ( " " );
    buf.append ( getTypeStr() );
    buf.append ( "\n" );

    if ( TimestampMode != TimeCurrent )
    {
        // This metric doesn't want to use the current timestamp.
        // It either wants to skip it (in which case it will remain as 0),
        // or include the timestamps of children that should be updating their times when needed.
        timestamp = 0;
    }

    for ( size_t i = 0; i < _children.size(); ++i )
    {
        PrometheusChild * const c = _children[ i ];

        if ( TimestampMode == TimeSet )
        {
            timestamp = c->getTimestamp();
        }

        c->appendData ( buf, Name, timestamp );
    }
}

String PrometheusMetric::generateLabelStr ( const String & labelValues )
{
    if ( _labelNames.isEmpty() )
    {
        return String();
    }

    // parse the comma separated list
    StringList labelValueList = getWSTrimmedList ( labelValues );

    // ensure the number of label values matches the number of label names
    if ( _labelNames.size() != labelValueList.size() )
    {
        LOG ( L_FATAL_ERROR, "PrometheusLib: Invalid label string due to mismatched number of label names and values" );

        assert ( false );

        return "INVALID_LABEL_CONFIG";
    }

    Buffer buf;

    // combine the strings
    for ( size_t i = 0; i < _labelNames.size(); ++i )
    {
        if ( i > 0 )
        {
            buf.append ( "," );
        }

        buf.append ( _labelNames[ i ] );
        buf.append ( "=\"" );
        buf.append ( labelValueList[ i ] );
        buf.append ( "\"" );
    }

    return buf.toString();
}
