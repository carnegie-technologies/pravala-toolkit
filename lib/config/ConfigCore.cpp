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

#include <climits>

#include "basic/Random.hpp"

#include "ConfigCore.hpp"

#define MAX_LINE_LEN    120
#define BI_PREFIX       "build_info."

using namespace Pravala;

ConfigString ConfigCore::optConfigPath (
        ConfigOpt::FlagBuiltInOnly, "config", 'C', "config_file", "Path to the configuration file" );
ConfigString ConfigCore::optProductName (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "product", "The name of the product", "unknown" );
ConfigString ConfigCore::optVersion (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "version", "Version type string", "unknown" );
ConfigString ConfigCore::optBuildTime (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "build_time", "Build time", "unknown" );
ConfigString ConfigCore::optBuildHost (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "build_host", "Build host", "unknown" );
ConfigString ConfigCore::optBuildUser (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "build_user", "Build user", "unknown" );
ConfigString ConfigCore::optBuildType (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "build_type", "Build type", "unknown" );
ConfigString ConfigCore::optSrcRevision (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "src_rev", "Source revision", "unknown" );
ConfigString ConfigCore::optEcfgTags (
        ConfigOpt::FlagBuiltInOnly, BI_PREFIX "ecfg_tags", "Build tags", "unknown" );

ConfigCore::Listener::~Listener()
{
    ConfigCore::get().unregisterListener ( this );
}

ConfigCore::EcfgInit::EcfgInit ( const char * ecfgData )
{
    size_t lineNumber = 0;
    HashMap<String, StringList> values;

    if ( !parseData ( String ( ecfgData ), lineNumber, values ) )
    {
        fprintf ( stderr, "Error parsing built-in configuration; Data ignored; Line number: %lu; Data: '\n%s\n'\n",
                  ( unsigned long ) ( lineNumber + 1 ), ecfgData );
        return;
    }

    for ( HashMap<String, StringList>::Iterator it ( values ); it.isValid(); it.next() )
    {
        ecfgValues().insert ( it.key(), it.value() );
    }
}

HashMap<String, StringList> & ConfigCore::EcfgInit::ecfgValues()
{
    static HashMap<String, StringList> global;

    return global;
}

ConfigCore & ConfigCore::get()
{
    static ConfigCore * global = 0;

    if ( !global )
    {
        global = new ConfigCore();
    }

    return *global;
}

ConfigCore::ConfigCore(): _initialized ( false )
{
    // All our code should always go through Random (which makes sure it's initialized properly).
    // But external libraries won't do that, so let's initialize it manually.
    // Note that if it has already been initialized by something else - nothing will happen.

    Random::init();

    loadConfig ( EcfgInit::ecfgValues(), LoadOptIgnoreErrors | LoadOptIsDefault );
    EcfgInit::ecfgValues().clear();
}

ConfigCore::~ConfigCore()
{
}

bool ConfigCore::parseData ( const String & data, size_t & lineNumber, HashMap<String, StringList> & values )
{
    if ( data.isEmpty() )
        return true;

    values.clear();

    const StringList lines = data.split ( "\n\r", true );

    for ( lineNumber = 0; lineNumber < lines.size(); ++lineNumber )
    {
        const String line ( lines.at ( lineNumber ).trimmed() );

        if ( line.startsWith ( "#" ) || line.isEmpty() )
            continue;

        const int sepIdx = line.find ( '=' );

        if ( sepIdx < 1 )
            return false;

        const String name ( line.substr ( 0, sepIdx ).trimmed() );

        if ( name.length() < 1 )
            return false;

        String value;
        bool readNextLine = false;

        if ( line.length() > sepIdx + 1 )
        {
            // If this line is not empty

            int valLen = line.length() - ( sepIdx + 1 );

            if ( line.endsWith ( "\\" ) )
            {
                readNextLine = true;
                ++valLen;
            }

            value = line.substr ( sepIdx + 1, valLen ).trimmed();
        }

        while ( readNextLine )
        {
            readNextLine = false;

            if ( ++lineNumber >= lines.size() )
                return false;

            String nextLine ( lines.at ( lineNumber ).trimmed() );

            if ( nextLine.endsWith ( "\\" ) )
            {
                readNextLine = true;
                nextLine = nextLine.substr ( 0, nextLine.length() - 1 ).trimmed();
            }

            if ( nextLine.length() > 0 )
            {
                value.append ( " " );
                value.append ( nextLine );
            }
        }

        values[ name ].append ( value.trimmed() );
    }

    return true;
}

