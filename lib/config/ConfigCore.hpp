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

#include <cstdio>

#include "basic/HashSet.hpp"
#include "error/Error.hpp"
#include "ConfigString.hpp"

namespace Pravala
{
/// @brief Core config management class
/// Provides some built-in configuration options and various methods for operating on config files.
class ConfigCore
{
    public:

        /// @brief A class used for initializing ECFG values
        class EcfgInit
        {
            public:
                /// @brief Constructor that initializes ECFG data
                /// @param [in] ecfgData The ECFG data to use (null-terminated string)
                EcfgInit ( const char * ecfgData );

            private:
                /// @brief Returns a mapping with ECFG values stored before they are applied
                /// @return a mapping with ECFG values stored before they are applied
                static HashMap<String, StringList> & ecfgValues();

                friend class ConfigCore;
        };

        /// @brief Should be inherited by objects that want to listen to notifications when config state changes.
        class Listener
        {
            protected:
                /// @brief Called every time the configuration is updated.
                virtual void configUpdated() = 0;

                /// @brief Destructor
                /// This will unregister the listener.
                virtual ~Listener();

                friend class ConfigCore;
        };

        static ConfigString optConfigPath; ///< Default config path
        static ConfigString optProductName; ///< MAS Client, ACE Server, etc.
        static ConfigString optVersion; ///< stable/2010_05_12, trunk, release/1.0.0, etc.
        static ConfigString optBuildTime; ///< 2010-05-13 14:17:51 UTC (date -u +"%Y-%m-%d %H:%M:%S %Z")
        static ConfigString optBuildHost; ///< hudson
        static ConfigString optBuildUser; ///< jakub
        static ConfigString optBuildType; ///< devel, stable, release
        static ConfigString optSrcRevision; ///< 1023 or 1023-custom (if working copy modified)
        static ConfigString optEcfgTags; ///< tags used by the ECFG generator

        /// @brief If set, errors while processing the contents of the config file will be ignored.
        ///         Errors reading from the file are never ignored.
        static const uint8_t LoadOptIgnoreErrors = ( 1 << 0 );

        /// @brief If set, any defaults in ConfigCore will be restored FIRST, before reloading the config file.
        ///         The config is NOT restored if there are any errors reading from the file.
        static const uint8_t LoadOptRestoreDefaultsFirst = ( 1 << 1 );

        /// @brief Returns the global instance of the ConfigCore object.
        /// @return global instance of the ConfigCore object.
        static ConfigCore & get();

        /// @brief Initializes config object
        ///
        /// It clears the list of properties, loads the properties from the file
        /// and merges them with the embedded configuration.
        ///
        /// @param [in] confPath The path to the configuration file. If empty the built-in
        ///              path is used.
        /// @return Standard error code. It returns Error::NoNameProvided when no path was used as the
        ///          argument and the built-in ECFG does not specify it either.
        ERRCODE init ( String confPath = String::EmptyString );

        /// @brief Returns the config path used by the ConfigCore object
        /// @return config path used by the ConfigCore object
        inline const String & getConfigPath() const
        {
            return _configPath;
        }

        /// @brief Processes pre-parsed list of command line options.
        /// @param [in] args The list with arguments. The first argument is NOT a program name.
        ///                  Multi-flag options should be split ('-abc=xyz' -> '-a -b -c=xyz').
        ///                  parseCmdLine() can be used to get the list of arguments in that format.
        /// @param [out] extraArgs The list with extra arguments that were not used by any of the options.
        ///                        It is not cleared first - only appended to.
        /// @return Description of the error, or empty string on success.
        String processCmdLine ( const StringList & args, StringList & extraArgs );

        /// @brief Reloads the config file.
        /// @param [in] loadOpts Bitmask of LoadOpt* values.
        /// @return Error message or an empty string on success.
        String reloadConfig ( uint8_t loadOpts = LoadOptIgnoreErrors | LoadOptRestoreDefaultsFirst );

