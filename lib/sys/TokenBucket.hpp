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

#include "Time.hpp"

namespace Pravala
{
/// @brief Implements a TokenBucket algorithm to allow throughput rate limiting.
class TokenBucket
{
    public:
        /// @brief Default constructor.
        /// Creates a bucket that is disabled (accepts all requests).
        /// @param [in] currentTime The time object to get current times from.
        TokenBucket ( const Time & currentTime );

        /// @brief Disables the bucket. All requests will succeed.
        void disable();

        /// @brief Enables (and fills) the bucket.
        /// @param [in] tokenRate The number of new tokens to be added per second.
        /// @param [in] maxTokens The max number of tokens to be stored in the bucket.
        void enable ( uint32_t tokenRate, uint32_t maxTokens );

        /// @brief Uses a number of tokens.
        /// @param [in] tokens The number of tokens requested.
        /// @return True if there were enough tokens available (or the bucket was disabled);
        ///         False if there were not enough tokens available.
        bool useTokens ( uint32_t tokens );

        /// @brief Get the number of tokens available.
        /// @note This returns max UINT32 if the token bucket is disabled.
        /// @return The number of tokens available.
        uint32_t getAvailableTokens();

        /// @brief Reduces the number of tokens in the bucket by the specified amount.
        /// This does not add more if time has elapsed since tokens were last added.
        /// @param [in] tokens The number of tokens to consume.
        inline void consumeTokens ( uint32_t tokens )
        {
            _tokens = ( _tokens > tokens ) ? ( _tokens - tokens ) : 0;
        }

    private:
        const Time & _currentTime; ///< The current time.
        Time _lastAdded; ///< The last time tokens were added.
        double _tokenRate; ///< The number of tokens to be added each millisecond.

        uint32_t _tokens; ///< The number of available tokens.
        uint32_t _maxTokens; ///< The max number of tokens this bucket can hold.
        bool _enabled; ///< Whether this bucket is enabled.
};
}
