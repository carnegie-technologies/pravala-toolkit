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

#include "basic/NoCopy.hpp"
#include "config/ConfigSwitch.hpp"
#include "event/EventManager.hpp"

namespace Pravala
{
class ConfigLogs;

/// @brief A class with various fields and operations typically used by programs in their main() functions.
class StdApp: public NoCopy, protected EventManager::SignalHandler
{
    public:
        /// @brief If enabled, any excessive command line arguments will cause an error.
        static const uint32_t FeatNoExtraArgs = ( 1 << 0 );

        /// @brief If enabled, standard configuration switches will be created
        static const uint32_t FeatEnableStdSwitches = ( 1 << 1 );

        /// @brief If enabled, 'daemonize' switch will be created
        static const uint32_t FeatEnableDaemonSwitch = ( 1 << 2 );

        /// @brief If enabled, standard logs will be created (using the default prefix)
        static const uint32_t FeatEnableLogs = ( 1 << 3 );

        /// @brief If enabled, 'verbosity' switch will be created.
        /// This also enables FeatEnableLogs.
        static const uint32_t FeatEnableVerbositySwitch = ( ( 1 << 4 ) | FeatEnableLogs );

        /// @brief If enabled, logs will be initialized inside init().
        /// This also enables FeatEnableLogs.
        static const uint32_t FeatInitLogs = ( ( 1 << 5 ) | FeatEnableLogs );

        /// @brief If enabled,the daemon mode will be started inside init().
        /// This also enables daemon switch.
        static const uint32_t FeatDaemonize = ( ( 1 << 6 ) | FeatEnableDaemonSwitch );

        /// @brief If enabled, Utils::setup() will be called inside init()
        static const uint32_t FeatUtilsSetup = ( 1 << 7 );

        /// @brief If enabled, Utils::setupDebugCore() will be called inside init()
        static const uint32_t FeatUtilsSetupDebug = ( 1 << 8 );

        /// @brief If enabled, setuid will be used inside init(). Does nothing on Windows.
        static const uint32_t FeatSuid = ( 1 << 9 );

        /// @brief If enabled, suid is optional and only causes warning.
        /// This also enables FeatSuid.
        static const uint32_t FeatOptionalSuid = ( ( 1 << 10 ) | FeatSuid );

        /// @brief If enabled, initFeatures() will make sure that AppDataPath is set.
        static const uint32_t FeatEnsureAppDataPath = ( 1 << 11 );

        /// @brief If enabled, initFeatures() will initialize the PacketDataStore.
        /// It also means that PacketDataStore::shutdown() will be called when StdApp is destroyed.
        static const uint32_t FeatInitPacketDataStore = ( 1 << 12 );

        /// @brief If enabled, initFeatures() will initialize the EventManager.
        /// It also means that EventManager::shutdown ( true ) will be called when StdApp is destroyed.
        static const uint32_t FeatInitEventManager = ( 1 << 13 );

        /// @brief Enables standard features and accepts extra arguments
        static const uint32_t FeatStdFeaturesWithArgs
            = ( FeatEnableLogs
                | FeatEnableStdSwitches
                | FeatEnableVerbositySwitch
                | FeatInitLogs
                | FeatUtilsSetup
                | FeatInitPacketDataStore
                | FeatInitEventManager
#ifndef NDEBUG
                | FeatUtilsSetupDebug
#endif
              );

        /// @brief Enables standard features; Error on extra arguments
        static const uint32_t FeatStdFeatures
            = ( FeatStdFeaturesWithArgs | FeatNoExtraArgs );

        /// @brief Enables all possible features
        static const uint32_t FeatAllFeatures
            = ( ( ~( ( uint32_t ) 0 ) )
#ifdef NDEBUG
                & ( ~FeatUtilsSetupDebug )
#endif
                 );

