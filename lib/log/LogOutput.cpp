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

#ifndef SYSTEM_WINDOWS
extern "C"
{
#include <unistd.h>
}
#endif

#include "sys/CalendarTime.hpp"

#include "LogOutput.hpp"
#include "LogManager.hpp"

#include "auto/log/Log/TextMessage.hpp"

using namespace Pravala;

#ifndef SYSTEM_WINDOWS
ConfigNumber<bool> TextLogOutput::optIncludeMemInfo (
        0,
        "log-mem-info", 0,
        "text_log_output.include_mem_info",
        "Include memory information in text logs. Currently it includes the size of RSS (as a number of pages).",
        false );
#endif

BinLogOutput::~BinLogOutput()
{
    LogManager::get().unsubscribe ( this );
}

TextLogOutput::~TextLogOutput()
{
    LogManager::get().unsubscribe ( this );
}

void BinLogOutput::sendTextLog ( Log::TextMessage & logMessage, String & )
{
    sendBinLog ( logMessage );
}

void TextLogOutput::formatMessage ( Log::TextMessage & logMessage, String & strMessage )
{
    formatMessage ( logMessage,
                    CalendarTime::getTimeDesc ( logMessage.hasTime()
                                                ? logMessage.getTime()
                                                : CalendarTime::getUTCEpochTimeMs() ),
                    strMessage );
}

void TextLogOutput::formatMessage ( Log::TextMessage & logMessage, const String & timeDesc, String & strMessage )
{
    strMessage.clear();

    strMessage.append ( timeDesc );

#ifndef SYSTEM_WINDOWS
    if ( optIncludeMemInfo.value() )
    {
        MemHandle mh ( String ( "/proc/%1/statm" ).arg ( getpid() ) );

        if ( !mh.isEmpty() )
        {
            StringList sList = mh.toStringList ( " " );

            if ( sList.size() >= 2 )
            {
                strMessage.append ( "|RSS:" ).append ( sList.at ( 1 ) );
            }
        }
    }
#endif

    if ( logMessage.hasName() )
    {
        strMessage.append ( "|" ).append ( logMessage.getName() );
    }

    if ( logMessage.hasLevel() )
    {
        strMessage.append ( "|" );

        const char * lvlDesc = logMessage.getLevel().toString();

        // We want to use everything up to the first space (or \0)
        size_t i = 0;

        while ( true )
        {
            if ( lvlDesc[ i ] == 0 || lvlDesc[ i ] == ' ' )
            {
                strMessage.append ( lvlDesc, i );
                break;
            }

            ++i;
        }
    }

    if ( logMessage.hasFuncName() )
    {
        strMessage.append ( "|" );
        strMessage.append ( logMessage.getFuncName() );
    }

    if ( logMessage.hasErrorCode() )
    {
        strMessage.append ( "|" );
        strMessage.append ( logMessage.getErrorCode().toString() );
    }

    if ( logMessage.hasContent() )
    {
        strMessage.append ( "|" );
        strMessage.append ( logMessage.getContent() );
    }

    if ( !strMessage.isEmpty() )
    {
        strMessage.append ( "\n" );
    }
}
