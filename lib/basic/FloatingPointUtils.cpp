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

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

#define USE_NUM_LIMITS    1

#include <limits>

extern "C"
{
#include <float.h>
}

#define isnan( x )    _isnan ( x )

// _finite is false for both infinity and NaN
#define isinf( x )    ( !_isnan ( x ) && !_finite ( x ) )

#elif !defined( PLATFORM_OPENWRT )

using std::isnan;
using std::isinf;
#endif

#include "FloatingPointUtils.hpp"

#define FL_BITS_32       32
#define FL_EXPBITS_32    8

#define FL_BITS_64       64
#define FL_EXPBITS_64    11

// Special IEEE 754 values:

// positive/negative infinity:
static const uint32_t P_INF_32 = 0x7F800000U;
static const uint32_t N_INF_32 = 0xFF800000U;
static const uint64_t P_INF_64 = 0x7FF0000000000000ULL;
static const uint64_t N_INF_64 = 0xFFF0000000000000ULL;

// NaNs ignore the sign bit; They have all following bits set to 1:
static const uint32_t NAN_32 = 0x7F800000U;
static const uint64_t NAN_64 = 0x7FF0000000000000ULL;

// If this is 1 then it's a quiet NaN, otherwise it's a signalling NaN:
static const uint32_t Q_NAN_BIT_32 = 0x400000U;
static const uint64_t Q_NAN_BIT_64 = 0x8000000000000ULL;

// And for a number to be NaN (and not infinity) there have to be some 1s in this mask:
// (the S_NAN_BIT is sufficient for this)
static const uint32_t NAN_MASK_32 = 0x7FFFFFU;
static const uint64_t NAN_MASK_64 = 0xFFFFFFFFFFFFFULL;

// And, for convenience, this is what we actually sent (we don't really use signalling NaNs at this point):
static const uint32_t SET_Q_NAN_32 = ( NAN_32 | Q_NAN_BIT_32 );
static const uint64_t SET_Q_NAN_64 = ( NAN_64 | Q_NAN_BIT_64 );

using namespace Pravala;

// Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
// The code is in public domain.
static uint64_t int_pack754 ( long double f, unsigned bits, unsigned expbits )
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if ( f == 0.0 ) return 0; // get this special case out of the way

    // check sign and begin normalization
    if ( f < 0 )
    {
        sign = 1;
        fnorm = -f;
    }
    else
    {
        sign = 0;
        fnorm = f;
    }

    // get the normalized form of f and track the exponent
    shift = 0;

    while ( fnorm >= 2.0 )
    {
        fnorm /= 2.0;
        shift++;
    }

    while ( fnorm < 1.0 )
    {
        fnorm *= 2.0;
        shift--;
    }

    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = ( long long ) ( fnorm * ( ( 1LL << significandbits ) + 0.5f ) );

    // get the biased exponent
    exp = shift + ( ( 1 << ( expbits - 1 ) ) - 1 ); // shift + bias

    // return the final answer
    return ( sign << ( bits - 1 ) ) | ( exp << ( bits - expbits - 1 ) ) | significand;
}

// Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
// The code is in public domain.
static long double int_unpack754 ( uint64_t i, unsigned bits, unsigned expbits )
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if ( i == 0 ) return 0.0;

    // pull the significand
    result = ( long double ) ( i & ( ( 1LL << significandbits ) - 1 ) ); // mask
    result /= ( 1LL << significandbits ); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = ( 1 << ( expbits - 1 ) ) - 1;
    shift = ( ( i >> significandbits ) & ( ( 1LL << expbits ) - 1 ) ) - bias;

    while ( shift > 0 )
    {
        result *= 2.0;
        shift--;
    }

    while ( shift < 0 )
    {
        result /= 2.0;
        shift++;
    }

    // sign it
    result *= ( i >> ( bits - 1 ) ) & 1 ? -1.0 : 1.0;

    return result;
}

uint32_t FloatingPointUtils::pack754 ( float value )
{
    if ( value == 0 )
    {
        return 0;
    }
    else if ( isnan ( value ) )
    {
        return SET_Q_NAN_32;
    }
    else if ( isinf ( value ) )
    {
        return ( ( value > 0 ) ? P_INF_32 : N_INF_32 );
    }
    else
    {
        return ( uint32_t ) int_pack754 ( value, FL_BITS_32, FL_EXPBITS_32 );
    }
}

uint64_t FloatingPointUtils::pack754 ( double value )
{
    if ( value == 0 )
    {
        return 0;
    }
    else if ( isnan ( value ) )
    {
        return SET_Q_NAN_64;
    }
    else if ( isinf ( value ) )
    {
        return ( ( value > 0 ) ? P_INF_64 : N_INF_64 );
    }
    else
    {
        return int_pack754 ( value, FL_BITS_64, FL_EXPBITS_64 );
    }
}

float FloatingPointUtils::unpack754 ( uint32_t value )
{
    if ( value == 0 )
    {
        return 0;
    }
    else if ( value == P_INF_32 )
    {
#ifdef USE_NUM_LIMITS
        return std::numeric_limits<float>::infinity();
#else
        return ( 1.0f / 0.0f );
#endif
    }
    else if ( value == N_INF_32 )
    {
#ifdef USE_NUM_LIMITS
        return -std::numeric_limits<float>::infinity();
#else
        return ( -1.0f / 0.0f );
#endif
    }
    else if ( ( value & NAN_32 ) == NAN_32 && ( value & NAN_MASK_32 ) != 0 )
    {
        // NAN_MASK_32 test should not be needed, since NAN_32 is the same as P_INF_32,
        // but let's keep it for clarity.

#ifdef USE_NUM_LIMITS
        return std::numeric_limits<float>::quiet_NaN();
#else
        return NAN;
#endif
    }
    else
    {
        return ( float ) int_unpack754 ( value, FL_BITS_32, FL_EXPBITS_32 );
    }
}

double FloatingPointUtils::unpack754 ( uint64_t value )
{
    if ( value == 0 )
    {
        return 0;
    }
    else if ( value == P_INF_64 )
    {
#ifdef USE_NUM_LIMITS
        return std::numeric_limits<double>::infinity();
#else
        return ( 1.0 / 0.0 );
#endif
    }
    else if ( value == N_INF_64 )
    {
#ifdef USE_NUM_LIMITS
        return -std::numeric_limits<double>::infinity();
#else
        return ( -1.0 / 0.0 );
#endif
    }
    else if ( ( value & NAN_64 ) == NAN_64 && ( value & NAN_MASK_64 ) != 0 )
    {
        // NAN_MASK_64 test should not be needed, since NAN_64 is the same as P_INF_64,
        // but let's keep it for clarity.

#ifdef USE_NUM_LIMITS
        return std::numeric_limits<double>::quiet_NaN();
#else
        return NAN;
#endif
    }
    else
    {
        return int_unpack754 ( value, FL_BITS_64, FL_EXPBITS_64 );
    }
}
