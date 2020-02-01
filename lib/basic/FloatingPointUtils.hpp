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

#include <cassert>
#include <cstddef>

extern "C"
{
#include <stdint.h>
}

namespace Pravala
{
/// @brief Contains functions for manipulating floating point values.
class FloatingPointUtils
{
    public:
        /// @brief Packs a float value into uint32_t according to IEEE 754
        /// It handles special values (zero, NaN, and +/- infinity)
        /// @param [in] value The float value to pack.
        /// @return Packed value as uint32_t
        static uint32_t pack754 ( float value );

        /// @brief Packs a double value into uint64_t according to IEEE 754
        /// It handles special values (zero, NaN, and +/- infinity)
        /// @param [in] value The double value to pack.
        /// @return Packed value as uint64_t
        static uint64_t pack754 ( double value );

        /// @brief Unpacks a float value from uint32_t according to IEEE 754
        /// It handles special values (zero, NaN, and +/- infinity)
        /// @param [in] value The uint32_t value to unpack.
        /// @return Unpacked float.
        static float unpack754 ( uint32_t value );

        /// @brief Unpacks a double value from uint64_t according to IEEE 754
        /// It handles special values (zero, NaN, and +/- infinity)
        /// @param [in] value The uint64_t value to unpack.
        /// @return Unpacked double.
        static double unpack754 ( uint64_t value );
};
}
