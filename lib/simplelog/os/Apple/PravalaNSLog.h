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

#ifdef __cplusplus
extern "C" {
#endif

/// @brief C-wrapper around the Obj-C standard logging function: NSLog.
/// @param [in] format Standard printf-based format string.
/// @param [in] ... Arguments to insert into the format string.
///
/// The attribute here says that our function takes printf style arguments that should be type-checked
/// against a format string. See syntax here:
/// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
__attribute__( ( __format__ ( __printf__, 1, 2 ) ) )
void Pravala_NSLog ( const char * format, ... );

#ifdef __cplusplus
}
#endif
