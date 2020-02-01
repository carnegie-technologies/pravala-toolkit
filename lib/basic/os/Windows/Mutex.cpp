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

#include <cstdio>

#include "../../Mutex.hpp"

extern "C"
{
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
}

using namespace Pravala;

Mutex::Mutex ( const char *, bool ): _mutexPtr ( 0 )
{
    _mutexPtr = ( HANDLE ) CreateMutex (
        0,                      // default security attributes
        FALSE,                  // initially not owned
        0 );                    // unnamed mutex

    if ( !_mutexPtr )
        fprintf ( stderr, "Error creating a mutex; Error: %u\n", GetLastError() );
}

Mutex::~Mutex()
{
    if ( _mutexPtr != 0 )
    {
        CloseHandle ( ( HANDLE ) _mutexPtr );
        _mutexPtr = 0;
    }
}

bool Mutex::lock()
{
    const uint32_t waitRes = WaitForSingleObject (
        ( HANDLE ) _mutexPtr,                             // handle to mutex
        INFINITE );                          // no time-out interval

    if ( waitRes == WAIT_OBJECT_0 )
    {
        // Success
        return true;
    }
    else
    {
        // Some error
        fprintf ( stderr, "Error waiting for a mutex; Result: %u\n", waitRes );
        return false;
    }
}

bool Mutex::tryLock()
{
    const uint32_t waitRes = WaitForSingleObject (
        ( HANDLE ) _mutexPtr,                             // handle to mutex
        0 );                          // 0 timeout, return right away if it is already locked

    if ( waitRes == WAIT_OBJECT_0 )
    {
        // Success
        return true;
    }
    else if ( waitRes == WAIT_TIMEOUT )
    {
        // Already locked
        return false;
    }
    else
    {
        // Some (other) error
        fprintf ( stderr, "Error waiting for a mutex; Result: %u\n", waitRes );
        return false;
    }
}

bool Mutex::unlock()
{
    if ( ReleaseMutex ( ( HANDLE ) _mutexPtr ) )
        return true;

    fprintf ( stderr, "Error releasing a mutex; Error: %u\n", GetLastError() );

    return false;
}