ERRCODE ConfigCore::loadConfig ( const HashMap< String, StringList > & config, uint8_t loadOpts )
{
    const bool ignoreErrors = ( loadOpts & LoadOptIgnoreErrors );
    const bool isDefault = ( loadOpts & LoadOptIsDefault );

    const List<ConfigOpt *> & cfgOptions = ConfigOpt::cfgOptions();

    for ( HashMap<String, StringList>::Iterator it ( config ); it.isValid(); it.next() )
    {
        bool foundOption = false;

        for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
        {
            ConfigOpt * opt = cfgOptions.at ( idx );

            assert ( opt != 0 );

            if ( !opt )
                continue;

            if ( it.key().matches ( opt->optName ) )
            {
                foundOption = true;

                if ( ( opt->_optFlags & ConfigOpt::FlagInitializeOnly ) && _initialized )
                {
                    // We are past initialization process; This option cannot be modified anymore.
                    // We silently skip it (but it was legal, so foundOption should be set)
                }
                else if ( ( opt->_optFlags & ConfigOpt::FlagBuiltInOnly ) && !isDefault )
                {
                    // It is not possible to set built-in options through a config file.
                    // We always want the warning (since that option shouldn't be in that config file at all),
                    // but only exit when we don't want to ignore errors.

                    fprintf ( stderr, "Could not modify built-in option '%s'\n", it.key().c_str() );

                    if ( !ignoreErrors )
                        return Error::ConfigError;
                }
                else if ( _cmdLineOptionsUsed.contains ( opt ) )
                {
                    // This option was set in command line. It is protected from config file changes!
                    // Let's print a warning, just in case...

                    fprintf ( stderr, "Not modifying '%s' option that was set in the command-line.\n",
                              it.key().c_str() );
                }
                else
                {
                    const StringList & vals = it.value();

                    for ( size_t v = 0; v < vals.size(); ++v )
                    {
                        const ERRCODE eCode = opt->loadOption ( it.key(), vals.at ( v ), isDefault );

                        if ( NOT_OK ( eCode ) )
                        {
                            fprintf ( stderr, "Error loading%s option: '%s = %s'; "
                                      "Matching name/filter: '%s'; Error: %s\n",
                                      isDefault ? " built-in" : "",
                                      it.key().c_str(),
                                      vals.at ( v ).c_str(),
                                      opt->optName.c_str(),
                                      eCode.toString() );

                            if ( !ignoreErrors )
                                return eCode;
                        }
                    }
                }
            }
        }

        if ( !foundOption )
        {
            fprintf ( stderr, "Unrecognized%s option: '%s = %s'\n",
                      isDefault ? " built-in" : "",
                      it.key().c_str(), String::join ( it.value() ).c_str() );

            if ( !ignoreErrors )
                return Error::ConfigError;
        }
    }

    return Error::Success;
}

ERRCODE ConfigCore::init ( String confPath )
{
    if ( _initialized || !_configPath.isEmpty() )
        return Error::AlreadyInitialized;

    if ( confPath.isEmpty() )
        confPath = optConfigPath.value();

    if ( confPath.isEmpty() )
    {
        if ( !checkRequiredOptions() )
            return Error::ConfigError;

        return Error::NoNameProvided;
    }

#ifndef SYSTEM_WINDOWS
    // Config file was provided (not using built in), convert it to an absolute path
    char absolutePath[ PATH_MAX ];

    // realPath returns a null pointer if invalid, otherwise it returns a pointer to absolutePath
    // (null terminated absolute path)
    if ( !realpath ( confPath.c_str(), absolutePath ) )
    {
        fprintf ( stderr, "Error loading configuration file: %s\n", confPath.c_str() );
        return Error::NotFound;
    }

    confPath = absolutePath;
#endif

    MemHandle buf;

    if ( !buf.readFile ( confPath ) )
    {
        fprintf ( stderr, "Error loading configuration file: %s\n", confPath.c_str() );
        return Error::OpenFailed;
    }

    size_t lineNumber = 0;
    HashMap<String, StringList> values;

    if ( !parseData ( buf.toString(), lineNumber, values ) )
    {
        fprintf ( stderr, "Error parsing configuration file: %s:%lu\n",
                  confPath.c_str(), ( unsigned long ) ( lineNumber + 1 ) );
        return Error::SyntaxError;
    }

    // We're loading the config file for the first time, so we don't ignore errors,
    // and we don't need to restore any values from defaults.
    ERRCODE eCode = loadConfig ( values, 0 );

    // No matter what the result was, we don't want to be able to initialize again.
    // Some options may have been loaded properly before the error occurred and we have no way
    // of telling or restoring the original state. So it's just safer to say that we are now "initialized"

    _initialized = true;

    if ( IS_OK ( eCode ) )
    {
        _configPath = confPath;

        if ( !checkRequiredOptions() )
            return Error::ConfigError;
    }

    return eCode;
}

