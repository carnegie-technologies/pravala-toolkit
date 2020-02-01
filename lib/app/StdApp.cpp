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

#include <cerrno>

#include "basic/Random.hpp"
#include "config/ConfigCore.hpp"
#include "sys/OsUtils.hpp"
#include "event/Utils.hpp"
#include "log/ConfigLogs.hpp"
#include "socket/PacketDataStore.hpp"

#include "StdApp.hpp"

using namespace Pravala;

StdApp * StdApp::_instance ( 0 );
String StdApp::_appDataPath;

StdApp::StdAppSwitch::StdAppSwitch (
        StdApp * app, StdApp::SwitchType swType,
        const char * cmdLineName, char cmdLineFlag, const String & helpText ):
    ConfigSwitch ( cmdLineName, cmdLineFlag, helpText ),
    _app ( app ), _swType ( swType )
{
}

ERRCODE StdApp::StdAppSwitch::loadOption (
        const String & /*name*/, const String & /*strValue*/, bool /*isDefault*/ )
{
    if ( _app != 0 )
        _app->switchActivated ( _swType );

    return Error::Success;
}

StdApp * StdApp::getInstance()
{
    return _instance;
}

String StdApp::getAppDataPath()
{
    if ( _appDataPath.isEmpty() )
    {
        _appDataPath = osGetAppDataPath();
    }

    return _appDataPath;
}

StdApp::StdApp ( int argc, char * argv[], uint32_t features, const char * additionalHelpText ):
    _progName ( ( argc > 0 ) ? argv[ 0 ] : "" ),
    _binaryDir ( osGetBinaryDir ( argc, argv ) ),
    _progArgs ( ( argc > 0 ) ? ( ConfigCore::parseCmdLine ( argc, argv ) ) : StringList() ),
    _extraCmdLineHelp ( additionalHelpText ),
    _feat (
            features
#ifdef SYSTEM_WINDOWS
            & ~( FeatEnableDaemonSwitch | FeatDaemonize )
#endif
    ),
    _logs ( 0 ),
    _daemonMode ( false ),
    _configTest ( false )
{
    assert ( !_instance );

    if ( _instance != 0 )
    {
        fprintf ( stderr, "An instance of the StdApp already exists!\n" );
        exit ( EXIT_FAILURE );
        return;
    }

    // All our code should always go through Random (which makes sure it's initialized properly).
    // But external libraries won't do that, so let's initialize it manually.
    // Note that if it has already been initialized by something else - nothing will happen.

    Random::init();

    _instance = this;

    if ( uses ( FeatUtilsSetupDebug ) )
    {
        Utils::setupDebugCore();
    }

    if ( uses ( FeatEnableDaemonSwitch ) )
    {
        _cfgOptions.append ( new StdAppSwitch ( this, SwitchDaemon, "daemonize", 'd', "Run in daemon mode" ) );
    }

    if ( uses ( FeatEnableStdSwitches ) )
    {
        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchCfgFullGen, "sample-full-config", 0,
                                 "Generate a sample config file with all the options and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchCfgGen, "sample-config", 0,
                                 "Generate a sample config file with only common options and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchCfgHelp, "help-config", 0,
                                 "Print the built-in configuration options (including help) and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchCfgList, "list-config", 0,
                                 "Print the built-in configuration options and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchCfgTest, "test-config", 'T',
                                 "Load and test configuration and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchVersion, "version", 'V',
                                 "Print build info and exit" ) );

        _cfgOptions.append ( new StdAppSwitch (
                                 this, SwitchHelp, "help", 'h',
                                 "Print this help and exit" ) );
    }

    if ( uses ( FeatEnableLogs ) )
    {
        _logs = new ConfigLogs();

        if ( uses ( FeatEnableVerbositySwitch ) )
            _logs->enableVerbositySwitch();

        if ( uses ( FeatEnableStdSwitches ) )
        {
            _cfgOptions.append ( new StdAppSwitch (
                                     this, SwitchLogList, "list-logs", 0,
                                     "Print the list of configured logging rules" ) );
        }
    }
}

