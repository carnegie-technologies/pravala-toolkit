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

extern "C"
{
#include <sys/stat.h>
}

#include <cerrno>
#include <cstdio>

#include "config/ConfigString.hpp"
#include "sys/File.hpp"

#include "../../StdApp.hpp"

using namespace Pravala;

static ConfigString optAppDir (
        ConfigOpt::FlagInitializeOnly, "app.data.dir",
        "Directory to put application data in, e.g. '/var/lib/foo'" );

String StdApp::osGetAppDataPath()
{
    const int ret = ::mkdir ( optAppDir.c_str(), 0755 );
    const String testFile = String ( "%1/test.file" ).arg ( optAppDir.value() );

    if ( ( ret == 0 || ( ret < 0 && errno == EEXIST ) )
         && File::isWritable ( testFile, true ) )
    {
        return optAppDir.value();
    }
    else
    {
        fprintf ( stderr, "Error creating directory '%s', using /tmp; Error: %s\n",
                  optAppDir.value().c_str(), strerror ( errno ) );

        return "/tmp";
    }
}

String StdApp::osGetBinaryDir ( int argc, char * argv[] )
{
    if ( argc < 1 || !argv[ 0 ] )
        return String::EmptyString;

    /// @todo TODO - Implement

    return String::EmptyString;
}
