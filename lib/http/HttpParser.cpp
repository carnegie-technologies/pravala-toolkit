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

/* Header parsing states:
 *
 * All other combinations are invalid.
 *
 * Based on note from 3rdparty/README.md
 *
 * Function called | _lastHeaderField | _lastHeaderValue | Action
 * ----------------+------------------+------------------+-----------------------------------------------------------
 * cbHeaderField   |  Has Data/Empty  |       Empty      | Just got next portion of the header name,
 *                 |                  |                  | append header name to _lastHeaderField
 * ----------------+------------------+------------------+-----------------------------------------------------------
 * cbHeaderField   |    Has Data      |     Has Data     | Call insertHeader() to insert last complete header, then
 *                 |                  |                  | append header name to the now cleared _lastHeaderField
 * ----------------+------------------+------------------+-----------------------------------------------------------
 * cbHeaderValue   |    Has Data      |  Has Data/Empty  | Append header value to _lastHeaderValue
 * ----------------+------------------+------------------+-----------------------------------------------------------
 * cbHeaderComplete|      Empty       |       Empty      | Set state to ParseHeadersDone
 * ----------------+------------------+------------------+-----------------------------------------------------------
 * cbHeaderComplete|    Has Data      |     Has Data     | Call insertHeader() to insert last complete header,
 *                 |                  |                  | then set state to ParseHeadersDone
 * ----------------+------------------+------------------+-----------------------------------------------------------
 */

// http_parser.h does some things that aren't quite pedantic, so tell it to ignore it.
// This supposedly works for clang too.
#ifndef _MSC_VER
#pragma GCC diagnostic push
#if ( __GNUC__ >= 6 ) || ( defined __clang__ )
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

