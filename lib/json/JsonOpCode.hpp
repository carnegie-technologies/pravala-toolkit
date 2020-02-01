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
/// @brief A JSON operation code class.
class JsonOpCode
{
    public:
        /// @brief all possible codes.
        enum Code
        {
            Success = 0, ///< Operation succeeded.

            Unknown = -1, ///< Unknown error.

            NotFound = -2, ///< The element was not found.

            InvalidIndex = -3, ///< The index was invalid (for array operation).

            InvalidDataType = -4, ///< There was invalid data type (a string instead of a number, etc.).

            InvalidDataRange = -5, ///< The data was was in invalid range (a number that doesn't fit in requested type)

            InvalidData = -6 ///< The data content was invalid.
        };

        /// @brief Default constructor.
        /// Sets the code value to 'Unknown'.
        inline JsonOpCode(): _code ( Unknown )
        {
        }

        /// @brief Constructor.
        /// @param [in] eCode the code to set.
        inline JsonOpCode ( const Code & eCode ): _code ( eCode )
        {
        }

        /// @brief Casts the operation code to bool. Success will be casted to 'true', everything else - to 'false'.
        inline operator bool() const
        {
            return ( _code == Success );
        }

        /// @brief Equality operator.
        /// @param [in] eCode the code to compare the value of this object against.
        /// @return True if the code passed is the same as this object's internal code; False otherwise.
        inline bool operator== ( const Code & eCode ) const
        {
            return ( _code == eCode );
        }

        /// @brief Inequality operator.
        /// @param [in] eCode the code to compare the value of this object against.
        /// @return True if the code passed is different than this object's internal code; False otherwise.
        inline bool operator!= ( const Code & eCode ) const
        {
            return ( _code != eCode );
        }

        /// @brief Exposes the internal code.
        /// @return The internal code.
        inline Code code() const
        {
            return _code;
        }

        /// @brief Checks if the code describes data-related problems.
        /// This can be used to verify whether there was no JSON error,
        /// or if the error was caused by a missing value, as opposed to trying to read data
        /// in the wrong format.
        /// @return True if the code is one of the InvalidData* codes; False otherwise.
        inline bool isDataError() const
        {
            switch ( _code )
            {
                case Success:
                case Unknown:
                case NotFound:
                case InvalidIndex:
                    return false;
                    break;

                case InvalidDataType:
                case InvalidDataRange:
                case InvalidData:
                    return true;
                    break;
            }

            return false;
        }

    private:
        Code _code; ///< Internal error code.
};
}
