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
#include "basic/HashSet.hpp"
#include "ProtoSpec.hpp"
#include "Symbol.hpp"

namespace Pravala
{
/// @brief The starting object for parsing input files
class ParserRoot
{
    public:
        /// @brief Adds a directory to the list of import directories.
        /// Those directories are used while looking for files included with 'import' keyword.
        /// @param [in] dir The directory to add to the list.
        /// @return True if the directory was added; False otherwise.
        bool addImportDir ( const String & dir );

        /// @brief Adds a file to the list of input files.
        /// @param [in] file The file path to add to the list.
        /// @return True if the path was added; False otherwise.
        bool addInputFile ( const String & file );

        /// @brief Returns the list of import directories.
        /// Those directories are used while looking for files included with 'import' keyword.
        /// @return The list of import directories.
        inline const StringList & getImportDirs() const
        {
            return _importDirs;
        }

        /// @brief Returns the list of input files.
        /// @return The list of input files.
        inline const StringList & getInputFiles() const
        {
            return _inputFiles;
        }

        /// @brief Runs the parsers for all input files
        /// @param [in,out] protoSpec The protocol specification object to append tokens to
        /// @return True if parsing was successful, false otherwise
        bool run ( ProtocolSpec & protoSpec );

    protected:
        StringList _importDirs; ///< The list of directories to use while looking for imported files.
        StringList _inputFiles; ///< The list of input files.

        /// @brief A set with MD5 sums of all files that already were parsed.
        HashSet<String> _parsedFiles;

        /// @brief A set with MD5 sums of all files that we want to generate the output for.
        HashSet<String> _generateOutputForFiles;

        friend class Parser;
};
}
