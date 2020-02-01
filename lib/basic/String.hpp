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

// Mostly to avoid problems with using wrong getHash version for object pointers...
// (if this is included from here, we are not sensitive to the order of includes later!)
#include "HashMap.hpp"

namespace Pravala
{
class String;
class StringPriv;
class WString;
class IpAddress;
union SockAddr;

/// @brief StringList type
/// List<String> is commonly used. For simplicty this type
/// is declared as StringList.
typedef List<String> StringList;

/// @brief Basic String class.
/// Provides strings with implicit data sharing.
/// It is NOT thread safe!
class String
{
    public:
        /// @brief An empty String
        const static String EmptyString;

        /// @brief A NULL character
        const static char ZeroChar;

        /// @brief Default constructor.
        /// Creates an empty string. Empty string doesn't allocate much memory,
        /// it only stores a null pointer.
        /// This class is NOT thread safe!
        inline String(): _priv ( 0 )
        {
        }

        /// @brief Constructor
        /// Creates a string from memory provided.
        /// @param [in] data Pointer to the memory that will be inserted to the string.
        /// @param [in] size Length of that memory.
        /// This class is NOT thread safe!
        String ( const char * data, int size );

        /// @brief Constructor
        /// Creates a string containing a copy of  C-string provided.
        /// @param [in] str C-String to copy to the new string object.
        /// This class is NOT thread safe!
        String ( const char * str );

        /// @brief Copy constructor
        /// Creates a copy of a string provided. Strings use implicit data sharing,
        /// so the data itself is not actually copied until one of the strings wants
        /// to modify it.
        /// @param [in] other String to copy.
        /// This class is NOT thread safe!
        String ( const String & other );

        /// @brief Destructor.
        ~String();

        /// @brief Assignment operator
        /// Assigns other to this string and returns a reference to this string.
        /// @param [in] other String to assign to this string.
        /// @return Reference to this string.
        String & operator= ( const String & other );

        /// @brief Assignment operator
        /// Assigns str to this string and returns a reference to this string.
        /// @param [in] str C-String to assign to this string.
        /// @return Reference to this string.
        String & operator= ( const char * str );

        /// @brief Equality operator
        /// @param [in] other String to compare.
        /// @return True if this string is equal (in terms of content) to the other one.
        bool operator== ( const String & other ) const;

        /// @brief Equality operator
        /// @param [in] str C-String to compare.
        /// @return True if this string is equal (in terms of content) to the other one.
        bool operator== ( const char * str ) const;

        /// @brief Not-Equality operator
        /// @param [in] other String to compare.
        /// @return True if this string is not equal (in terms of content) to the other one.
        inline bool operator!= ( const String & other ) const
        {
            return !operator== ( other );
        }

        /// @brief Not-Equality operator
        /// @param [in] str C-String to compare.
        /// @return True if this string is not equal (in terms of content) to the other one.
        inline bool operator!= ( const char * str ) const
        {
            return !operator== ( str );
        }

        /// @brief Less-Than operator
        /// @param [in] other String to compare.
        /// @return True if this string is smaller than the other one.
        bool operator< ( const String & other ) const;

        /// @brief Greater-Than operator
        /// @param [in] other String to compare.
        /// @return True if this string is greater than the other one.
        bool operator> ( const String & other ) const;

        /// @brief Returns a writable reference to a character at a given index in the string.
        /// Note that this function will create a non-shared copy of internal string's data.
        /// If read-only access is sufficient, it is better to use at() function
        /// or constant version of the operator instead.
        /// @param [in] idx Requested index. This index has to be valid!
        /// @return Reference to the character at that position.
        char & operator[] ( int idx );

        /// @brief Returns character at a given index in the string.
        /// @param [in] idx Requested index. This index has to be valid!
        /// @return The character at that position.
        char operator[] ( int idx ) const;

        /// @brief Returns this string converted to a wide string.
        /// @param [out] isOk If not 0, it is set to true if the conversion succeeded, and false if it failed.
        /// @return this string converted to a wide string.
        WString toWString ( bool * isOk = 0 ) const;

        /// @brief Compares this string to another string.
        ///
        /// @param [in] other Second string
        /// @param [in] caseSensitive If false (it is true by default) strings will
        /// be compared ignoring the case.
        /// @param [in] len If len is >= 0, only compare up up to first 'len' characters.
        ///                 It it is < 0, compare whole strings.
        /// @return -1, 0, 1 if this string is less, equal or greater than the second one
        /// (ignoring the case or not).
        inline int compare ( const String & other, bool caseSensitive = true, int len = -1 ) const
        {
            return compare ( c_str(), length(), other.c_str(), other.length(), caseSensitive, len );
        }

        /// @brief Compares this string to C-string.
        ///
        /// @param [in] str C-string to compare with
        /// @param [in] caseSensitive If false (it is true by default) strings will
        /// be compared ignoring the case.
        /// @param [in] len If len is >= 0, only compare up up to first 'len' characters.
        ///                 It it is < 0, compare whole strings.
        /// @return -1, 0, 1 if this string is less, equal or greater than the second one
        /// (ignoring the case or not).
        inline int compare ( const char * str, bool caseSensitive = true, int len = -1 ) const
        {
            return compare ( c_str(), length(), str, ( int ) strlen ( str ), caseSensitive, len );
        }

