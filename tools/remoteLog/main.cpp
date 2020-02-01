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

#include "sys/OsUtils.hpp"
#include "log/LogManager.hpp"
#include "app/StdApp.hpp"
#include "ctrl/ConfigCtrlSock.hpp"

#include "RemoteLog.hpp"

using namespace Pravala;

static ConfigCtrlSock optCtrlSock ( "connect", 'c', "remote_log.connect_sock", "Remote Logger" );

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs,
            "All additional arguments are treated as log names to follow.\n"
            "They should be in 'log_name.log_level' format.\n"
    );
    app.init();

    List<RemoteLog::LogDesc> logDescs;

    for ( size_t i = 0; i < app.getExtraCmdLineArgs().size(); ++i )
    {
        StringList logSpec = String ( app.getExtraCmdLineArgs().at ( i ) ).split ( ".", true );
        Log::LogLevel lLevel;

        if ( logSpec.size() == 2 && logSpec.at ( 0 ).length() > 0
             && LogManager::str2LogLevel ( logSpec.at ( 1 ), lLevel ) )
        {
            RemoteLog::LogDesc lDesc;

            lDesc.name = logSpec[ 0 ];
            lDesc.level = lLevel;

            logDescs.append ( lDesc );
        }
        else
        {
            fprintf ( stderr, "Invalid log element (log_name.log_level required): '%s'\n",
                      app.getExtraCmdLineArgs().at ( i ).c_str() );
            app.printHelp ( stderr );
            return EXIT_FAILURE;
        }
    }

    if ( optCtrlSock.isEmpty() )
    {
        fprintf ( stderr, "You need to provide a control socket to connect to (with -c option)\n" );
        app.printHelp ( stderr );
        return EXIT_FAILURE;
    }

    RemoteLog remoteLog ( logDescs );

    const ERRCODE eCode = remoteLog.startCtrlLink ( optCtrlSock.value() );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Adding control link failed; Code: %s\n", eCode.toString() );
        app.printHelp ( stderr );
        return EXIT_FAILURE;
    }

    EventManager::run();

    return EXIT_SUCCESS;
}
