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

#include "sys/CalendarTime.hpp"

#include "PrometheusManager.hpp"
#include "PrometheusMetric.hpp"
#include "PacketDataStoreMetrics.hpp"

using namespace Pravala;

TextLog PrometheusManager::_log ( "prometheus_manager" );

static PacketDataStoreBlocksGauge gaugePacketDataStoreFreeBlocks ( PacketDataStoreBlocksGauge::TypeFree );
static PacketDataStoreBlocksGauge gaugePacketDataStoreAllocatedBlocks ( PacketDataStoreBlocksGauge::TypeAllocated );

static PacketDataStoreMissesCounter counterPacketDataStoreMisses;

PrometheusManager::PrometheusManager():
    _maxAllocatedBufSize ( 0 )
{
}

PrometheusManager & PrometheusManager::get()
{
    static PrometheusManager * global = 0;

    if ( !global )
    {
        global = new PrometheusManager();
    }

    return *global;
}

void PrometheusManager::registerMetric ( PrometheusMetric & metricToRegister )
{
    if ( metricToRegister.Name.isEmpty() )
        return;

    if ( _metrics.contains ( metricToRegister.Name ) )
    {
        LOG ( L_FATAL_ERROR, "Prometheus metric already registered under the same name: '"
              << metricToRegister.Name << "'" );

        assert ( false );
    }
    else
    {
        _metrics.insert ( metricToRegister.Name, &metricToRegister );
    }
}

void PrometheusManager::unregisterMetric ( PrometheusMetric & metricToRemove )
{
    if ( _metrics.remove ( metricToRemove.Name ) < 1 )
    {
        LOG ( L_WARN, "Prometheus metric '" << metricToRemove.Name << "' was not registered" );
    }
}

MemHandle PrometheusManager::getData()
{
    Buffer buf ( _maxAllocatedBufSize );

    const uint64_t curTime = CalendarTime::getUTCEpochTimeMs();

    for ( HashMap<String, PrometheusMetric *>::Iterator it ( _metrics ); it.isValid(); it.next() )
    {
        it.value()->appendData ( buf, curTime );
    }

    // Save the maximum allocated buffer size for the text exposition internally, so that we can pre-allocate the next
    // buffer with a size that should be sufficient for what is required.

    if ( buf.size() > _maxAllocatedBufSize )
    {
        _maxAllocatedBufSize = buf.size();
    }

    return buf.getHandle();
}
