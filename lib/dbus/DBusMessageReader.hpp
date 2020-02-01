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
/// @brief A wrapper class around a D-Bus message for reading.
class DBusMessageReader: public DBusMessage
{
    public:
        /// @brief Determines if there are any more arguments to read.
        /// Checks the current container or, if we are not in a container, checks the top level message
        /// @return True if there are more arguments to read; False otherwise
        bool hasNextArg();

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( bool & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( char & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( int8_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( uint8_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( int16_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( uint16_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( int32_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( uint32_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( int64_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( uint64_t & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( double & value );

        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArg ( String & value );

        /// @brief Retrieves the next argument from this message, which must be an object path.
        /// On success it also advances internal argument iterator.
        /// @param [out] value The value retrieved. Only set on success.
        /// @return True on success; False on failure.
        bool getNextArgObjectPath ( String & value );

        /// @brief Open the container at the current iterator position.
        /// After this call, getNextArg will read arguments from inside the container.
        /// @return True on success; False if the next argument is not a container.
        bool openContainer();

        /// @brief Close the most recently opened container.
        /// After this call, getNextArg will read arguments right after the container
        /// (even if there were more unread arguments in the container that was closed).
        /// @return True if a container was closed and there are more arguments to read; False otherwise.
        bool closeContainer();

        /// @brief Resets the internal argument iterator.
        /// If any containers are open, they will be closed and the next getNextArg() call will return the first
        /// argument of the entire message.
        void restartArgReadIterator();

    protected:
        /// @brief Constructor.
        /// @param [in] msg The D-Bus message to wrap. The reference counter will be incremented. It CANNOT be NULL.
        DBusMessageReader ( DBusC::DBusMessage * msg );

    private:
        /// @brief Retrieves the next argument from this message.
        /// On success it also advances internal argument iterator.
        /// @param [in] type D-Bus' type identifier.
        /// @param [in] typeDesc This type's description (used for logging).
        /// @param [out] value The value to get.
        /// @tparam T The type of the argument.
        /// @return True on success; False on failure.
        template<typename T> bool getNextArg ( int type, const char * typeDesc, T & value );

        friend class DBusMessageWriter;
        friend class DBusConnection;
};

/// @brief Streaming operator
/// Appends a DBusMessage's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value DBusMessage for which description should be appended
/// @return Reference to the TextMessage object
inline TextMessage & operator<< ( TextMessage & textMessage, const DBusMessageReader & value )
{
    return textMessage << ( ( const DBusMessage & ) value );
}
}
