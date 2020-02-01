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

namespace Pravala
{
/// @brief A wrapper around libc's rand() and srand() functions.
/// It should be used instead of calling rand/srand directly to make sure that the random number generator
/// is initialized properly.
class Random
{
    public:
        /// @brief Initializes random number generator (using srand()).
        /// It only does anything the first time the generator is initialized.
        /// @note It uses time() as the seed, so it may not work as expected if the application is restarted too often.
        static void init();

        /// @brief Initializes random number generator using the given value.
        /// It only does anything the first time the generator is initialized.
        /// @param [in] seed The seed value to initialize the random number generator with.
        static void init ( unsigned int seed );

        /// @brief Returns a pseudo-random integer in the range 0 to RAND_MAX inclusive.
        /// It also calls init() if it has never been called before.
        /// @return A pseudo-random integer in the range 0 to RAND_MAX inclusive.
        static int rand();

        /// @brief Returns a pseudo-random integer in the range 0 to limit-1 inclusive.
        /// It also calls init() if it has never been called before.
        /// @note This function doesn't introduce a "modulo bias" of simple 'rand() % limit' operation.
        /// @param [in] limit The limit for the values generated. If <= 1, this function always returns 0.
        /// @return A pseudo-random integer in the range 0 to limit-1 inclusive.
        static int rand ( int limit );
};
}
