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
// This must be in this order!
#include <winsock2.h>
#include <windows.h>
#include <WinBase.h>
}

#include "../../Process.hpp"

using namespace Pravala;

void Process::copyCurrentEnvironment ( HashMap<String, String> & env )
{
    char * envPtr = ::GetEnvironmentStrings();

    assert ( envPtr != 0 );

    if ( !envPtr ) return;

    char * entry = envPtr;

    // The format of the string in envPtr is:
    // key1=val1\0key2=val2\0\0
    // That is, '\0' separates each key/value, and an empty string means end of list.
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms683187%28v=vs.85%29.aspx
    //
    // At the start of the loop, entry points to the start of the current entry.
    // At the end of the loop, entry points to the end of the last entry.
    for ( int idx = 0; entry[ idx ] != 0; ++idx )
    {
        if ( entry[ idx ] == '=' )
        {
            if ( idx > 0 )
            {
                String key ( entry, idx );
                String val ( entry + idx + 1 );
                env.insert ( key, val );

                // key + "=" + value to put entry at the null position
                idx += key.length() + 1 + val.length();

                assert ( entry[ idx ] == '\0' );
            }
            else
            {
                // This is weird, it means we have an empty variable. Exit the loop.
                break;
            }
        }
    }

    ::FreeEnvironmentStrings ( envPtr );
}
