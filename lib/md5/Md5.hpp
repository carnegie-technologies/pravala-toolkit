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

#include "basic/String.hpp"

namespace Pravala
{
class MemHandle;
class Buffer;

/// @brief Simple class for generating MD5 sums
class Md5
{
    public:
        /// @brief Constructor
        Md5();

        /// @brief Resets the object to the initial state, so a new MD5 calculation can be performed
        void reset();

        /// @brief Update context with additional data
        /// Can be used several times
        /// @param [in] data Data to append
        /// @param [in] size Size of the data to append
        /// @return True if the data was appended; False if this object has already been finalized
        bool update ( const char * data, size_t size );

        /// @brief Update context with additional data
        /// Can be used several times
        /// @param [in] str Null-terminated string to append. The final NULL is not appended.
        /// @return True if the data was appended; False if this object has already been finalized
        bool update ( const char * str );

        /// @brief Update context with additional data
        /// Can be used several times
        /// @param [in] str String to append. The final NULL is not appended.
        /// @return True if the data was appended; False if this object has already been finalized
        bool update ( const String & str );

        /// @brief Update context with additional data
        /// Can be used several times
        /// @param [in] buf The buffer to append
        /// @return True if the data was appended; False if this object has already been finalized
        bool update ( const Buffer & buf );

        /// @brief Update context with additional data
        /// Can be used several times
        /// @param [in] mem Memory handle to append
        /// @return True if the data was appended; False if this object has already been finalized
        bool update ( const MemHandle & mem );

        /// @brief Reads the MD5 sum in binary form
        /// It can be used several times (returning the same result). If required it finalizes the object
        /// @param [out] result The MD5 sum is placed in this buffer
        void getResult ( unsigned char result[ 16 ] );

        /// @brief Reads the MD5 sum in string (hex) format
        /// It can be used several times (returning the same result). If required it finalizes the object
        /// @param [in] lowerCase Whether the generated string should use lower case for hex codes
        /// @return The MD5 sum in string (hex) format
        String getResult ( bool lowerCase = true );

        /// @brief Helper function to easily read MD5 sum of a single memory segment
        /// @param [in] data Data to process
        /// @param [in] size Size of the data
        /// @param [in] lowerCase Whether the generated string should use lower case for hex codes
        /// @return The MD5 sum in string (hex) format
        static String generate ( const char * data, size_t size, bool lowerCase = true );

        /// @brief Helper function to easily read MD5 sum of a single variable.
        /// The variable type has to be supported by one of update() functions.
        /// @param [in] fromVar The variable to use
        /// @param [in] lowerCase Whether the generated string should use lower case for hex codes
        /// @return The MD5 sum in string (hex) format
        template<typename T> static inline String generate ( const T & fromVar, bool lowerCase = true )
        {
            Md5 md5;

            if ( !md5.update ( fromVar ) )
                return String::EmptyString;

            return md5.getResult ( lowerCase );
        }

    private:
        bool _isFinalized; ///< Flag for determining whether this Md5 object has already been finalized

        /// @brief MD5 context
        struct Context
        {
            uint32_t a, b, c, d; ///< Values used by the MD5 algorithm
            uint32_t lo, hi; ///< Values used by the MD5 algorithm
            uint32_t block[ 16 ]; ///< A buffer used during calculations
            uint8_t buffer[ 64 ]; ///< Pending data (when blocks are not completely full)

            /// @brief constructor
            Context();

            /// @brief Resets context to the initial state
            void reset();
        } _ctx; ///< The context object

        /// @brief Processes one or more 64-byte data blocks
        /// It does NOT update the bit counters. There are no alignment requirements.
        /// @param [in] data Data to process
        /// @param [in] size Size of the data. It has to be a multiple of 64!
        /// @return The pointer to the first byte after the last block processed
        const unsigned char * body ( const unsigned char * data, size_t size );
        void finalize();
};
}
