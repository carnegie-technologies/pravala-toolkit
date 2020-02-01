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

#include "DataCounter.hpp"

#include <cassert>

using namespace Pravala;

void DataCounter::addData ( uint32_t size )
{
    if ( size <= 0 )
        return;

    uint32_t oldSize = _dataCounter;

    _dataCounter += size;

    if ( _dataCounter < oldSize )
        ++_overflowCounter;
}

int32_t DataCounter::getDiff ( const DataCounter & other ) const
{
    if ( _overflowCounter < other._overflowCounter )
    {
        // We surely can't express the difference in 'int'
        // if the overflow counters differ by more than 1
        if ( _overflowCounter + 1 < other._overflowCounter )
            return 0x7FFFFFFF;

        // Now the other overflow should be greater by one
        assert ( _overflowCounter + 1 == other._overflowCounter );

        // So the difference between the data counters is
        // the sum of other data counter, and whatever
        // is missing between our data counter and
        // the end of the range (which would increment our overflow
        // counter).

        // If the other data counter is greater than int's
        // max value we can't express it
        if ( other._dataCounter >= 0x7FFFFFFF )
            return 0x7FFFFFFF;

        // This is the difference between out data counter and the end of the range
        uint32_t extra = 0xFFFFFFFF - _dataCounter + 1;

        // If this difference is bigger than int's max value,
        // we can't express it
        if ( extra >= 0x7FFFFFFF )
            return 0x7FFFFFFF;

        // When we subtract the other data counter from int's max value
        // we get "the space" which can be used for aditional data.
        // If this is smaller than 'extra', we can't add that extra
        // without going over int's range.
        if ( 0x7FFFFFFF - other._dataCounter < extra )
            return 0x7FFFFFFF;

        // To make sure that we can fit in int
        assert ( extra + other._dataCounter <= 0x7FFFFFFF );

        return extra + other._dataCounter;
    }

    if ( _overflowCounter == other._overflowCounter
         && _dataCounter <= other._dataCounter )
    {
        assert ( _dataCounter <= other._dataCounter );

        // We can't express the difference in 'int'
        if ( other._dataCounter - _dataCounter >= 0x7FFFFFFF )
            return 0x7FFFFFFF;

        return other._dataCounter - _dataCounter;
    }

    assert ( _overflowCounter > other._overflowCounter
             || ( _overflowCounter == other._overflowCounter
                  && _dataCounter > other._dataCounter ) );

    // "this" counter is bigger Let's just use getDiff
    // in the 'other' counter and return the negative value.
    // We are "loosing" one negative value (0xFFFFFFFF),
    // but it's easier this way ;)

    return -other.getDiff ( *this );
}

int32_t DataCounter::getDiffAndSet ( const DataCounter & other )
{
    int32_t ret = getDiff ( other );

    _dataCounter = other._dataCounter;
    _overflowCounter = other._overflowCounter;

    return ret;
}
