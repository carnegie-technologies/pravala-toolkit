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

#include "log/TextMessage.hpp"

namespace Pravala
{
class DBusErrorWrapper;

/// @brief Carries a name and a message of a DBus error.
class DBusError
{
    public:
        /// @brief Configures this object based on the DBusErrorWrapper.
        /// @param [in] err The wrapper object to take the values from.
        void set ( const DBusErrorWrapper & err );

        /// @brief Checks if this object contains an error.
        /// @return True if this object contains an error; False otherwise.
        inline bool isSet() const
        {
            return ( !_name.isEmpty() || !_message.isEmpty() );
        }

        /// @brief Returns the name of the error.
        /// @return The name of the error.
        inline const String & getName() const
        {
            return _name;
        }

        /// @brief Returns the error message.
        /// @return The error message.
        inline const String & getMessage() const
        {
            return _message;
        }

    private:
        String _name; ///< The name of the error.
        String _message; ///< The error message.
};

/// @brief Streaming operator
/// Appends DBusError's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value DBusError for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const DBusError & value );
}
