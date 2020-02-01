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

//
// This, if defined, disables object pools.
// Useful for tracking memory leaks (when something generates new pooled object
// and doesn't unreference it properly).
// When pools are used, valgrind reports those leaks with original allocation
// (so before potentially several get-from-pool return-to-pool cycles,
// which are all over the place and usually useless
//
// #define OBJECT_POOL_SKIP_POOL 1
