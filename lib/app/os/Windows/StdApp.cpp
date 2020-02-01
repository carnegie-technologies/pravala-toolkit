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
#include <cerrno>

extern "C"
{
// This must be in this order!
#include <winsock2.h>
#include <windows.h>
#include <WinBase.h>

#include <direct.h> // for _mkdir
}

#include "config/ConfigCore.hpp"
#include "sys/File.hpp"

#include "../../StdApp.hpp"

using namespace Pravala;

static ConfigString optAppDirName (
        ConfigOpt::FlagInitializeOnly, "app.data.subdir",
        "Sub-directory to put application data in, i.e. if this is 'foo', then"
        " application data path will be returned as something like 'c:\\program data\\foo'"
        " or 'c:\\users\\bar\\appdata\\foo'."
);

String StdApp::osGetAppDataPath()
{
    // From: http://msdn.microsoft.com/en-us/library/windows/desktop/ms683188%28v=vs.85%29.aspx
    // Max len is 32767 bytes incl. null terminator
    char buf[ 32767 ];

    String compName;
    String userName;
    String appDataPath;

    DWORD ret = ::GetEnvironmentVariable ( "COMPUTERNAME", buf, sizeof ( buf ) );

    if ( ret > 0 )
    {
        compName = String ( "%1$" ).arg ( buf );

        ret = ::GetEnvironmentVariable ( "USERNAME", buf, sizeof ( buf ) );
    }

    if ( ret > 0 )
    {
        userName = buf;
    }

    // If the username name is "COMPUTERNAME$", we are the system user, so we should use PROGRAMDATA
    // instead of APPDATA
    if ( !userName.isEmpty() && compName == userName )
    {
        ret = ::GetEnvironmentVariable ( "PROGRAMDATA", buf, sizeof ( buf ) );
    }
    else
    {
        ret = ::GetEnvironmentVariable ( "APPDATA", buf, sizeof ( buf ) );
    }

    if ( ret > 0 )
    {
        appDataPath = String ( "%1/%2" ).arg ( buf, optAppDirName.value() );

        if ( _mkdir ( appDataPath.c_str() ) != 0 && errno != EEXIST )
        {
            fprintf ( stderr, "Could not create directory '%s', using current directory\n", appDataPath.c_str() );

            appDataPath = ".";
        }
        else
        {
            const String testFile = String ( "%1/test.file" ).arg ( appDataPath );

            if ( !File::isWritable ( testFile, true ) )
            {
                fprintf ( stderr, "Failed to write to '%s', using current directory\n", testFile.c_str() );

                appDataPath = ".";
            }
        }
    }
    else
    {
        fprintf ( stderr, "Failed to get PROGRAMDATA/APPDATA environment variable, using current directory\n" );

        appDataPath = ".";
    }

    return appDataPath;
}

String StdApp::osGetBinaryDir ( int argc, char * argv[] )
{
    if ( argc < 1 || !argv[ 0 ] )
        return String::EmptyString;

    // Try to find the binary path first from argv[0] (binaryName) then GetCurrentDirectory

    String binPath = argv[ 0 ];
    const int slashPos = binPath.findLastOf ( "\\" );

    if ( slashPos < 0 )
    {
        TCHAR path[ MAX_PATH + 1 ];

        if ( !GetCurrentDirectory ( sizeof ( path ), path ) )
        {
            fprintf ( stderr, "Could not initialize the binary path (error running GetCurrentDirectory)\n" );
            return String::EmptyString;
        }

        binPath = path;
    }
    else
    {
        binPath = binPath.substr ( 0, slashPos );
    }

    if ( binPath.isEmpty() )
    {
        fprintf ( stderr, "Could not initialize the binary path\n" );
    }

    return binPath;
}