        /// @brief Returns a pointer to the global StdApp (if it exists).
        /// The first StdApp created will set this pointer.
        /// Creating more than one StdApp instances is considered an error...
        /// When that StdApp is deleted, the pointer will be cleared.
        /// @return Pointer to the first StdApp created, or 0 if it was never created or has been removed.
        static StdApp * getInstance();

        /// @brief Constructor.
        /// @param [in] argc The number of elements in argv array
        /// @param [in] argv Command line arguments; The first entry is the program name.
        /// @param [in] features Flags that control which features will be used.
        /// @param [in] additionalHelpText The additional help text to be included.
        StdApp (
            int argc, char * argv[],
            uint32_t features = FeatStdFeatures,
            const char * additionalHelpText = 0 );

        /// @brief Destructor.
        virtual ~StdApp();

        /// @brief Checks whether given features are enabled.
        /// It only returns true if all (if specified more than one) features are enabled.
        /// @param [in] features Features to check
        /// @return True if all requested features are enabled.
        inline bool uses ( uint32_t features ) const
        {
            return ( ( _feat & features ) == features );
        }

        /// @brief Returns the program name/path used.
        /// @return the program name/path used.
        inline const String & getProgName() const
        {
            return _progName;
        }

        /// @brief Returns the directory where the program's binary is.
        /// @return the directory where the program's binary is.
        inline const String & getBinaryDir() const
        {
            return _binaryDir;
        }

        /// @brief Returns additional command line arguments, that were not parsed as options.
        /// @return additional command line arguments, that were not parsed as options.
        inline const StringList & getExtraCmdLineArgs() const
        {
            return _extraCmdLineArgs;
        }

        /// @brief Returns true if the daemon mode was enabled with the command-line option.
        /// @return true if the daemon mode was enabled with the command-line option.
        inline bool useDaemonMode() const
        {
            return _daemonMode;
        }

        /// @brief Returns a pointer to internal ConfigLogs object
        /// @return a pointer to internal ConfigLogs object
        inline ConfigLogs * getLogs() const
        {
            return _logs;
        }

        /// @brief Initializes all enabled features.
        /// Calls initConfig() followed by initFeatures().
        /// @param [in] exitOnError If set to true and there is an error,
        ///                          it will terminate the program with the error code.
        /// @note Even if exitOnError is set to 'false', this function may cause the program to exit if 'test-config'
        ///        option is used.
        /// @return Operation result; 0 on success, or error code on error.
        ///          If exitOnError is enabled, this function does not return on error.
        int init ( bool exitOnError = true );

        /// @brief Processes the command line options and initializes the config
        /// On error it will print the error message and help text (using stderr).
        /// @param [in] exitOnError If set to true and there is an error,
        ///                          it will terminate the program with the error code.
        /// @note Even if exitOnError is set to 'false', this function may cause the program to exit if 'test-config'
        ///        option is used.
        /// @return Operation result; 0 on success, or error code on error.
        ///          If exitOnError is enabled, this function does not return on error.
        int initConfig ( bool exitOnError = true );

        /// @brief Initializes enabled features
        /// @param [in] exitOnError If set to true and there is an error,
        ///                          it will terminate the program with the error code.
        /// @note Even if exitOnError is set to 'false', this function may cause the program to exit if 'test-config'
        ///        option is used.
        /// @return Operation result; 0 on success, or error code on error.
        ///          If exitOnError is enabled, this function does not return on error.
        virtual int initFeatures ( bool exitOnError = true );

        /// @brief Enables config reloading
        /// This will use EventManager, so should be done after forking, etc.!
        /// @return Standard error code.
        ERRCODE enableConfigReloads();

        /// @brief Disables config reloading
        void disableConfigReloads();

        /// @brief Prints the help text.
        /// @param [in] toFile Output file.
        void printHelp ( FILE * toFile = stderr );

        /// @brief Checks if the daemon mode was enabled.
        /// @return True if the daemon mode was enabled; False otherwise.
        inline bool isDaemonMode() const
        {
            return _daemonMode;
        }

