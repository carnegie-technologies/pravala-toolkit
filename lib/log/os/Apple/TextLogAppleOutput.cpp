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
#include "PravalaNSLogSimple.h"
}

#include "TextLogAppleOutput.hpp"
#include "auto/log/Log/TextMessage.hpp"

#ifndef SYSTEM_APPLE
#error This code should ONLY be used on Apple systems!
#endif

using namespace Pravala;

void TextLogAppleOutput::sendTextLog ( Log::TextMessage & logMessage, String & strMessage )
{
    if ( strMessage.isEmpty() )
    {
        formatMessage ( logMessage, strMessage );

        if ( strMessage.isEmpty() )
            return;
    }

    assert ( strMessage.length() > 0 );

    Pravala_NSLog_Simple ( strMessage.c_str() );
}
