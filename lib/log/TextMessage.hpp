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

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <guiddef.h>
}
#endif

#include "error/Error.hpp"

#include "auto/log/Log/TextMessage.hpp"

namespace Pravala
{
class Time;
class IpAddress;
class IpSubnet;
class ExtProtoError;

union SockAddr;

/// @brief A wrapper around Log::TextMessage to make appending values more efficient
class TextMessage: public Log::TextMessage
{
    public:

        /// @brief Sets the value of 'errorCode'
        /// This version is used for mapping from ProtoError values.
        /// This field is [OPTIONAL].
        /// @param [in] newValue The value to set
        /// @return reference to this class (for chaining)
        inline Pravala::Log::TextMessage & setErrorCode ( const Pravala::ProtoError & newValue )
        {
            return Log::TextMessage::setErrorCode ( ERRCODE::protoToErrCode ( newValue.getCode() ) );
        }

        /// @brief Sets the value of 'errorCode'
        /// This version is used for the auto-generated ErrorCode values.
        /// This field is [OPTIONAL].
        /// @param [in] newValue The value to set
        /// @return reference to this class (for chaining)
        inline Pravala::Log::TextMessage & setErrorCode ( const ErrorCode & newValue )
        {
            return Log::TextMessage::setErrorCode ( newValue );
        }

        /// @brief Streaming operator
        /// Appends a null-terminated string to this buffer
        /// @param [in] value Null-terminated string to append.
        /// @return reference to the stream object.
        inline TextMessage & operator<< ( const char * value )
        {
            _buf.append ( value );
            return *this;
        }

        /// @brief Streaming operator
        /// Appends a null-terminated string to this buffer
        /// @param [in] value Null-terminated string to append.
        /// @return reference to the stream object.
        inline TextMessage & operator<< ( char * value )
        {
            _buf.append ( value );
            return *this;
        }

        /// @brief Streaming operator
        /// Appends a string to this buffer
        /// @param [in] value String to append.
        /// @return reference to the stream object.
        inline TextMessage & operator<< ( const String & value )
        {
            _buf.append ( value );
            return *this;
        }

        /// @brief Streaming operator
        /// Appends a wide string to this buffer
        /// @param [in] value String to append.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const WString & value );

        /// @brief Streaming operator
        /// Appends a content of a buffer to this buffer
        /// @param [in] value Buffer to append.
        /// @return reference to the stream object.
        inline TextMessage & operator<< ( const Buffer & value )
        {
            _buf.append ( value );
            return *this;
        }

        /// @brief Streaming operator
        /// Appends Time's description to this buffer
        /// @param [in] value Time object for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const Time & value );

        /// @brief Streaming operator
        /// Appends an IpAddress's description to this buffer
        /// @param [in] value IpAddress for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const IpAddress & value );

        /// @brief Streaming operator
        /// Appends a description of a list of IpAddresses to this buffer
        /// @param [in] value The list of IpAddress objects for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const List<IpAddress> & value );

        /// @brief Streaming operator
        /// Appends an IpSubnet's description to this buffer
        /// @param [in] value IpSubnet for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const IpSubnet & value );

        /// @brief Streaming operator
        /// Appends an SockAddr's description to this buffer
        /// @param [in] value SockAddr for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const SockAddr & value );

        /// @brief Streaming operator
        /// Appends an ErrorCode's description to this buffer
        /// @param [in] value ErrorCode for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const ErrorCode & value );

        /// @brief Streaming operator
        /// Appends an ErrorCode's description to this buffer
        /// @param [in] value ErrorCode for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const ERRCODE & value );

        /// @brief Streaming operator
        /// Appends an ExtProtoError's description to this buffer
        /// @param [in] value ExtProtoError for which description should be appended.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const ExtProtoError & value );

#ifdef SYSTEM_WINDOWS
        /// @brief Streaming operator
        /// Appends a GUID to this buffer
        /// @param [in] value GUID to append.
        /// @return reference to the stream object.
        TextMessage & operator<< ( const GUID & value );
#endif

        /// @brief Template streaming operator
        /// Appends a numeric value to this buffer String::number(value)
        /// is used for conversion, so the value has to have a type that is supported
        /// by one of String::number versions.
        /// @param [in] value Value to append.
        /// @return reference to the stream object.
        template<typename T> inline TextMessage & operator<< ( const T & value )
        {
            _buf.append ( String::number ( value ) );

            return *this;
        }

        /// @brief Returns a reference to internal buffer
        /// @return a reference to internal buffer
        inline Buffer & getInternalBuf()
        {
            return _buf;
        }

    private:
        Buffer _buf; ///< Internal buffer for content as it's generated
};
}