String ConfigCore::reloadConfig ( uint8_t loadOpts )
{
    MemHandle buf;

    if ( !buf.readFile ( _configPath ) )
    {
        return String ( "Could not read configuration file: %1\n" ).arg ( _configPath );
    }

    String errorMsg;
    const ERRCODE eCode = loadConfigData ( buf.toString(), loadOpts, errorMsg );

    if ( NOT_OK ( eCode ) )
    {
        return String ( "Could not parse configuration file: %1; %2" ).arg ( _configPath, errorMsg );
    }

    return String::EmptyString;
}

ERRCODE ConfigCore::loadConfigData ( const String & data, uint8_t loadOpts, String & errorMsg )
{
    size_t lineNumber = 0;
    HashMap<String, StringList> values;

    if ( !parseData ( data, lineNumber, values ) )
    {
        errorMsg = String ( "Syntax error on line %1" ).arg ( lineNumber + 1 );

        return Error::SyntaxError;
    }

    const List<ConfigOpt *> & cfgOptions = ConfigOpt::cfgOptions();

    if ( loadOpts & LoadOptRestoreDefaultsFirst )
    {
        for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
        {
            ConfigOpt * opt = cfgOptions.at ( idx );

            assert ( opt != 0 );

            // We don't want to restore values of options set using the command line
            if ( _cmdLineOptionsUsed.contains ( opt ) )
                continue;

            // We also don't want to touch options that are marked as built-in-only or initialize-only.
            if ( ( opt->_optFlags & ( ConfigOpt::FlagBuiltInOnly | ConfigOpt::FlagInitializeOnly ) ) != 0 )
                continue;

            if ( opt != 0 )
            {
                if ( opt->_optFlags & ConfigOpt::FlagIsDefaultSet )
                {
                    opt->_optFlags |= ConfigOpt::FlagIsSet;
                }
                else
                {
                    opt->_optFlags &= ~ConfigOpt::FlagIsSet;
                }

                opt->restoreDefaults();
            }
        }
    }

    // We're reloading from a file, so ensure only public flags are set.
    const ERRCODE eCode = loadConfig ( values, loadOpts & LoadOptPublicBitMask );

    if ( NOT_OK ( eCode ) )
    {
        errorMsg = eCode.toString();
    }

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        assert ( opt != 0 );

        if ( opt != 0 )
        {
            opt->configUpdated();
        }
    }

    // The Iterator will create a copy and iterate over it.
    for ( HashSet<Listener *>::Iterator it ( _listeners ); it.isValid(); it.next() )
    {
        if ( _listeners.contains ( it.value() ) )
        {
            assert ( it.value() != 0 );

            it.value()->configUpdated();
        }
    }

    return eCode;
}

StringList ConfigCore::parseCmdLine ( int argc, char * argv[] )
{
    assert ( argc > 0 );

    StringList ret;

    // Let's split '-abcd' into '-a -b -c -d'

    for ( int idx = 1; idx < argc; ++idx )
    {
        String arg ( argv[ idx ] );

        if ( arg.length() < 2 || !arg.startsWith ( "-" ) || arg.startsWith ( "--" ) )
        {
            // 'foo' or '--bar' or 'x'
            ret.append ( arg );
            continue;
        }

        // '-foo'

        assert ( arg.length() >= 2 );

        for ( int i = 1; i < arg.length(); ++i )
        {
            if ( i + 1 < arg.length() && arg[ i + 1 ] == '=' )
            {
                // '-abc=def' -> '-a -b -c=def'

                ret.append ( String ( "-%1" ).arg ( arg.substr ( i ) ) );
                break;
            }

            ret.append ( String ( "-%1" ).arg ( arg[ i ] ) );
        }
    }

    return ret;
}

