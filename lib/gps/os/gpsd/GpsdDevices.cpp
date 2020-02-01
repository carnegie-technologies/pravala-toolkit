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

#include "GpsdDevices.hpp"

#include "json/JsonArray.hpp"

using namespace Pravala;

TextLog GpsdDevices::_log ( "gpsd_devices" );

bool GpsdDevices::parse ( const Json & devices )
{
    clear();

    if ( !devices.isObject() )
    {
        LOG ( L_ERROR, "Provided Devices JSON is not a JSON object; ignoring" );
        return false;
    }

    String classField;
    if ( !devices.get ( "class", classField ) )
    {
        LOG ( L_ERROR, "Provided Devices JSON lacks a field named 'class'; ignoring" );
        return false;
    }

    if ( classField != "DEVICES" )
    {
        LOG ( L_ERROR, "Provided Devices JSON has a class of '" << classField << "'; not 'DEVICES'; ignoring" );
        return false;
    }

    JsonArray devicesArr;
    if ( !devices.get ( "devices", devicesArr ) )
    {
        LOG ( L_ERROR, "Provided Devices JSON has no field named 'devices'; ignoring" );
        return false;
    }

    for ( size_t i = 0; i < devicesArr.size(); ++i )
    {
        Json device;
        if ( !devicesArr.get ( i, device ) )
        {
            LOG ( L_ERROR, "Error retrieving Device JSON object from 'devices' array at index " << i
                  << "; ignoring rest of message" );
            return false;
        }

        String deviceClass;
        if ( !device.get ( "class", deviceClass ) )
        {
            LOG ( L_ERROR, "Error retrieving 'class' from Device JSON object at index " << i
                  << "; ignoring rest of message" );
            return false;
        }

        if ( deviceClass != "DEVICE" )
        {
            LOG ( L_ERROR, "Provided Device JSON at index " << i << " has a class of '" << classField
                  << "'; not 'DEVICE'; ignoring rest of message" );
            return false;
        }

        String devicePath;
        if ( !device.get ( "path", devicePath ) )
        {
            LOG ( L_ERROR, "Error retrieving 'path' from Device JSON object at index " << i
                  << "; ignoring rest of message " );
            return false;
        }

        _devices.append ( devicePath );
    }

    return true;
}

void GpsdDevices::clear()
{
    _devices.clear();
}
