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

#include "../../GpsMonitor.hpp"

#include "GpsMonitorPriv.hpp"

using namespace Pravala;

GpsMonitor::GpsMonitor ( Receiver & receiver ): _receiver ( receiver ), _p ( new GpsMonitorPriv ( *this ) )
{
    assert ( _p != 0 );
}

GpsMonitor::~GpsMonitor()
{
    if ( _p != 0 )
    {
        delete _p;
        _p = 0;
    }
}

bool GpsMonitor::start()
{
    assert ( _p != 0 );
    return _p->start();
}

void GpsMonitor::stop()
{
    assert ( _p != 0 );
    _p->stop();
}