extern "C"
{
#include "http_parser.h"
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"

#include "HttpParser.hpp"

using namespace Pravala;

namespace Pravala
{
/// @brief Class containing the static callbacks for the HTTP parsing library.
class HttpParserCallbacks
{
    public:
        /// @brief Callback called by the 3rdparty http parser when it parses the URL
        /// @param [in] parser Parser state of the parser
        /// @param [in] at Start of the URL
        /// @param [in] length Length of the URL
        /// @return 0 if operation was successful, 1 if an error occurred
        static int cbUrl ( http_parser * parser, const char * at, size_t length );

        /// @brief Callback called by the 3rdparty http parser when it parses the status line
        /// @param [in] parser Parser state of the parser
        /// @param [in] at Start of the status line
        /// @param [in] length Length of the status line
        /// @return 0 if operation was successful, 1 if an error occurred
        static int cbStatus ( http_parser * parser, const char * at, size_t length );

        /// @brief Callback called by the 3rdparty http parser when it parses (part of) the header field's name
        /// See cpp file for details
        /// @param [in] parser Parser state of the parser
        /// @param [in] at Start of the header name
        /// @param [in] length Length of this part of the header name
        /// @return 0 if operation was successful, 1 if an error occurred
        static int cbHeaderField ( http_parser * parser, const char * at, size_t length );

        /// @brief Callback called by the 3rdparty http parser when it parses (part of) the header field's value
        /// See cpp file for details
        /// @param [in] parser Parser state of the parser
        /// @param [in] at Start of the header value
        /// @param [in] length Length of this part of the header value
        /// @return 0 if operation was successful, 1 if an error occurred
        static int cbHeaderValue ( http_parser * parser, const char * at, size_t length );

        /// @brief Callback called by the 3rdparty http parser when it completes parsing the entire header
        /// @param [in] parser Parser state of the parser
        /// @return 0 if operation was successful, 1 if an error occurred
        static int cbHeaderComplete ( http_parser * parser );
};
}

static http_parser_settings parserSettings =
{
    0, // on_message_begin
    HttpParserCallbacks::cbUrl, // on_url
    HttpParserCallbacks::cbStatus, // on_status
    HttpParserCallbacks::cbHeaderField, // on_header_field
    HttpParserCallbacks::cbHeaderValue, // on_header_value
    HttpParserCallbacks::cbHeaderComplete, // on_headers_complete
    0, // on_body
    0 // on_message_complete
};

const int HttpParser::MethodInvalid ( -1 );
const int HttpParser::MethodGet ( HTTP_GET );

HttpParser::HttpParser():
    _parserState ( new http_parser() ),
    _curParserState ( ParseIncomplete )
{
    assert ( _parserState != 0 );

    reset();
}

HttpParser::~HttpParser()
{
    delete _parserState;
}

void HttpParser::reset()
{
    assert ( _parserState != 0 );

    _curParserState = ParseIncomplete;

    _url.clear();
    _responseStatus.clear();

    _headers.clear();
    _lastHeaderField.clear();
    _lastHeaderValue.clear();

    // This clears parserState, and does not allocate any memory on its own,
    // so we don't have to do anything to clean it up.
    http_parser_init ( _parserState, HTTP_BOTH );

    // Stop parsing as soon as the headers are done, this must be called AFTER init
    _parserState->flags = F_SKIPBODY;

    _parserState->data = this;
}

HttpParser::HttpParserState HttpParser::parse ( RwBuffer & buf )
{
    size_t ret = parse ( buf.get(), buf.size() );

    if ( ret > 0 )
    {
        buf.consumeData ( ret );
    }

    return _curParserState;
}

HttpParser::HttpParserState HttpParser::parse ( MemHandle & mh )
{
    size_t ret = parse ( mh.get(), mh.size() );

    if ( ret > 0 )
    {
        mh.consume ( ret );
    }

    return _curParserState;
}

size_t HttpParser::parse ( const char * data, size_t len )
{
    assert ( _parserState != 0 );

    size_t ret = http_parser_execute ( _parserState, &parserSettings, data, len );

    // If the error is a fatal error, then set parser state to failed.
    if ( _parserState->http_errno != HPE_OK && _parserState->http_errno != HPE_INVALID_EOF_STATE )
    {
        _curParserState = HttpParser::ParseFailed;
    }

    return ret;
}

int HttpParser::getMethod() const
{
    assert ( _parserState != 0 );

    return ( _curParserState == ParseHeadersDone ) ? _parserState->method : MethodInvalid;
}

String HttpParser::getMethodName() const
{
    assert ( _parserState != 0 );

    return ( _curParserState == ParseHeadersDone )
           ? http_method_str ( ( http_method ) _parserState->method )
           : "Invalid";
}

const char * HttpParser::getErrorName() const
{
    assert ( _parserState != 0 );

    return http_errno_name ( ( http_errno ) _parserState->http_errno );
}

const char * HttpParser::getErrorDesc() const
{
    assert ( _parserState != 0 );

    return http_errno_description ( ( http_errno ) _parserState->http_errno );
}

bool HttpParser::isUpgrade() const
{
    assert ( _parserState != 0 );

    return _parserState->upgrade != 0;
}

bool HttpParser::isHttp10() const
{
    assert ( _parserState != 0 );

    return _parserState->http_major == 1 && _parserState->http_minor == 0;
}

void HttpParser::insertHeader()
{
    // See top of file for details.
    // Only insert header if both fields are non-empty.
    if ( _lastHeaderField.isEmpty() || _lastHeaderValue.isEmpty() )
        return;

    if ( _headers.contains ( _lastHeaderField ) )
    {
        // If the header already exists in our map (i.e. appears multiple times), we can collapse
        // it under one name by adding a comma then the value according to RFC 2616.
        //
        // We leave it up to the code that uses the parser to determine if this value makes sense.
        //
        // We do not want to split the header into entries here since the comma character may have
        // special meaning for some headers.
        _headers[ _lastHeaderField ].append ( "," ).append ( _lastHeaderValue );
    }
    else
    {
        _headers.insert ( _lastHeaderField, _lastHeaderValue );
    }

    _lastHeaderField.clear();
    _lastHeaderValue.clear();
}

int HttpParserCallbacks::cbStatus ( http_parser * parser, const char * at, size_t length )
{
    assert ( parser != 0 );

    HttpParser * p = reinterpret_cast<HttpParser *> ( parser->data );

    assert ( p != 0 );

    p->_responseStatus = String ( at, length );

    return 0;
}

int HttpParserCallbacks::cbUrl ( http_parser * parser, const char * at, size_t length )
{
    assert ( parser != 0 );

    HttpParser * p = reinterpret_cast<HttpParser *> ( parser->data );

    assert ( p != 0 );
    assert ( length > 0 );

    p->_url = String ( at, length );

    return 0;
}

int HttpParserCallbacks::cbHeaderField ( http_parser * parser, const char * at, size_t length )
{
    // See top of file for details
    assert ( parser != 0 );

    HttpParser * p = reinterpret_cast<HttpParser *> ( parser->data );

    assert ( p != 0 );

    p->insertHeader();

    // After we call insertHeader(), the value should be empty
    assert ( p->_lastHeaderValue.isEmpty() );

    p->_lastHeaderField.append ( at, length );

    return 0;
}

int HttpParserCallbacks::cbHeaderValue ( http_parser * parser, const char * at, size_t length )
{
    // See top of file for details
    assert ( parser != 0 );

    HttpParser * p = reinterpret_cast<HttpParser *> ( parser->data );

    assert ( p != 0 );
    assert ( !p->_lastHeaderField.isEmpty() );

    p->_lastHeaderValue.append ( at, length );

    return 0;
}

int HttpParserCallbacks::cbHeaderComplete ( http_parser * parser )
{
    assert ( parser != 0 );

    HttpParser * p = reinterpret_cast<HttpParser *> ( parser->data );

    assert ( p != 0 );

    p->insertHeader();
    p->_curParserState = HttpParser::ParseHeadersDone;

    return 0;
}
