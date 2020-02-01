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

#include <cstdlib>
#include <ctime>

#include "Math.hpp"
#include "Random.hpp"

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
#include "basic/MsvcSupport.hpp"
#else
extern "C"
{
#include <unistd.h>
}
#endif

using namespace Pravala;

static bool isInitialized = false;

void Random::init()
{
    unsigned p = getpid();

    // Programs started close to each other will likely have similar PIDs, the difference will be at the end.
    // This makes it grow similar to the number of seconds returned by time().
    // We don't want them to cancel each other out, so let's reverse the bytes in the PID:
    p = ( p << 24 ) | ( ( p & 0xFF00 ) << 8 ) | ( ( p & 0xFF0000 ) >> 8 ) | ( p >> 24 );

    init ( ( unsigned int ) ( time ( 0 ) ^ p ) );
}

void Random::init ( unsigned int seed )
{
    if ( !isInitialized )
    {
        ::srand ( seed );
        isInitialized = true;
    }
}

int Random::rand()
{
    if ( !isInitialized )
    {
        init();
    }

    return ::rand();
}

int Random::rand ( int limit )
{
    // Random values from given limit [0;limit).
    //
    // The naive approach is to return rand() % limit.
    //
    // The problem is that it introduces a "modulo bias" - if rand's range is not evenly divisible by the limit.
    //
    // The straightforward solution is to keep picking a value, until it is smaller than the limit.
    // But if the limit is low, it could take a long time.
    //
    // So instead of using the original limit we could choose another, large, value that is divisible by limit,
    // and find a result that is less than that. Then modulo can be used safely.
    //
    // On the other hand, if the limit is large, the new limit constructed that way could be small and result
    // in bad performance. So here we use one or the other, depending on the value of limit.

    if ( !isInitialized )
    {
        init();
    }

    if ( limit <= 1 )
    {
        return 0;
    }

    // tmpLimit will be divisible by limit
    const int tmpLimit = ( RAND_MAX - ( RAND_MAX % limit ) );
    int rnd;

    if ( limit >= tmpLimit )
    {
        // limit is large, so we use first (straightforward) method:

        do
        {
            rnd = ::rand();
        }
        while ( rnd >= limit );
    }
    else
    {
        // limit is small, so we use second method:

        do
        {
            rnd = ::rand();
        }
        while ( rnd >= tmpLimit );

        rnd %= limit;
    }

    return rnd;
}
