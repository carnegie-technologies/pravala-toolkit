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

#pragma once

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

#include <cstdlib>

typedef int ssize_t;
typedef int pid_t;

#define snprintf    _snprintf
#define strtoll     _strtoi64
#define strtoull    _strtoui64
#define getpid      _getpid

inline float strtof ( const char * str, char ** ptr )
{
    return ( float ) strtod ( str, ptr );
}
#endif
