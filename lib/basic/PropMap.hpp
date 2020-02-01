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

extern "C"
{
#include <sys/types.h>
}

#include "String.hpp"
#include "MsvcSupport.hpp"

namespace Pravala
{
class MemHandle;
class Buffer;

class PropMap: public HashMap<String, String>
{
    public:
        /// @brief Default constructor.
        /// Creates an empty PropMap object.
        inline PropMap()
        {
        }

        /// @brief Copy constructor.
        /// @param [in] other The object to create a copy of
        inline PropMap ( const HashMapImpl<String, String> & other ): HashMap<String, String> ( other )
        {
        }

        /// @brief Assignment operator
        /// @param [in] other The object to create a copy of
        /// @return A reference to this object.
        inline PropMap & operator= ( const HashMapImpl<String, String> & other )
        {
            HashMap<String, String>::operator= ( other );
            return *this;
        }

        /// @brief Appends the serialized content of the PropMap to the buffer provided
        /// @param [in,out] toBuffer The buffer to write (append) the data to
        /// @param [in] entrySeparator The entry separator to use. By default it's a new line character.
        ///                             It could contain multiple characters.
        /// @param [in] propSeparator The property separator entry separator to use.
        ///                             A single character, '=' by default
        void writeToBuffer (
            Buffer & toBuffer,
            const String & entrySeparator = "\n", char propSeparator = '=' );

        /// @brief Serializes content of the PropMap
        ///
        /// If the content will be appended to another buffer, it's better to use 'writeToBuffer()' method instead.
        /// @param [in] entrySeparator The entry separator to use. By default it's a new line character.
        ///                             It could contain multiple characters.
        /// @param [in] propSeparator The property separator entry separator to use. A single character, '=' by default
        /// @return The buffer with the serialized content of the PropMap object
        MemHandle getBuffer ( const String & entrySeparator = "\n", char propSeparator = '=' );

        /// @brief Reads content from the file and adds it to the mapping
        /// The existing content is NOT removed, unless it is overwritten by the new values.
        /// To load completely new content, use clear() function first.
        /// @param [in] filePath The path to the file
        /// @param [in] propSeparators Property separator(s) to be used (instead of '=')
        /// @param [in] commentChars Comment character(s) to be used (instead of '#')
        /// @param [in] listSeparators Separator(s) to be used for splitting the input into list of properties.
        ///                 By default it is "\n\r" and causes the input to be treated as a list of lines, one entry
        ///                 per line. If other values are used, other types of data can be processed as well.
        /// @return 0 on success, or the first entry number that could not be parsed properly
        ///          (the first entry has number 1, not 0). On error, processing is interrupted, so remaining entries
        ///          will not be added. If the file does not exist, or cannot be read properly, a -1 is returned.
        ssize_t appendFile (
            const String & filePath,
            const String & propSeparators = "=", const String & commentChars = "#",
            const String & listSeparators = "\n\r" );

        /// @brief Reads content from the buffer and adds it to the mapping
        /// The existing content is NOT removed, unless it is overwritten by the new values.
        /// To load completely new content, use clear() function first.
        /// @param [in] fromBuffer The content to be added to the map
        /// @param [in] propSeparators Property separator(s) to be used (instead of '=')
        /// @param [in] commentChars Comment character(s) to be used (instead of '#')
        /// @param [in] listSeparators Separator(s) to be used for splitting the input into list of properties.
        ///                 By default it is "\n\r" and causes the input to be treated as a list of lines, one entry
        ///                 per line. If other values are used, other types of data can be processed as well.
        /// @return 0 on success, or the first entry number that could not be parsed properly
        ///          (the first entry has number 1, not 0). On error, processing is interrupted, so remaining entries
        ///          will not be added.
        size_t appendData (
            const MemHandle & fromBuffer,
            const String & propSeparators = "=", const String & commentChars = "#",
            const String & listSeparators = "\n\r" );

        /// @brief Parses given list of properties and adds the content to the map
        /// The existing content is NOT removed, unless it is overwritten by the new values.
        /// To load completely new content, use clear() function first.
        /// @param [in] list The list on properties to be added to the map
        /// @param [in] propSeparators Property separator(s) to be used (instead of '=')
        /// @param [in] commentChars Comment character(s) to be used (instead of '#')
        /// @return 0 on success, or the first entry number that could not be parsed properly
        ///          (the first entry has number 1, not 0). On error, processing is interrupted, so remaining entries
        ///          will not be added.
        size_t appendLines (
            const StringList & list,
            const String & propSeparators = "=", const String & commentCharss = "#" );

        /// @brief Sets property's numeric value.
        /// @param [in] propKey Name of property's key.
        /// @param [in] propValue Value to be stored under that key. It should be one of the supported numeric types.
        template<typename T> void insertNumber ( const String & propKey, T propValue )
        {
            insert ( propKey, String::number ( propValue ) );
        }

        /// @brief Tries to read a property's numeric value.
        /// @param [in] propKey The key to retrieve
        /// @param [out] propValue The numeric value; remains the same if return is false
        /// @return true if the key exists and can be converted from string; false otherwise
        template<typename T> inline bool getNumber ( const String & propKey, T & propValue ) const
        {
            String strVal;

            return ( find ( propKey, strVal ) && strVal.toNumber ( propValue ) );
        }

        /// @brief Parses a single property entry/line.
        ///
        /// @param [in] entry Line containing the property to load. It should be in 'key = value' format.
        /// @param [out] propName The parsed name of the property. Trimmed and in lowercase
        /// @param [out] propValue The value of the property. Trimmed.
        /// @param [in] propSeparators Property separator(s) to be used (instead of '=')
        /// @param [in] commentChars Comment character(s) to be used (instead of '#')
        /// @return True if the entry was parsed properly (or if it was empty or contained only a comment);
        ///         False if there was a syntax error
        static bool parseProperty (
            const String & entry, String & propName, String & propValue,
            const String & propSeparators = "=", const String & commentChars = "#" );
};
}
