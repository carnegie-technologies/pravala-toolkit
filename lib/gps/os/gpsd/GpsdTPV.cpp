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

#include "GpsdTPV.hpp"

using namespace Pravala;

// Values for GPS mode detailed in http://catb.org/gpsd/gpsd_json.html, section: TPV

// We don't know the mode of the GPS device yet.
#define GPS_MODE_UNKNOWN    0
// The GPS device does not have a fix on our location.
#define GPS_MODE_NO_FIX     1
// 2D fix, we will have x & y coordinates along with speed, direction, etc.
#define GPS_MODE_2D_FIX     2
// 3D fix, we will now have everything in 2D fix mode + altitude.
#define GPS_MODE_3D_FIX     3

/*
    Whitespaces added by me. Each object is split by a newline.

    Sample gpsd TPV with no lock:
    {"class":"TPV","tag":"GSA","device":"/dev/ttyACM0","mode":1}

    Sample gpsd TPV with data:
    {"class":"TPV","tag":"RMC","device":"/dev/ttyACM0","mode":3,"time":"2013-09-24T16:13:43.000Z","ept":0.005,
    "lat":43.458530000,"lon":-80.471866667,"alt":338.900,"epx":20.156,"epy":32.706,"epv":138.724,
    "track":70.2000,"speed":0.000,"climb":-1.100,"eps":65.41}

    Command to enable:
    ?WATCH={"enable":true,"json":true}

    Command to disable:
    ?WATCH={"enable":false,"json":false}

    Response to ?WATCH
    {"class":"DEVICES","devices":[{"class":"DEVICE","path":"/dev/ttyACM0","activated":"2013-09-24T01:02:06.260Z",
    "native":0,"bps":115200,"parity":"N","stopbits":1,"cycle":1.00}]}
    {"class":"WATCH","enable":true,"json":false,"nmea":false,"raw":0,"scaled":false,"timing":false}
 */

TextLog GpsdTPV::_log ( "gpsd_tpv" );

GpsdTPV::GpsdTPV(): _hasGpsLock ( false )
{
}

bool GpsdTPV::parse ( const Json & tpv )
{
    clear();

    if ( !tpv.isObject() )
    {
        LOG ( L_ERROR, "Provided TPV JSON is not a JSON object; ignoring" );
        return false;
    }

    String classField;
    if ( !tpv.get ( "class", classField ) )
    {
        LOG ( L_ERROR, "Provided TPV JSON lacks a field named 'class'; ignoring" );
        return false;
    }

    if ( classField != "TPV" )
    {
        LOG ( L_ERROR, "Provided TPV JSON has a class of '" << classField << "'; not 'TPV'; ignoring" );
        return false;
    }

    int mode = GPS_MODE_UNKNOWN;
    if ( !tpv.get ( "mode", mode ) )
    {
        LOG ( L_ERROR, "Provided TPV JSON lacks a field named 'mode'; ignoring" );
        return false;
    }

    if ( mode > GPS_MODE_3D_FIX || mode < GPS_MODE_UNKNOWN )
    {
        LOG ( L_ERROR, "Provided TPV JSON has an invalid value for 'mode': " << mode
              << "; must be >= " << GPS_MODE_UNKNOWN << " && <= " << GPS_MODE_3D_FIX << "; ignoring" );
        return false;
    }

    if ( mode < GPS_MODE_2D_FIX )
    {
        _hasGpsLock = false;
        LOG ( L_DEBUG2, "Provided TPV JSON is not locked; ignoring rest of message" );

        // While 'time' and 'ept' may be present if we aren't locked, we will not parse the time values
        // because they are presumably erroneous enough without a lock it isn't worth trying to retrieve them.
        return true;
    }

    _hasGpsLock = true;

    // Since mode is 2 or 3, we should have both latitude and longitude. Lacking them is an error.
    if ( !tpv.get ( "lat", _location.latitude.value ) || !tpv.get ( "lon", _location.longitude.value ) )
    {
        LOG ( L_ERROR, "Provided TPV JSON is in mode " << mode << " but lacks a 'lat' or 'lon' value; "
              "ignoring rest of message" );
        return false;
    }

    _location.latitude.isValid = true;
    _location.longitude.isValid = true;

    // Since mode is 2 or 3, we MAY have the latitude and longitude errors (if they can be calculated).
    if ( !tpv.get ( "epx", _location.latitude.error ) )
    {
        _location.latitude.error = 0;
    }

    if ( !tpv.get ( "epy", _location.longitude.error ) )
    {
        _location.longitude.error = 0;
    }

    if ( mode == GPS_MODE_3D_FIX )
    {
        if ( !tpv.get ( "alt", _location.altitude.value ) )
        {
            LOG ( L_ERROR, "Provided TPV JSON is in mode " << mode << " but it lacks an 'alt' value; "
                  "ignoring rest of message" );
            return false;
        }

        _location.altitude.isValid = true;

        if ( !tpv.get ( "epv", _location.altitude.error ) )
        {
            _location.altitude.error = -1;
        }
    }

    if ( tpv.get ( "track", _vector.direction.value ) )
        _vector.direction.isValid = true;

    if ( !tpv.get ( "epd", _vector.direction.error ) )
        _vector.direction.error = -1;

    if ( tpv.get ( "speed", _vector.speed.value ) )
        _vector.speed.isValid = true;

    if ( !tpv.get ( "eps", _vector.speed.error ) )
        _vector.speed.error = -1;

    if ( tpv.get ( "climb", _vector.climb.value ) )
        _vector.climb.isValid = true;

    if ( !tpv.get ( "epc", _vector.climb.error ) )
        _vector.climb.error = -1;

    // We don't really care if this fails or succeeds; it is an optional field
    tpv.get ( "device", _deviceName );

    return true;
}

void GpsdTPV::clear()
{
    _hasGpsLock = false;
    _deviceName.clear();

    _location.clear();
    _vector.clear();
}
