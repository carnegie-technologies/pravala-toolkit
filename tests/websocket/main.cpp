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

#include <cstdlib>
#include <cstdio>

#include "app/StdApp.hpp"

#include "WSTest.hpp"

using namespace Pravala;

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeatures );
    app.init();

    WSTest st;

    if ( !st.start() )
    {
        fprintf ( stderr, "Could not start the WSTest\n" );
        return EXIT_FAILURE;
    }

    // EventManager will exit when it receives SIGINT interrupt (Ctrl-C)
    EventManager::run();

    return EXIT_SUCCESS;
}
