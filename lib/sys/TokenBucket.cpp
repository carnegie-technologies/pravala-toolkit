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

#include "basic/Math.hpp"

#include "TokenBucket.hpp"

using namespace Pravala;

TokenBucket::TokenBucket ( const Time & currentTime ):
    _currentTime ( currentTime ),
    _tokenRate ( 0 ),
    _tokens ( 0 ),
    _maxTokens ( 0 ),
    _enabled ( false )
{
    disable();
}

void TokenBucket::disable()
{
    _enabled = false;
}

void TokenBucket::enable ( uint32_t tokenRate, uint32_t maxTokens )
{
    _enabled = true;
    _lastAdded = _currentTime;

    _maxTokens = _tokens = maxTokens;

    // Our internal rate is in tokens per one millisecond.
    _tokenRate = ( ( double ) tokenRate ) / 1000.0;
}

bool TokenBucket::useTokens ( uint32_t tokens )
{
    if ( !_enabled )
        return true;

    if ( getAvailableTokens() < tokens )
    {
        return false;
    }

    assert ( tokens <= _tokens );

    _tokens -= tokens;
    return true;
}

uint32_t TokenBucket::getAvailableTokens()
{
    if ( !_enabled )
        return 0xFFFFFFFFUL;

    const long diff = _currentTime.getDiffInMilliSeconds ( _lastAdded );

    if ( diff > 0 )
    {
        const double newTokens = ( _tokenRate * diff ) + ( double ) _tokens;

        _tokens = ( uint32_t ) ( ( newTokens > _maxTokens ) ? _maxTokens : newTokens );
        _lastAdded = _currentTime;
    }

    return _tokens;
}
