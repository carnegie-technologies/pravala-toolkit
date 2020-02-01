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
#include <cstring>

#include "basic/MemHandle.hpp"
#include "basic/PropMap.hpp"

#define OPT_BEG    "#_ECFG_OPT_BEGIN_"
#define OPT_END    "#_ECFG_OPT_END_"

using namespace Pravala;

enum InspectResult
{
    FileNotFound,
    BlockOk,
    BlockInvalid,
    BlockNotFound
};

InspectResult inspectFile ( const char * fileName )
{
    bool isOk = false;
    const MemHandle input ( fileName, &isOk );

    if ( !isOk )
    {
        fprintf ( stderr, "Error reading file '%s'\n", fileName );

        return FileNotFound;
    }

    const char * mem = input.get();
    const size_t lenBeg = strlen ( OPT_BEG );
    const size_t lenEnd = strlen ( OPT_END );

    size_t sz;

    // +1 for the fact that if at the end of the input
    // there is '#BEG#END' we still want to find it. Without that +1 we wouldn't.
    if ( lenBeg + lenEnd < input.size() )
    {
        sz = input.size() - ( lenBeg + lenEnd ) + 1;
    }
    else
    {
        sz = 0;
    }

    InspectResult res = BlockNotFound;

    for ( size_t i = 0; i < sz; ++i )
    {
        assert ( i + lenBeg <= input.size() );

        if ( memcmp ( mem + i, OPT_BEG, lenBeg ) != 0 )
        {
            // for ( size_t i = 0; i < sz; ++i ) loop
            continue;
        }

        size_t offBeg = i + lenBeg;

        for ( size_t j = offBeg; j < sz; ++j )
        {
            assert ( j + lenEnd <= input.size() );

            if ( memcmp ( mem + j, OPT_END, lenEnd ) != 0 )
            {
                // for ( size_t j = offBeg; j < sz; ++j ) loop
                continue;
            }

            assert ( j >= offBeg );

            StringList lines = String ( mem + offBeg, j - offBeg ).split ( "\n" );

            bool correct = true;

            // We run this as long as all the lines inside are correct (contain only printable characters and have
            // correct syntax). If there are issues we want to interrupt the loop AND not "use" that ECFG block.
            for ( size_t k = 0; correct && k < lines.size(); ++k )
            {
                const String & str = lines[ k ];

                for ( int l = 0; correct && l < str.length(); ++l )
                {
                    if ( str[ l ] < ' ' || str[ l ] > '~' )
                        correct = false;
                }

                if ( correct )
                {
                    String a;
                    String b;
                    correct = PropMap::parseProperty ( str, a, b );
                }
            }

            if ( correct )
            {
                fprintf ( stdout, "* '%s': Found ECFG block *\n\n", fileName );

                for ( size_t k = 0; correct && k < lines.size(); ++k )
                {
                    const String & str = lines[ k ];

                    fprintf ( stdout, "%s\n", str.c_str() );
                }

                fprintf ( stdout, "\n" );

                // -1 for the increment that will happen at the end at i's 'for' loop
                i = j + lenEnd - 1;
                j = sz;

                if ( res == BlockNotFound )
                {
                    res = BlockOk;
                }
            }
            else
            {
                fprintf ( stdout, "* '%s': Found incorrect ECFG block *\n\n", fileName );
                res = BlockInvalid;
            }
        }
    }

    if ( res == BlockNotFound )
    {
        fprintf ( stdout, "* '%s': No correct ECFG blocks found *\n\n", fileName );
    }

    return res;
}

int main ( int argc, char * argv[] )
{
    if ( argc < 2 )
    {
        fprintf ( stderr, "\n %s binary_name [binary_name] ...\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    fprintf ( stdout, "\n" );

    bool allOk = true;

    for ( int i = 1; i < argc; ++i )
    {
        if ( inspectFile ( argv[ i ] ) != BlockOk )
            allOk = false;
    }

    return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
