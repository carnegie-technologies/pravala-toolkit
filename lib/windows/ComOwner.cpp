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

#include "ComOwner.hpp"

extern "C"
{
#include <objbase.h>
}

#include <atlbase.h>

#pragma comment(lib, "ole32.lib")

// We need a global COM module to allow classes inheriting from CComModule to initialize properly.
static CComModule _Module;

using namespace Pravala;

ComOwner::ComOwner(): _initRet ( CoInitializeEx ( 0, COINIT_MULTITHREADED ) )
{
}

ComOwner::~ComOwner()
{
    // We only want to unitialize COM if we successfully initialized it on startup.
    if ( SUCCEEDED ( _initRet ) )
    {
        CoUninitialize();
    }
}

bool ComOwner::isComReady() const
{
    // If COM was successfully initialized COM is ready to be used.
    return ( SUCCEEDED ( _initRet ) );
}
