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

#include "basic/MsvcSupport.hpp"

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )

extern "C"
{
#include <io.h>
}

#ifndef STDIN_FILENO
#define STDIN_FILENO     0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO    1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO    2
#endif

#ifndef S_IWUSR
#define S_IWUSR          0x0080
#endif

#ifndef S_IRUSR
#define S_IRUSR          0x0100
#endif

#else

extern "C"
{
#include <unistd.h>
}
#endif
