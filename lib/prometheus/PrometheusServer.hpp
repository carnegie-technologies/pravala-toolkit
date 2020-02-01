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
#include "log/TextLog.hpp"
#include "http/HttpServer.hpp"

namespace Pravala
{
/// @brief The Prometheus server.
/// Provides HTTP endpoint for remote Prometheus Server to connect to.
/// Provides the Prometheus text exposition of the registered metrics to the remote Prometheus server
class PrometheusServer: protected HttpServer::Owner
{
    public:
        /// @brief Default constructor.
        PrometheusServer();

        /// @brief Creates a new listening socket (IP version)
        /// @param [in] localAddr Local address to listen on.
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow.
        ERRCODE addListener ( const SockAddr & localAddr, int backlog = 4 );

        /// @brief Closes all listeners.
        void closeListeners();

        /// @brief Destructor.
        virtual ~PrometheusServer();

    protected:
        virtual int httpHandleGetRequest (
            HttpServer * server, const SockAddr & remoteAddr, HttpParser & request,
            HashMap<String, String> & respHeaders, String & respContentType, MemHandle & respPayload );

    private:
        static TextLog _log; ///< Log stream.

        HttpServer _httpServer; ///< HTTP server that we use to handle incoming requests.
};
}
