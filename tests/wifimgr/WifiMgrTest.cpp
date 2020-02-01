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

#include "WifiMgrTest.hpp"

#include <cstdio>

using namespace Pravala;

WifiMgrTest::WifiMgrTest ( const String & wpaSockName ): _wmMonitor ( *this, wpaSockName )
{
}

WifiMgrTest::~WifiMgrTest()
{
}

void WifiMgrTest::wifiScanResultReady ( WifiMgrMonitor * monitor )
{
    ( void ) monitor;
    assert ( &_wmMonitor == monitor );

    fprintf ( stdout, "Received Wi-Fi scan results ready\n" );
}

void WifiMgrTest::wifiStateChanged ( WifiMgrMonitor * monitor, WifiMgrTypes::State state )
{
    ( void ) monitor;
    assert ( &_wmMonitor == monitor );

    fprintf ( stdout, "Received Wi-Fi state change: %d\n", state );
}
