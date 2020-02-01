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
#include "RttStat.hpp"

// Calculations use alpha (that should be 1/8) and beta (that should be 1/4).
// These define the denominators:
#define ALPHA_DENOM    8
#define BETA_DENOM     4

using namespace Pravala;

RttStat::RttStat(): _sRtt ( 0 ), _rttVar ( 0 ), _minRtt ( 0 )
{
}

void RttStat::clear()
{
    _sRtt = _rttVar = _minRtt = 0;
}

void RttStat::addRtt ( uint32_t r )
{
    // First measurement:
    // SRTT = R
    // RTTVAR = R/2
    //
    // After that (in that order):
    // RTTVAR = (1 - beta) * RTTVAR + beta * |SRTT - R|
    // SRTT   = (1 - alpha) * SRTT + alpha * R

    if ( r < 1 )
    {
        r = 1;
    }

    if ( _minRtt < 1 )
    {
        _sRtt = _minRtt = r;
        _rttVar = r / 2;
        return;
    }

    if ( r < _minRtt )
    {
        _minRtt = r;
    }

    // Instead of multiplying by 1/8 or (1 - 1/8) we divide by 8, or multiply by 7 and then divide by 8.
    // Also, we don't let sRTT to drop below MinRTT (it is possible due to rounding errors if RTTs are small).

    _rttVar = ( BETA_DENOM - 1 ) * _rttVar / BETA_DENOM + abs_diff<uint32_t> ( r, _sRtt ) / BETA_DENOM;
    _sRtt = max ( _minRtt, ( ALPHA_DENOM - 1 ) * _sRtt / ALPHA_DENOM + r / ALPHA_DENOM );
}

uint32_t RttStat::getRto() const
{
    // RTO = SRTT + max (G, K*RTTVAR).
    // G is clock granularity, which we ignore here, and K is 4.
    // Also, RTO should be at least half a second second (500 ms) - this is more aggressive than 1 second
    // used by RFC-6298.
    // We use more aggressive values because this RTO is used at the link level, not per flow.
    // If those flows were regular TCP flows, each one of them would do its own timeouts.
    // However, we do this at the link level and for potentially multiple flows,
    // so we want to be more aggressive at competing with other TCP traffic that may be present.

    return max<uint32_t> ( _sRtt + 4 * _rttVar, 500 );
}
