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

#include <cstdio>

#include "basic/String.hpp"
#include "basic/HashSet.hpp"
#include "Symbol.hpp"

namespace Pravala
{
/// @brief A class that represents an output file for the generated language
///
/// Different languages will have their own classes for specific things, but
/// this is a base with some common things.
///
/// It contains "sections", each section consists of a list of lines.
/// When the object is written to the file, all lines from all sections are
/// written in order (and followed by \n)
///
class FileObject
{
    public:
        /// @brief Constructs a new file object
        ///
        /// @param [in] path A path to the generated file
        /// @param [in] singleIndent A single level of indentation. At each indentation level, the prefix
        ///     added at the beginning of each line will be increased by this value. Should be at least
        ///     one space. Two spaces by default.
        FileObject ( const String & path, const String & singleIndent = "  " );

        /// @brief Destructor.
        virtual ~FileObject();

        /// @brief Finalizes the file
        /// This function can make any modifications or adjustments of the content of the file
        /// just before the file is written to disk.
        virtual void finalizeFile();

        /// @brief Writes output to the given file handle
        /// @param [in] toFile The file to write the output to
        void writeOutput ( FILE * toFile );

        /// @brief Appends a new section of the file
        /// This section will be also the new active section
        /// @return The ID of the new section
        int appendSection();

        /// @brief Returns current section ID
        /// @return Current section ID
        inline int getSectionId() const
        {
            return _sectionId;
        }

        /// @brief Sets the 'current section' to the section with the ID given
        ///
        /// All methods appending something to the file will, from now on, operate on this new section.
        ///
        /// @param [in] sectionId The ID of the section to set as current.
        void setSection ( const int sectionId );

        /// @brief Increases the indentation level
        /// It applies only to the current line.
        /// @param [in] iLevel Number of indentation levels to increase.
        ///                    It could be negative (useful when base indentation level is > 0),
        ///                    but it will be decreased at most to the '0' level.
        /// @return reference to this FileObject (for chaining)
        FileObject & i ( int iLevel );

        /// @brief Adds section of lines
        /// @param [in] value The section to be appended. '\n' characters are interpreted as EOLs.
        ///                   It will be split, and each line will be prepended with current indent level.
        ///                   If the line starts with spaces, each PAIR of spaces will be replaced with a single
        ///                   indent level.
        /// @return reference to this FileObject (for chaining)
        FileObject & alines ( const String & value );

        /// @brief Adds section of lines
        /// @param [in] iLevel Number of indentation levels to increase.
        /// @param [in] value The section to be appended. '\n' characters are interpreted as EOLs.
        ///                   It will be split, and each line will be prepended with current indent level.
        ///                   If the line starts with spaces, each PAIR of spaces will be replaced with a single
        ///                   indent level.
        /// @return reference to this FileObject (for chaining)
        FileObject & alines ( int iLevel, const String & value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] value The value to append
        /// @return reference to this FileObject (for chaining)
        FileObject & a ( const String & value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] iLevel Number of indentation levels to increase.
        /// @param [in] value The value to append
        /// @return reference to this FileObject (for chaining)
        FileObject & a ( int iLevel, const String & value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] value The value to append
        /// @return reference to this FileObject (for chaining)
        FileObject & a ( int value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] value The value to append
        /// @return reference to this FileObject (for chaining)
        FileObject & ae ( const String & value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] iLevel Number of indentation levels to increase.
        /// @param [in] value The value to append
        /// @return reference to this FileObject (for chaining)
        FileObject & ae ( int iLevel, const String & value );

        /// @brief Starts a new line
        /// Works like EOL (end of line - "\n") in regular strings
        /// It doesn't add any indentation to the new line, the base level will be
        /// appended once the a() or i() is used for the first time on that line.
        /// @param force [in] Normally, e() will not create multiple empty lines one after
        ///                   another. To force it to create the empty line, this parameter
        ///                   can be set to true.
        /// @return reference to this FileObject (for chaining)
        FileObject & e ( bool force = false );

        /// @brief Returns the number of lines added to the current section
        /// @return Number of lines in the current section
        int getLineCount() const;

        /// @brief Increases the base indentation level by one
        /// It applies to all lines that will follow.
        /// Each line is prefixed with the 'base' number of indents.
        /// This function increases this base number.
        /// It doesn't affect existing lines, only lines created from now on.
        /// @param [in] byWhat By how many levels it should be increased. 1 by default.
        void incBaseIndent ( int byWhat = 1 );

        /// @brief Decreases the base indentation level by one
        /// It applies to all lines that will follow.
        /// Each line is prefixed with the 'base' number of indents.
        /// This function decreases this base number.
        /// It doesn't affect existing lines, only lines created from now on.
        /// @param [in] byWhat By how many levels it should be decreased. 1 by default.
        void decBaseIndent ( int byWhat = 1 );

        /// @return The path of this file object
        inline const String & getPath() const
        {
            return _path;
        }

        /// @brief Adds a doxygen comment line followed by the value passed, followed by the end of line
        /// @param [in] value The actual comment. If empty, an empty doxygen comment line will be added.
        /// @return reference to this FileObject (for chaining)
        virtual FileObject & ce ( const String & value = String::EmptyString );

        /// @brief Adds a doxygen comment line followed by the value passed
        /// @param [in] value The actual comment
        /// @return reference to this FileObject (for chaining)
        virtual FileObject & c ( const String & value ) = 0;

        /// @brief Function generates comments for the field
        /// @param [in] elem The element to generate comments for
        /// @param [in] indent The base indentation to use
        virtual void genVarComments ( Element * elem, int indent = 0 );

    protected:
        int _baseIndent; ///< Current base indent level
        int _sectionId; ///< The ID of the current section (and index in _data list)
        String _path; ///< The path of this file object
        String _singleIndent; ///< The string representing a single indent level
        List<StringList> _data; ///< List of sections, each section is a list of strings

        /// @return It returns the last line in the current section.
        ///         If the section is empty, it appends an empty line and return it.
        String & getLineToAppend();
};

/// @brief A class for generating simple, text files.
class TextFile: public FileObject
{
    public:
        /// @brief Constructor.
        ///
        /// @param [in] path A path to the generated file
        /// @param [in] singleIndent A single level of indentation. At each indentation level, the prefix
        ///     added at the beginning of each line will be increased by this value. Should be at least
        ///     one space. Two spaces by default.
        TextFile ( const String & path, const String & singleIndent = "  " );

        virtual FileObject & c ( const String & value );
};
}
