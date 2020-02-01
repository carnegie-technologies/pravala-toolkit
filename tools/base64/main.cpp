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
#include <unistd.h>
}

#include <cstdio>
#include <cstdlib>

#include "basic/Buffer.hpp"
#include "base64/Base64.hpp"

#define READ_SIZE    4096

using namespace Pravala;

inline void printHelp ( const char * me )
{
    fprintf ( stderr, "Usage: %s [e|d] [n]\n", me );
    fprintf ( stderr, "   e = Encode to base64\n" );
    fprintf ( stderr, "   d = Decode from base64\n" );
    fprintf ( stderr, "   n = When encoding, add newlines (ignored when decoding)\n\n" );
    fprintf ( stderr, "Input is read from STDIN and output sent to STDOUT\n" );
}

void readInput ( Buffer & inbuf )
{
    size_t ret = -1;

    do
    {
        char * const buf = inbuf.getAppendable ( READ_SIZE );

        assert ( buf != 0 );

        if ( !buf )
            break;

        ret = read ( STDIN_FILENO, buf, READ_SIZE );

        if ( ret > 0 )
        {
            inbuf.markAppended ( ret );
        }
    }
    while ( ret > 0 );
}

int main ( int argc, char * argv[] )
{
    if ( argc < 2 )
    {
        printHelp ( argv[ 0 ] );
        return EXIT_FAILURE;
    }

    Buffer inbuf;

    if ( strcmp ( argv[ 1 ], "e" ) == 0 )
    {
        readInput ( inbuf );

        int addNewline = 0;

        if ( argc > 2 && strcmp ( argv[ 2 ], "n" ) == 0 )
        {
            addNewline = 1;
        }

        MemHandle out = Base64::encode ( inbuf.getHandle(), addNewline );
        ssize_t ret = write ( STDOUT_FILENO, out.get(), out.size() );

        if ( ret < 0 )
            return EXIT_FAILURE;
    }
    else if ( strcmp ( argv[ 1 ], "d" ) == 0 )
    {
        readInput ( inbuf );

        MemHandle out = Base64::decode ( inbuf.getHandle() );
        ssize_t ret = write ( STDOUT_FILENO, out.get(), out.size() );

        if ( ret < 0 )
            return EXIT_FAILURE;
    }
    else
    {
        printHelp ( argv[ 0 ] );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
