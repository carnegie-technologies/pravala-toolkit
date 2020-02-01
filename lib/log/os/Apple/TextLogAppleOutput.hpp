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

#pragma once

#include "../../LogOutput.hpp"

namespace Pravala
{
#ifndef SYSTEM_APPLE
#error This code should ONLY be used on Apple systems!
#endif

/// @brief Class for logging to the standard Apple log stream
class TextLogAppleOutput: public TextLogOutput
{
    protected:
        /// @brief Sends log data to the receiver.
        /// @param [in] logMessage The log message object
        /// @param [in,out] strMessage The string with text version of the message. An empty string is passed
        ///                         at the beginning, and as soon as one of the TextLogOutputs
        ///                         generates the string version, it can be reused by subsequent outputs.
        virtual void sendTextLog ( Log::TextMessage & logMessage, String & strMessage );
};
}
