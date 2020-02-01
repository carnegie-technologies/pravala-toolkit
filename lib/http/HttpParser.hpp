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
#include "basic/HashMap.hpp"
#include "basic/String.hpp"

struct http_parser;

namespace Pravala
{
class MemHandle;
class RwBuffer;

/// @brief Parses an HTTP header
class HttpParser: public NoCopy
{
    public:
        /// @brief Current parsing state
        enum HttpParserState
        {
            ParseIncomplete,    ///< Complete headers have not been received yet
            ParseFailed,        ///< Fatal error parsing
            ParseHeadersDone    ///< Headers complete
        };

        static const int MethodInvalid; ///< Invalid method.
        static const int MethodGet; ///< The GET method.

        /// @brief Constructor
        HttpParser();

        /// @brief Destructor
        ~HttpParser();

        /// @brief Reset this HttpParser object to its initially constructed state
        /// After calling this function, all fields are cleared and it is safe to parse a new HTTP header.
        void reset();

        /// @brief Parse and consume a portion of the HTTP header
        ///
        /// As data is (externally) appended to the buffer, this function should be called repeatedly with the same
        /// buffer for it to consume and parse the header out of it.
        ///
        /// @note This is just a wrapper around parse ( data, len );
        /// @param [in,out] buf Buffer containing a portion of the HTTP header. Some bytes will be consumed
        /// as it is parsed.
        /// @return Current HTTP parser state (after the parse operation)
        HttpParserState parse ( RwBuffer & buf );

        /// @brief Parse and consume a portion of the HTTP header
        ///
        /// As data is (externally) appended to the buffer, this function should be called repeatedly with the same
        /// buffer for it to consume and parse the header out of it.
        ///
        /// @note This is just a wrapper around parse ( data, len );
        /// @param [in,out] mh MemHandle containing a portion of the HTTP header. Some bytes will be consumed
        /// as it is parsed.
        /// @return Current HTTP parser state (after the parse operation)
        HttpParserState parse ( MemHandle & mh );

        /// @brief Parse a portion of the HTTP header
        ///
        /// This function will parse up to len bytes starting from data. It will return the number of bytes actually
        /// parsed. If parsing did not complete (i.e. there was insufficient data), this function should be called
        /// again with data pointing to the first byte that was not previously parsed.
        ///
        /// getState() should be called after calling this function to determine if the parse needs more data,
        /// has completed, or has failed.
        ///
        /// @param [in] data Start of this portion of the header
        /// @param [in] len Length of data available to parse
        /// @return Number of bytes parsed
        size_t parse ( const char * data, size_t len );

        /// @brief Gets the current HTTP parser state
        /// @return Current HTTP parser state
        inline HttpParserState getState() const
        {
            return _curParserState;
        }

        /// @brief Returns the HTTP method used.
        /// @return The number identifying the method (one of Method* values).
        int getMethod() const;

        /// @brief Returns the name of HTTP method used.
        /// @return The name of the HTTP method used.
        String getMethodName() const;

        /// @brief Returns a string representation of the current HTTP parser error name
        /// @return A string representation of the current HTTP parser error name
        const char * getErrorName() const;

        /// @brief Returns a string representation of the current HTTP parser error description
        /// @return A string representation of the current HTTP parser error description
        const char * getErrorDesc() const;

        /// @brief Returns true if this HTTP header contained an HTTP upgrade
        /// @note This value is only valid if getState() == ParseHeadersDone
        /// @return True if this HTTP header contained an HTTP upgrade
        bool isUpgrade() const;

        /// @brief Returns true if this HTTP header was HTTP/1.0 (instead of 1.1 or newer)
        ///
        /// HTTP/1.0 is "special" in the sense that it doesn't support a lot of things, such as
        /// headers in certain types of responses, extended error status, etc., enough that
        /// we need to handle HTTP/1.0 connections differently than HTTP/1.1 or newer.
        ///
        /// @note This value is only valid if getState() == ParseHeadersDone
        /// @return True if this HTTP header was HTTP/1.0 (instead of 1.1 or newer)
        bool isHttp10() const;

        /// @brief Returns the URL of the request/response
        /// @note This value is only valid if getState() == ParseHeadersDone
        /// @return The URL of the request/response
        inline const String & getUrl() const
        {
            return _url;
        }

        /// @brief If this is a response, returns the status line of the response
        /// @note This value is only valid if getState() == ParseHeadersDone
        /// Sample response: HTTP/1.1 200 OK
        /// @return The status line of the response
        inline const String & getResponseStatus() const
        {
            return _responseStatus;
        }

        /// @brief Returns a map of all headers
        /// @note This value is only valid if getState() == ParseHeadersDone
        /// @return The headers <header name, header value>
        /// If a header with the same name appears multiple times, it is appended to the value prefixed with a comma.
        inline const HashMap<String, String> & getHeaders() const
        {
            return _headers;
        }

    private:
        struct http_parser * _parserState; ///< Current state of the HTTP parser

        /// @brief Parsed headers <header name, header value>
        /// If a header with the same name appears multiple times, it is appended to the value prefixed with a comma.
        HashMap<String, String> _headers;

        String _url; ///< URL of request/response
        String _responseStatus; ///< If this is a response, the status line of the response
        HttpParserState _curParserState; ///< The current HTTP parser state

        String _lastHeaderField; ///< Potentially incomplete portion of the name of last header field parsed
        String _lastHeaderValue; ///< Potentially incomplete portion of the value of last header field parsed

        /// @brief If both _lastHeaderField and _lastHeaderValue are non-empty, then use their values to
        /// add a header to _headers, and clear both _last* fields. Otherwise this function does nothing.
        ///
        /// See cpp file for details
        void insertHeader();

        friend class HttpParserCallbacks;
};
}
