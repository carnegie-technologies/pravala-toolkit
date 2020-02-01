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

#include <android/log.h>

#include "TextLogAndroidOutput.hpp"
#include "auto/log/Log/TextMessage.hpp"

#ifndef PLATFORM_ANDROID
#error This code should ONLY be used on Android platform!
#endif

using namespace Pravala;

void TextLogAndroidOutput::sendTextLog ( Log::TextMessage & logMessage, String & strMessage )
{
    if ( strMessage.isEmpty() )
    {
        formatMessage ( logMessage, strMessage );

        if ( strMessage.isEmpty() )
            return;
    }

    assert ( strMessage.length() > 0 );

    int logPrio;

    switch ( logMessage.getLevel().value() )
    {
        case Log::LogLevel::Debug2:
        case Log::LogLevel::Debug3:
        case Log::LogLevel::Debug4:
        case Log::LogLevel::Debug:
            // We could use ANDROID_LOG_VERBOSE for debug2-4 instead but it's more difficult to use.
            // They should be disabled by default, and if we enable them we probably want to see them
            // regardless of logcat settings (VERBOSE level is not displayed by default).
            logPrio = ANDROID_LOG_DEBUG;
            break;

        case Log::LogLevel::Info:
            logPrio = ANDROID_LOG_INFO;
            break;

        case Log::LogLevel::Warning:
            logPrio = ANDROID_LOG_WARN;
            break;

        case Log::LogLevel::Error:
            logPrio = ANDROID_LOG_ERROR;
            break;

        case Log::LogLevel::FatalError:
            logPrio = ANDROID_LOG_FATAL;
            break;

        default:
            logPrio = ANDROID_LOG_INFO;
            break;
    }

    __android_log_print ( logPrio, "Pravala", "%s", strMessage.c_str() );
}
