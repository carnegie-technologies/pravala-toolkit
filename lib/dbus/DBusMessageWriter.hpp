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

#include "DBusMessage.hpp"

namespace Pravala
{
class DBusMessageReader;

/// @brief A wrapper class around a D-Bus message for writing.
/// @note This only supports writing basic types, and dictionaries of the form a{sv} where v is a basic type
/// wrapped in a variant.
class DBusMessageWriter: public DBusMessage
{
    public:
        /// @brief Constructor for a request message.
        /// @param [in] destination The name of the destination this message should be sent to.
        /// @param [in] path The object path to put in the message.
        /// @param [in] iface The name of the interface to put in the message.
        /// @param [in] method The name of the method to put in the message.
        DBusMessageWriter (
            const String & destination,
            const String & path,
            const String & iface,
            const String & method );

        /// @brief Constructor for a response message.
        /// @param [in] request The request message we're replying to
        DBusMessageWriter ( DBusMessageReader & request );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( bool value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( char value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( int8_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( uint8_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( int16_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( uint16_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( int32_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( uint32_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( int64_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( uint64_t value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( double value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendArg ( const char * value, bool asVariant = false );

        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        inline bool appendArg ( const String & value, bool asVariant = false )
        {
            return appendArg ( value.c_str(), asVariant );
        }

        /// @brief Appends an object path to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        bool appendObjectPath ( const String & value, bool asVariant = false );

        /// @brief Append a dictionary entry.
        /// The value will automatically be wrapped in a variant.
        /// @note Can only be used after a dictionary has been opened (and has not been closed yet).
        /// @param [in] key The key to use for this dictionary entry.
        /// @param [in] value The value to store for the key.
        /// @return True on success; False on failure.
        /// @note If this fails, the message may become invalidated and should be checked with isValid()
        /// and must be recreated from scratch.
        template<typename T> inline bool appendDictEntry ( const String & key, const T & value )
        {
            if ( !openDictEntry ( key ) )
            {
                return false;
            }

            _allowBasicAppend = true;

            appendArg ( value, true );

            _allowBasicAppend = false;

            return closeDictEntry();
        }

        /// @brief Begin writing a dictionary.
        /// After this call, only appendDictEntry can be used, until closeDictionary is called.
        /// This fails if a dictionary is already open.
        /// @return True on success; False on failure.
        bool openDictionary();

        /// @brief Finish writing a dictionary.
        /// This fails if a dictionary was not open, or if the DBus library could not allocate memory.
        /// @return True on success; False on failure.
        /// @note If this fails, the message may become invalidated and should be checked with isValid()
        /// and must be recreated from scratch.
        bool closeDictionary();

        /// @brief Returns the serial number of the message this message is a response to.
        /// @return The serial number of the message this message is a response to, or 0 if it is unset.
        uint32_t getReplySerial() const;

        /// @brief Whether or not this message is valid. May become invalid after some container operations.
        /// @return True if this message is valid; False otherwise.
        inline bool isValid() const
        {
            return _valid;
        }

    private:
        /// @brief Appends an argument to the list of arguments.
        /// On success it also advances internal argument iterator.
        /// @param [in] type D-Bus' type identifier.
        /// @param [in] typeSig This type's DBus signature (needed for variants)
        /// @param [in] typeDesc This type's description (used for logging).
        /// @param [in] value The value to append.
        /// @param [in] asVariant Whether or not the value should be wrapped in a variant
        /// @tparam T The type of the argument.
        /// @note When using 'asVariant', the message may become invalid on failure,
        /// and should be checked with isValid()
        /// @return True on success; False on failure.
        template<typename T> bool appendArg (
            int type,
            const char * typeSig,
            const char * typeDesc,
            const T & value,
            bool asVariant );

        /// @brief Open a dictionary entry and add the key.
        /// @param [in] key The key to use for this dictionary entry.
        /// @return True on success; False on failure.
        bool openDictEntry ( const String & key );

        /// @brief Close a dictionary entry.
        /// @return True on success; False on failure.
        bool closeDictEntry();

        /// @brief The mode to use for appending
        enum AppendMode
        {
            Basic, ///< Only allow appending basic types (i.e. appendArg)
            DictEntry ///< Only allow appending dictionary entries (i.e. appendDictEntry)
        };

        AppendMode _appendMode; ///< The current append mode

        /// @brief If true, the basic appendArg functions can be used even in DictEntry mode.
        /// This is used so that internally we can still use the appendArg functions.
        bool _allowBasicAppend;

        /// @brief If false, the message is not valid and must be discarded.
        /// This only happens if closing a container fails, or if a failure occurred in the constructor.
        bool _valid;

        friend class DBusConnection;
};

/// @brief Streaming operator
/// Appends a DBusMessage's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value DBusMessage for which description should be appended
/// @return Reference to the TextMessage object
inline TextMessage & operator<< ( TextMessage & textMessage, const DBusMessageWriter & value )
{
    return textMessage << ( ( const DBusMessage & ) value );
}
}
