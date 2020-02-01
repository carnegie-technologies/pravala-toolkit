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
#include <curl/curl.h>
}

#include "basic/Mutex.hpp"
#include "Curl.hpp"

using namespace Pravala;

const String Curl::ContentTypeJson ( "application/json" );
const String Curl::ContentTypeTextHtml ( "text/html" );
const String Curl::ContentTypeTextPlain ( "text/plain" );

static Mutex globMutex ( "Curl" );
static bool globInitialized ( false );

int Curl::globalInit ( String * errorStr )
{
    CURLcode ret = CURLE_OK;

    // Quick check. No need to protect it.
    if ( globInitialized )
        return ret;

    MutexLock m ( globMutex );

    if ( !globInitialized )
    {
        ret = curl_global_init ( CURL_GLOBAL_ALL );

        if ( ret == CURLE_OK )
        {
            globInitialized = true;
        }
        else if ( errorStr != 0 )
        {
            *errorStr = curl_easy_strerror ( ret );
        }
    }

    return ret;
}

size_t Curl::detectHeadersSize ( const char * data, size_t size )
{
    static const char patternA[] = "\n\n";
    static const size_t patternALen = strlen ( patternA );

    static const char patternB[] = "\r\n\r\n";
    static const size_t patternBLen = strlen ( patternB );

    if ( !data )
    {
        return 0;
    }

    for ( size_t i = patternALen; i < size; ++i )
    {
        if ( memcmp ( data + i - patternALen, patternA, patternALen ) == 0
             || ( i >= patternBLen && memcmp ( data + i - patternBLen, patternB, patternBLen ) == 0 ) )
        {
            return i;
        }
    }

    return 0;
}
