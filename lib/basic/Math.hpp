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

#pragma once

namespace Pravala
{
/// @brief Returns absolute (positive) difference between two numbers
/// @param [in] a First number
/// @param [in] b Second number
/// @return absolute difference between two numbers
template<typename T> inline T abs_diff ( T a, T b )
{
    return ( a > b ) ? ( a - b ) : ( b - a );
}

// In the case where the max macro is defined (such as in MSVC),
// we should undef it so we use our type safe template version.
#ifdef max
#undef max
#endif

/// @brief Returns max of two numbers
/// @param [in] a First number
/// @param [in] b Second number
/// @return max of two numbers
template<typename T> inline T max ( T a, T b )
{
    return ( a > b ) ? ( a ) : ( b );
}

// In the case where the min macro is defined (such as in MSVC),
// we should undef it so we use our type safe template version.
#ifdef min
#undef min
#endif

/// @brief Returns min of two numbers
/// @param [in] a First number
/// @param [in] b Second number
/// @return min of two numbers
template<typename T> inline T min ( T a, T b )
{
    return ( a < b ) ? ( a ) : ( b );
}

/// @brief Limit a value to the provided range.
/// @param [in] value The value to limit
/// @param [in] minValue The range minimum.
/// @param [in] maxValue The range maximum.
/// @return A value in the range [min, max].
template<typename T> inline T limit ( T value, T minValue, T maxValue )
{
    return ( value <= minValue ) ? minValue : ( ( value >= maxValue ) ? maxValue : value );
}

/// @brief Calculates the distance between two locations
/// @param [in] lat1 Latitude of first location (degrees)
/// @param [in] lon1 Longiture of first location (degrees)
/// @param [in] lat2 Latitude of second location (degrees)
/// @param [in] lon2 Longitude of second location (degrees)
/// @return The (positive) distance between these two points in kilometres
double distance_between (
        const double lat1, const double lon1,
        const double lat2, const double lon2 );
}
