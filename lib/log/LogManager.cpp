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
#include <cassert>

#include "LogManager.hpp"
#include "TextLog.hpp"
#include "BinLog.hpp"

using namespace Pravala;

LogManager & LogManager::get()
{
    static LogManager * global = 0;

    if ( !global )
    {
        global = new LogManager();

        assert ( !global->_log );

        global->_log = new TextLog ( "log_manager" );
    }

    return *global;
}

LogManager::LogManager(): _log ( 0 )
{
}

LogManager::~LogManager()
{
    TextLog * tmpLog = _log;

    _log = 0;

    delete tmpLog;
}

void LogManager::registerLog ( TextLog * log )
{
    assert ( log != 0 );

    if ( !log )
        return;

#ifndef NDEBUG
    for ( size_t i = 0; i < _textLogs.size(); ++i )
    {
        assert ( _textLogs[ i ] != log );
    }
#endif

    _textLogs.append ( log );

    if ( _log != 0 && _log != log )
        SLOG ( ( *_log ), L_DEBUG4, "Registering text log: '" << log->LogName << "'" );
}

void LogManager::unregisterLog ( TextLog * log )
{
    assert ( log != 0 );

    if ( !log )
        return;

    _textLogs.removeValue ( log );

    assert ( !_textLogs.findValue ( log ) );

    if ( _log != 0 && _log != log )
        SLOG ( ( *_log ), L_DEBUG4, "Unregistering text log: '" << log->LogName << "'" );
}

StringList LogManager::getRegisteredTextLogs()
{
    StringList ret;

    for ( size_t i = 0; i < _textLogs.size(); ++i )
    {
        assert ( _textLogs[ i ] != 0 );

        ret.append ( _textLogs[ i ]->LogName );
    }

    return ret;
}

void LogManager::registerLog ( BinLog * log )
{
    assert ( log != 0 );

    if ( !log )
        return;

#ifndef NDEBUG
    for ( size_t i = 0; i < _binLogs.size(); ++i )
    {
        assert ( _binLogs[ i ] != log );
    }
#endif

    _binLogs.append ( log );

    if ( _log != 0 )
        SLOG ( ( *_log ), L_DEBUG4, "Registering binary log: '" << log->LogName << "' ["
               << log->LogType.toString() << "]" );
}

void LogManager::unregisterLog ( BinLog * log )
{
    assert ( log != 0 );

    if ( !log )
        return;

    _binLogs.removeValue ( log );

    assert ( !_binLogs.findValue ( log ) );

    if ( _log != 0 )
        SLOG ( ( *_log ), L_DEBUG4, "Unregistering binary log: '" << log->LogName << "' ["
               << log->LogType.toString() << "]" );
}

List< LogManager::LogDesc > LogManager::getRegisteredBinLogs()
{
    List< LogDesc > ret;

    for ( size_t i = 0; i < _binLogs.size(); ++i )
    {
        assert ( _binLogs[ i ] != 0 );

        if ( _binLogs[ i ] != 0 )
        {
            ret.append ( LogDesc ( _binLogs[ i ]->LogType, _binLogs[ i ]->LogName ) );
        }
    }

    return ret;
}

bool LogManager::subscribe ( const String & logPattern, Log::LogLevel logLevel, TextLogOutput * logOutput )
{
    assert ( logOutput != 0 );

    bool ret = false;

    if ( !logOutput )
        return ret;

    for ( size_t i = 0; i < _textLogs.size(); ++i )
    {
        assert ( _textLogs[ i ] != 0 );

        if ( _textLogs[ i ]->LogName.matches ( logPattern ) )
        {
            _textLogs[ i ]->subscribeOutput ( logOutput, logLevel );
            ret = true;
        }
    }

    return ret;
}