        /// @brief Loads the given config data on top of the current configuration.
        /// @param [in] data The data string to parse.
        /// @param [in] loadOpts Bitmask of LoadOpt* values.
        /// @param [out] errorMsg Detailed error message set on failure.
        /// @return Standard error message.
        ERRCODE loadConfigData ( const String & data, uint8_t loadOpts, String & errorMsg );

        /// @brief Prints command-line help.
        /// @param [in] toFile The file to write the it to
        /// @param [in] progName The name of the program.
        void printCmdLineHelp ( FILE * toFile, const char * progName );

        /// @brief Prints build information
        /// @param [in] toFile The file to write the info to
        void printBuildInfo ( FILE * toFile );

        /// @brief Prints the configuration options
        /// @param [in] toFile The file to write the options to
        /// @param [in] includeHelp Whether help text should be included
        void printOptions ( FILE * toFile, bool includeHelp = false );

        /// @brief Generates a sample config file
        /// @param [in] toFile The file to write the sample config to
        /// @param [in] includeUncommon If false (default) only common options will be included
        void printSampleConfig ( FILE * toFile, bool includeUncommon = false );

        /// @brief Registers an object to listen to notifications when config state changes.
        /// @param [in] listener Pointer to the listener to register.
        ///                      Null pointers are ignored.
        void registerListener ( Listener * listener );

        /// @brief Unregisters an object from listening to notifications when config state changes.
        /// @param [in] listener Pointer to the listener to unregister.
        ///                      Null pointers are ignored.
        void unregisterListener ( Listener * listener );

        /// @brief Parses the command line options.
        /// @param [in] argc The number of arguments in argv
        /// @param [in] argv The list with arguments.
        ///                  The first entry is the name of the binary which will NOT be included in the output.
        /// @param [out] extraArgs The list in which extra arguments will be stored.
        ///                         This list is NOT cleared, only appended to.
        /// @return Parsed command line.
        static StringList parseCmdLine ( int argc, char * argv[] );

    protected:
        /// @brief Bitmask for all LoadOpt* that are considered public flags.
        /// We reserve LoadOpt* with bit index >= 4 as protected internal flags.
        static const uint8_t LoadOptPublicBitMask = ( LoadOptIgnoreErrors | LoadOptRestoreDefaultsFirst );

        /// @brief If set, the loaded options will be treated as built-in defaults.
        static const uint8_t LoadOptIsDefault = ( 1 << 4 );

        bool _initialized; ///< Whether ConfigCore has been initialized with initConfig()
        String _configPath; ///< The name of the file with configuration

        /// @brief The set of config options that were set using the command line.
        /// These options will NOT be set or reset when loading/reloading the configuration.
        HashSet<ConfigOpt *> _cmdLineOptionsUsed;

        /// @brief Loads configuration data.
        /// @param [in] config The mapping with key:value configuration pairs.
        /// @param [in] loadOpts Bitmask of LoadOpt* values.
        /// @return Standard error code.
        ERRCODE loadConfig ( const HashMap<String, StringList> & config, uint8_t loadOpts );

        /// @brief A helper function that checks if all required options have been set.
        /// @return True if all options that use FlagReqNonEmpty have a valid value set; False otherwise.
        bool checkRequiredOptions();

        /// @brief Helper function that parses data into the HashMap with key:value pairs.
        /// @param [in] data The data string to parse.
        /// @param [out] lineNumber The number of line processed; If 'false' is returned it contains
        ///                          the number of the error line.
        /// @param [out] values The result is stored in this HashMap.
        /// @return True if the data has been parsed properly; False otherwise.
        static bool parseData ( const String & data, size_t & lineNumber, HashMap<String, StringList> & values );

    private:
        HashSet<Listener *> _listeners; ///< Registered listeners

        /// @brief Constructor
        ConfigCore();

        /// @brief Destructor
        ~ConfigCore();
};
}