        /// @brief Appends the string str onto the end of this string.
        /// @param [in] str String to append.
        /// @return Reference to this string.
        String & append ( const String & str );

        /// @brief Appends the C-string str onto the end of this string.
        /// @param [in] str C-String to append.
        /// @return Reference to this string.
        String & append ( const char * str );

        /// @brief Appends the memory onto the end of this string.
        /// @param [in] data Pointer to the memory to append.
        /// @param [in] size The size of that memory.
        /// @return Reference to this string.
        String & append ( const char * data, int size );

        /// @brief Appends one character onto the end of this string.
        /// @param [in] ch Character to append.
        /// @return Reference to this string.
        String & append ( char ch );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a The argument to use
        /// @return Reference to this object
        String & arg ( const char * a );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a The argument to use
        /// @return Reference to this object
        String & arg ( char * a );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98).
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a The argument to use
        /// @return Reference to this object
        String & arg ( const String & a );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98).
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a The argument to use
        /// @return Reference to this object
        String & arg ( const IpAddress & a );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98).
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a The argument to use
        /// @return Reference to this object
        String & arg ( const SockAddr & a );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] c The argument to use
        /// @return Reference to this object
        String & arg ( char c );

        /// @brief Replaces all occurrences of %1 in the string with provided value
        /// All other arguments will be decremented (%2 -> %1, %3 -> %2, ... %99 -> %98)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] value The value to use
        /// @return Reference to this object
        template<typename T> inline String & arg ( const T & value )
        {
            return arg ( String::number ( value ) );
        }

        /// @brief Replaces all occurrences of %1 and %2 with given values
        /// All other arguments will be decremented (%99 -> %97, %98 -> %96, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @return Reference to this object
        String & arg ( const String & a1, const String & a2 );

        /// @brief Replaces all occurrences of %1 - %3 with given values
        /// All other arguments will be decremented (%99 -> %96, %98 -> %95, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @return Reference to this object
        String & arg ( const String & a1, const String & a2, const String & a3 );

        /// @brief Replaces all occurrences of %1 - %4 with given values
        /// All other arguments will be decremented (%99 -> %95, %98 -> %94, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4 );

        /// @brief Replaces all occurrences of %1 - %5 with given values
        /// All other arguments will be decremented (%99 -> %94, %98 -> %93, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a5 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4, const String & a5 );

        /// @brief Replaces all occurrences of %1 - %6 with given values
        /// All other arguments will be decremented (%99 -> %93, %98 -> %92, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a5 The next argument
        /// @param [in] a6 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4, const String & a5, const String & a6 );

        /// @brief Replaces all occurrences of %1 - %7 with given values
        /// All other arguments will be decremented (%99 -> %92, %98 -> %91, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a5 The next argument
        /// @param [in] a6 The next argument
        /// @param [in] a7 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4, const String & a5, const String & a6,
            const String & a7 );

        /// @brief Replaces all occurrences of %1 - %8 with given values
        /// All other arguments will be decremented (%99 -> %91, %98 -> %90, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a5 The next argument
        /// @param [in] a6 The next argument
        /// @param [in] a7 The next argument
        /// @param [in] a8 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4, const String & a5, const String & a6,
            const String & a7, const String & a8 );

        /// @brief Replaces all occurrences of %1 - %9 with given values
        /// All other arguments will be decremented (%99 -> %90, %98 -> %89, ...)
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a5 The next argument
        /// @param [in] a6 The next argument
        /// @param [in] a7 The next argument
        /// @param [in] a8 The next argument
        /// @param [in] a9 The next argument
        /// @return Reference to this object
        String & arg (
            const String & a1, const String & a2, const String & a3,
            const String & a4, const String & a5, const String & a6,
            const String & a7, const String & a8, const String & a9 );

        /// @brief Checks if string is empty.
        /// @return true if the string is empty (has length = 0); false otherwise.
        bool isEmpty() const;

        /// @brief Returns a pointer to internal C-string memory.
        /// This function returns pointer to internal memory segment,
        /// which is simply null-terminated C-String. It is possible, that there
        /// are NULL characters inside that C-String, which will make it look shorter
        /// than what length() function reports.
        /// @return Pointer to internal C-String.
        const char * c_str() const;

        /// @brief Returns string's length.
        /// This function returns number of characters in the string.
        /// It doesn't count terminating null character.
        /// @return String's length.
        int length() const;

        /// @brief Function determines whether a character is a whitespace or not
        /// @param [in] c The character to examine
        /// @return True if the character is a whitespace, false otherwise.
        static inline bool isSpace ( char c )
        {
            switch ( c )
            {
                case '\t':
                case '\n':
                case '\v':
                case '\f':
                case '\r':
                case ' ':
                    return true;
                    break;
            }

            return false;
        }

        /// @brief Function determines whether a character is a whitespace or 0, or not
        /// @param [in] c The character to examine
        /// @return True if the character is a whitespace or 0, false otherwise.
        static inline bool isSpaceOrNull ( char c )
        {
            return ( c == 0 || isSpace ( c ) );
        }

        /// @brief Reserves memory size for future appends.
        /// This reserves a given memory size for future appends.
        /// It might be useful if we know the expected size of the string
        /// beforehand, and after this call multiple, small appends that don't
        /// append more data than reserved won't cause any memory reallocations.
        /// If the string is not empty when this function is called, the total
        /// memory will be equal to the sum of current length and the size reserved.
        /// @param [in] size Memory size to be reserved.
        void reserve ( int size );

        /// @brief Releases unused memory.
        /// If the string uses more memory than it's actually needed,
        /// this function releases unused memory.
        void squeeze();

        /// @brief Clears the string.
        /// This function clears the string. It will have length = 0 after this call.
        /// This doesn't release memory allocated. To release any memory used, call
        /// squeeze() after calling this function.
        void clear();

        /// @brief Tests if a string matches a pattern.
        ///
        /// Patterns can contain any characters.
        /// It does not support full regexp, only very simplified patterns:
        /// '*' represents any characters (zero or more)
        /// '?' represents any character (one)
        /// All other characters (like '.') are treated as normal characters.
        /// There is no way to explicitly test for '*' or '?'
        /// characters in given string.
        /// @param [in] pattern Pattern to be used.
        /// @return True, if this string matches given pattern.
        inline bool matches ( const String & pattern ) const
        {
            return matches ( pattern.c_str(), c_str() );
        }

        /// @brief Tests if a string matches a pattern.
        ///
        /// For a description look at non-static version with String argument.
        /// @param [in] pattern pattern to be used.
        /// @param [in] str c-string that should be tested.
        /// @return True, if given string matches given pattern.
        static bool matches ( const char * pattern, const char * str );

        /// @brief Tests if a string matches a pattern.
        ///
        /// This function works the same as version with String argument.
        /// @param [in] pattern Pattern to be used.
        /// @return True, if this string matches given pattern.
        inline bool matches ( const char * pattern ) const
        {
            return matches ( pattern, c_str() );
        }

        /// @brief Returns a substring of the current string.
        /// @param [in] pos Index of first character to include.
        /// @param [in] n Number of characters to include. If < 0,
        /// the entire string (starting at pos) is be included.
        /// @return Requested substring.
        String substr ( int pos = 0, int n = -1 ) const;

        /// @brief Returns a lowercase copy of the string.
        ///
        /// @return Lowercase version of the string.
        String toLower() const;

        /// @brief Returns an uppercase copy of the string.
        ///
        /// @return Uppercase version of the string.
        String toUpper() const;

        /// @brief Returns a capitalized copy of the string.
        ///
        /// @return Capitalized version of the string.
        String capitalize() const;

        /// @brief Returns a copy of this string that has whitespace characters removed from the start and the end.
        ///
        /// Returns a copy of this string, that has '\\t', '\\n', '\\v', '\\f', '\\r', ' ' and NULL characters
        /// removed from the beginning and the end.
        /// @return Trimmed copy of the string.
        String trimmed() const;

        /// @brief Returns a reversed copy of the string.
        /// All the characters in returned string will be in the reverse order.
        /// @return Reversed copy of the string.
        String reversed() const;

        /// @brief Returns a copy of this string that is trimmed and has all whitespace characters inside changed to
        /// single spaces.
        ///
        /// Returns a copy of this string, that has '\\t', '\\n', '\\v', '\\f', '\\r', ' ' and NULL characters
        /// removed from the beginning and the end. Also all those characters (and their multiple occurrences)
        /// inside of the string changed to a single space.
        /// @return Simplified copy of the string.
        String simplified() const;

        /// @brief Returns a copy of this string that has all occurrences of specified characters removed.
        /// @note This function is always case-sensitive.
        /// @param [in] toRemove String with all characters that should be removed.
        /// @return Copy of the string with specified characters removed.
        String removeChars ( const char * toRemove ) const;

        /// @brief Returns a copy of this string that has all occurrences of one string replaced with the other one.
        ///
        /// Returns a copy of this string, that has each occurrence of 'what' string
        /// replaced with 'withWhat' string. If 'withWhat' is empty, all occurrences
        /// of 'what' are removed.
        /// @param [in] what String that should be replaced.
        /// @param [in] withWhat String that should be used instead.
        /// @return Copy of the string after replacements.
        String replace ( const char * what, const char * withWhat ) const;

        /// @brief Returns a copy of this string that has all occurrences of one string replaced with the other one.
        ///
        /// Returns a copy of this string, that has each occurrence of 'what' string
        /// replaced with 'withWhat' string. If 'withWhat' is empty, all occurrences
        /// of 'what' are removed.
        /// @param [in] what String that should be replaced.
        /// @param [in] withWhat String that should be used instead.
        /// @return Copy of the string after replacements.
        String replace ( const char * what, const String & withWhat ) const;

        /// @brief Searches for characters in string
        /// Searches the string for any of the characters that are part of str
        /// and returns the position of the first occurrence in the string.
        /// When pos is specified, the search starts at the character in this position.
        /// @param [in] str String containing characters to look for.
        /// @param [in] pos Starting position, 0 by default (entire string is searched).
        /// @return Returns a position of the character found, or -1 if nothing could be found.
        int findFirstOf ( const String & str, int pos = 0 ) const;

        /// @brief Searches (in reverse) for characters in string
        /// Searches the string for any of the characters that are part of str
        /// and returns the position of the last occurrence in the string.
        /// When pos is specified, the search starts at the character in this position (and the search moves backwards)
        /// @param [in] str String containing characters to look for.
        /// @param [in] pos Starting position, -1 by default (entire string is searched).
        /// @return Returns a position of the character found, or -1 if nothing could be found.
        int findLastOf ( const String & str, int pos = -1 ) const;

        /// @brief Checks if this string starts with a certain sequence
        /// @param [in] str string to look for at the beginning
        /// @param [in] caseSensitive if true (default), than the search is case-sensitive
        /// @return True if this string starts with a given sequence, false otherwise
        bool startsWith ( const String & str, bool caseSensitive = true ) const;

        /// @brief Checks if this string ends with a certain sequence
        /// @param [in] str string to look for at the end
        /// @param [in] caseSensitive if true (default), than the search is case-sensitive
        /// @return True if this string ends with a given sequence, false otherwise
        bool endsWith ( const String & str, bool caseSensitive = true ) const;

        /// @brief Finds first occurrence of a given string in this string
        /// Searches the string for the first occurrence of the given substring.
        /// When pos is specified, the search starts at the character in this position.
        /// @param [in] str String too look for.
        /// @param [in] pos Starting position, 0 by default (entire string is searched).
        /// @return Returns a position of the substring found, or -1 if nothing could be found.
        inline int find ( const String & str, int pos = 0 ) const
        {
            return find ( str.c_str(), str.length(), pos );
        }

        /// @brief Finds first occurrence of a given C-string in this string
        /// Searches the string for the first occurrence of the given substring.
        /// When pos is specified, the search starts at the character in this position.
        /// @param [in] str String to look for.
        /// @param [in] pos Starting position, 0 by default (entire string is searched).
        /// @return Returns a position of the substring found, or -1 if nothing could be found.
        inline int find ( const char * str, int pos = 0 ) const
        {
            return find ( str, ( int ) strlen ( str ), pos );
        }

        /// @brief Finds first occurrence of a given character in this string
        /// Searches the string for the first occurrence of the given character.
        /// When pos is specified, the search starts at the character in this position.
        /// @param [in] c Character to look for.
        /// @param [in] pos Starting position, 0 by default (entire string is searched).
        /// @return Returns a position of the substring found, or -1 if nothing could be found.
        int find ( char c, int pos = 0 ) const;

        /// @brief Splits the string into substrings wherever one of separators occurs.
        ///
        /// This function splits the string into a list of substrings
        /// wherever one of characters in 'separators' string occurs.
        /// @param [in] separators Separators to be used. Each character in this string
        ///             is treated as a separate separator.
        /// @param [in] keepEmpty Should empty substrings be included in the list
        /// @return Generated list of substrings.
        inline StringList split ( const String & separators, bool keepEmpty = false ) const
        {
            return toStringList ( c_str(), length(), separators, keepEmpty );
        }

        /// @brief Puts all elements of StringList into one string separated with a separator.
        ///
        /// @param [in] strList StringList to join.
        /// @param [in] separator Separator to be used.
        /// @return Joined representation of the given string list.
        static String join ( const StringList & strList, const String & separator = "," );

        /// @brief Puts all elements of List of numbers into one string separated with a separator.
        ///
        /// @param [in] list List of numbers to join.
        /// @param [in] separator Separator to be used.
        /// @return Joined representation of the given list of numbers.
        template<typename T> static String join ( const List<T> & l, const String & separator )
        {
            String ret;

            for ( size_t i = 0; i < l.size(); ++i )
            {
                if ( i > 0 )
                {
                    ret.append ( separator );
                }

                ret.append ( String::number ( l.at ( i ) ) );
            }

            return ret;
        }

        /// @brief Converts string to a bool value
        /// Converts this string to a boolean value. True boolean values can be represented by
        /// any of: 1, on, yes, true (in lower/upper/mixed case). False values can be represented
        /// by any of: 0, off, no, false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @return Converted value.
        bool toBool ( bool * ok = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( bool & value ) const
        {
            bool ok = false;
            const bool tmp = toBool ( &ok );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to a long value
        /// Converts this string to a long value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        long toLong ( bool * ok = 0, int base = 0 ) const;

        /// @brief Converts string to an unsigned long value
        /// Converts this string to a unsigned long value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        unsigned long toULong ( bool * ok = 0, int base = 0 ) const;

        /// @brief Converts string to a long long value
        /// Converts this string to a long long value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        long long toLongLong ( bool * ok = 0, int base = 0 ) const;

        /// @brief Converts string to an unsigned long long value
        /// Converts this string to a unsigned long long value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        unsigned long long toULongLong ( bool * ok = 0, int base = 0 ) const;

        /// @brief Converts string to a float value
        /// Converts this string to a float value.
        /// The  expected  form  of  the string is optional leading white space, an optional plus ('+')
        /// or minus sign ('-') and then either a decimal number, or a hexadecimal number, or an infinity,
        /// or a NAN (not-a-number).
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @return Converted value.
        float toFloat ( bool * ok = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( float & value ) const
        {
            bool ok = false;
            const float tmp = toFloat ( &ok );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to a double value
        /// Converts this string to a double value.
        /// The  expected  form  of  the string is optional leading white space, an optional plus ('+')
        /// or minus sign ('-') and then either a decimal number, or a hexadecimal number, or an infinity,
        /// or a NAN (not-a-number).
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @return Converted value.
        double toDouble ( bool * ok = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( double & value ) const
        {
            bool ok = false;
            const double tmp = toDouble ( &ok );
            ok && ( value = tmp );
            return ok;
        }

//    /// @brief Converts string to a long double value
//    /// Converts this string to a long double value.
//    /// The  expected  form  of  the string is optional leading white space, an optional plus ('+')
//    /// or minus sign ('-') and then either a decimal number, or a hexadecimal number, or an infinity,
//    /// or a NAN (not-a-number).
//    /// @param [out] ok If used, the result of a conversion is stored there.
//    /// @return Converted value.
//    long double toLongDouble ( bool * ok = 0 ) const;

        /// @brief Converts string to a signed int8 value
        /// Converts this string to an int8_t value using base base, which is 10 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        int8_t toInt8 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( int8_t & value, int base = 0 ) const
        {
            bool ok = false;
            const int8_t tmp = toInt8 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to uint8_t value
        /// Converts this string to an unsigned int value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        uint8_t toUInt8 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( uint8_t & value, int base = 0 ) const
        {
            bool ok = false;
            const uint8_t tmp = toUInt8 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to a signed int16 value
        /// Converts this string to an int16_t value using base base, which is 10 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        int16_t toInt16 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( int16_t & value, int base = 0 ) const
        {
            bool ok = false;
            const int16_t tmp = toInt16 ( &ok, base );
            /// @warning If there is a third digit after %, the behaviour is undefined.
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to uint16_t value
        /// Converts this string to an unsigned int value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        uint16_t toUInt16 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( uint16_t & value, int base = 0 ) const
        {
            bool ok = false;
            const uint16_t tmp = toUInt16 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to int32_t value
        /// Converts this string to an int value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        int32_t toInt32 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( int32_t & value, int base = 0 ) const
        {
            bool ok = false;
            const int32_t tmp = toInt32 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to uint32_t value
        /// Converts this string to an unsigned int value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        uint32_t toUInt32 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( uint32_t & value, int base = 0 ) const
        {
            bool ok = false;
            const uint32_t tmp = toUInt32 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to a signed int64 value
        /// Converts this string to an int64_t value using base base, which is 10 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' or '-' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        int64_t toInt64 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( int64_t & value, int base = 0 ) const
        {
            bool ok = false;
            const int64_t tmp = toInt64 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

        /// @brief Converts string to uint64_t value
        /// Converts this string to an unsigned int value using base base, which is 0 by default.
        /// The string may begin with an arbitrary amount of white space followed by a single optional '+' sign.
        /// If base is zero or 16, the string may then include a "0x" prefix, and the number will be read in base 16;
        /// otherwise, a zero base is taken as 10 (decimal) unless the next character is '0', in which case it is
        /// taken as 8 (octal). In case of overflow, the minimal (maximal) possible value is returned,
        /// and ok set to false.
        /// @param [out] ok If used, the result of a conversion is stored there.
        /// @param [in] base Base for the conversion. It must be between 2 and 36 inclusive, or 0.
        /// @return Converted value.
        uint64_t toUInt64 ( bool * ok = 0, int base = 0 ) const;

        /// @brief A helper function to convert this string to a number given as output parameter and return the status
        /// @param [out] value Converted value; In case of error it is not modified
        /// @param [in] base Base for the conversion
        /// @return True if the conversion was successful; False if it failed
        inline bool toNumber ( uint64_t & value, int base = 0 ) const
        {
            bool ok = false;
            const uint64_t tmp = toUInt64 ( &ok, base );
            ok && ( value = tmp );
            return ok;
        }

#if defined( SYSTEM_APPLE ) && !defined( PLATFORM_IOS )
        /// @brief A helper function to convert this string to a number given as output parameter and return the status.
        /// @note This is only available on MacOS.
        /// On all other platforms, size_t is the same type as either uint32_t or uint64_t.
        /// @param [out] value Converted value; In case of error it is not modified.
        /// @param [in] base Base for the conversion.
        /// @return True if the converesion was successful; False if it failed.
        inline bool toNumber ( size_t & value, int base = 0 ) const
        {
            bool ok = false;
            const uint64_t tmp64 = toUInt64 ( &ok, base );
            const size_t tmpSize = ( size_t ) tmp64;

            // If size_t is smaller than uint64_t on this platform, the result might not fit in size_t.
            if ( !ok || ( uint64_t ) tmpSize != tmp64 )
            {
                return false;
            }

            value = tmpSize;
            return true;
        }
#endif

        /// @brief Styles of integer conversion
        /// They correspond to conversion types used by sprintf
        enum IntStyle
        {
            Int_Dec = 0, ///< Decimal conversion (signed or not, depending on the type)
            Int_Oct = 1, ///< Octal conversion
            Int_Hex = 2, ///< Hexadecimal conversion with lowercase letters
            Int_HEX = 3 ///< Hexadecimal conversion with uppercase letters
        };

        /// @brief Styles of floating point conversion
        /// They correspond to conversion types used by sprintf
        enum DoubleStyle
        {
            Double_f = 0, ///< [-]ddd.ddd, [-]inf or [-]infinity, nan, etc.
            Double_F = 1, ///< [-]ddd.ddd, [-]INF or [-]INFINITY, NAN, etc.
            Double_e = 2, ///< [-]d.ddde±dd
            Double_E = 3, ///< [-]d.dddE±dd
            Double_g = 4, ///< Depending on the number and precision, it's either f or e type
            Double_G = 5, ///< Depending on the number and precision, it's either F or E type
            Double_a = 6, ///< [-]0xh.hhhhp±d, lowercase letters
            Double_A = 7 ///< [-]0Xh.hhhhp±d, uppercase letters
        };

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            char num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            short num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            long int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            long long int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            unsigned char num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            unsigned short int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            unsigned int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            unsigned long int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Int_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the number
        static String number (
            unsigned long long int num, IntStyle style = Int_Dec,
            int width = 0, bool zeroFill = false );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Double_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @param [in] precision Precision of the conversion. 0 for default.
        /// @return String representation of the number
        static String number (
            double num, DoubleStyle style = Double_g,
            int width = 0, bool zeroFill = false, int precision = 0 );

        /// @brief Converts a number to a string.
        /// @param [in] num Number to convert
        /// @param [in] style Style of representation. One of Double_*
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @param [in] precision Precision of the conversion. 0 for default.
        /// @return String representation of the number
        static String number (
            long double num, DoubleStyle style = Double_g,
            int width = 0, bool zeroFill = false, int precision = 0 );

        /// @brief Converts a boolean value to a string.
        /// @param [in] num Boolean value to convert
        /// @return String representation of the number
        static String number ( bool num );

        /// @brief Converts a pointer address to a string.
        /// @param [in] ptr The pointer to convert.
        /// @param [in] style Style of representation. One of Int_*.
        /// @param [in] width Width of the field. 0 is no fixed width.
        /// @param [in] zeroFill If true, the field is filled with '0' characters.
        /// @return String representation of the pointer's address.
        template<typename T> inline static String number (
            const T * ptr, IntStyle style = Int_HEX, int width = 0, bool zeroFill = true )
        {
            return number ( ( size_t ) ptr, style, width, zeroFill );
        }

        /// @brief Generates a two-number hex description of the value
        /// @param [out] buf The 2 character buffer where the hex description will be stored.
        ///                  (it is NOT null-terminated)!
        /// @param [in] num The number to convert
        /// @param [in] useLowercase If true, lowercase letters will be used
        /// @return Null-terminated two letter HEX description of the number
        static void hexDesc ( char buf[ 2 ], uint8_t num, bool useLowercase = false );

        /// @brief Returns a hex dump of a data segment
        /// @param [in] data The pointer to data
        /// @param [in] dataSize The size of the data
        /// @param [in] include0x Should the '0x' be included before each value or not
        /// @param [in] separator The string used as the hex separator
        /// @param [in] useLowercase If true, lowercase letters will be used
        /// @return String with the hex dump of the data
        static String hexDump (
            const char * data, size_t dataSize,
            bool include0x = true, const String & separator = " ",
            bool useLowercase = false );

        /// @brief Splits the content of the memory into a list of strings wherever one of separators occurs
        ///
        /// This function splits the content of the memory into a list of strings
        /// wherever one of characters in 'separators' string occurs.
        /// @param [in] data The pointer to the data to be split
        /// @param [in] size The size of the data
        /// @param [in] separators Separators to be used. Each character in this string
        ///             is treated as a separate separator.
        /// @param [in] keepEmpty Should empty strings be included in the list
        /// @return Generated list of strings.
        static StringList toStringList (
            const char * data, size_t size,
            const String & separators, bool keepEmpty = false );

        /// @brief Returns a value of reference counter.
        /// This is probably useful only for debugging.
        /// @return Value of reference counter associated with shared data.
        int getRefCount() const;

    protected:
        /// @brief Pointer to the private data of the string.
        /// StringPriv includes reference counter so the memory can be shared between
        /// multiple String objects.
        StringPriv * _priv;

        /// @brief A helper function for checking the amount of memory actually allocated.
        /// Mostly for testing/debugging.
        int getAllocatedSize() const;

        /// @brief Ensures that private data exists and is not shared with other strings.
        /// This function will check ifthe string contains private data. If not,
        /// private data object is created. Otherwise, private copy of existing data
        /// is created (only if it is shared). It also might reserve memory for future
        /// appends. Memory reservation is performed only if current private data
        /// is shared AND contains some characters. Otherwise memory is not preallocated.
        /// @param [in] additionalSize How much memory (on top of any characters used)
        ///  should be reserved (if current memory is not empty and shared).
        void ensureOwnCopy ( int additionalSize );

        /// @brief Compares two C-strings.
        ///
        /// @param [in] str1 First string.
        /// @param [in] len1 Length of the first string.
        /// @param [in] str2 Second string.
        /// @param [in] len2 Length of the second string.
        /// @param [in] caseSensitive If false strings will be compared ignoring the case.
        /// @param [in] len Number of characters to compare. It has to be valid!
        /// @return -1, 0, 1 if this string is less, equal or greater than the second one
        /// (ignoring the case or not).
        static int compare (
            const char * str1, int len1,
            const char * str2, int len2,
            bool caseSensitive, int len );

        /// @brief Finds first occurrence of a given string in this string
        /// Searches the string for the first occurrence of the given substring (given by
        /// pointer to the memory and its length).
        /// When pos is specified, the search starts at the character in this position.
        /// @param [in] str String too look for.
        /// @param [in] sLen Length of the given string.
        /// @param [in] pos Starting position, 0 by default (entire string is searched).
        /// @return Returns a position of the substring found, or -1 if nothing could be found.
        int find ( const char * str, int sLen, int pos ) const;

        /// @brief Replaces occurrences of %1 %2 %3 ... %99 in the string with appropriate entries in the list.
        /// If the list is too short, argument numbers that cannot be replaced will be decremented (by the list size).
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] strList List of replacements
        /// @return Reference to this object (for chaining)
        String & replaceArgs ( const StringList & strList );

        /// @brief Returns a copy of this string that has all occurrences of one string replaced with the other one.
        ///
        /// Returns a copy of this string, that has each occurrence of 'what' string
        /// replaced with 'withWhat' string. If 'withWhat' is empty, all occurrences
        /// of 'what' are removed.
        /// @param [in] what String that should be replaced.
        /// @param [in] wLen The length of 'what' string.
        /// @param [in] withWhat String that should be used instead.
        /// @param [in] withWhatLen The length of 'withWhat' string.
        /// @return Copy of the string after replacements.
        String replace ( const char * what, int wLen, const char * withWhat, int wwLen ) const;

        /// @brief Works like replaceArgs, but takes the number and several optional parameters
        /// @warning If there is a third digit after %, the behaviour is undefined.
        /// @param [in] numValues Number of values to use
        /// @param [in] a1 The first argument
        /// @param [in] a2 The next argument
        /// @param [in] a3 The next argument
        /// @param [in] a4 The next argument
        /// @param [in] a6 The next argument
        /// @param [in] a7 The next argument
        /// @param [in] a8 The next argument
        /// @param [in] a9 The next argument
        /// @return Reference to this object (for chaining)
        String & replaceArgs (
            int numValues,
            const String & a1,
            const String & a2 = String::EmptyString,
            const String & a3 = String::EmptyString,
            const String & a4 = String::EmptyString,
            const String & a5 = String::EmptyString,
            const String & a6 = String::EmptyString,
            const String & a7 = String::EmptyString,
            const String & a8 = String::EmptyString,
            const String & a9 = String::EmptyString );
};

template<typename V> class HashMap<String, V>: public HashMapImpl<String, V>
{
    public:
        /// @brief Constant iterator over all hash map's elements that match the pattern given
        /// Very similar to the regular Iterator, but skips the elements that do not match the pattern provided
        class MatchIterator: public HashMapImpl<String, V>::Iterator
        {
            public:
                /// @brief Creates a constant pattern iterator over given HashMap
                /// @param [in] hMap HashMap to iterate over
                /// @param [in] pattern The pattern to be used. All keys not matching this pattern
                ///                      (using String::matches() method) will be skipped.
                MatchIterator ( const HashMapImpl<String, V> & hMap, const String & pattern );

                /// @brief Advances the iterator.
                /// It behaves the same way as the next() function from the regular Iterator,
                /// but it skips entries that do not match given pattern.
                /// @return True if the current key/value can be retrieved, false otherwise.
                bool next();

            protected:
                String _pattern; ///< The pattern to use
        };

        /// @brief MutableIterator over all hash map's elements.
        /// Similar to MatchIterator, but allows HashMap and values to be modified
        class MatchMutableIterator: public HashMapImpl<String, V>::MutableIterator
        {
            public:
                /// @brief Creates an iterator over given HashMap
                /// @param [in] hMap HashMap to iterate over
                /// @param [in] pattern The pattern to be used. All keys not matching this pattern
                ///                      (using String::matches() method) will be skipped.
                MatchMutableIterator ( HashMapImpl<String, V> & map, const String & pattern );

                /// @brief Removes current value and advances the iterator
                /// It behaves the same way as the remove() function from the regular MutableIterator,
                /// but it skips entries that do not match given pattern
                /// (it doesn't remove them, just ignores them)
                /// @return True if the new value is valid, false otherwise.
                bool remove();

                /// @brief Advances the iterator.
                /// It behaves the same way as the next() function from the regular MutableIterator,
                /// but it skips entries that do not match given pattern.
                bool next();

            protected:
                String _pattern; ///< The pattern to use
        };

        /// @brief Default constructor.
        /// Creates an empty HashMap.
        inline HashMap()
        {
        }

        /// @brief Copy constructor.
        /// The HashMap uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashMap objects sharing
        /// data needs to modify it.
        /// @param [in] other HashMap object to copy.
        inline HashMap ( const HashMapImpl<String, V> & other ): HashMapImpl<String, V>::HashMapImpl ( other )
        {
        }

        /// @brief Assignment operator.
        /// The HashMap uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashMap objects sharing
        /// data needs to modify it.
        /// @param [in] other HashMap object to copy.
        /// @return Reference to this HashMap.
        inline HashMap & operator= ( const HashMapImpl<String, V> & other )
        {
            HashMapImpl<String, V>::operator= ( other );
            return *this;
        }

        /// @brief Returns constant pattern iterator
        /// It is a helper function, "MatchIterator iter ( hashMap, keyPattern );" can be used instead
        /// @param [in] pattern The pattern to be used. All keys not matching this pattern
        ///                      (using String::matches() method) will be skipped.
        /// @return Constant pattern iterator over this HashMap configured to use the pattern provided
        inline MatchIterator findPattern ( const String & pattern ) const
        {
            return MatchIterator ( *this, pattern );
        }

        /// @brief Returns pattern iterator
        /// It is a helper function, "MatchMutableIterator iter ( hashMap, keyPattern );" can be used instead
        /// @param [in] pattern The pattern to be used. All keys not matching this pattern
        ///                      (using String::matches() method) will be skipped.
        /// @return Pattern iterator over this HashMap configured to use the pattern provided
        inline MatchMutableIterator findPattern ( const String & pattern )
        {
            return MatchMutableIterator ( *this, pattern );
        }

        /// @brief Returns the list of keys matching given pattern
        ///
        /// Default pattern returns all keys. This method can be useful for small HashMap,
        /// but is significantly less efficient than the MatchMutableIterator and MatchIterator methods.
        /// Especially for map objects with large number of keys iterator methods should be used instead!
        ///
        /// @param [in] pattern The pattern used while searching for key names.
        ///             To match keys to the pattern String::matches() function is used.
        /// @return StringList of keys that match given pattern.
        StringList getKeys ( const String & pattern = "*" ) const;

        /// @brief Removes entries with matching keys
        ///
        /// @param [in] pattern The pattern used while searching for key names.
        ///             To match keys to the pattern String::matches() function is used.
        /// @return Number of entries removed
        size_t removePattern ( const String & pattern );
};

template<typename V> HashMap<String, V>::MatchIterator::MatchIterator (
        const HashMapImpl<String, V> & hMap, const String & pattern ):
    HashMapImpl<String, V>::Iterator ( hMap ),
    _pattern ( pattern )
{
    while ( HashMapImpl<String, V>::Iterator::isValid()
            && !HashMapImpl<String, V>::Iterator::key().matches ( _pattern ) )
    {
        HashMapImpl<String, V>::Iterator::next();
    }
}

template<typename V> bool HashMap<String, V>::MatchIterator::next()
{
    while ( HashMapImpl<String, V>::Iterator::next() )
    {
        if ( HashMapImpl<String, V>::Iterator::key().matches ( _pattern ) )
            return true;
    }

    return false;
}

template<typename V> HashMap<String, V>::MatchMutableIterator::MatchMutableIterator (
        HashMapImpl<String, V> & hMap, const String & pattern ):
    HashMapImpl<String, V>::MutableIterator ( hMap ),
    _pattern ( pattern )
{
    while ( HashMapImpl<String, V>::MutableIterator::isValid()
            && !HashMapImpl<String, V>::MutableIterator::key().matches ( _pattern ) )
    {
        HashMapImpl<String, V>::MutableIterator::next();
    }
}

template<typename V> bool HashMap<String, V>::MatchMutableIterator::next()
{
    while ( HashMapImpl<String, V>::MutableIterator::next() )
    {
        if ( HashMapImpl<String, V>::MutableIterator::key().matches ( _pattern ) )
            return true;
    }

    return false;
}

template<typename V> bool HashMap<String, V>::MatchMutableIterator::remove()
{
    // If we remove (or not) something, and the new element is invalid, there is nothing else to do!
    if ( !HashMapImpl<String, V>::MutableIterator::remove() )
        return false;

    // Otherwise we are pointing at the next element, which may, or may not, match our pattern
    do
    {
        // If the element (either selected by the remove() above, or next() in the while loop)
        // matches the pattern - we're done.
        if ( HashMapImpl<String, V>::MutableIterator::key().matches ( _pattern ) )
            return true;

        // Otherwise we need to iterate to the next one.
    }
    while ( HashMapImpl<String, V>::MutableIterator::next() );

    return false;
}

template<typename V> StringList HashMap< String, V >::getKeys ( const String & pattern ) const
{
    StringList ret;

    for ( MatchIterator it ( *this, pattern ); it.isValid(); it.next() )
    {
        if ( it.key().matches ( pattern ) )
            ret.append ( it.key() );
    }

    return ret;
}

template<typename V> size_t HashMap< String, V >::removePattern ( const String & pattern )
{
    MatchMutableIterator it ( *this, pattern );
    size_t ret = 0;

    while ( it.isValid() )
    {
        if ( it.key().matches ( pattern ) )
        {
            ++ret;
            it.remove();
        }
        else
        {
            it.next();
        }
    }

    return ret;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const String & key );

DECLARE_EMBEDDED_TYPE_INFO ( String, TYPE_CONF_STD_SHARED );
}
