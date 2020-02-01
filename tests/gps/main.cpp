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

#include <cstdio>

#include "config/ConfigCore.hpp"
#include "event/EventManager.hpp"
#include "log/ConfigLogs.hpp"

#include "gps/GpsMonitor.hpp"

using namespace Pravala;

class GpsTest: public GpsMonitor::Receiver
{
    public:
        GpsTest(): _monitor ( *this )
        {
        }

        void start()
        {
            _monitor.start();
        }

    protected:
        void locationChanged ( const GpsCoordinate & location )
        {
            fprintf ( stdout, "GPS location" );

            if ( location.latitude.isValid )
            {
                fprintf ( stdout, "|lat=%f\167 +/- %fm", location.latitude.value, location.latitude.error );
            }
            else
            {
                fprintf ( stdout, "|no lat" );
            }

            if ( location.longitude.isValid )
            {
                fprintf ( stdout, "|lon=%f\167 +/- %fm", location.longitude.value, location.longitude.error );
            }
            else
            {
                fprintf ( stdout, "|no lon" );
            }

            if ( location.altitude.isValid )
            {
                fprintf ( stdout, "|alt=%fm +/- %fm", location.altitude.value, location.altitude.error );
            }
            else
            {
                fprintf ( stdout, "|no alt" );
            }

            fprintf ( stdout, "\n" );
        }

        void vectorChanged ( const GpsVector & vector )
        {
            fprintf ( stdout, "GPS vector" );

            if ( vector.direction.isValid )
            {
                fprintf ( stdout, "|dir=%f\167 +/- %f\167", vector.direction.value, vector.direction.error );
            }
            else
            {
                fprintf ( stdout, "|no lat" );
            }

            if ( vector.speed.isValid )
            {
                fprintf ( stdout, "|speed=%fm/s +/- %fm/s", vector.speed.value, vector.speed.error );
            }
            else
            {
                fprintf ( stdout, "|no speed" );
            }

            if ( vector.climb.isValid )
            {
                fprintf ( stdout, "|climb=%fm/s +/- %fm/s", vector.climb.value, vector.climb.error );
            }
            else
            {
                fprintf ( stdout, "|no climb" );
            }

            fprintf ( stdout, "\n" );
        }

        void devicesChanged ( const List<String> & devices )
        {
            fprintf ( stdout, "GPS devices" );

            for ( size_t i = 0; i < devices.size(); ++i )
            {
                fprintf ( stdout, "|path=%s", devices.at ( i ).c_str() );
            }

            fprintf ( stdout, "\n" );
        }

    private:
        GpsMonitor _monitor;
};

int main()
{
    // The purpose of this test is to display everything we receive, so always debug in verbose mode
    ConfigLogs logs;
    ConfigCore::EcfgInit ecfgInit (
            "log.0 = : *\n"
            "log.default_level = max\n"
    );
    logs.init();

    GpsTest test;
    test.start();

    fprintf ( stderr, "GpsClient started.\n" );

    // EventManager will exit when it receives STDINT interrupt (Ctrl-C)
    EventManager::run();

    return EXIT_SUCCESS;
}
