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

#include "LogOutput.hpp"
#include "auto/log/Log/LogType.hpp"

namespace Pravala
{
#define BLOG( log, x ) \
    do { \
        if ( ( log ).isActive() ) \
        { \
            ( log ).send ( x ); \
        } \
    } while ( false )

/// @brief Class that provides binary logging using stream operations
class BinLog
{
    public:
        const Log::LogType LogType; ///< The type of this log object
        const String LogName; ///< The name of this log object

        /// @brief Destructor
        ~BinLog();

        /// @brief Constructor
        /// Creates a BinLog object (usually it will be static object) with a given ID.
        /// @param [in] logName The name of this log stream. It should be unique, and it CANNOT include a '.' character
        BinLog ( const Log::LogType & logType, const char * logName );

        /// @brief Checks if this log is active
        /// @return True if there is someone listening to this log; false otherwise
        inline bool isActive() const
        {
            return _isActive;
        }

        /// @brief Sends given text log message to its outputs
        /// @param [in] logMessage The message to send. This method sets 'name' and 'time' fields
        ///                         and will overwrite existing values
        void send ( Log::LogMessage & logMessage );

        /// @brief Subscribes given output
        /// @param [in] output Output to subscribe
        void subscribeOutput ( BinLogOutput * output );

        /// @brief Unsubscribes given output from this log object
        /// @param [in] output Output to unsubscribe
        void unsubscribeOutput ( BinLogOutput * output );

    private:
        List<BinLogOutput *> _outputs; ///< Outputs subscribed to this log
        bool _isActive; ///< Set to true if there is at least one subscriber

        /// @brief No default constructor
        BinLog();

        friend class LogManager;
};
}
