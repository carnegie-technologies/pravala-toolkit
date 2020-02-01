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

#include "basic/HashMap.hpp"
#include "HttpServer.hpp"

namespace Pravala
{
/// @brief A wrapper around HttpServer, that is its own owner and can respond to GET requests.
class SimpleHttpServer: public HttpServer, protected HttpServer::Owner
{
    public:
        /// @brief Default constructor.
        SimpleHttpServer();

        /// @brief Remove content entry.
        /// @param [in] url The URL to remove. "/" is added in front of the URL if it doesn't start with one.
        void removeContent ( const String & url );

        /// @brief Set content entry with no payload.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] respCode The response code to return.
        void setContent ( const String & url, int respCode );

        /// @brief Set content entry.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] contentType The type of the content to return (text/plain, text/html, etc.)
        /// @param [in] content The content to return.
        void setContent ( const String & url, const String & contentType, const MemHandle & content );

        /// @brief Set content entry.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] contentType The type of the content to return (text/plain, text/html, etc.)
        /// @param [in] content The content to return.
        void setContent ( const String & url, const String & contentType, const String & content );

        /// @brief Set content entry.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] contentType The type of the content to return (text/plain, text/html, etc.)
        /// @param [in] content The content to return.
        void setContent ( const String & url, const String & contentType, const char * content );

        /// @brief Set content entry.
        /// This is a helper wrapper, that uses 'text/plain' as the content type.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] content The content to return.
        inline void setPlainTextContent ( const String & url, const char * content )
        {
            setContent ( url, "text/plain", content );
        }

        /// @brief Sets content entry.
        /// This is a helper wrapper, that uses 'text/html' as the content type.
        /// If there already is an entry using the same URL, it will be replaced.
        /// @param [in] url The URL to set. "/" is added in front of the URL if it doesn't start with one.
        /// @param [in] content The content to return.
        inline void setHtmlContent ( const String & url, const char * content )
        {
            setContent ( url, "text/html", content );
        }

    protected:
        /// @brief Describes a single URL handled by this server.
        struct UrlData
        {
            MemHandle data;     ///< The data to be returned.
            String contentType; ///< The content type for the data to return.
            int respCode;       ///< Response code to use for that URL.
        };

        /// @brief The data served by this server.
        /// Key is the URL (just the path, should start with "/").
        /// Value is the content of the specified URL.
        HashMap<String, UrlData> _data;

        virtual int httpHandleGetRequest (
            HttpServer * server, const SockAddr & remoteAddr, HttpParser & request,
            HashMap<String, String> & respHeaders, String & respContentType, MemHandle & respPayload );

        /// @brief Sanitizes given URL.
        /// It removes some invalid characters, and also adds "/" in front to the URL if it doesn't start with it.
        /// @param [in] url The URL to sanitize.
        /// @return Sanitized version of the URL.
        static String sanitizeUrl ( const String & url );
};
}
