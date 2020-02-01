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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include "app/StdApp.hpp"
#include "config/ConfigAddrSpec.hpp"

#include "socks5/Socks5TcpProxyServer.hpp"

using namespace Pravala;

static ConfigAddrSpec optListenAddr (
        ConfigOpt::FlagIsCommon,
        "listen-addr", 'l',
        "socks5_proxy.listen_addr",
        "The address (address:port) for the SOCKS5 proxy to listen at",
        "0.0.0.0", 45554
);

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv );

    app.init();

    Socks5TcpProxyServer proxyServer ( L_INFO );

    ERRCODE eCode = proxyServer.addListener ( optListenAddr.value() );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error adding listener on %s: %s\n",
                  optListenAddr.value().toString().c_str(), eCode.toString() );
        return EXIT_FAILURE;
    }

    printf ( "Running SOCKS5 proxy server listening on %s\n", optListenAddr.value().toString().c_str() );

    EventManager::run();
    EventManager::shutdown();

    return EXIT_SUCCESS;
}
