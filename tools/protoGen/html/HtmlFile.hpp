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

namespace Pravala
{
/// @brief A class that represents an output file for HTML language generator
///
/// It contains helper functions typical to HTML output
///
class HtmlFile: public FileObject
{
    public:

        /// @brief Constructs a new HTML file object
        ///
        /// @param [in] path A path to the generated file
        HtmlFile ( const String & path );

        /// @brief Sets the current section to 'TOC'
        ///
        /// This section contains the table of contents
        inline void setTocSection()
        {
            setSection ( _secToc );
        }

        /// @brief Sets the current section to 'content'
        ///
        /// This section contains the symbol definitions
        inline void setContentSection()
        {
            setSection ( _secContent );
        }

        /// @brief Adds a comment line followed by the value passed
        /// @param [in] value The actual comment
        /// @return reference to this FileObject (for chaining)
        virtual FileObject & c ( const String & value );

    private:
        int _secToc; ///< The ID of the 'TOC' section
        int _secContent; ///< The ID of the 'content' section
};
}
