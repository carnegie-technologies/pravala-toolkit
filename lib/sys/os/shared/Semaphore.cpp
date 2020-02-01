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

#include <cerrno>

extern "C"
{
#include <unistd.h>
}

#include "../../CalendarTime.hpp"
#include "../../Semaphore.hpp"

using namespace Pravala;

Semaphore::Semaphore ( const char * name ):
    _semaphore ( 0 ),
    _name ( String ( "%1_%2" ).arg ( name ).arg ( getpid() ) )
{
}

int Semaphore::wait()
{
    if ( !_semaphore )
    {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;

    do
    {
        ret = sem_wait ( _semaphore );
    }
    while ( ret != 0 && errno == EINTR );

    return ret;
}

int Semaphore::post()
{
    if ( !_semaphore )
    {
        errno = EINVAL;
        return -1;
    }

    return sem_post ( _semaphore );
}

int Semaphore::timedWait ( uint32_t timeout )
{
    if ( !_semaphore )
    {
        errno = EINVAL;
        return -1;
    }

    struct timespec ts;

    const uint64_t expireTimeMs = CalendarTime::getUTCEpochTimeMs() + timeout;

    ts.tv_sec = expireTimeMs / 1000;
    ts.tv_nsec = ( expireTimeMs % 1000 ) * 1000000;

    int ret = 0;

    do
    {
        ret = sem_timedwait ( _semaphore, &ts );
    }
    while ( ret != 0 && errno == EINTR );

    return ret;
}
