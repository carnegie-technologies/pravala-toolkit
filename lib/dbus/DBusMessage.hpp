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

#include "basic/NoCopy.hpp"
#include "log/TextLog.hpp"

namespace DBusC
{
struct DBusMessage;
struct DBusMessageIter;
}

namespace Pravala
{
/// @brief A wrapper class around a D-Bus message.
/// This is used as a base for the reader and writer classes, and should not be used directly.
class DBusMessage: public NoCopy
{
    public:
        /// @brief Possible message types
        enum Type
        {
            TypeInvalid = 0,       ///< Invalid message
            TypeMethodCall = 1,    ///< Request message
            TypeMethodReturn = 2,  ///< Response message
            TypeError = 3,         ///< Error message
            TypeSignal = 4         ///< Signal message
        };

        /// @brief Destructor.
        virtual ~DBusMessage();

        /// @brief Exposes the destination stored in this message.
        /// @return The destination stored in the message.
        inline const String & getDestination() const
        {
            return _dest;
        }

        /// @brief Exposes the sender stored in this message.
        /// @return The sender stored in the message.
        inline const String & getSender() const
        {
            return _sender;
        }

        /// @brief Exposes the path stored in this message.
        /// @return The path in the message.
        inline const String & getPath() const
        {
            return _path;
        }

        /// @brief Exposes the interface name stored in this message.
        /// @return The interface name in the message.
        inline const String & getInterface() const
        {
            return _iface;
        }

        /// @brief Exposes the method name stored in this message.
        /// @return The method name in the message.
        inline const String & getMethod() const
        {
            return _method;
        }

        /// @brief Returns the type of this message.
        /// @return The type of this message.
        Type getType() const;

        /// @brief Returns the name of this message's type.
        /// @return The name of this message's type.
        inline const char * getTypeName() const
        {
            return getTypeName ( getType() );
        }

        /// @brief Checks if this is a signal message.
        /// @return True if this is a signal message; False otherwise.
        inline bool isSignal() const
        {
            return ( getType() == TypeSignal );
        }

        /// @brief Checks if this is an error message.
        /// @return True if this is an error message; False otherwise.
        inline bool isError() const
        {
            return ( getType() == TypeError );
        }

        /// @brief Checks if this is a method call message.
        /// @return True if this is a method call message; False otherwise.
        inline bool isMethodCall() const
        {
            return ( getType() == TypeMethodCall );
        }

        /// @brief Checks if this is a method call response message.
        /// @return True if this is a method call response message; False otherwise.
        inline bool isMethodReturn() const
        {
            return ( getType() == TypeMethodReturn );
        }

        /// @brief Returns the serial number of this message.
        /// @return The serial number of this message, or 0 if it is unset.
        uint32_t getSerial() const;

        /// @brief Generates a string with description of all the arguments stored in this message.
        /// It doesn't affect the iterator used by getNextArg() methods.
        /// @return A string with description of all the arguments stored in this message.
        String dumpArgs() const;

        /// @brief Appends the description of this message to provided buffer.
        /// @param [in] toBuf The buffer to append to. It is not cleared.
        virtual void describe ( Buffer & toBuf ) const;

        /// @brief Converts a message type value to this type's name.
        /// @param [in] type The type value to convert.
        /// @return A name of the message type.
        static const char * getTypeName ( Type type );

        /// @brief Checks if the given string is a legal interface name.
        /// This should also be used for error names, since the rules are the same.
        /// @param [in] name The name to check.
        /// @return True if the given string is a valid interface name; False otherwise.
        static bool isLegalInterfaceName ( const String & name );

        /// @brief Checks if the given string is a legal member name.
        /// @param [in] name The name to check.
        /// @return True if the given string is a valid member name; False otherwise.
        static bool isLegalMemberName ( const String & name );

    protected:
        static TextLog _log; ///< Log stream.

        /// @brief Constructor
        /// @param [in] destination The name of the destination of this message.
        /// @param [in] sender The name of the sender of this message.
        /// @param [in] path The object path of this message.
        /// @param [in] iface The name of the interface of this message.
        /// @param [in] method The name of the method of this message.
        DBusMessage (
            const String & destination,
            const String & sender,
            const String & path,
            const String & iface,
            const String & method );

        const String _dest;   ///< Destination of this message.
        const String _sender; ///< Sender of this message.
        const String _path;   ///< Path of this message.
        const String _iface;  ///< Interface name of this message.
        const String _method; ///< Method name of this message.

        /// @brief Internal argument iterator stack used for reading or appending arguments
        List<DBusC::DBusMessageIter *> _iters;

        DBusC::DBusMessage * _msg; ///< Internal D-Bus message pointer.

        /// @brief Exposes the internal D-Bus message pointer.
        inline DBusC::DBusMessage * getInternalMessage()
        {
            return _msg;
        }

        friend class DBusConnection;
};

/// @brief Streaming operator
/// Appends a DBusMessage's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value DBusMessage for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const DBusMessage & value );
}