StdApp::~StdApp()
{
    delete _logs;
    _logs = 0;

    for ( size_t i = 0; i < _cfgOptions.size(); ++i )
    {
        delete _cfgOptions.at ( i );
    }

    _cfgOptions.clear();

    assert ( _instance == this );

    if ( _instance == this )
    {
        _instance = 0;
    }

    if ( uses ( FeatInitEventManager ) )
    {
        // Just in case...
        EventManager::signalUnsubscribe ( this );

#ifndef NDEBUG
        const ERRCODE eCode = EventManager::shutdown ( false );

        if ( NOT_OK ( eCode ) && eCode != Error::NotInitialized )
        {
            fprintf ( stderr, "Error shutting down EventManager: %s; Forcing it...\n", eCode.toString() );

            EventManager::shutdown ( true );
        }
#else
        EventManager::shutdown ( true );
#endif
    }

    if ( uses ( FeatInitPacketDataStore ) )
    {
        PacketDataStore::shutdown();
    }
}

ERRCODE StdApp::enableConfigReloads()
{
    if ( ConfigCore::get().getConfigPath().isEmpty() )
        return Error::NotInitialized;

    EventManager::signalSubscribe ( this );

    return Error::Success;
}

void StdApp::disableConfigReloads()
{
    EventManager::signalUnsubscribe ( this );
}

void StdApp::printHelp ( FILE * toFile )
{
    ConfigCore::get().printCmdLineHelp ( toFile, _progName.c_str() );

    if ( !_extraCmdLineHelp.isEmpty() )
    {
        fprintf ( toFile, "%s\n", _extraCmdLineHelp.c_str() );
    }
}

void StdApp::switchActivated ( StdApp::SwitchType swType )
{
    _activatedSwitches.append ( swType );
}

void StdApp::runSwitch ( StdApp::SwitchType swType )
{
    switch ( swType )
    {
        case SwitchHelp:
            printHelp ( stdout );
            exit ( EXIT_SUCCESS );
            break;

        case SwitchVersion:
            ConfigCore::get().printBuildInfo ( stdout );
            exit ( EXIT_SUCCESS );
            break;

        case SwitchLogList:
            if ( !_logs )
            {
                fprintf ( stderr, "Logs are not enabled\n" );
                exit ( EXIT_FAILURE );
            }

            _logs->printConfig ( stdout );
            exit ( EXIT_SUCCESS );
            break;

        case SwitchCfgList:
        case SwitchCfgHelp:
            fprintf ( stdout, "\nBuilt-in configuration options:\n" );
            ConfigCore::get().printOptions ( stdout, swType == SwitchCfgHelp );
            exit ( EXIT_SUCCESS );
            break;

        case SwitchCfgGen:
        case SwitchCfgFullGen:
            ConfigCore::get().printSampleConfig ( stdout, swType == SwitchCfgFullGen );
            fprintf ( stdout, "\n" );
            exit ( EXIT_SUCCESS );
            break;

        case SwitchCfgTest:
            _configTest = true;

            // If we are just testing the configuration, we do NOT want the daemon mode!
            _daemonMode = false;
            break;

        case SwitchDaemon:
            if ( !_configTest )
            {
                // If we are just testing the configuration, we do NOT want the daemon mode!
                _daemonMode = true;
            }
            break;
    }
}

void StdApp::receiveSignalEvent ( int sigRcvd )
{
    if ( sigRcvd != EventManager::SignalHUP )
        return;

    String errDesc = ConfigCore::get().reloadConfig (
        ConfigCore::LoadOptIgnoreErrors | ConfigCore::LoadOptRestoreDefaultsFirst );

    if ( !errDesc.isEmpty() )
        fprintf ( stderr, "Error reloading the configuration file:\n%s\n", errDesc.c_str() );
}

