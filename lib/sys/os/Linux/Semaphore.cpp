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

#include "../shared/Semaphore.cpp"

using namespace Pravala;

Semaphore::~Semaphore()
{
    if ( _semaphore != 0 )
    {
        sem_destroy ( _semaphore );
        delete _semaphore;
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

    _semaphore = new sem_t;

    if ( sem_init ( _semaphore, 0, value ) < 0 )
    {
        delete _semaphore;
        _semaphore = 0;
        return -1;
    }

    return 0;
}
