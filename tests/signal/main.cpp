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
#include <ctime>
#include <cerrno>
#include <unistd.h>

#include "event/EventManager.hpp"

using namespace Pravala;

class Tester: public EventManager::SignalHandler
{
    void receiveSignalEvent ( int sigRcvd )
    {
        printf ( "Signal received: %2d", sigRcvd );

        if ( sigRcvd == EventManager::SignalHUP )
        {
            printf ( " [ SIGHUP]" );
        }
        else if ( sigRcvd == EventManager::SignalUsr1 )
        {
            printf ( " [SIGUSR1]" );
        }
        else if ( sigRcvd == EventManager::SignalUsr2 )
        {
            printf ( " [SIGUSR2]" );
        }

        printf ( "\n" );
    }
};

int main ( int /* argc */, char * /* argv */[] )
{
    printf ( "\nSignal tester: Send SIGHUP, SIGUSR1 or SIGUSR2 to PID %d\n\n", ( int ) getpid() );

    Tester tester;

    EventManager::signalSubscribe ( &tester );
    EventManager::run();

    return EXIT_SUCCESS;
}
