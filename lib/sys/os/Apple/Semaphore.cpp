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

extern "C"
{
// On OSX sem_timedwait is not implemented by the system libraries, so use a custom implementation. See sem_timedwait.c
#include "sem_timedwait.h"
}

#include <cstdio>

#include "../shared/Semaphore.cpp"

using namespace Pravala;

Semaphore::~Semaphore()
{
    if ( _semaphore != 0 )
    {
        // sem_close handles deletion.
        sem_close ( _semaphore );
        _semaphore = 0;
    }
}

int Semaphore::init ( unsigned int value )
{
    if ( _semaphore != 0 )
    {
        errno = EINVAL;
        return -1;
    }

    _semaphore = sem_open ( _name.c_str(), O_CREAT | O_EXCL, 0600, value );

    if ( _semaphore == SEM_FAILED )
    {
        _semaphore = 0;
        return -1;
    }

    // Since the semaphore isn't shared between processes, the name isn't useful and we can remove it.
    // The semaphore itself will not be removed until sem_close() is called.
    if ( sem_unlink ( _name.c_str() ) < 0 )
    {
        fprintf ( stderr, "Error unlinking semaphore: %s\n", strerror ( errno ) );

        sem_close ( _semaphore );
        _semaphore = 0;
        return -1;
    }

    return 0;
}
