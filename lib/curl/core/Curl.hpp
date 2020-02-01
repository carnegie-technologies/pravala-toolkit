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

#include "basic/String.hpp"

namespace Pravala
{
/// @brief Common definitions and functionality for cURL-related wrappers.
class Curl
{
    public:
        /// @brief HTTP methods supported by this class.
        enum HttpMethod
        {
            MethodGet    = 0, ///< HTTP "GET" method.
            MethodPost   = 1, ///< HTTP "POST" method.
            MethodPut    = 2, ///< HTTP "PUT" method.
            MethodDelete = 3  ///< HTTP "DELETE" method.
        };

        /// @brief Some HTTP status codes.
        enum HttpStatus
        {
            StatusOK               = 200, ///< Success.
            StatusNotFound         = 404, ///< Not found.
            StatusMethodNotAllowed = 405  ///< Method not allowed.
        };

        static const String ContentTypeJson;       ///< Content type to be used for application/json content.
        static const String ContentTypeTextHtml;   ///< Content type to be used for text/html content.
        static const String ContentTypeTextPlain; ///< Content type to be used for text/plain content.

        /// @brief Performs global cURL initialization.
        /// @param [out] errorStr If used, error message will be stored there. Only set on error.
        /// @return The code of the operation. One of the values defined in CURLcode. 0 means 'success'.
        static int globalInit ( String * errorStr = 0 );

        /// @brief Looks for the last character that belongs to HTTP headers.
        /// It will inspect the data and look for '\r\n\r\n' (or '\n\n') sub-string and
        /// return the size of the header segment (including the header-end characters).
        /// @param [in] data Pointer to the data to look for HTTP headers in.
        /// @param [in] size The size of the data.
        /// @return The size of the headers, or 0 if not found.
        static size_t detectHeadersSize ( const char * data, size_t size );
};
}
