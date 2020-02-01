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

#include <cmath>
#include <cstdlib>

#include "Math.hpp"

namespace Pravala
{
double distance_between (
        const double lat1, const double lon1,
        const double lat2, const double lon2 )
{
    static const double EARTH_RADIUS_KM = 6378.1;
    static const double PI = 3.14159265358979323846; // from math.h, not standard C++ constant
    static const double PI_180 = PI / 180;

    // Calculate the distance between two GPS coordinates using the haversine formula
    // (see http://www.movable-type.co.uk/scripts/latlong.html)

    const double deltaLat = ( lat2 - lat1 ) * PI_180;
    const double deltaLong = ( lon2 - lon1 ) * PI_180;

    const double sinHalfLat = sin ( deltaLat / 2 );
    const double sinHalfLong = sin ( deltaLong / 2 );

    const double a = ( sinHalfLat * sinHalfLat )
                     + ( cos ( lat1 * PI_180 )
                         * cos ( lat2 * PI_180 )
                         * sinHalfLong * sinHalfLong );

    const double c = 2.0 * atan2 ( sqrt ( a ), sqrt ( 1.0 - a ) );

    return std::abs ( c * EARTH_RADIUS_KM );
}
}
