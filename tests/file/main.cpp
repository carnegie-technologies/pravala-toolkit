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

#include "basic/MemHandle.hpp"
#include "basic/internal/MemBlock.hpp"

using namespace Pravala;

int main ( int argc, char * argv[] )
{
    if ( argc != 2 )
    {
        fprintf ( stderr, "Usage: %s file_path\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    bool isOk = true;
    MemHandle mh ( argv[ 1 ], &isOk );

    if ( !isOk )
    {
        fprintf ( stderr, "Error opening file '%s'\n", argv[ 1 ] );
        return EXIT_FAILURE;
    }

    printf ( "File        : '%s'\n", argv[ 1 ] );
    printf ( "Size        :  %d bytes\n", ( int ) mh.size() );
    printf ( "Memory type :  %d", mh.getMemoryType() );

    switch ( mh.getMemoryType() )
    {
        case MemBlock::TypeBuffer:
            printf ( " (buffer)" );
            break;

        case MemBlock::TypeMMapRO:
            printf ( " (read-only memory map)" );
            break;
    }

    printf ( "\nData        :\n" );

    for ( size_t i = 0; i < mh.size(); ++i )
    {
        char c = mh.get ( i )[ 0 ];

        if ( c < 9 || c > 126 )
            c = '.';

        printf ( "%c", c );
    }

    printf ( "\n" );

    return EXIT_SUCCESS;
}
