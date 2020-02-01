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

#include "app/StdApp.hpp"
#include "config/ConfigAddrSpec.hpp"
#include "http/SimpleHttpServer.hpp"
#include "prometheus/PrometheusServer.hpp"

namespace Pravala
{
/// @brief A wrapper around StdApp that adds server-specific features.
/// At the moment it adds support for Prometheus and HTTP liveness probe (as used by Kubernetes) servers.
class ServerApp: public StdApp
{
    public:
        /// @brief Configures built-in HTTP liveness probe server (used by Kubernetes).
        static ConfigAddrSpec optHttpLivenessServer;

        /// @brief Configures built-in Prometheus server.
        static ConfigAddrSpec optPrometheusServer;

        /// @brief Constructor.
        /// @param [in] argc The number of elements in argv array
        /// @param [in] argv Command line arguments; The first entry is the program name.
        /// @param [in] features Flags that control which features will be used.
        /// @param [in] additionalHelpText The additional help text to be included.
        ServerApp (
            int argc, char * argv[],
            uint32_t features = StdApp::FeatStdFeatures,
            const char * additionalHelpText = 0 );

        /// @brief Destructor.
        virtual ~ServerApp();

        /// @brief Exposes built-in HTTP liveness probe server (as used by Kubernetes).
        /// @return Built-in HTTP liveness probe server.
        inline SimpleHttpServer & getHttpLivenessServer()
        {
            return _httpLivenessServer;
        }

        /// @brief Exposes built-in Prometheus server.
        /// @return Built-in Prometheus server.
        inline PrometheusServer & getPrometheusServer()
        {
            return _prometheusServer;
        }

        virtual int initFeatures ( bool exitOnError = true );

    protected:
        SimpleHttpServer _httpLivenessServer; ///< Built-in HTTP liveness probe server (used by Kubernetes).
        PrometheusServer _prometheusServer;   ///< Built-in Prometheus server.
};
}
