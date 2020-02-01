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

#include "error/Error.hpp"

#include "config/ConfigNumber.hpp"

namespace Pravala
{
namespace Log
{
class TextMessage;
class LogMessage;
class LogLevel;
}

class Buffer;

/// @brief Interface for objects that wish to receive text log data.
class TextLogOutput
{
    public:
#ifndef SYSTEM_WINDOWS
        /// @brief If set, text logs will include memory usage information.
        static ConfigNumber<bool> optIncludeMemInfo;
#endif

        /// @brief Generates a human-friendly version of the message
        /// It is a helper function that can be used by various TextLogOutputs. It serializes
        /// the buffer into human-friendly format that can be written into the log file.
        /// The message includes the EOL character.
        /// @param [in] logMessage The log message object to serialize
        /// @param [out] strMessage The output string. It is cleared before serialization
        /// @return Standard error code
        static void formatMessage ( Log::TextMessage & logMessage, String & strMessage );

        /// @brief Generates a human-friendly version of the message
        /// It is a helper function that can be used by various TextLogOutputs. It serializes
        /// the buffer into human-friendly format that can be written into the log file.
        /// The message includes the EOL character.
        /// This version ignores the time field in the log message and uses the timeDesc provided instead.
        /// @param [in] logMessage The log message object to serialize
        /// @param [in] timeDesc The time description to use
        /// @param [out] strMessage The output string. It is cleared before serialization
        /// @return Standard error code
        static void formatMessage ( Log::TextMessage & logMessage, const String & timeDesc, String & strMessage );

        /// @brief Virtual destructor.
        virtual ~TextLogOutput();

    protected:
        /// @brief Sends log data to the receiver.
        /// @param [in] logMessage The log message object
        /// @param [in,out] strMessage The string with text version of the message. An empty string is passed
        ///                         at the beginning, and as soon as one of the TextLogOutputs
        ///                         generates the string version, it can be reused by subsequent outputs.
        ///                         This is done for performance reasons. Each of the text outputs will,
        ///                         most likely, serialize the message using formatMessage() function.
        ///                         The first one that does that, should put the result in this variable,
        ///                         making it possible for all subsequent outputs to just reuse it,
        ///                         without having to serialize it from scratch. If the output serializes
        ///                         data differently (like, for example, BinLogOutput), it should ignore
        ///                         this parameter. We also don't serialize each message by default before
        ///                         passing it to text outputs because we don't know if any of them wants to use it.
        ///                         For example, if all of them happen to be bin outputs, none of them will
        ///                         serialize it using the formatMessage() functions.
        virtual void sendTextLog ( Log::TextMessage & logMessage, String & strMessage ) = 0;

        friend class TextLog;
};

/// @brief Interface for objects that wish to receive any log data.
/// Any BinLogOutput can also receive messages from text logs.
/// BinLogOutput inherits TextLogOutput because it is more general. It should be able to accept
/// any kind of message, and can be used anywhere TextLogOutput is used. The other way around is not true,
/// TextLogOutput cannot accept any log messages other than TextMessage.
class BinLogOutput: public TextLogOutput
{
    protected:
        /// @brief Virtual destructor.
        virtual ~BinLogOutput();

        /// @brief Sends log data to the receiver.
        /// @param [in] logMessage The log message object
        /// @param [in,out] strMessage The string with text version of the message. This output ignores it.
        virtual void sendTextLog ( Log::TextMessage & logMessage, String & strMessage );

        /// @brief Sends log data to the receiver.
        /// @param [in] logMessage The log message object
        virtual void sendBinLog ( Log::LogMessage & logMessage ) = 0;

        friend class BinLog;
};
}
