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
class FileObject;

/// @brief A class that helps create comments
/// It adds EOL to the file when it's destroyed
/// and starts each line with configured prefix
class CommentBlock
{
    public:
        /// @brief Constructor
        /// @param [in] myFile The file in which we are writing
        /// @param [in] commentPrefix The prefix to use at the beginning of each line
        CommentBlock ( FileObject & myFile, const String & commentPrefix );

        /// @brief Appends a value at the end of the current line
        /// @param [in] value The value to append
        /// @return reference to this CommentBlock (for chaining)
        CommentBlock & a ( const String & value );

        /// @brief Appends a value at the end of the current line
        /// @param [in] value The value to append
        /// @return reference to this CommentBlock (for chaining)
        CommentBlock & a ( long value );

        /// @brief Adds an EOL and starts a new comment line
        /// @return reference to this CommentBlock (for chaining)
        CommentBlock & e();

        /// @brief Destructor
        ~CommentBlock();

    protected:
        FileObject & _myFile; ///< Our file object
        const String _commentPrefix; ///< Comment prefix ('///' ' * ' etc.)
        String _text; ///< Text in this line
        int _numLines; ///< Number of lines written to the file
};
}
