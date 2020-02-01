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
#include "event/EventManager.hpp"
#include "TextLog.hpp"
#include "LogManager.hpp"

using namespace Pravala;

TextLog::TextLog ( const char * logName ):
    LogName ( logName ), _minLogLevel ( Log::LogLevel::FatalError ), _isActive ( false )
{
    assert ( LogName.find ( '.' ) < 0 );

    LogManager::get().registerLog ( this );
}

TextLog::~TextLog()
{
    LogManager::get().unregisterLog ( this );
}

void TextLog::findMinLevel()
{
    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        if ( i == 0 || _minLogLevel > _outputs[ i ].logLevel.value() )
            _minLogLevel = _outputs[ i ].logLevel.value();
    }
}

void TextLog::unsubscribeOutput ( TextLogOutput * output )
{
    if ( !output )
        return;

    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        if ( _outputs[ i ].output == output )
        {
            _outputs.removeIndex ( i );
            break;
        }
    }

    if ( _outputs.size() < 1 )
    {
        _isActive = false;
        return;
    }

    findMinLevel();
}

void TextLog::subscribeOutput ( TextLogOutput * output, Log::LogLevel logLevel )
{
    if ( !output )
        return;

    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        if ( _outputs[ i ].output == output )
        {
            _outputs[ i ].logLevel = logLevel;

            findMinLevel();

            assert ( _isActive );

            return;
        }
    }

    OutputInfo oi;

    oi.output = output;
    oi.logLevel = logLevel;

    _outputs.append ( oi );

    _isActive = true;

    findMinLevel();
}

void TextLog::send ( TextMessage & logMessage )
{
    assert ( _isActive );

    if ( !_isActive )
        return;

    logMessage.setName ( LogName );
    logMessage.setTime ( CalendarTime::getUTCEpochTimeMs() );
    logMessage.setContent ( logMessage.getInternalBuf().toString() );

    String str;

    for ( size_t i = 0; i < _outputs.size(); ++i )
    {
        if ( _outputs[ i ].logLevel.value() <= logMessage.getLevel().value() )
        {
            _outputs[ i ].output->sendTextLog ( logMessage, str );
        }
    }
}

String TextLog::dumpPrintable ( const char * data, int dataSize )
{
    if ( !data || dataSize < 1 )
        return String::EmptyString;

    String ret;

    ret.reserve ( dataSize );

    bool wasSpace = true;

    for ( int i = 0; i < dataSize; ++i )
    {
        char c = data[ i ];

        // We want to treat a space (code 32) as a 'special' character and not display multiple spaces.
        // Instead we display a single space for each sequence of of spaces and/or other special characters.
        if ( c > 32 && c < 127 )
        {
            wasSpace = false;
            ret.append ( c );
        }
        else
        {
            // We only add a single space for any number of consecutive special characters
            if ( !wasSpace )
                ret.append ( ' ' );

            wasSpace = true;
        }
    }

    return ret;
}

TextLogLimited::TextLogLimited ( const char * logName, uint16_t maxLogs, uint16_t timeInterval ):
    TextLog ( logName ),
    TimeInterval ( ( timeInterval > 0 ) ? timeInterval : 1 ),
    MaxLogs ( ( maxLogs > 0 ) ? maxLogs : 1 )
{
}

TextLogLimited::~TextLogLimited()
{
}

bool TextLogLimited::shouldLog ( const Log::LogLevel::_EnumType & logLevel, uint16_t & counter )
{
    if ( !TextLog::shouldLog ( logLevel ) )
        return false;

    const Time & now = EventManager::getCurrentTime();

    if ( now.isGreaterEqualThan ( _logPeriodStart, TimeInterval ) )
    {
        _logPeriodStart = now;
        counter = 0;

        return true;
    }

    return ( counter < MaxLogs );
}

void TextLogLimited::send ( TextMessage & logMessage, uint16_t & counter )
{
    if ( counter < MaxLogs && ++counter >= MaxLogs )
    {
        logMessage << " [throttling log message]";
    }

    TextLog::send ( logMessage );
}
