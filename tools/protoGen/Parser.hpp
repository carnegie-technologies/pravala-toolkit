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

#include "ParserRoot.hpp"

namespace Pravala
{
/// @brief Input file parser
///
/// It reads all tokens from a single protocol description file.
/// It appends those tokens to the protocol spec object.
/// If needed, it runs other parsers (to read 'imported' files).
class Parser
{
    protected:
        /// @brief Creates a parser object
        /// @param [in] inputFile The file to read (and parse)
        Parser ( const String & inputFile );

        /// @brief Runs the parser
        /// @param [in] parserRoot ParserRoot object to use
        /// @param [in,out] protoSpec The protocol specification object to append tokens to
        /// @return True if parsing was successful, false otherwise
        bool run ( ParserRoot & parserRoot, ProtocolSpec & protoSpec );

    private:
        size_t _lineNumber; ///< The number of the current line in the input file
        String _inputPath; ///< The path to the input file
        String _inputFile; ///< The filename of the input file (without directories)
        String _inputDir; ///< The path to the input file without the filename (just directories)

        /// @brief Runs the parsing of individual lines from the input file
        /// @param [in] lines The list of input lines
        /// @param [in] generateOutput Should the output be generated or not
        /// @param [in] parserRoot ParserRoot object to use
        /// @param [in,out] protoSpec The protocol specification object to append tokens to
        /// @return True if parsing was successful, false otherwise
        bool doRun (
            const StringList & lines, bool generateOutput,
            ParserRoot & parserRoot, ProtocolSpec & protoSpec );

        /// @brief A helper function for detecting basic characters.
        ///
        /// Basic characters are letters, number, '_' and '.'.
        /// All sequences of basic characters are "glued" together to form a single token
        ///
        /// @param [in] c The character to examine
        /// @return True if the character passed is a basic character
        static bool isBasicChar ( char c );

        /// @brief A helper function for detecting special characters
        ///
        /// Special characters are all characters other than basic characters and whitespaces.
        /// Each special character is treated as a basic character sequence separator
        /// (just like any whitespace chatacter), but it is also a separate token on its own
        /// (which whitespace characters aren't)
        ///
        /// @param [in] c The character to examine
        /// @return True if the character passed is a special character
        static bool isSpecialChar ( char c );

        /// @brief A helper function that tests whether a file exists.
        /// @param [in] path The path of the file to use.
        /// @return True if the given file exists and can be opened; False otherwise.
        static bool isFile ( const String & path );

        friend class ParserRoot;
};
}
