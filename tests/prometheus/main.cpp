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

#include "basic/Random.hpp"
#include "config/ConfigAddrSpec.hpp"
#include "sys/SocketApi.hpp"
#include "app/StdApp.hpp"
#include "event/Timer.hpp"

#include "prometheus/PrometheusServer.hpp"
#include "prometheus/BasicPrometheusCounter.hpp"
#include "prometheus/BasicPrometheusGauge.hpp"
#include "prometheus/PrometheusHistogram.hpp"

using namespace Pravala;

// instrumentation metrics

// Counter without labels
static BasicPrometheusCounter upTimeCounter (
        PrometheusMetric::TimeCurrent, // The data is always current
        "test_data_up_time",
        "The number of seconds this has been running." );

// Counter with labels
static PrometheusCounterMetric counterMetric (
        PrometheusMetric::TimeCurrent, // The data is always current
        "test_data_labelled_counter",
        "Name1, Name2,      Name3",
        "A counter with some labels." );

static BasicPrometheusCounter count1 (
        counterMetric,
        "Something, SomethingElse,Label3" );
static BasicPrometheusCounter count2 (
        counterMetric,
        "Something, Label2, Label3" );
static BasicPrometheusCounter count3 (
        counterMetric,
        "Label1,Label2,Label3" );
static BasicPrometheusCounter count4 (
        counterMetric,
        "aaa,bbb,ccc" );

// Gauge without labels
static BasicPrometheusGauge childGauge (
        PrometheusMetric::TimeCurrent, // The data is always current
        "test_data_gauge",
        "A gauge without labels." );

// Gauge with labels
static PrometheusGaugeMetric gaugeMetric (
        PrometheusMetric::TimeCurrent, // The data is always current
        "test_data_labelled_gauge",
        "GaugeName1, GaugeName2",
        "A gauge with some labels." );

static BasicPrometheusGauge gauge1 (
        gaugeMetric,
        "Foo,Bar" );
static BasicPrometheusGauge gauge2 (
        gaugeMetric,
        "All,Nothing" );

// The upper bounds of the histogram buckets
static const int64_t upperBounds[] = { 5, 10, 20, 50, 75, 90 };

// Histogram without labels
static PrometheusHistogram childHistogram (
        "test_data_histogram",
        sizeof ( upperBounds ) / sizeof ( upperBounds[ 0 ] ),
        upperBounds,
        "A histogram without labels." );

// Histogram with labels
static PrometheusHistogramMetric histogramMetric (
        "test_data_labelled_histogram",
        "HistoLabel1, HistoLabel2, HistoLabel3",
        sizeof ( upperBounds ) / sizeof ( upperBounds[ 0 ] ),
        upperBounds,
        "A histogram with labels." );
static PrometheusHistogram histo1 (
        histogramMetric,
        "Bad, OK, Good" );
static PrometheusHistogram histo2 (
        histogramMetric,
        "OK, Good, Better" );
static PrometheusHistogram histo3 (
        histogramMetric,
        "Best,Best,Best" );

class DataGenerator: public Timer::Receiver
{
    public:
        DataGenerator(): _timer ( FixedTimer ( *this, 1000 ) )
        {
            // kick data generation every second
            _timer.start();
        }

        virtual void timerExpired ( Timer * )
        {
            _timer.start(); // restart timer

            // generate some random data
            int randData = Random::rand ( 100 );

            upTimeCounter.increment();

            // selectively increment/decrement/observe stuff based on the data received
            if ( randData < 10 )
            {
                count1.increment();
                gauge2.increment ( 7 );
                histo1.observe ( randData );
            }
            else if ( randData < 50 )
            {
                count2.increment();
                gauge1.increment ( 150 );
                histo1.observe ( randData );
            }
            else if ( randData < 70 )
            {
                count3.increment();
                gauge1.decrement ( 500 );
                histo2.observe ( randData );
            }
            else if ( randData < 90 )
            {
                count4.increment();
                gauge2.decrement ( 10 );
                histo2.observe ( randData );
            }
            else
            {
                histo3.observe ( randData );
            }

            childHistogram.observe ( randData );
        }

    private:
        FixedTimer _timer;
};

static ConfigAddrSpec optPrometheusServer (
        "listen-address",
        'l',
        "The address to listen on for Prometheus requests",
        "0.0.0.0",
        9999
);

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv );

    app.initConfig();
    app.initFeatures();

    // start Prometheus Server and instrument stuff
    PrometheusServer promServer;

    if ( optPrometheusServer.isSet() )
    {
        printf ( "Starting Prometheus listener on '%s'\n", optPrometheusServer.value().toString().c_str() );

        ERRCODE eCode = promServer.addListener ( optPrometheusServer.value() );

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Could not start a Prometheus listener on '%s': %s\n",
                      optPrometheusServer.value().toString().c_str(), eCode.toString() );

            return EXIT_FAILURE;
        }
    }
    else
    {
        fprintf ( stderr, "Invalid server address/port specified.\n" );
        app.printHelp ( stderr );
        return EXIT_FAILURE;
    }

    DataGenerator dGen;

    EventManager::run();
    return EXIT_SUCCESS;
}