String ConfigCore::processCmdLine ( const StringList & args, StringList & extraArgs )
{
    const List<ConfigOpt *> & cmdOptions = ConfigOpt::cmdLineOptions();

    {
        // Sanity check!

        HashMap<char, ConfigOpt *> cmdFlags;
        HashMap<String, ConfigOpt *> cmdNames;

        for ( size_t i = 0; i < cmdOptions.size(); ++i )
        {
            ConfigOpt * const opt = cmdOptions.at ( i );

            if ( !opt )
                continue;

            if ( opt->optCmdLineFlag != 0 )
            {
                if ( cmdFlags.contains ( opt->optCmdLineFlag ) )
                {
                    return String ( "Multiple options (%1, %2) use the same command-line flag: '%3'" )
                           .arg ( opt->optCmdLineName, cmdFlags.value ( opt->optCmdLineFlag )->optCmdLineName )
                           .arg ( opt->optCmdLineFlag );
                }

                cmdFlags.insert ( opt->optCmdLineFlag, opt );
            }

            if ( !opt->optCmdLineName.isEmpty() )
            {
                if ( cmdNames.contains ( opt->optCmdLineName ) )
                {
                    return String ( "Multiple options use the same command-line name: '%1'" )
                           .arg ( opt->optCmdLineName );
                }

                cmdNames.insert ( opt->optCmdLineName, opt );
            }
        }
    }

    for ( size_t idx = 0; idx < args.size(); ++idx )
    {
        String arg ( args[ idx ] );

        if ( !arg.startsWith ( "-" ) )
        {
            extraArgs.append ( arg );
            continue;
        }

        ConfigOpt * cfgOpt = 0;
        String optName;
        String optValue;
        bool useOptValue = false;

        const int eqIndex = arg.find ( '=' );

        if ( eqIndex >= 0 )
        {
            useOptValue = true;
            optValue = arg.substr ( eqIndex + 1 );
            arg = arg.substr ( 0, eqIndex );
        }

        if ( arg.length() > 2 && arg.startsWith ( "--" ) )
        {
            optName = arg.substr ( 2 );

            for ( size_t i = 0; i < cmdOptions.size(); ++i )
            {
                ConfigOpt * const opt = cmdOptions.at ( i );

                if ( opt != 0 && !opt->optCmdLineName.isEmpty() && optName.matches ( opt->optCmdLineName ) )
                {
                    if ( cfgOpt != 0 )
                    {
                        return String ( "Multiple options ('%1', '%2') match the command-line option name: '%3'" )
                               .arg ( cfgOpt->optCmdLineName, opt->optCmdLineName, optName );
                    }

                    cfgOpt = opt;
                }
            }
        }
        else if ( arg.length() == 2 )
        {
            const char optFlag = arg[ 1 ];

            optName.append ( optFlag );

            for ( size_t i = 0; i < cmdOptions.size(); ++i )
            {
                ConfigOpt * const opt = cmdOptions.at ( i );

                if ( opt != 0 && opt->optCmdLineFlag != 0 && optFlag == opt->optCmdLineFlag )
                {
                    cfgOpt = opt;
                    break;
                }
            }
        }

        if ( !cfgOpt )
        {
            if ( optName.isEmpty() )
            {
                return String ( "Invalid command-line option name: '%1'" ).arg ( arg );
            }

            return String ( "Unrecognized command-line option: '%1'" ).arg ( arg );
        }

        assert ( cfgOpt != 0 );

        // This option will be protected from configuration file changes
        _cmdLineOptionsUsed.insert ( cfgOpt );

        if ( ( cfgOpt->_optFlags & ConfigOpt::FlagIsSimpleSwitch ) == 0 )
        {
            if ( !useOptValue )
            {
                if ( ++idx >= args.size() )
                    return String ( "Missing argument for command-line option '%1'" ).arg ( arg );

                optValue = args[ idx ];
            }

            const ERRCODE eCode = cfgOpt->loadOption ( optName, optValue, false );

            if ( NOT_OK ( eCode ) )
            {
                return String ( "Invalid value (%1) for command-line option '%2': %3" )
                       .arg ( optValue, arg, eCode.toString() );
            }

            continue;
        }

        if ( useOptValue )
        {
            return String ( "Simple switch command-line option '%1' does not take any arguments!" ).arg ( arg );
        }

        const ERRCODE eCode = cfgOpt->loadOption ( optName, "", false );

        if ( NOT_OK ( eCode ) )
        {
            return String ( "Unexpected error while setting a simple switch command-line option '%1': %2" )
                   .arg ( arg, eCode.toString() );
        }
    }

    return String::EmptyString;
}

