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
#include "BinLog.hpp"
#include "LogManager.hpp"

#include "auto/log/Log/LogMessage.hpp"

using namespace Pravala;

BinLog::BinLog ( const Log::LogType & logType, const char * logName ):
    LogType ( logType ), LogName ( logName ), _isActive ( false )
{
    assert ( LogName.find ( '.' ) < 0 );

    LogManager::get().registerLog ( this );
}

BinLog::~BinLog()
{
    LogManager::get().unregisterLog ( this );
}

void BinLog::unsubscribeOutput ( BinLogOutput * output )
{
    if ( !output )
        return;

    _outputs.removeValue ( output );

    if ( _outputs.size() < 1 )
    {
        _isActive = false;
    }
}

void BinLog::subscribeOutput ( BinLogOutput * output )
{
    if ( !output )
        return;

    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        if ( _outputs[ i ] == output )
        {
            assert ( _isActive );

            return;
        }
    }

    _outputs.append ( output );

    _isActive = true;
}

void BinLog::send ( Log::LogMessage & logMessage )
{
    assert ( _isActive );

    if ( !_isActive )
        return;

    logMessage.setName ( LogName );
    logMessage.setTime ( CalendarTime::getUTCEpochTimeMs() );

    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        _outputs[ i ]->sendBinLog ( logMessage );
    }
}
