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

#include "event/EventManager.hpp"
#include "app/StdApp.hpp"

#include "NetMgrCtrlTest.hpp"

using namespace Pravala;

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv );
    app.init();

    NetMgrCtrlTest t;

    ERRCODE eCode = t.run();

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error running NetMgrCtrlTest: %s\n", eCode.toString() );
        return EXIT_FAILURE;
    }

    fprintf ( stderr, "NetMgrCtrlTest completed.\n" );

    return EXIT_SUCCESS;
}
