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

#define THREAD_LOCAL    __declspec ( thread )

#elif defined( PLATFORM_ANDROID ) || defined( PLATFORM_IOS )

// Thread-local support using __thread is broken on Android...
// and not supported on iOS.
#define THREAD_LOCAL

#else

#define THREAD_LOCAL    __thread
#endif