        /// @brief Checks if config test mode was enabled.
        /// @return True if config test mode was enabled; False otherwise.
        inline bool isConfigTestMode() const
        {
            return _configTest;
        }

        /// @brief Get the path to a directory that the app can write to.
        /// It is used, for example, for storing the database, etc.).
        /// On Android, the app home directory;
        /// On Windows it's %appdata%/AppName if running as a user.
        /// and %programdata%/AppName if it's running as a service.
        /// @return the path to a directory that the app can write to.
        static String getAppDataPath();

    protected:
        /// @brief Types of built-in switches
        enum SwitchType
        {
            SwitchVersion, ///< 'version' command
            SwitchHelp, ///< 'help' command
            SwitchLogList, ///< 'list-logs' command
            SwitchCfgList, ///< 'list-config' command
            SwitchCfgHelp, ///< 'help-config' command
            SwitchCfgGen, ///< 'sample-config' command
            SwitchCfgFullGen, ///< 'sample-full-config' command
            SwitchCfgTest, ///< 'test-config' command
            SwitchDaemon ///< 'daemonize' command
        };

        /// @brief Class for creating built-in switches
        class StdAppSwitch: public ConfigSwitch
        {
            public:
                /// @brief Constructor.
                /// @param [in] cmdLineName If not empty, this name will be used as this option's command line name.
                /// @param [in] cmdLineFlag If not 0, this letter will be used as this option's command line flag.
                /// @param [in] helpText The help text for this option.
                StdAppSwitch (
                    StdApp * app, SwitchType swType,
                    const char * cmdLineName, char cmdLineFlag, const String & helpText );

            protected:
                virtual ERRCODE loadOption ( const String & name, const String & strValue, bool isDefault );

            private:
                StdApp * const _app; ///< My application
                const SwitchType _swType; ///< The type of the switch
        };

        virtual void receiveSignalEvent ( int sigRcvd );

    private:
        /// @brief Pointer to the first StdApp object created.
        /// In general, there should be only one...
        static StdApp * _instance;

        /// @brief The path to a directory that the app can write to.
        /// It is used, for example, for storing the database, etc.).
        /// On Android, the app home directory;
        /// On Windows it's %appdata%/AppName if running as a user.
        /// and %programdata%/AppName if it's running as a service.
        static String _appDataPath;

        const String _progName; ///< The name of the program (argv[0])
        const String _binaryDir; ///< The directory in which the program's binary is.
        const StringList _progArgs; ///< Pre-parsed command-line arguments
        const String _extraCmdLineHelp; ///< Additional help content.
        const uint32_t _feat; ///< Enabled features.

        ConfigLogs * _logs; ///< ConfigLogs object
        List<ConfigOpt *> _cfgOptions; ///< All config options created

        /// @brief The list of switches that were activated during config initialization.
        /// It is used to delay actually running them until after the configuration has been initialized.
        List<SwitchType> _activatedSwitches;

        StringList _extraCmdLineArgs; ///< Additional command line arguments (that were not parsed as options)

        bool _daemonMode; ///< True if the daemon mode was enabled, false otherwise
        bool _configTest; ///< Whether config-test option was used.

        /// @brief Called whenever one of the switches is activated.
        /// @param [in] swType The type of the switch
        void switchActivated ( SwitchType swType );

        /// @brief Executes a specific switch.
        /// @param [in] swType The type of the switch
        void runSwitch ( SwitchType swType );

        /// @brief OS-specific function that determines program binary's directory.
        /// It is called during _binaryDir initialization.
        /// @param [in] argc The number of elements in argv array
        /// @param [in] argv Command line arguments; The first entry is the program name.
        /// @return Program binary's directory.
        static String osGetBinaryDir ( int argc, char * argv[] );

        /// @brief OS-specific function that reads application's data path.
        /// @return Application's data path.
        static String osGetAppDataPath();

        friend class StdAppSwitch;
};
}
