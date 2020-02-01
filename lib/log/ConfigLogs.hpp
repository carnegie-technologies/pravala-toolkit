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
#include "config/ConfigSwitch.hpp"
#include "auto/log/Log/LogLevel.hpp"

namespace Pravala
{
class TextLogOutput;

/// @brief Config option that holds log configuration
class ConfigLogs: public NoCopy, public ConfigOpt
{
    public:
        /// @brief Constructor
        /// @param [in] logPrefix The log prefix to use
        /// @param [in] stdVerbositySwitch If set to true, the standard '-v/--verbose' command line
        ///                                 switch will be created for this log object.
        /// @param [in] flags The initial flags to set
        ConfigLogs ( const char * logPrefix = "log", bool stdVerbositySwitch = true, uint8_t flags = FlagIsCommon );

        /// @brief Returns a map representation of the option value(s)
        /// It simply returns a map with a single entry: opt_name:value_list.
        /// @return string representation of the option value(s)
        virtual HashMap<String, StringList> getValues() const;

        /// @brief Destructor
        virtual ~ConfigLogs();

        /// @brief Initializes all logs
        /// @param [in] daemonMode Whether we are running in daemon mode
        /// In the daemon mode, no new STDOUT or STDERR outputs will be generated.
        /// On Android, ANDROID output will be used instead.
        /// @return Standard error code
        ERRCODE init ( bool daemonMode = false );

        /// @brief Closes all existing outputs
        void closeAllOutputs();

        /// @brief Enables the verbosity switch option.
        /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
        /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
        /// @param [in] helpText The help text for this option.
        /// @return True if the switch was added; False if it already existed (and was not added again)
        bool enableVerbositySwitch (
            const char * cmdLineName = "verbose",
            char cmdLineFlag = 'v',
            const char * helpText = 0 );

        /// @brief Prints logging configuration.
        /// @param [in] toFile The file to write the configuration to.
        void printConfig ( FILE * toFile );

    protected:
        /// @brief Describes one subscription
        struct LogDesc
        {
            String filterName; ///< The name/filter of the log stream
            Log::LogLevel level; ///< The subscription level
            bool useLevel; ///< Whether the log level is set or no

            /// @brief constructor
            LogDesc(): useLevel ( false )
            {
            }

            /// @brief Loads content of the LogDesc from the provided string
            /// @param [in] str The string to use
            /// @return True if the string is correct; False otherwise
            bool loadFromStr ( const String & str );

            /// @brief Compares two LogDesc objects based on their log level.
            /// @param [in] other The second LogDesc object to compare
            /// @return A value indicating whether the first LogDesc object uses more critical (and less "chatty")
            ///          log level than the second one.
            inline bool operator> ( const LogDesc & other ) const
            {
                return ( level.value() > other.level.value() );
            }
        };

        /// @brief Contains the list of LogDesc objects for a single logging rule
        struct RuleDesc
        {
            String outputName; ///< The name/path of the output
            List<LogDesc> logs; ///< The list of LogDesc objects for this rule
        };

        /// @brief Contains this option's "value"
        struct OptValue
        {
            Log::LogLevel defaultLogLevel; ///< Default log level set

            /// @brief All configured log subscriptions
            /// Each key is the 'rule ID'
            /// Each value if the RuleDesc object
            HashMap<uint16_t, RuleDesc> logConfig;
        };

        OptValue _defaultValue; ///< Default value
        OptValue _value; ///< Current value
        const String _prefix; ///< The prefix used, including the dot (e.g. "log.")

        /// @brief Existing outputs (path:object mapping)
        HashMap<String, TextLogOutput *> _outputs;

        ConfigSwitch * _verbositySwitch; ///< Verbosity switch option (if enabled)

        bool _daemonMode; ///< Whether we're running in daemon mode

        /// @brief Reads the value of this option from the string provided
        /// If this option uses a name filter (like "log.*") this function will be called several times,
        /// once for every matching option.
        /// @param [in] name The name of the config option
        /// @param [in] strValue The value of the config option.
        ///                      Note that using an empty string will cause the option to become 'unset'
        /// @param [in] isDefault True if this value is to be used as the new, built-in, default
        /// @return Standard error code
        virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault );

        /// @brief Called just before the configuration is (re)loaded
        /// It restores the config option to its default value
        virtual void restoreDefaults();

        /// @brief Called every time the configuration is updated
        virtual void configUpdated();

        /// @brief Performs the actual initialization
        /// @param [in] ignoreErrors If set to true, it will try to load all existing log subscriptions
        ///                          even if some of them fail and return the last error;
        ///                          If false, it will return the first error and stop adding subscriptions.
        /// @return Standard error code
        ERRCODE doInit ( bool ignoreErrors );

        /// @brief Adds a log subscription
        /// @param [in] outputName The name/path of the output
        /// @param [in] logDesc The description of the log. It should have the correct log level set
        ///                     (even if it's the default one).
        /// @return Standard error code
        ERRCODE addLogSubscription ( const String & outputName, const LogDesc & logDesc );

        /// @brief A helper function that returns the default log level adjusted using 'verbosity' settings.
        /// @return Default log level to use.
        Log::LogLevel getAdjustedDefaultLogLevel() const;

        /// @brief A helper function that generates a sorted list of logging rules.
        /// Those rules are sorted in ascending rule ID order.
        /// Stream filters in each rule are sorted in ascending "chattiness" order.
        /// Also, each filter that has useLevel set to false, will have the current value of adjusted
        /// default log level set as its level.
        /// @return Sorted list of logging rules.
        List<RuleDesc> getSortedRules() const;
};
}
