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
#include <unistd.h>
#include <pthread.h>
}

#include <cstdio>
#include <cstring>
#include <cerrno>

#define LOG_DEBUG( FORMAT_STRING, ... )

#ifdef MUTEX_DEBUGGING

#undef LOG_DEBUG

#ifdef PLATFORM_ANDROID

#include <android/log.h>

// __android_log_print already appends a newline
#define LOG_DEBUG( FORMAT_STRING, ... ) \
    __android_log_print ( ANDROID_LOG_DEBUG, "Mutex", "%6s:%03d %06lu:%012lx 0x%012lx (%s): " FORMAT_STRING, \
                          __FUNCTION__, __LINE__, ( unsigned long ) getpid(), ( unsigned long ) pthread_self(), \
                          ( ( unsigned long ) _mutexPtr ), _name, ## __VA_ARGS__ )

#else

#define LOG_DEBUG( FORMAT_STRING, ... ) \
    fprintf ( stderr, "Mutex: %6s:%03d %06lu:%012lx 0x%012lx (%s): " FORMAT_STRING "\n", \
              __FUNCTION__, __LINE__, ( unsigned long ) getpid(), ( unsigned long ) pthread_self(), \
              ( ( unsigned long ) _mutexPtr ), _name, ## __VA_ARGS__ )
#endif
#endif

#include "../../Mutex.hpp"

using namespace Pravala;

Mutex::Mutex ( const char * name, bool fastMode ): _mutexPtr ( new pthread_mutex_t )
{
#ifdef MUTEX_DEBUGGING
    const int len = ( name != 0 ) ? strlen ( name ) : 0;

    _name = new char[ len + 1 ];

    if ( name != 0 )
    {
        strncpy ( _name, name, len );
    }

    _name[ len ] = 0;
#else
    ( void ) name;
#endif

    pthread_mutexattr_t mAttr;

    LOG_DEBUG ( "%s", "Initializing attributes" );

    int ret = pthread_mutexattr_init ( &mAttr );

    LOG_DEBUG ( "%sInitialized attributes: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

    if ( ret != 0 )
        fprintf ( stderr, "Error calling pthread_mutexattr_init; Error: %d\n", ret );

    if ( fastMode )
    {
        LOG_DEBUG ( "%s", "Setting 'default' attribute" );

        ret = pthread_mutexattr_settype ( &mAttr, PTHREAD_MUTEX_DEFAULT );

        LOG_DEBUG ( "%sSet 'default' attribute: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, errno );

        if ( ret != 0 )
            fprintf ( stderr, "Error calling pthread_mutexattr_settype(default); Error: %d\n", ret );
    }
    else
    {
        LOG_DEBUG ( "%s", "Setting 'recursive' attribute" );

        ret = pthread_mutexattr_settype ( &mAttr, PTHREAD_MUTEX_RECURSIVE );

        LOG_DEBUG ( "%sSet 'recursive' attribute: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

        if ( ret != 0 )
            fprintf ( stderr, "Error calling pthread_mutexattr_settype(recursive); Error: %d\n", ret );
    }

    LOG_DEBUG ( "%s", "Initializing" );

    ret = pthread_mutex_init ( ( pthread_mutex_t * ) _mutexPtr, &mAttr );

    LOG_DEBUG ( "%sInitialized: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

    pthread_mutexattr_destroy ( &mAttr );

    if ( ret != 0 )
    {
        fprintf ( stderr, "Error creating a mutex; Error: %d\n", ret );

        delete ( ( pthread_mutex_t * ) _mutexPtr );
        _mutexPtr = 0;
    }
}

Mutex::~Mutex()
{
    if ( _mutexPtr != 0 )
    {
        LOG_DEBUG ( "%s", "Destroying" );

        const int ret = pthread_mutex_destroy ( ( pthread_mutex_t * ) _mutexPtr );

        LOG_DEBUG ( "%sDestroyed: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

        if ( ret != 0 )
            fprintf ( stderr, "Error destroying a mutex; Error: %s [%d]\n", strerror ( ret ), ret );

        delete ( ( pthread_mutex_t * ) _mutexPtr );
        _mutexPtr = 0;
    }

#ifdef MUTEX_DEBUGGING
    delete[] _name;
    _name = 0;
#endif
}

bool Mutex::lock()
{
    if ( !_mutexPtr )
    {
        fprintf ( stderr, "Trying to lock uninitialized mutex; Ignoring!\n" );
        return false;
    }

    LOG_DEBUG ( "%s", "Locking" );

    const int ret = pthread_mutex_lock ( ( pthread_mutex_t * ) _mutexPtr );

    LOG_DEBUG ( "%sLocked: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

    if ( ret == 0 )
    {
        // Success
        return true;
    }
    else
    {
        // Some error
        fprintf ( stderr, "Error locking a mutex; Error: %d\n", ret );
        return false;
    }
}

bool Mutex::tryLock()
{
    if ( !_mutexPtr )
    {
        fprintf ( stderr, "Trying to lock uninitialized mutex; Ignoring!\n" );
        return false;
    }

    LOG_DEBUG ( "%s", "Try-Locking" );

    const int ret = pthread_mutex_trylock ( ( pthread_mutex_t * ) _mutexPtr );

    LOG_DEBUG ( "%sTry-Locked: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

    if ( ret == 0 )
    {
        // Success
        return true;
    }
    else if ( ret == EBUSY )
    {
        // Already locked
        return false;
    }
    else
    {
        // Some (other) error
        fprintf ( stderr, "Error locking a mutex; Error: %d\n", ret );
        return false;
    }
}

bool Mutex::unlock()
{
    if ( !_mutexPtr )
    {
        fprintf ( stderr, "Trying to unlock uninitialized mutex; Ignoring!\n" );
        return false;
    }

    LOG_DEBUG ( "%s", "Unlocking" );

    const int ret = pthread_mutex_unlock ( ( pthread_mutex_t * ) _mutexPtr );

    LOG_DEBUG ( "%sUnlocked: %d (%d)", ( ret == 0 ) ? "" : "NOT ", ret, ( ret != 0 ) ? errno : 0 );

    if ( ret == 0 )
        return true;

    fprintf ( stderr, "Error unlocking a mutex; Error: %d\n", ret );
    return false;
}
