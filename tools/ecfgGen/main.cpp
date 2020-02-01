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
#include <cerrno>

#include "EcfgGen.hpp"
#include "app/StdApp.hpp"
#include "sys/OsUtils.hpp"

using namespace Pravala;

int main ( int argc, char * argv[] )
{
    String commandLine;

    for ( int i = 0; i < argc; ++i )
    {
        if ( i > 0 )
            commandLine.append ( " " );

        commandLine.append ( "'" );
        commandLine.append ( argv[ i ] );
        commandLine.append ( "'" );
    }

    // The last argument (0) is so we wouldn't enable any features (like standard built-in options).
    StdApp app ( argc, argv, 0 );

    fprintf ( stderr, "Running ECFG generator\n" );

    EcfgGenerator gen;

    if ( app.init ( false ) != EXIT_SUCCESS )
    {
        fprintf ( stderr, "\nCommand line was: \n %s\n", commandLine.c_str() );
        return EXIT_FAILURE;
    }

    if ( !gen.run() )
    {
        fprintf ( stderr, "Error running the generator!\n" );
        app.printHelp ( stderr );
        fprintf ( stderr, "\nCommand line was: \n %s\n", commandLine.c_str() );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
