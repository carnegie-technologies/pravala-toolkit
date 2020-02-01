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

#include "basic/Timestamp.hpp"
#include "Date.hpp"

using namespace Pravala;

Date::Date(): _year ( 0 ), _month ( 0 ), _day ( 0 )
{
}

Date::Date ( const struct tm & cal ):
    _year ( 1900 + cal.tm_year ), _month ( 1 + cal.tm_mon ), _day ( cal.tm_mday )
{
}

bool Date::operator< ( const Date & other ) const
{
    if ( _year < other._year )
        return true;

    if ( _year > other._year )
        return false;

    // years are the same

    if ( _month < other._month )
        return true;

    if ( _month > other._month )
        return false;

    // months are the same

    return ( _day < other._day );
}

bool Date::operator> ( const Date & other ) const
{
    if ( _year > other._year )
        return true;

    if ( _year < other._year )
        return false;

    // years are the same

    if ( _month > other._month )
        return true;

    if ( _month < other._month )
        return false;

    // months are the same

    return ( _day > other._day );
}

bool Date::set ( const String & strDate )
{
    const StringList strList = strDate.split ( "-", true );

    if ( strList.size() != 3 )
        return false;

    uint16_t y = 0;
    uint8_t m = 0;
    uint8_t d = 0;

    // We have to force the base to 10, since the numbers can start with 0.
    // Automatic base would treat them as octal numbers.

    if ( strList.at ( 0 ).toNumber ( y, 10 )
         && strList.at ( 1 ).toNumber ( m, 10 )
         && strList.at ( 2 ).toNumber ( d, 10 )
         && Timestamp::isValidDate ( y, m, d ) )
    {
        _year = y;
        _month = m;
        _day = d;

        return true;
    }

    return false;
}
