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

#include <cwchar>

#include "String.hpp"

namespace Pravala
{
/// @brief VERY basic WString class.
/// Unlike String class it does NOT provide implicit data sharing.
class WString
{
    public:
        /// @brief An empty WString
        const static WString EmptyString;

        /// @brief A NULL character
        const static wchar_t ZeroChar;

        /// @brief Default constructor.
        /// Creates an empty string. Empty string doesn't allocate much memory,
        /// it only stores a null pointer.
        /// This class is NOT thread safe!
        inline WString(): _data ( 0 ), _length ( 0 )
        {
        }

        /// @brief Constructor
        /// Creates a string containing a copy of  C-string provided.
        /// @param [in] str C-String to copy to the new string object.
        /// This class is NOT thread safe!
        inline WString ( const wchar_t * str ): _data ( 0 ), _length ( 0 )
        {
            if ( !str )
                return;

            append ( str );
        }

        /// @brief Copy constructor.
        /// Creates a copy of a string provided.
        /// @param [in] other String to copy.
        WString ( const WString & other );

        /// @brief Destructor.
        ~WString();

        /// @brief Assignment operator
        /// Assigns other to this string and returns a reference to this string.
        /// @param [in] other String to assign to this string.
        /// @return Reference to this string.
        WString & operator= ( const WString & other );

        /// @brief Assignment operator
        /// Assigns str to this string and returns a reference to this string.
        /// @param [in] str C-String to assign to this string.
        /// @return Reference to this string.
        WString & operator= ( const wchar_t * str );

        /// @brief Equality operator
        /// @param [in] other String to compare.
        /// @return True if this string is equal (in terms of content) to the other one.
        bool operator== ( const WString & other ) const;

        /// @brief Equality operator
        /// @param [in] str C-String to compare.
        /// @return True if this string is equal (in terms of content) to the other one.
        bool operator== ( const wchar_t * str ) const;

        /// @brief Not-Equality operator
        /// @param [in] other String to compare.
        /// @return True if this string is not equal (in terms of content) to the other one.
        inline bool operator!= ( const WString & other ) const
        {
            return !operator== ( other );
        }

        /// @brief Not-Equality operator
        /// @param [in] str C-String to compare.
        /// @return True if this string is not equal (in terms of content) to the other one.
        inline bool operator!= ( const wchar_t * str ) const
        {
            return !operator== ( str );
        }

        /// @brief Returns a writable reference to a character at a given index in the string.
        /// @param [in] idx Requested index. This index has to be valid!
        /// @return Reference to the character at that position.
        wchar_t & operator[] ( int idx );

        /// @brief Returns character at a given index in the string.
        /// @param [in] idx Requested index. This index has to be valid!
        /// @return The character at that position.
        wchar_t operator[] ( int idx ) const;

        /// @brief Returns this wide string converted to a regular string.
        /// @param [out] isOk If not 0, it is set to true if the conversion succeeded, and false if it failed.
        /// @return this wide string converted to a regular string.
        String toString ( bool * isOk = 0 ) const;

        /// @brief Appends the wide string str onto the end of this wide string.
        /// @param [in] str Wide string to append.
        /// @return Reference to this string.
        WString & append ( const WString & str );

        /// @brief Appends the memory onto the end of this string.
        /// @param [in] data Pointer to the memory to append.
        /// @return Reference to this string.
        WString & append ( const wchar_t * data );

        /// @brief Appends the memory onto the end of this string.
        /// @param [in] data Pointer to the memory to append.
        /// @param [in] size The size of that memory.
        /// @return Reference to this string.
        WString & append ( const wchar_t * data, int size );

        /// @brief Clears the string.
        /// This function clears the string. It will have length = 0 after this call.
        /// This doesn't release memory allocated. To release any memory used, call
        /// squeeze() after calling this function.
        void clear();

        /// @brief Returns a pointer to internal C-string memory.
        /// This function returns pointer to internal memory segment,
        /// which is simply null-terminated C-String. It is possible, that there
        /// are NULL characters inside that C-String, which will make it look shorter
        /// than what length() function reports.
        /// @return Pointer to internal C-String.
        inline const wchar_t * c_wstr() const
        {
            if ( !_data || _length < 1 )
            {
                return &ZeroChar;
            }

            return _data;
        }

        /// @brief Returns string's length.
        /// This function returns number of wide characters in the string.
        /// It doesn't count terminating null character.
        /// @return String's length.
        inline int length() const
        {
            return _length;
        }

        /// @brief Checks if string is empty.
        /// @return true if the string is empty (has length = 0); false otherwise.
        inline bool isEmpty() const
        {
            return ( _length <= 0 );
        }

    protected:
        wchar_t * _data; ///< Pointer to the actual string content.
        int _length; ///< Length of the string (in wide characters); Does not include NULL character.
};

/// @brief Hash function needed for storing WString objects in HashMap and HashSet containers.
/// @param [in] key The WString to hash
/// @return The hash code
size_t getHash ( const WString & key );
}
