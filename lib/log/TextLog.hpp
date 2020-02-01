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

#include "sys/Time.hpp"
#include "LogOutput.hpp"
#include "TextMessage.hpp"

namespace Pravala
{
#ifdef NO_LOGGING
#define SLOG( stream, logLevel, x )                              do {} while ( false )
#define SLOG_NFUNC( stream, logLevel, x )                        do {} while ( false )
#define SLOG_ERR( stream, logLevel, error_code, x )              do { ( void ) error_code; } while ( false )
#define SLOG_NFUNC_ERR( stream, logLevel, error_code, x )        do { ( void ) error_code; } while ( false )

#define SLOG_LIM( stream, logLevel, x )                          do {} while ( false )
#define SLOG_NFUNC_LIM( stream, logLevel, x )                    do {} while ( false )
#define SLOG_ERR_LIM( stream, logLevel, error_code, x )          do { ( void ) error_code; } while ( false )
#define SLOG_NFUNC_ERR_LIM( stream, logLevel, error_code, x )    do { ( void ) error_code; } while ( false )
#else
#define SLOG( stream, logLevel, x ) \
    do { \
        if ( stream.shouldLog ( logLevel ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setFuncName ( __FUNCTION__ ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg ); \
        } \
    } while ( false )

#define SLOG_NFUNC( stream, logLevel, x ) \
    do { \
        if ( stream.shouldLog ( logLevel ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg ); \
        } \
    } while ( false )

#define SLOG_ERR( stream, logLevel, error_code, x ) \
    do { \
        if ( stream.shouldLog ( logLevel ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setFuncName ( __FUNCTION__ ); \
            __tmpLogMsg.setErrorCode ( error_code ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg ); \
        } \
    } while ( false )

#define SLOG_NFUNC_ERR( stream, logLevel, error_code, x ) \
    do { \
        if ( stream.shouldLog ( logLevel ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setErrorCode ( error_code ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg ); \
        } \
    } while ( false )

#define SLOG_LIM( stream, logLevel, x ) \
    do { \
        static uint16_t __int_log_counter = 0; \
        if ( stream.shouldLog ( logLevel, __int_log_counter ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setFuncName ( __FUNCTION__ ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg, __int_log_counter ); \
        } \
    } while ( false )

#define SLOG_NFUNC_LIM( stream, logLevel, x ) \
    do { \
        static uint16_t __int_log_counter = 0; \
        if ( stream.shouldLog ( logLevel, __int_log_counter ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg, __int_log_counter ); \
        } \
    } while ( false )

#define SLOG_ERR_LIM( stream, logLevel, error_code, x ) \
    do { \
        static uint16_t __int_log_counter = 0; \
        if ( stream.shouldLog ( logLevel, __int_log_counter ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setFuncName ( __FUNCTION__ ); \
            __tmpLogMsg.setErrorCode ( error_code ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg, __int_log_counter ); \
        } \
    } while ( false )

#define SLOG_NFUNC_ERR_LIM( stream, logLevel, error_code, x ) \
    do { \
        static uint16_t __int_log_counter = 0; \
        if ( stream.shouldLog ( logLevel, __int_log_counter ) ) \
        { \
            TextMessage __tmpLogMsg; \
            __tmpLogMsg.setLevel ( logLevel ); \
            __tmpLogMsg.setErrorCode ( error_code ); \
            __tmpLogMsg << x; \
            stream.send ( __tmpLogMsg, __int_log_counter ); \
        } \
    } while ( false )
#endif

#define LOG( logLevel, x )                              SLOG ( _log, logLevel, x )
#define LOG_NFUNC( logLevel, x )                        SLOG_NFUNC ( _log, logLevel, x )
#define LOG_ERR( logLevel, error_code, x )              SLOG_ERR ( _log, logLevel, error_code, x )
#define LOG_NFUNC_ERR( logLevel, error_code, x )        SLOG_NFUNC_ERR ( _log, logLevel, error_code, x )

#define LOG_LIM( logLevel, x )                          SLOG_LIM ( _log, logLevel, x )
#define LOG_NFUNC_LIM( logLevel, x )                    SLOG_NFUNC_LIM ( _log, logLevel, x )
#define LOG_ERR_LIM( logLevel, error_code, x )          SLOG_ERR_LIM ( _log, logLevel, error_code, x )
#define LOG_NFUNC_ERR_LIM( logLevel, error_code, x )    SLOG_NFUNC_ERR_LIM ( _log, logLevel, error_code, x )

#define INT2HEX( value ) \
    ( String ( "0x" ).append ( String::number ( ( value ), String::Int_HEX ) ) )

#define L_DEBUG4         ( Pravala::Log::LogLevel::Debug4 )
#define L_DEBUG3         ( Pravala::Log::LogLevel::Debug3 )
#define L_DEBUG2         ( Pravala::Log::LogLevel::Debug2 )
#define L_DEBUG          ( Pravala::Log::LogLevel::Debug )
#define L_INFO           ( Pravala::Log::LogLevel::Info )
#define L_WARN           ( Pravala::Log::LogLevel::Warning )
#define L_ERROR          ( Pravala::Log::LogLevel::Error )
#define L_FATAL_ERROR    ( Pravala::Log::LogLevel::FatalError )

/// @brief Class that provides logging using stream operations
class TextLog
{
    public:
        const String LogName; ///< The name of this log object

        /// @brief Destructor
        ~TextLog();

        /// @brief Constructor
        /// Creates a TextLog object (usually it will be static object) with a given ID.
        /// @param [in] logName The name of this log stream. It should be unique, and it CANNOT include a '.' character
        TextLog ( const char * logName );

        /// @brief Checks whether this log should be used
        /// @param [in] logLevel The log level of the message
        /// @return True if there is someone listening to messages at this level; false otherwise
        inline bool shouldLog ( const Log::LogLevel & logLevel ) const
        {
            // This assumes that the values of more important log levels are HIGHER
            // So if 'debug' has a value of 10, 'warning' could have a value of '30'
            return ( _isActive && logLevel.value() >= _minLogLevel );
        }

        /// @brief Checks whether this log should be used
        /// @param [in] logLevel The log level of the message
        /// @return True if there is someone listening to messages at this level; false otherwise
        inline bool shouldLog ( const Log::LogLevel::_EnumType & logLevel ) const
        {
            // This assumes that the values of more important log levels are HIGHER
            // So if 'debug' has a value of 10, 'warning' could have a value of '30'
            return ( _isActive && logLevel >= _minLogLevel );
        }

        /// @brief Sends given text log message to its outputs
        /// @param [in] logMessage The message to send. This method sets 'name' 'time' and 'content' fields
        ///                         and will overwrite existing values
        void send ( TextMessage & logMessage );

        /// @brief Subscribes (or modifies the log level of) given output
        /// @param [in] output Output to subscribe/modify
        /// @param [in] logLevel The log level to use. If this log level is less verbose than the level
        ///                       already set, nothing happens. This function can be only used to increase verbosity.
        void subscribeOutput ( TextLogOutput * output, Log::LogLevel logLevel );

        /// @brief Unsubscribes given output from this log object
        /// @param [in] output Output to unsubscribe
        void unsubscribeOutput ( TextLogOutput * output );

        /// @brief A helper function for dumping printable bytes of binary data
        ///
        /// It is not a data conversion function.
        /// It should be used just to get an idea about the content of any binary data.
        /// The result will contain all characters with ASCII codes between 33 and 126.
        /// Instead of each sequence of special characters of any length, exactly one space
        /// is used. Spaces are also treated as special characters.
        ///
        /// For example, following string:
        /// "Abc\n\rX  Y    Z\n   \nFoo     Bar\n"
        /// Will be converted to:
        /// "Abc X Y Z Foo Bar "
        ///
        /// @param [in] data Pointer to the beginning of the binary data
        /// @param [in] dataSize Number of bytes to dump
        /// @return Printable bytes in the binary data
        static String dumpPrintable ( const char * data, int dataSize );

    private:
        /// @brief A structure that describes a single output
        struct OutputInfo
        {
            TextLogOutput * output; ///< Pointer to the output
            Log::LogLevel logLevel; ///< Log level of this output
        };

        List<OutputInfo> _outputs; ///< Outputs subscribed to this log

        Log::LogLevel::_EnumType _minLogLevel; ///< The lowest log level used by the outputs
        bool _isActive; ///< Set to true if there is at least one subscriber

        /// @brief No default constructor
        TextLog();

        /// @brief Finds the minimum log level used by the outputs
        void findMinLevel();

        friend class LogManager;
};

/// @brief Class that can be used to create log streams that limit the number of logs generated in a time period.
/// It does NOT use a sliding window, but after each time interval it will clear all the counters and start over.
/// It can be used with *_LIM() family of logging macros (unlike the regular TextLog).
/// It can still be used with regular logging macros for log messages that should never be limited.
/// @note This uses EventManager::getCurrentTime().
class TextLogLimited: public TextLog
{
    public:
        /// @brief The length of time (in seconds) over which the number of messages will be counted.
        const uint16_t TimeInterval;

        /// @brief The max number of logs to generate during every TimeInterval period.
        const uint16_t MaxLogs;

        /// @brief Destructor.
        ~TextLogLimited();

        /// @brief Constructor.
        /// This version creates a brand new log stream.
        /// @param [in] logName The name of this log stream. It should be unique, and it CANNOT include a '.' character.
        /// @param [in] maxLogs The max number of logs to generate over given time period.
        /// @param [in] timeInterval The length of time (in seconds) over which the number of messages will be counted.
        ///                          Note that there is no sliding window, basically after every
        ///                          time interval since the beginning the counters will be cleared.
        TextLogLimited ( const char * logName, uint16_t maxLogs = 4, uint16_t timeInterval = 60 );

        /// @brief Checks whether this log should be used.
        /// @note This does not actually increment the counter, but it may clear it.
        /// @param [in] logLevel The log level of the message.
        /// @param [in,out] counter The value to be used as a log counter.
        /// @return True if the message at this level should be logged; False otherwise.
        inline bool shouldLog ( const Log::LogLevel & logLevel, uint16_t & counter )
        {
            return shouldLog ( logLevel.value(), counter );
        }

        /// @brief Checks whether this log should be used.
        /// @note This does not actually increment the counter, but it may clear it.
        /// @param [in] logLevel The log level of the message.
        /// @param [in,out] counter The value to be used as a log counter.
        /// @return True if the message at this level should be logged; False otherwise.
        bool shouldLog ( const Log::LogLevel::_EnumType & logLevel, uint16_t & counter );

        /// @brief Sends given text log message to its outputs.
        /// It also increments the appropriate counters and append a note to the message
        /// with explanation that this log level is now being throttled.
        /// @note This does NOT check if the log should be generated, and will send the message regardless.
        /// @param [in] logMessage The message to send. This method sets 'name' 'time' and 'content' fields
        ///                         and will overwrite existing values.
        /// @param [in,out] counter The value to be used as a log counter.
        void send ( TextMessage & logMessage, uint16_t & counter );

        using TextLog::shouldLog;
        using TextLog::send;

    protected:
        /// @brief The beginning of the current logging time period.
        /// Set to current time by the first shouldLog() call after each TimeInterval.
        Time _logPeriodStart;
};
}