bool ConfigCore::checkRequiredOptions()
{
    // We want to check all options - both regular and command-line.
    // t = 0 - regular options
    // t = 1 - command line options
    // we may test some of the options twice (if it is registered for both), but it doesn't matter.

    for ( int t = 0; t < 2; ++t )
    {
        List<ConfigOpt *> & optList = ( t == 0 )
                                      ? ( ConfigOpt::cfgOptions() )
                                      : ( ConfigOpt::cmdLineOptions() );

        for ( size_t i = 0; i < optList.size(); ++i )
        {
            ConfigOpt * opt = optList.at ( i );

            // Let's skip if:
            // invalid option,
            // or option that is configured,
            // or option that doesn't use FlagReqNonEmpty flag.

            if ( !opt || opt->isNonEmpty() || ( opt->_optFlags & ConfigOpt::FlagReqNonEmpty ) == 0 )
            {
                continue;
            }

            String optName = opt->optName;
            String cmdOptName;

            if ( opt->optCmdLineFlag != 0 )
            {
                cmdOptName = String ( "-%1" ).arg ( opt->optCmdLineFlag );
            }

            if ( !opt->optCmdLineName.isEmpty() )
            {
                if ( !cmdOptName.isEmpty() )
                    cmdOptName.append ( "/" );

                cmdOptName.append ( String ( "--%1" ).arg ( opt->optCmdLineName ) );
            }

            if ( cmdOptName.isEmpty() )
            {
                // This option has no command-line name - use config-file name.
                // Config-file name shouldn't be empty if command-line one is (we should have at least one name),
                // but even if both are empty - we don't care.

                fprintf ( stderr, "Invalid or missing config option '%s'\n", optName.c_str() );
            }
            else if ( optName.isEmpty() )
            {
                // Command-line name, but no regular name

                fprintf ( stderr, "Invalid or missing command-line option '%s'\n", cmdOptName.c_str() );
            }
            else
            {
                // Both config-file and command-line names

                fprintf ( stderr, "Invalid or missing config option '%s' (command-line: '%s')\n",
                          optName.c_str(), cmdOptName.c_str() );
            }

            return false;
        }
    }

    return true;
}

void ConfigCore::printBuildInfo ( FILE * toFile )
{
    if ( !toFile )
        return;

    fprintf ( toFile, "%s %s %s %s %s %s\n",
              optProductName.c_str(),
              optVersion.c_str(),
              optSrcRevision.c_str(),
              optBuildType.c_str(),
              optEcfgTags.c_str(),
              optBuildTime.c_str() );
}

