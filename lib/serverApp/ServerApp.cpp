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

#include "ServerApp.hpp"

using namespace Pravala;

ConfigAddrSpec ServerApp::optHttpLivenessServer (
        ConfigOpt::FlagIsCommon,
        "http-liveness-server",
        0,
        "server.http_liveness_server",
        "The address to listen on for HTTP liveness probe requests"
);

ConfigAddrSpec ServerApp::optPrometheusServer (
        ConfigOpt::FlagIsCommon,
        "prometheus-server",
        0,
        "server.prometheus_server",
        "The address to listen on for Prometheus requests"
);

ServerApp::ServerApp ( int argc, char * argv[], uint32_t features, const char * additionalHelpText ):
    StdApp ( argc, argv, features, additionalHelpText )
{
}

ServerApp::~ServerApp()
{
}

int ServerApp::initFeatures ( bool exitOnError )
{
    int ret = StdApp::initFeatures ( exitOnError );

    if ( ret != EXIT_SUCCESS )
        return ret;

    bool wasError = false;

    if ( optPrometheusServer.isSet() )
    {
        ERRCODE eCode = _prometheusServer.addListener ( optPrometheusServer.value() );

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Could not start a Prometheus listener on '%s': %s\n",
                      optPrometheusServer.value().toString().c_str(), eCode.toString() );

            wasError = true;
        }
    }

    if ( !wasError && optHttpLivenessServer.value().hasIpAddr() )
    {
        _httpLivenessServer.setHtmlContent ( "/", "<html></html>\n" );

        ERRCODE eCode = _httpLivenessServer.addListener ( optHttpLivenessServer.value() );

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Could not start a HTTP listener on '%s': %s\n",
                      optHttpLivenessServer.value().toString().c_str(), eCode.toString() );

            wasError = true;
        }
    }

    if ( wasError )
    {
        printHelp ( stderr );

        if ( exitOnError || isConfigTestMode() )
        {
            exit ( EXIT_FAILURE );
        }

        return EXIT_FAILURE;
    }

    return ret;
}
