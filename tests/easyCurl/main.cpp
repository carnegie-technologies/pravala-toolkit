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

// Simple test tool for EasyCurl wrapper.
//
// It could be, for example, used against http://httpbin.org server:
// EasyCurlTest get http://httpbin.org/get
// EasyCurlTest delete http://httpbin.org/delete
// EasyCurlTest post http://httpbin.org/post file.txt
// EasyCurlTest put http://httpbin.org/put file.txt

#include <cstdio>
#include <cstdlib>

#include "curl/core/EasyCurl.hpp"

using namespace Pravala;

static int printHelp ( const char * argv0, const String & msg )
{
    fprintf ( stderr, "Error: %s\n", msg.c_str() );
    fprintf ( stderr, "Usage: %s get|post|put|delete url [file]\n", argv0 );

    return EXIT_FAILURE;
}

int main ( int argc, char * argv[] )
{
    if ( argc < 3 )
        return printHelp ( argv[ 0 ], "Not enough arguments" );

    const String op ( String ( argv[ 1 ] ).toUpper() );

    EasyCurl * curl = 0;

    if ( op == "GET" || op == "DELETE" )
    {
        if ( argc > 3 )
            return printHelp ( argv[ 0 ], "Too many arguments" );

        curl = new EasyCurl ( argv[ 2 ], ( op == "GET" ) ? EasyCurl::MethodGet : EasyCurl::MethodDelete );
    }
    else if ( op == "POST" || op == "PUT" )
    {
        if ( argc > 4 )
            return printHelp ( argv[ 0 ], "Too many arguments" );

        MemHandle mh;

        if ( argc > 3 )
        {
            mh = MemHandle ( argv[ 3 ] );

            if ( mh.isEmpty() )
                return printHelp ( argv[ 0 ], String ( "Error reading data to %s from '%s'" ).arg ( op, argv[ 3 ] ) );
        }
        else if ( op == "PUT" )
        {
            return printHelp ( argv[ 0 ], "PUT request requires a file argument" );
        }

        curl = new EasyCurl ( argv[ 2 ],
                              ( op == "POST" ) ? EasyCurl::MethodPost : EasyCurl::MethodPut,
                              0,
                              mh,
                              mh.isEmpty() ? "" : "text/plain" );
    }
    else
    {
        return printHelp ( argv[ 0 ], String ( "Unknown operation '%1'" ).arg ( op ) );
    }

    assert ( curl != 0 );

    ERRCODE eCode = curl->run();

    printf ( "%s on '%s' completed\n", op.c_str(), curl->Url.c_str() );
    printf ( "Result: %s; HTTP: %d\n", eCode.toString(), curl->getResponseCode() );
    printf ( "\nHTTP headers:\n" );

    for ( HashMap<String, String>::Iterator it ( curl->getRcvdHeaders() ); it.isValid(); it.next() )
    {
        printf ( "%s : %s\n", it.key().c_str(), it.value().c_str() );
    }

    printf ( "\nData:\n%s\n", curl->getRcvdData().toString().c_str() );

    delete curl;
    curl = 0;

    return EXIT_SUCCESS;
}
