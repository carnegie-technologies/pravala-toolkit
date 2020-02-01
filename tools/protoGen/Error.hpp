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

#include <cstdarg>

#include "basic/String.hpp"

namespace Pravala
{
/// @brief The class that is used by the protocol compiler to throw errors during parsing and code generation
///
/// Its only purpose is to simplify throwing errors with different number of description parts.
///
class Error
{
    public:
        /// @brief A constructor
        ///
        /// Takes a number of strings, and appends them in order to the internal
        /// error message string. This is to make throwing errors easy.
        ///
        /// @param [in] messageA A part of the error message
        /// @param [in] messageB A part of the error message (optional)
        /// @param [in] messageC A part of the error message (optional)
        /// @param [in] messageD A part of the error message (optional)
        /// @param [in] messageE A part of the error message (optional)
        /// @param [in] messageF A part of the error message (optional)
        /// @param [in] messageG A part of the error message (optional)
        /// @param [in] messageH A part of the error message (optional)
        /// @param [in] messageI A part of the error message (optional)
        /// @param [in] messageJ A part of the error message (optional)
        /// @param [in] messageK A part of the error message (optional)
        /// @param [in] messageL A part of the error message (optional)
        /// @param [in] messageM A part of the error message (optional)
        /// @param [in] messageN A part of the error message (optional)
        /// @param [in] messageO A part of the error message (optional)
        /// @param [in] messageP A part of the error message (optional)
        /// @param [in] messageQ A part of the error message (optional)
        /// @param [in] messageR A part of the error message (optional)
        /// @param [in] messageS A part of the error message (optional)
        /// @param [in] messageT A part of the error message (optional)
        /// @param [in] messageU A part of the error message (optional)
        /// @param [in] messageV A part of the error message (optional)
        /// @param [in] messageW A part of the error message (optional)
        /// @param [in] messageX A part of the error message (optional)
        /// @param [in] messageY A part of the error message (optional)
        /// @param [in] messageZ A part of the error message (optional)
        Error (
            const String & messageA,
            const String & messageB = String::EmptyString,
            const String & messageC = String::EmptyString,
            const String & messageD = String::EmptyString,
            const String & messageE = String::EmptyString,
            const String & messageF = String::EmptyString,
            const String & messageG = String::EmptyString,
            const String & messageH = String::EmptyString,
            const String & messageI = String::EmptyString,
            const String & messageJ = String::EmptyString,
            const String & messageK = String::EmptyString,
            const String & messageL = String::EmptyString,
            const String & messageM = String::EmptyString,
            const String & messageN = String::EmptyString,
            const String & messageO = String::EmptyString,
            const String & messageP = String::EmptyString,
            const String & messageQ = String::EmptyString,
            const String & messageR = String::EmptyString,
            const String & messageS = String::EmptyString,
            const String & messageT = String::EmptyString,
            const String & messageU = String::EmptyString,
            const String & messageV = String::EmptyString,
            const String & messageW = String::EmptyString,
            const String & messageX = String::EmptyString,
            const String & messageY = String::EmptyString,
            const String & messageZ = String::EmptyString ):
            _message ( messageA )
        {
            _message.append ( messageB );
            _message.append ( messageC );
            _message.append ( messageD );
            _message.append ( messageE );
            _message.append ( messageF );
            _message.append ( messageG );
            _message.append ( messageH );
            _message.append ( messageI );
            _message.append ( messageJ );
            _message.append ( messageK );
            _message.append ( messageL );
            _message.append ( messageM );
            _message.append ( messageN );
            _message.append ( messageO );
            _message.append ( messageP );
            _message.append ( messageQ );
            _message.append ( messageR );
            _message.append ( messageS );
            _message.append ( messageT );
            _message.append ( messageU );
            _message.append ( messageV );
            _message.append ( messageW );
            _message.append ( messageX );
            _message.append ( messageY );
            _message.append ( messageZ );
        }

        /// @brief Returns the error message
        /// @return The error message
        inline const String & getMessage() const
        {
            return _message;
        }

        /// @brief Appends a string to the error message
        /// @param [in] message The c-string to be appended
        /// @return Reference to this Error object (for chaining)
        inline Error & a ( const String & message )
        {
            _message.append ( message );

            return *this;
        }

        /// @brief Appends a string to the error message
        /// @param [in] message The c-string to be appended
        /// @return Reference to this Error object (for chaining)
        inline Error & a ( const char * message )
        {
            _message.append ( message );

            return *this;
        }

    private:
        String _message; ///< The actual error message
};
}
