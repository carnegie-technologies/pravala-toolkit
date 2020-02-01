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

#include "basic/List.hpp"
#include "basic/String.hpp"
#include "ProtoError.hpp"

namespace Pravala
{
/// @brief A class that holds a list of protocol error codes and messages associated with them.
class ExtProtoError
{
    public:
        /// @brief A single error entry.
        struct Entry
        {
            String message; ///< The error message.
            ProtoError code; ///< The error code.
        };

        /// @brief Adds another error to the list (at the beginning of the list).
        /// @param [in] code Error code to append.
        /// @param [in] message Error message to append.
        inline void add ( const ProtoError & code, const String & message = String::EmptyString )
        {
            Entry e;
            e.message = message;
            e.code = code;

            _errors.prepend ( e );
        }

        /// @brief Exposes the internal list of errors.
        /// @return The list of errors.
        inline const List<Entry> & getErrors() const
        {
            return _errors;
        }

        /// @brief Generates a string with the content of ExtProtoError.
        /// @return String description of extended error object.
        inline String toString() const
        {
            String ret;

            for ( size_t i = 0; i < _errors.size(); ++i )
            {
                if ( !ret.isEmpty() )
                    ret.append ( "; " );

                ret.append ( String ( "%1 (%2)" ).arg ( _errors.at ( i ).code.toString(), _errors.at ( i ).message ) );
            }

            return ret;
        }

        /// @brief Clears the list of errors.
        inline void clear()
        {
            _errors.clear();
        }

    private:
        List<Entry> _errors; ///< Error entries.
};
}
