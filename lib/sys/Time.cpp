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

#include <cassert>
#include <cstring>

#include "Time.hpp"

using namespace Pravala;

Time::Time()
{
    clear();
}

Time::Time ( uint32_t s, uint32_t ms )
{
    _myTime.sec = s + ms / ONE_SEC_IN_MSEC;
    _myTime.msec = ms % ONE_SEC_IN_MSEC;
}

Time::Time ( const Time & other ): _myTime ( other._myTime )
{
}

void Time::clear()
{
    memset ( &_myTime, 0, sizeof ( _myTime ) );
}

Time & Time::operator= ( const Time & other )
{
    if ( &other == this ) return *this;

    _myTime = other._myTime;

    return *this;
}

bool Time::isGreaterThan ( const Time & other, uint32_t moreThanSeconds ) const
{
    assert ( _myTime.msec < ONE_SEC_IN_MSEC );
    assert ( other._myTime.msec < ONE_SEC_IN_MSEC );

    const uint32_t tmpSec = other._myTime.sec + moreThanSeconds;

    if ( _myTime.sec > tmpSec ) return true;

    if ( _myTime.sec < tmpSec ) return false;

    assert ( _myTime.sec == tmpSec );

    return ( _myTime.msec > other._myTime.msec );
}

bool Time::isGreaterEqualThan ( const Time & other, uint32_t moreThanSeconds ) const
{
    assert ( _myTime.msec < ONE_SEC_IN_MSEC );
    assert ( other._myTime.msec < ONE_SEC_IN_MSEC );

    const uint32_t tmpSec = other._myTime.sec + moreThanSeconds;

    if ( _myTime.sec > tmpSec ) return true;

    if ( _myTime.sec < tmpSec ) return false;

    assert ( _myTime.sec == tmpSec );

    return ( _myTime.msec >= other._myTime.msec );
}

bool Time::isGreaterThanMilliseconds ( const Time & other, uint32_t moreThanMilliseconds ) const
{
    Time tmpTime ( other );

    tmpTime.increaseMilliseconds ( moreThanMilliseconds );

    return operator> ( tmpTime );
}

bool Time::isGreaterEqualThanMilliseconds ( const Time & other, uint32_t moreThanMilliseconds ) const
{
    Time tmpTime ( other );

    tmpTime.increaseMilliseconds ( moreThanMilliseconds );

    return operator>= ( tmpTime );
}

bool Time::operator< ( const Time & other ) const
{
    if ( _myTime.sec < other._myTime.sec ) return true;

    if ( _myTime.sec > other._myTime.sec ) return false;

    assert ( _myTime.sec == other._myTime.sec );

    return ( _myTime.msec < other._myTime.msec );
}

bool Time::operator<= ( const Time & other ) const
{
    if ( _myTime.sec < other._myTime.sec ) return true;

    if ( _myTime.sec > other._myTime.sec ) return false;

    assert ( _myTime.sec == other._myTime.sec );

    return ( _myTime.msec <= other._myTime.msec );
}

bool Time::operator> ( const Time & other ) const
{
    if ( _myTime.sec > other._myTime.sec ) return true;

    if ( _myTime.sec < other._myTime.sec ) return false;

    assert ( _myTime.sec == other._myTime.sec );

    return ( _myTime.msec > other._myTime.msec );
}

bool Time::operator>= ( const Time & other ) const
{
    if ( _myTime.sec > other._myTime.sec ) return true;

    if ( _myTime.sec < other._myTime.sec ) return false;

    assert ( _myTime.sec == other._myTime.sec );

    return ( _myTime.msec >= other._myTime.msec );
}

bool Time::decreaseSeconds ( uint32_t s )
{
    if ( _myTime.sec >= s )
    {
        _myTime.sec -= s;
        return true;
    }
    else
    {
        clear();
        return false;
    }
}

bool Time::decreaseMilliseconds ( uint32_t ms )
{
    uint64_t orgMs = ( ( uint64_t ) _myTime.sec ) * ONE_SEC_IN_MSEC + _myTime.msec;

    if ( orgMs < ms )
    {
        _myTime.sec = 0;
        _myTime.msec = 0;
        return false;
    }

    orgMs -= ms;

    _myTime.sec = orgMs / ONE_SEC_IN_MSEC;
    _myTime.msec = orgMs % ONE_SEC_IN_MSEC;

    return true;
}
