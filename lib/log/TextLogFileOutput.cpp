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

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cassert>

extern "C"
{
#include <sys/stat.h>
#include <fcntl.h>
}

#include "sys/OsUtils.hpp"
#include "TextLogFileOutput.hpp"

#ifdef SYSTEM_WINDOWS
#ifndef S_IRGRP
#define S_IRGRP    ( S_IRUSR >> 3 )
#endif
#endif

using namespace Pravala;

TextLogFileOutput::TextLogFileOutput ( int fd, bool autoClose ):
    _myFd ( fd ), _autoClose ( autoClose )
{
}

TextLogFileOutput::TextLogFileOutput ( const String & fileName ):
    _myFd ( -1 ), _autoClose ( true )
{
    _myFd = open ( fileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP );

    if ( _myFd < 0 )
    {
        perror ( "TextLogFileOutput: open()" );
    }
}

TextLogFileOutput::~TextLogFileOutput()
{
    if ( _myFd >= 0 )
    {
        if ( _autoClose )
        {
            ::close ( _myFd );
        }

        _myFd = -1;
    }
}

void TextLogFileOutput::sendTextLog ( Log::TextMessage & logMessage, String & strMessage )
{
    if ( _myFd < 0 )
        return;

    if ( strMessage.isEmpty() )
    {
        formatMessage ( logMessage, strMessage );

        if ( strMessage.isEmpty() )
            return;
    }

    assert ( strMessage.length() > 0 );

    ssize_t ret = write ( _myFd, strMessage.c_str(), strMessage.length() );

    if ( ret < 0 )
    {
        if ( errno != EPIPE )
        {
            // If the output is piped to another process (like grep) we will get PIPE errors
            // if user presses Ctrl-C; Let's not print that...

            perror ( "TextLogFileOutput: write()" );
        }
    }
    else if ( ret != strMessage.length() )
    {
        fprintf ( stderr, "Write to fd %d failed; Error: %s\n", _myFd, strerror ( errno ) );
    }
}
