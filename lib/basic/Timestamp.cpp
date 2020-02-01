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

#include "Timestamp.hpp"

using namespace Pravala;

// Helper macro to convert given number to a string using 0-padding and given field width.
#define NUM_STR( what, width )    ( String::number ( ( what ), String::Int_Dec, ( width ), true ) )

Timestamp::TimeDesc::TimeDesc()
{
    memset ( this, 0, sizeof ( *this ) );
}

Timestamp::TimeDesc::TimeDesc (
        uint16_t year_, uint8_t month_, uint8_t day_,
        uint8_t h_, uint8_t m_, uint8_t s_, uint16_t ms_ ):
    year ( year_ ),
    month ( month_ ),
    day ( day_ ),
    hour ( h_ ),
    minute ( m_ ),
    second ( s_ ),
    millisecond ( ms_ )
{
}

Timestamp::TimeDesc::TimeDesc ( const struct tm & time, uint16_t ms_ ):
    year ( 1900 + time.tm_year ),
    // tm_mon in <ctime> is represented as "Months since January" (ie. 0-11 so need to offset by 1)
    month ( time.tm_mon + 1 ),
    day ( time.tm_mday ),
    hour ( time.tm_hour ),
    minute ( time.tm_min ),
    second ( time.tm_sec ),
    millisecond ( ms_ )
{
}

bool Timestamp::TimeDesc::isValid() const
{
    // NOTE: It's possible that the leap second is only allowed at the end of a month,
    //       which means this could be improved. For now we ignore that!

    return ( isValidDate ( year, month, day ) && isValidTime ( hour, minute, second, millisecond ) );
}

// MSVC doesn't like static const integrals defined in implementation files.
// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it...
#ifndef _MSC_VER
const uint64_t Timestamp::MinBinValue;
#endif

Timestamp::Timestamp(): _value ( MinBinValue )
{
}

void Timestamp::clear()
{
    _value = MinBinValue;
}

bool Timestamp::setBinValue ( uint64_t value )
{
    TimeDesc tDesc;

    return ( convertBinValue ( value, tDesc ) && setUtcTime ( tDesc ) );
}

bool Timestamp::convertBinValue ( uint64_t value, Timestamp::TimeDesc & tDesc )
{
    // Bits (starting from LSB):
    // 10b - milliseconds
    //  6b - seconds
    //  6b - minutes
    //  5b - hours
    //  5b - days
    //  4b - months
    // 14b - years

    tDesc.millisecond = ( value & 0x03FF );
    value >>= 10;

    tDesc.second = ( value & 0x003F );
    value >>= 6;

    tDesc.minute = ( value & 0x003F );
    value >>= 6;

    tDesc.hour = ( value & 0x001F );
    value >>= 5;

    tDesc.day = ( value & 0x001F );
    value >>= 5;

    tDesc.month = ( value & 0x000F );
    value >>= 4;

    tDesc.year = ( value & 0x03FFF );
    value >>= 14;

    // The result should be 0, or there is an error!
    return ( value == 0 );
}

bool Timestamp::setUtcTime ( const Timestamp::TimeDesc & tDesc )
{
    if ( !tDesc.isValid() )
        return false;

    // Bits
    // (starting from MSB - we add the value starting from LSB, then shift it left to make space for the next field):
    // 14b - years
    //  4b - months
    //  5b - days
    //  5b - hours
    //  6b - minutes
    //  6b - seconds
    // 10b - milliseconds

    _value = ( tDesc.year & 0x03FFF );

    _value <<= 4;
    _value += ( tDesc.month & 0x000F );

    _value <<= 5;
    _value += ( tDesc.day & 0x001F );

    _value <<= 5;
    _value += ( tDesc.hour & 0x001F );

    _value <<= 6;
    _value += ( tDesc.minute & 0x003F );

    _value <<= 6;
    _value += ( tDesc.second & 0x003F );

    _value <<= 10;
    _value += ( tDesc.millisecond & 0x03FF );

    return true;
}

bool Timestamp::setUtcTime ( struct tm & time, uint16_t ms )
{
    return setUtcTime ( TimeDesc ( time, ms ) );
}

const String Timestamp::toString() const
{
    TimeDesc tDesc;

    const bool ret = convertBinValue ( _value, tDesc );

    ( void ) ret;
    assert ( ret );

    return String ( "%1-%2-%3T%4:%5:%6.%7Z" )
           .arg ( NUM_STR ( tDesc.year, 4 ),
                  NUM_STR ( tDesc.month, 2 ),
                  NUM_STR ( tDesc.day, 2 ),
                  NUM_STR ( tDesc.hour, 2 ),
                  NUM_STR ( tDesc.minute, 2 ),
                  NUM_STR ( tDesc.second, 2 ),
                  NUM_STR ( tDesc.millisecond, 3 ) );
}

bool Timestamp::isValidDate ( int y, int m, int d )
{
    if ( y < 0 || y > 9999 || d < 1 )
    {
        // For RFC-3339, year should be <= 9999.
        return false;
    }

    switch ( m )
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return ( d <= 31 );
            break;

        case 4:
        case 6:
        case 9:
        case 11:
            return ( d <= 30 );
            break;

        case 2:
            if ( y % 400 == 0 )
            {
                return ( d <= 29 );
            }
            else if ( y % 100 == 0 )
            {
                return ( d <= 28 );
            }
            else if ( y % 4 == 0 )
            {
                return ( d <= 29 );
            }

            return ( d <= 28 );
            break;
    }

    return false;
}

bool Timestamp::isValidTime ( int h, int m, int s, int ms )
{
    // NOTE: We allow s to be '60' to allow time with the leap second to be stored (see RFC-3339).

    return ( h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 61 && ms >= 0 && ms < 1000 );
}
