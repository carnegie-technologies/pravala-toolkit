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

#include "internal/PrometheusManager.hpp"
#include "PrometheusServer.hpp"

using namespace Pravala;

TextLog PrometheusServer::_log ( "prometheus_server" );

PrometheusServer::PrometheusServer():
    _httpServer ( *this )
{
}

PrometheusServer::~PrometheusServer()
{
}

void PrometheusServer::closeListeners()
{
    _httpServer.closeListeners();
}

ERRCODE PrometheusServer::addListener ( const SockAddr & localAddr, int backlog )
{
    const ERRCODE eCode = _httpServer.addListener ( localAddr, backlog );

    LOG_ERR ( IS_OK ( eCode ) ? L_INFO : L_ERROR, eCode, "Adding listener: " << localAddr );

    return eCode;
}

int PrometheusServer::httpHandleGetRequest (
        HttpServer * server, const SockAddr & remoteAddr, HttpParser & request,
        HashMap<String, String> & /*respHeaders*/, String & respContentType, MemHandle & respPayload )
{
    ( void ) server;
    ( void ) remoteAddr;
    assert ( server == &_httpServer );

    if ( _log.shouldLog ( L_DEBUG3 ) )
    {
        const String agentKey = request.getHeaders().value ( "User-Agent" );

        LOG ( L_DEBUG3, remoteAddr << ": Agent: " << agentKey
              << ( ( agentKey.startsWith ( "Prometheus/" ) ) ? " (Prometheus client)" : " (Other client)" ) );
    }

    respContentType = "text/plain; version=0.0.4";
    respPayload = PrometheusManager::get().getData();

    return HttpServer::StatusOK;
}
