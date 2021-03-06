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

#include "../FileObject.hpp"
#include "../CommentBlock.hpp"

namespace Pravala
{
/// @brief A class that represents an output file for CSharp language generators
///
/// It contains sections and helper functions typical to CSharp language
///
class CSharpFile: public FileObject
{
    public:

        /// @brief Helps create another part of the comment, which closes the tag
        class CommentTag: public CommentBlock
        {
            public:
                /// @brief Constructor
                /// @param [in] myFile The file in which we are writing
                /// @param [in] closingTag The closing tag "/param", "/exception", etc. (with < > brackets)
                /// @param [in] eolAfterTextA If true, there will be an EOL added after textA
                /// @param [in] textA Optional text to add right away
                /// @param [in] textB Optional text to add right away
                CommentTag (
                    CSharpFile & myFile, const char * closingTagName, bool eolAfterTextA,
                    const String & textA = String::EmptyString,
                    const String & textB = String::EmptyString );

                /// @brief Destructor
                /// Closes the tag. If there were any EOLs added, it closes the tag in new line
                ~CommentTag();

            protected:
                const String _closingTag; ///< The closing tag
        };

        /// @brief Constructs a new CSharp file object
        ///
        /// @param [in] path A path to the generated file
        /// @param [in] singleIndent A single level of indentation. At each indentation level, the prefix
        ///     added at the beginning of each line will be increased by this value. Should be at least
        ///     one space. Two spaces by default.
        CSharpFile ( const String & path, const String & singleIndent = "  " );

        /// @brief Sets the current section to 'header'
        ///
        /// This section is at the very beginning of the file.
        inline void setHeaderSection()
        {
            setSection ( _secHeader );
        }

        /// @brief Sets the current section to 'body'
        ///
        /// This section contains the body of the class generated
        inline void setBodySection()
        {
            setSection ( _secBody );
        }

        /// @brief Sets the current section to 'footer'
        ///
        /// This section follows the body section.
        inline void setFooterSection()
        {
            setSection ( _secFooter );
        }

        /// @brief Adds a specified path to file's imports
        /// @param [in] path The path to import
        void addImport ( const String & path );

        /// @brief Adds "autogenerated, do not edit" comment to the currently used section
        void genAutogenComment();

        /// @brief Creates a summary comment block in the file
        /// It starts with two EOLs
        /// @param [in] textA Optional text to add right away
        /// @param [in] textB Optional text to add right away
        /// @param [in] textC Optional text to add right away
        /// @param [in] textD Optional text to add right away
        /// @return new CommentTag object
        CommentTag csum (
            const String & textA = String::EmptyString,
            const String & textB = String::EmptyString,
            const String & textC = String::EmptyString,
            const String & textD = String::EmptyString );

        /// @brief Creates a 'remarks' comment block in the file
        /// @param [in] crefName The 'cref' of the exception
        /// @param [in] textA Optional text to add right away
        /// @param [in] textB Optional text to add right away
        /// @param [in] textC Optional text to add right away
        /// @param [in] textD Optional text to add right away
        /// @return new CommentTag object
        CommentTag cdesc (
            const String & textA = String::EmptyString,
            const String & textB = String::EmptyString,
            const String & textC = String::EmptyString,
            const String & textD = String::EmptyString );

        /// @brief Creates a return comment block in the file
        /// @param [in] textA Optional text to add right away
        /// @param [in] textB Optional text to add right away
        /// @param [in] textC Optional text to add right away
        /// @param [in] textD Optional text to add right away
        /// @return new CommentTag object
        CommentTag cret (
            const String & textA = String::EmptyString,
            const String & textB = String::EmptyString,
            const String & textC = String::EmptyString,
            const String & textD = String::EmptyString );

        /// @brief Creates a 'param' comment block in the file
        /// @param [in] paramName The name of the parameter
        /// @param [in] textA Optional text to add right away
        /// @param [in] textB Optional text to add right away
        /// @param [in] textC Optional text to add right away
        /// @param [in] textD Optional text to add right away
        /// @return new CommentTag object
        CommentTag cparam (
            const String & paramName,
            const String & textA = String::EmptyString,
            const String & textB = String::EmptyString,
            const String & textC = String::EmptyString,
            const String & textD = String::EmptyString );

        /// @brief Creates an 'exception' comment block in the file
        /// @param [in] crefName The 'cref' of the exception
        /// @param [in] textA Optional text to add right away
        /// @param [in] textB Optional text to add right away
        /// @param [in] textC Optional text to add right away
        /// @param [in] textD Optional text to add right away
        /// @return new CommentTag object
        CommentTag cexc (
            const String & crefName,
            const String & textA = String::EmptyString,
            const String & textB = String::EmptyString,
            const String & textC = String::EmptyString,
            const String & textD = String::EmptyString );

        /// @brief Adds a comment line followed by the value passed
        /// @param [in] value The actual comment
        /// @return reference to this FileObject (for chaining)
        virtual FileObject & c ( const String & value );

    private:
        int _secHeader; ///< The ID of the 'header' section
        int _secBody; ///< The ID of the 'body' section
        int _secFooter; ///< The ID of the 'footer' section

        HashSet<String> _imports; ///< Contains all imports
};
}