void LogManager::unsubscribe ( TextLogOutput * logOutput, const String & logPattern )
{
    assert ( logOutput != 0 );

    if ( !logOutput )
        return;

    for ( size_t i = 0; i < _textLogs.size(); ++i )
    {
        assert ( _textLogs[ i ] != 0 );

        if ( logPattern.isEmpty() || _textLogs[ i ]->LogName.matches ( logPattern ) )
        {
            _textLogs[ i ]->unsubscribeOutput ( logOutput );
        }
    }
}

bool LogManager::subscribe ( const String & logPattern, BinLogOutput * logOutput )
{
    assert ( logOutput != 0 );

    bool ret = false;

    if ( !logOutput )
        return ret;

    for ( size_t i = 0; i < _binLogs.size(); ++i )
    {
        assert ( _binLogs[ i ] != 0 );

        if ( _binLogs[ i ]->LogName.matches ( logPattern ) )
        {
            _binLogs[ i ]->subscribeOutput ( logOutput );
            ret = true;
        }
    }

    return ret;
}

void LogManager::unsubscribe ( BinLogOutput * logOutput, const String & logPattern )
{
    assert ( logOutput != 0 );

    if ( !logOutput )
        return;

    for ( size_t i = 0; i < _binLogs.size(); ++i )
    {
        assert ( _binLogs[ i ] != 0 );

        if ( logPattern.isEmpty() || _binLogs[ i ]->LogName.matches ( logPattern ) )
        {
            _binLogs[ i ]->unsubscribeOutput ( logOutput );
        }
    }

    // It could be subscribed to text logs as well!
    unsubscribe ( ( TextLogOutput * ) logOutput, logPattern );
}

bool LogManager::str2LogLevel ( String levelName, Log::LogLevel & logLevel )
{
    levelName = levelName.removeChars ( "_- ." );

    if ( levelName.compare ( "fatalerror", false ) == 0
         || levelName.compare ( "fatal", false ) == 0 )
    {
        logLevel = Log::LogLevel::FatalError;
        return true;
    }

    if ( levelName.compare ( "error", false ) == 0 )
    {
        logLevel = Log::LogLevel::Error;
        return true;
    }

    if ( levelName.compare ( "warning", false ) == 0
         || levelName.compare ( "warn", false ) == 0 )
    {
        logLevel = Log::LogLevel::Warning;
        return true;
    }

    if ( levelName.compare ( "info", false ) == 0 )
    {
        logLevel = Log::LogLevel::Info;
        return true;
    }

    if ( levelName.compare ( "debug", false ) == 0
         || levelName.compare ( "debug1", false ) == 0 )
    {
        logLevel = Log::LogLevel::Debug;
        return true;
    }

    if ( levelName.compare ( "debug2", false ) == 0 )
    {
        logLevel = Log::LogLevel::Debug2;
        return true;
    }

    if ( levelName.compare ( "debug3", false ) == 0 )
    {
        logLevel = Log::LogLevel::Debug3;
        return true;
    }

    // We use special, 'fake' level: MAX or ALL, which is the same as the most verbose level supported

    if ( levelName.compare ( "debug4", false ) == 0
         || levelName.compare ( "max", false ) == 0
         || levelName.compare ( "all", false ) == 0 )
    {
        logLevel = Log::LogLevel::Debug4;
        return true;
    }

    return false;
}

const char * LogManager::logLevel2Str ( const Log::LogLevel & forLevel )
{
    switch ( forLevel.value() )
    {
        case Log::LogLevel::Invalid:
            return "invalid";
            break;

        case Log::LogLevel::Debug4:
            return "debug4";
            break;

        case Log::LogLevel::Debug3:
            return "debug3";
            break;

        case Log::LogLevel::Debug2:
            return "debug";
            break;

        case Log::LogLevel::Debug:
            return "debug";
            break;

        case Log::LogLevel::Info:
            return "info";
            break;

        case Log::LogLevel::Warning:
            return "warning";
            break;

        case Log::LogLevel::Error:
            return "error";
            break;

        case Log::LogLevel::FatalError:
            return "fatal_error";
            break;
    }

    return "";
}