void ConfigCore::printSampleConfig ( FILE * toFile, bool includeUncommon )
{
    if ( !toFile )
        return;

    fprintf ( toFile, "#\n# Sample configuration file (using%s default options)%s%s\n#\n",
              includeUncommon ? " all" : " common",
              ( optProductName.isEmpty() ? "" : " for " ),
              optProductName.c_str() );

    const List<ConfigOpt *> & cfgOptions = ConfigOpt::cfgOptions();

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        if ( !opt )
            continue;

        // These cannot be set using config file anyway!
        if ( opt->_optFlags & ConfigOpt::FlagBuiltInOnly )
            continue;

        // We only want to include common options
        if ( !includeUncommon && ( !( opt->_optFlags & ConfigOpt::FlagIsCommon ) ) )
            continue;

        StringList helpLines = opt->optHelpText.split ( "\n", true );

        for ( size_t l = 0; l < helpLines.size(); ++l )
        {
            const StringList helpWords = helpLines.at ( l ).split ( " \t", true );

            int lineLen = 1024;

            for ( size_t i = 0; i < helpWords.size(); ++i )
            {
                if ( lineLen + 1 + helpWords.at ( i ).length() >= MAX_LINE_LEN )
                {
                    fprintf ( toFile, "\n#" );
                    lineLen = 2;
                }

                fprintf ( toFile, " " );
                ++lineLen;

                fprintf ( toFile, "%s", helpWords.at ( i ).c_str() );
                lineLen += helpWords.at ( i ).length();
            }
        }

        const HashMap<String, StringList> vals ( opt->getValues() );
        HashMap<String, StringList>::Iterator it ( vals );

        if ( !it.isValid() || it.key() != opt->optName )
        {
            fprintf ( toFile, "\n#%s =\n", opt->optName.c_str() );
        }

        for ( ; it.isValid(); it.next() )
        {
            const StringList & vList = it.value();

            for ( size_t v = 0; v < vList.size(); ++v )
            {
                fprintf ( toFile, "\n%s =%s%s", it.key().c_str(),
                          ( vList.at ( v ).isEmpty() ? "" : " " ), vList.at ( v ).c_str() );
            }

            fprintf ( toFile, "\n" );
        }
    }

    fprintf ( toFile, "\n" );
}

void ConfigCore::printOptions ( FILE * toFile, bool includeHelp )
{
    if ( !toFile )
        return;

    const List<ConfigOpt *> & cfgOptions = ConfigOpt::cfgOptions();

    int maxLength = 0;

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        assert ( opt != 0 );

        if ( !opt )
            continue;

        if ( opt->optName.length() > maxLength )
            maxLength = opt->optName.length();

        const HashMap<String, StringList> vals ( opt->getValues() );

        for ( HashMap<String, StringList>::Iterator it ( vals ); it.isValid(); it.next() )
        {
            if ( it.key().length() > maxLength )
                maxLength = it.key().length();
        }
    }

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        if ( !opt )
            continue;

        const HashMap<String, StringList> vals ( opt->getValues() );

        if ( includeHelp )
        {
            StringList helpLines = opt->optHelpText.split ( "\n", true );

            for ( size_t l = 0; l < helpLines.size(); ++l )
            {
                const StringList helpWords = helpLines.at ( l ).split ( " \t", true );

                int lineLen = 1024;

                for ( size_t i = 0; i < helpWords.size(); ++i )
                {
                    if ( lineLen + 1 + helpWords.at ( i ).length() >= MAX_LINE_LEN )
                    {
                        fprintf ( toFile, "\n  " );
                        lineLen = 2;
                    }

                    fprintf ( toFile, " " );
                    ++lineLen;

                    fprintf ( toFile, "%s", helpWords.at ( i ).c_str() );
                    lineLen += helpWords.at ( i ).length();
                }
            }
        }

        HashMap<String, StringList>::Iterator it ( vals );

        if ( includeHelp && ( !it.isValid() || it.key() != opt->optName ) )
        {
            fprintf ( toFile, "\n*  %s", opt->optName.c_str() );

            for ( int len = maxLength; len > opt->optName.length(); --len )
            {
                fprintf ( toFile, " " );
            }

            fprintf ( toFile, " =" );
        }

        for ( ; it.isValid(); it.next() )
        {
            const StringList & vList = it.value();

            for ( size_t v = 0; v < vList.size(); ++v )
            {
                fprintf ( toFile, "\n%s%s", includeHelp ? "*  " : "", it.key().c_str() );

                for ( int len = maxLength; len > it.key().length(); --len )
                {
                    fprintf ( toFile, " " );
                }

                fprintf ( toFile, " =%s%s", ( vList.at ( v ).isEmpty() ? "" : " " ), vList.at ( v ).c_str() );
            }
        }

        if ( includeHelp )
            fprintf ( toFile, "\n" );
    }

    fprintf ( toFile, "\n" );
}

