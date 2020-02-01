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

#include "basic/NoCopy.hpp"
#include "LogOutput.hpp"
#include "auto/log/Log/LogType.hpp"

namespace Pravala
{
class TextLog;
class BinLog;

/// @brief Class responsible for managing streams and their subscribers.
class LogManager: public NoCopy
{
    public:
        /// @brief A structure describing log streams
        struct LogDesc
        {
            Log::LogType logType; ///< The type of the log
            String logName; ///< The name of the log

            /// @brief Constructor
            /// @param [in] lType The type of the log
            /// @param [in] lName The name of the log
            LogDesc ( const Log::LogType & lType, const String & lName ):
                logType ( lType ), logName ( lName )
            {
            }
        };

        /// @brief Returns a reference to the global LogManager.
        static LogManager & get();

        /// @brief Subscribes given binary LogOutput to all the logs with matching ids.
        /// @param [in] logPattern Pattern to be used for log id matching.
        ///  If pattern '*' is used, this LogOutput is subscribed to all the logs.
        /// @param [in] logOutput LogOutput to subscribe.
        /// @return True if the output has been subscribed to at least one log stream; False otherwise
        bool subscribe ( const String & logPattern, BinLogOutput * logOutput );

        /// @brief Subscribes given text LogOutput to all the logs with matching ids.
        /// @param [in] logPattern Pattern to be used for log id matching.
        ///  If pattern '*' is used, this LogOutput is subscribed to all the logs.
        /// @param [in] logLevel The log level to use
        /// @param [in] logOutput TextLogOutput to subscribe.
        /// @return True if the output has been subscribed to at least one log stream; False otherwise
        bool subscribe ( const String & logPattern, Log::LogLevel logLevel, TextLogOutput * logOutput );

        /// @brief Unsubscribes given LogOutput from all the logs.
        /// @param [in] logOutput LogOutput to unsubscribe.
        /// @param [in] logPattern If used, it will be unsubscribed only from matching streams
        void unsubscribe ( TextLogOutput * logOutput, const String & logPattern = "" );

        /// @brief Unsubscribes given LogOutput from all the logs.
        /// @param [in] logOutput LogOutput to unsubscribe.
        /// @param [in] logPattern If used, it will be unsubscribed only from matching streams
        void unsubscribe ( BinLogOutput * logOutput, const String & logPattern = "" );

        /// @brief Returns a list of registered bin logs.
        /// @return A list of registered logs.
        List<LogDesc> getRegisteredBinLogs();

        /// @brief Returns a list of registered text logs.
        /// @return A list of registered logs.
        StringList getRegisteredTextLogs();

        /// @brief Converts a log level name to LogLevel value
        /// @param [in] levelName The name of the log level to convert
        /// @param [out] logLevel Converted value. If the name is incorrect it is not modified.
        /// @return True if the conversion was successful, false otherwise
        static bool str2LogLevel ( String levelName, Log::LogLevel & logLevel );

        /// @brief Returns the log level name for the specific level
        /// @param [in] forLevel the log level
        /// @return the name of this level
        static const char * logLevel2Str ( const Log::LogLevel & forLevel );

    private:
        TextLog * _log; ///< Internal log stream.

        /// @brief List of registered binary Logs
        List<BinLog *> _binLogs;

        /// @brief List of registered text Logs
        List<TextLog *> _textLogs;

        /// @brief Private constructor
        /// Only one LogManager allowed.
        LogManager();

        /// @brief Private destructor
        ~LogManager();

        /// @brief Registers a log.
        /// @param [in] log Log to register.
        void registerLog ( TextLog * log );

        /// @brief Registers a log.
        /// @param [in] log Log to register.
        void registerLog ( BinLog * log );

        /// @brief Unregisters a log.
        /// @param [in] log Log to unregister.
        void unregisterLog ( TextLog * log );

        /// @brief Unregisters a log.
        /// @param [in] log Log to unregister.
        void unregisterLog ( BinLog * log );

        friend class TextLog;
        friend class BinLog;
};
}