int StdApp::init ( bool exitOnError )
{
    int ret = initConfig ( exitOnError );

    if ( ret == EXIT_SUCCESS )
        ret = initFeatures ( exitOnError );

    return ret;
}

int StdApp::initConfig ( bool exitOnError )
{
    String errDesc = ConfigCore::get().processCmdLine ( _progArgs, _extraCmdLineArgs );

    bool doExit = false;

    if ( !errDesc.isEmpty() )
    {
        fprintf ( stderr, "\n%s\n", errDesc.c_str() );
        doExit = true;
    }

    ERRCODE eCode;

    if ( !doExit && NOT_OK ( eCode = ConfigCore::get().init() ) && eCode != Error::NoNameProvided )
    {
        fprintf ( stderr, "\nError initializing configuration: %s\n", eCode.toString() );
        doExit = true;
    }

    if ( !doExit && !_activatedSwitches.isEmpty() )
    {
        for ( size_t i = 0; i < _activatedSwitches.size(); ++i )
        {
            runSwitch ( _activatedSwitches.at ( i ) );
        }

        _activatedSwitches.clear();
    }

    if ( !doExit && uses ( FeatNoExtraArgs ) && !_extraCmdLineArgs.isEmpty() )
    {
        fprintf ( stderr, "\nExcessive command line argument: '%s'\n", _extraCmdLineArgs.first().c_str() );
        doExit = true;
    }

    if ( doExit )
    {
        printHelp ( stderr );

        if ( exitOnError || _configTest )
        {
            exit ( EXIT_FAILURE );
        }

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int StdApp::initFeatures ( bool exitOnError )
{
    bool doExit = false;

#ifndef SYSTEM_WINDOWS
    if ( !doExit && uses ( FeatSuid ) && seteuid ( 0 ) != 0 )
    {
        fprintf ( stderr, "\nFailed to set UID to 0; seteuid(): %s\n", strerror ( errno ) );

        if ( uses ( FeatOptionalSuid ) )
        {
            fprintf ( stderr, "Some features of the program may be unavailable!\n" );
        }
        else
        {
            doExit = true;
        }
    }
#endif

    // This has to be done after ConfigCore's init:
    if ( !doExit && uses ( FeatEnsureAppDataPath ) )
    {
        _appDataPath = osGetAppDataPath();

        if ( _appDataPath.isEmpty() )
        {
            fprintf ( stderr, "Error reading application's data path\n" );
            doExit = true;
        }
    }

    if ( !doExit && uses ( FeatUtilsSetup ) )
    {
        Utils::setup();
    }

    ERRCODE eCode;

    if ( !doExit && _logs != 0 && uses ( FeatInitLogs ) && NOT_OK ( eCode = _logs->init ( _daemonMode ) ) )
    {
        fprintf ( stderr, "\nCould not initialize configured logs: %s\n", eCode.toString() );
        doExit = true;
    }

    if ( !doExit && uses ( FeatDaemonize ) && _daemonMode )
    {
        eCode = Utils::daemonize ( true );

        if ( NOT_OK ( eCode ) && eCode != Error::NothingToDo && eCode != Error::ForkChild )
        {
            fprintf ( stderr, "\nError trying to daemonize the processs: %s\n", eCode.toString() );
            doExit = true;
        }
    }

    if ( !doExit && uses ( FeatInitPacketDataStore ) )
    {
        PacketDataStore::init();
    }

    if ( !doExit && uses ( FeatInitEventManager ) )
    {
        eCode = EventManager::init();

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "\nError trying to initialized EventManager: %s\n", eCode.toString() );
            doExit = true;
        }
    }

    if ( doExit )
    {
        printHelp ( stderr );

        if ( exitOnError || _configTest )
        {
            exit ( EXIT_FAILURE );
        }

        return EXIT_FAILURE;
    }

    if ( _configTest )
    {
        printf ( "%s: Configuration is correct.\n", _progName.c_str() );
        exit ( EXIT_SUCCESS );
    }

    return EXIT_SUCCESS;
}