void ConfigCore::printCmdLineHelp ( FILE * toFile, const char * progName )
{
    fprintf ( toFile, "\nUsage: %s [options]\n\n"
              " Possible options:\n\n", progName );

    const List<ConfigOpt *> & cfgOptions = ConfigOpt::cmdLineOptions();

    int maxLength = 0;
    bool hasValues = false;

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        assert ( opt != 0 );

        if ( !opt )
            continue;

        if ( opt->optCmdLineName.length() > maxLength )
        {
            maxLength = opt->optCmdLineName.length();
        }

        if ( ( opt->_optFlags & ConfigOpt::FlagIsSimpleSwitch ) == 0 )
            hasValues = true;
    }

    if ( hasValues )
    {
        // If there are some long options that don't use values, we don't need to reserve that much space for them:
        maxLength = 0;

        for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
        {
            ConfigOpt * opt = cfgOptions.at ( idx );

            assert ( opt != 0 );

            if ( !opt )
                continue;

            int useLen = opt->optCmdLineName.length();

            if ( ( opt->_optFlags & ConfigOpt::FlagIsSimpleSwitch ) != 0
                 && useLen > 6 )
            {
                useLen -= 6;
            }

            if ( useLen > maxLength )
            {
                maxLength = useLen;
            }
        }
    }

    // Formats of the line:
    // '  -f help'
    // '  -f value help'
    // '  -f,  --flag_name help'
    // '  -f,  --flag_name=value help'
    // We assume there are always some simple flags.

    int helpIndent = 5;

    if ( hasValues )
        helpIndent += 6;

    if ( maxLength > 0 )
    {
        // ',  --name'
        helpIndent += 5 + maxLength;
    }

    for ( size_t idx = 0; idx < cfgOptions.size(); ++idx )
    {
        ConfigOpt * opt = cfgOptions.at ( idx );

        if ( !opt )
            continue;

        if ( opt->optCmdLineFlag != 0 )
        {
            fprintf ( toFile, "  -%c", opt->optCmdLineFlag );
        }
        else
        {
            fprintf ( toFile, "    " );
        }

        int indent = 4;

        if ( !opt->optCmdLineName.isEmpty() )
        {
            fprintf ( toFile, "%c  --%s",
                      ( opt->optCmdLineFlag != 0 ) ? ',' : ' ',
                      opt->optCmdLineName.c_str() );

            indent += 5 + opt->optCmdLineName.length();
        }

        if ( ( opt->_optFlags & ConfigOpt::FlagIsSimpleSwitch ) == 0 )
        {
            fprintf ( toFile, "%cvalue", ( !opt->optCmdLineName.isEmpty() ) ? '=' : ' ' );
            indent += 6;
        }

        while ( indent++ < helpIndent )
        {
            fprintf ( toFile, " " );
        }

        StringList helpLines = opt->optHelpText.split ( "\n", true );

        for ( size_t l = 0; l < helpLines.size(); ++l )
        {
            if ( l > 0 )
            {
                fprintf ( toFile, "\n" );

                for ( int j = 0; j < helpIndent; ++j )
                {
                    fprintf ( toFile, " " );
                }
            }

            const StringList helpWords = helpLines.at ( l ).split ( " \t" );

            int lineLen = helpIndent;

            for ( size_t i = 0; i < helpWords.size(); ++i )
            {
                if ( lineLen + helpWords.at ( i ).length() >= MAX_LINE_LEN )
                {
                    fprintf ( toFile, "\n" );

                    for ( int j = 0; j < helpIndent; ++j )
                    {
                        fprintf ( toFile, " " );
                    }

                    lineLen = helpIndent;
                }

                fprintf ( toFile, " " );
                ++lineLen;

                fprintf ( toFile, "%s", helpWords.at ( i ).c_str() );
                lineLen += helpWords.at ( i ).length();
            }
        }

        if ( ( opt->_optFlags & ConfigOpt::FlagIsDefaultSet ) != 0 && !opt->optCmdLineDefault.isEmpty() )
        {
            fprintf ( toFile, "\n" );

            for ( int j = 0; j < helpIndent; ++j )
            {
                fprintf ( toFile, " " );
            }

            fprintf ( toFile, " Default value: '%s'", opt->optCmdLineDefault.c_str() );
        }

        fprintf ( toFile, "\n" );
    }

    fprintf ( toFile, "\n" );
}

void ConfigCore::registerListener ( Listener * listener )
{
    if ( listener != 0 )
    {
        _listeners.insert ( listener );
    }
}

void ConfigCore::unregisterListener ( Listener * listener )
{
    if ( listener != 0 )
    {
        _listeners.remove ( listener );
    }
}
