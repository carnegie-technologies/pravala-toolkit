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

#include "basic/MemHandle.hpp"

namespace Pravala
{
namespace Base64
{
/// @brief Encode some data into base64
/// @param [in] s Pointer to data to encode
/// @param [in] len Length of data to encode
/// @param [in] addLineBreaks True to add newlines every 72 characters and at the end of the last line
/// e.g. for MIME encoding.
/// @return MemHandle containing base64 encoded data
/// If this MemHandle is empty, either len was 0 or there was insufficient memory.
MemHandle encode ( const char * s, size_t len, bool addLineBreaks = false );

/// @brief Encode some data into base64
/// @param [in] mh MemHandle containing data to encode
/// @param [in] addLineBreaks True to add newlines every 72 characters and at the end of the last line
/// e.g. for MIME encoding.
/// @return MemHandle containing base64 encoded data
/// If this MemHandle is empty, either len was 0 or there was insufficient memory.
inline MemHandle encode ( const MemHandle & mh, bool addLineBreaks = false )
{
    return encode ( mh.get(), mh.size(), addLineBreaks );
}

/// @brief Decode some data from base64
/// @param [in] s Pointer to data to decode
/// @param [in] len Length of data to decode
/// @return MemHandle containing decoded data
/// If this MemHandle is empty, either len was 0 or there was insufficient memory.
MemHandle decode ( const char * s, size_t len );

/// @brief Decode some data from base64
/// @param [in] mh MemHandle containing data to decode
/// @return MemHandle containing decoded data
/// If this MemHandle is empty, either len was 0 or there was insufficient memory.
inline MemHandle decode ( const MemHandle & mh )
{
    return decode ( mh.get(), mh.size() );
}

/// @brief Decode some data from base64
/// @param [in] s String containing data to decode
/// @return MemHandle containing decoded data
/// If this MemHandle is empty, either len was 0 or there was insufficient memory.
inline MemHandle decode ( const String & s )
{
    return decode ( s.c_str(), s.length() );
}
}
}
