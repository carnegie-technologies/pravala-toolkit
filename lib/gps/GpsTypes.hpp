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
/// @brief This template contains the elements of a value read from a GPS device.
/// Example values which are stored using this template are latitude (a double), longitude (a double),
/// altitude (a double)
/// These values all contain their value, and potentially the error estimate associated with the value.
/// We add the isValid flag to avoid encoding invalid value states in value, since different stored values
/// may represent an erroneous value differently (i.e. 0 may not be invalid).
template<typename T> struct GpsValue
{
    bool isValid; ///< Whether the following two fields are valid or not. If isValid == false; ignore value & error.
    T value; ///< The value of the GPS field
    double error; ///< The associated error measurement of this field. -1 if no error value is known.

    /// @brief Default constructor.
    GpsValue(): isValid ( false ), value ( 0 ), error ( -1 )
    {
    }

    /// @brief Constructor.
    /// @param [in] value_ The value to store
    /// @param [in] error_ The error to store (-1 for an invalid error)
    GpsValue ( T value_, double error_ ): isValid ( true ), value ( value_ ), error ( error_ )
    {
    }

    /// @brief Compare whether a GpsValue object contains the same values as this object.
    /// For now the values need to be identical for this to return true, in the future we may wish to
    /// take into consideration the relative error values.
    /// @param [in] other The object to compare against.
    /// @return true if the objects contain the same values or both are invalid; false otherwise.
    inline bool operator== ( const GpsValue<T> & other ) const
    {
        if ( !isValid && !other.isValid )
            return true;

        return ( isValid == other.isValid && value == other.value && error == other.error );
    }

    /// @brief Compare whether a GpsValue object is different than this object.
    /// Inverse operation to == above.
    /// @return true if the objects contain different values; false if the objects contain the same values.
    inline bool operator!= ( const GpsValue<T> & other ) const
    {
        return !( *this == other );
    }

    /// @brief Reset this object.
    inline void clear()
    {
        isValid = false;
        error = -1;
        value = 0;
    }
};

/// @brief A GPS co-ordinate (i.e. a latitude, a longitude and an altitude).
struct GpsCoordinate
{
    /// @brief Compare whether two locations are the same.
    /// @param other The object to compare against.
    /// @return true if the same; false otherwise.
    inline bool operator== ( const GpsCoordinate & other ) const
    {
        return ( latitude == other.latitude && longitude == other.longitude && altitude == other.altitude );
    }

    /// @brief Negate the action of operator==
    /// @param [in] other The object to compare against.
    /// @return true if different; false if the same.
    inline bool operator!= ( const GpsCoordinate & other ) const
    {
        return !( *this == other );
    }

    /// @brief Calculate the radius of accuracy for this measurement; based upon the errors of the
    /// latitude and longitude. This yields the 68% (first standard deviation) accuracy.
    /// This copies the Android implementation, which also yields the first standard deviation for accuracy.
    ///
    /// @note We don't pre-calculate & store this value since it isn't necessarily going to be requested,
    /// and it is fairly expensive to calculate, so the caller should be smart to cache this value if needed.
    ///
    /// @return The calculated radius of the accuracy circle in metres; -1 if latitude or longitude invalid.
    double calcAccuracyRadius() const;

    /// @brief The .value stores the latitude in degrees, the .error contains error estimate in metres.
    /// Yes, the .value is degrees and the .error is in metres.
    GpsValue<double> latitude;

    /// @brief The .value stores the longitude in degrees, the .error contains error estimate in metres.
    /// As above.
    GpsValue<double> longitude;

    /// @brief The .value stores the altitude in metres, the .error contains error estimate in metres.
    GpsValue<double> altitude;

    /// @brief Reset the values in this object.
    void clear();
};

/// @brief A GPS motion measurement (i.e. speed, direction, and rate of climb).
struct GpsVector
{
    /// @brief .value stores the direction of travel in degrees from true north,
    ///        .error contains error estimate in degrees.
    GpsValue<double> direction;

    /// @brief The .value stores the speed in metres/sec, .error contains the error estimate in metres/sec.
    GpsValue<double> speed;

    /// @brief .value stores the climb (+ve) or sink (-ve) rates in metres/sec
    ///        .error is error estimate in metres/sec.
    GpsValue<double> climb;

    /// @brief Reset the values in this object.
    void clear();
};
}
