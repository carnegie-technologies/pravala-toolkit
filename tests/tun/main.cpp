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

#include "config/ConfigSwitch.hpp"
#include "app/StdApp.hpp"

#include "TunTest.hpp"

using namespace Pravala;

static ConfigSwitch swDoExit ( "exit", 'e', "When set, the test will exit right away after creating the interfaces." );

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeatures | StdApp::FeatSuid );
    app.init();

    TunTest st;

    if ( !st.start() )
    {
        fprintf ( stderr, "Could not start the TunTest\n" );
        return EXIT_FAILURE;
    }

    if ( !swDoExit.isSet() )
    {
        // EventManager will exit when it receives SIGINT interrupt (Ctrl-C)
        EventManager::run();
    }

    return EXIT_SUCCESS;
}
