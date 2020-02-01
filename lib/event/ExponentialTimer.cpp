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
#include "ExponentialTimer.hpp"

using namespace Pravala;

ExponentialTimer::ExponentialTimer (
        Timer::Receiver & receiver, uint32_t startingInterval, double backoffMultiplier,
        uint32_t maxInterval, bool useTimerTime ):
    Timer ( receiver ),
    BackoffMultiplier ( max ( 1.0, backoffMultiplier ) ),
    StartingInterval ( startingInterval ),
    MaxInterval ( max ( startingInterval, maxInterval ) ),
    UseTimerTime ( useTimerTime ),
    _nextInterval ( StartingInterval )
{
}

uint32_t ExponentialTimer::start()
{
    const uint32_t curInterval = _nextInterval;

    _nextInterval = min<uint32_t> ( ( uint32_t ) ( ( ( double ) curInterval ) * BackoffMultiplier ), MaxInterval );

    Timer::start ( curInterval, UseTimerTime );

    return curInterval;
}

void ExponentialTimer::stop()
{
    _nextInterval = StartingInterval;

    Timer::stop();
}
