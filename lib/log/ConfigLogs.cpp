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

#include <cstdio>

#include "sys/OsUtils.hpp"
#include "ConfigLogs.hpp"
#include "LogManager.hpp"
#include "TextLogFileOutput.hpp"

#if defined( PLATFORM_ANDROID )
#include "os/Android/TextLogAndroidOutput.hpp"
#elif defined( SYSTEM_APPLE )
#include "os/Apple/TextLogAppleOutput.hpp"
#endif

#define DEFAULT_LVL       "default_level"

#define OUTPUT_STD        "STDOUT"
#define OUTPUT_ERR        "STDERR"
#define OUTPUT_ANDROID    "ANDROID"
#define OUTPUT_APPLE      "APPLE"

using namespace Pravala;

/// @brief A helper function that makes sure that the log prefix is sane
/// @param [in] prefix The prefix to use
/// @return log prefix
static String checkPrefix ( const char * prefix )
{
    String ret ( prefix );

    while ( ret.endsWith ( "." ) )
    {
        ret = ret.substr ( 0, ret.length() - 1 );
    }

    if ( ret.isEmpty() )
    {
        return "log";
    }

    return ret;
}

/// @brief A helper function that generates the help text for log option, based on the log prefix
/// @param [in] prefix The prefix to use
/// @return option's help text
static String genOptHelp ( const char * prefix )
{
#ifdef NO_LOGGING
    ( void ) prefix;

    return "This binary does not support logging\n";
#else
    String ret ( "Logging configuration\nAll log options have to match '" );
    ret.append ( checkPrefix ( prefix ) );
    ret.append ( ".*' filter.\n\n" );
    ret.append ( checkPrefix ( prefix ) );
    ret.append (
        "." DEFAULT_LVL " configures the default logging level\n"
        "Possible (at the moment) levels are:\n"
        "  max/all (the same as the most verbose level supported - right now it's debug4)\n"
        "  debug4\n"
        "  debug3\n"
        "  debug2\n"
        "  debug1/debug\n"
        "  info\n"
        "  warning/warn\n"
        "  error\n"
        "  fatal_error/fatal\n\n"
        "Log entry (other than '" DEFAULT_LVL "' one) looks as follows:\n\n"
    );

    ret.append ( checkPrefix ( prefix ) );
    ret.append (
        ".id = [output] : stream_name_filter_1[.level] [stream_name_filter_2[.level]] ... \n\n"
        "'id' is the (16 bit unsigned) id of the log option. All the log options are processed in ascending ID order, "
        "so id '0' is the default rule, and all other rules will be applied later. "
        "Each log option, when applied, sets the new level for that particular log subscription, "
        "which means that logging rules can both increase and decrease verbosity of particular streams. "
        "For example, if rule '0' adds '*.debug' logging to some file and there is another rule with ID '5', "
        "that subscribes the same output to 'tun.warning', all the log streams will generate 'debug' messages, "
        "except for 'tun' logging stream, that will only generate warnings.\n\n"
        "On the other hand, multiple filters in the same log entry, are processed in increasing \"chattiness\" order, "
        "which means that if more than one filter in the same rule matches a specific stream, then that stream "
        "will use the most chatty level of all the matching filters.\n\n"
        "'output' is the log file path, or a name of one of the standard log outputs. "
        "If it is omitted the standard log output (for the specific platform) is used. "
        "Special output names:\n\n"

#ifndef PLATFORM_ANDROID
        "  STDOUT  - standard output (the default one)\n"
#else
        "  STDOUT  - standard output\n"
#endif

        "  STDERR  - standard error output\n"

#ifdef PLATFORM_ANDROID
        "  ANDROID - Android's system log (the default one)\n"
        "             This output is also used instead of STDOUT and STDERR when running in daemon mode.\n"
#endif
        "\n"
        "Several stream name filters can be used at the same time, and 'stream_name_filter' is either the name "
        "of the log stream (client_host, flow_matcher, etc.), or a filter (client*, scheduler_*, a*, *, etc.).\n\n"
        "The 'level' configures the verbosity of the log messages captured. "
        "If it is omitted, the default level is used (configured with '" DEFAULT_LVL "' option)."
    );

    return ret;
#endif
}

ConfigLogs::ConfigLogs ( const char * logPrefix, bool stdVerbositySwitch, uint8_t flags ):
    ConfigOpt ( flags, checkPrefix ( logPrefix ).append ( ".*" ).c_str(), genOptHelp ( logPrefix ).c_str() ),
    _prefix ( checkPrefix ( logPrefix ).append ( "." ) ),
    _verbositySwitch ( 0 ), _daemonMode ( false )
{
    _defaultValue.defaultLogLevel = Log::LogLevel::Warning;

    LogDesc lDesc;
    lDesc.filterName = "*";

    _defaultValue.logConfig[ 0 ].logs.append ( lDesc );

    _value = _defaultValue;
    _optFlags |= FlagIsSet | FlagIsDefaultSet;

    if ( stdVerbositySwitch )
        enableVerbositySwitch();
}

ConfigLogs::~ConfigLogs()
{
    closeAllOutputs();

    delete _verbositySwitch;
    _verbositySwitch = 0;
}

void ConfigLogs::closeAllOutputs()
{
    for ( HashMap<String, TextLogOutput *>::Iterator it ( _outputs ); it.isValid(); it.next() )
    {
        delete it.value();
    }

    _outputs.clear();
}

bool ConfigLogs::enableVerbositySwitch ( const char * cmdLineName, char cmdLineFlag, const char * helpText )
{
    if ( _verbositySwitch != 0 )
        return false;

    if ( !helpText )
        helpText = "Increases verbosity level. Can be used multiple times.";

    _verbositySwitch = new ConfigSwitch ( cmdLineName, cmdLineFlag, helpText );

    return true;
}

void ConfigLogs::configUpdated()
{
    closeAllOutputs();

    ERRCODE eCode ( doInit ( true ) );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error re-initializing log system: %s\n", eCode.toString() );
    }
}

ERRCODE ConfigLogs::init ( bool daemonMode )
{
    _daemonMode = daemonMode;

    return doInit ( false );
}

List<ConfigLogs::RuleDesc> ConfigLogs::getSortedRules() const
{
    List<uint16_t> rules;

    for ( HashMap<uint16_t, RuleDesc>::Iterator it ( _value.logConfig ); it.isValid(); it.next() )
    {
        rules.append ( it.key() );
    }

    rules.sortAscending();

    List<RuleDesc> ret;

    const Log::LogLevel defLevel = getAdjustedDefaultLogLevel();

    for ( size_t i = 0; i < rules.size(); ++i )
    {
        RuleDesc ruleDesc = _value.logConfig.value ( rules.at ( i ) );

        if ( ruleDesc.logs.isEmpty() )
            continue;

        for ( size_t r = 0; r < ruleDesc.logs.size(); ++r )
        {
            if ( !ruleDesc.logs[ r ].useLevel )
            {
                ruleDesc.logs[ r ].level = defLevel;
            }
        }

        // This will sort entries based on their log level.
        // The entries with the highest level (as in more critical) will end up being at the beginning of the list.
        // Debugging entries will end up at the end of the list.
        // This way, when we apply a single log rule and a log stream matches several filters in the same rule,
        // we will only increase the chattiness.

        ruleDesc.logs.sortDescending();

        ret.append ( ruleDesc );
    }

    return ret;
}

ERRCODE ConfigLogs::doInit ( bool ignoreErrors )
{
    if ( !_outputs.isEmpty() )
        return Error::AlreadyInitialized;

    const List<RuleDesc> rules = getSortedRules();

    ERRCODE retCode = Error::Success;

    for ( size_t i = 0; i < rules.size(); ++i )
    {
        const RuleDesc & ruleDesc = rules.at ( i );

        if ( ruleDesc.logs.isEmpty() )
            continue;

        const List<LogDesc> & logs = ruleDesc.logs;

        for ( size_t r = 0; r < logs.size(); ++r )
        {
            const ERRCODE tmpCode = addLogSubscription ( ruleDesc.outputName, logs.at ( r ) );

            if ( NOT_OK ( tmpCode ) )
            {
                retCode = tmpCode;

                if ( !ignoreErrors )
                {
                    fprintf ( stderr, "Adding log subscription failed; "
                              "Output: '%s'; Log: '%s'; Level: '%s'; Error: '%s'\n",
                              ruleDesc.outputName.c_str(), logs.at ( r ).filterName.c_str(),
                              LogManager::logLevel2Str ( logs.at ( r ).level ),
                              tmpCode.toString() );

                    return retCode;
                }
            }
        }
    }

    return retCode;
}

void ConfigLogs::printConfig ( FILE * toFile )
{
    if ( !toFile )
        return;

    fprintf ( toFile, "\nLogging configuration:\n\n Default level: %s\n"
              "Rules (in processing order, later rules override earlier rules):\n\n",
              LogManager::logLevel2Str ( _value.defaultLogLevel ) );

    const List<RuleDesc> rules = getSortedRules();

    for ( size_t i = 0; i < rules.size(); ++i )
    {
        const RuleDesc & ruleDesc = rules.at ( i );

        if ( ruleDesc.logs.isEmpty() )
            continue;

        const List<LogDesc> & logs = ruleDesc.logs;

        for ( size_t r = 0; r < logs.size(); ++r )
        {
            fprintf ( toFile, "  - %s.%s -> '%s'\n",
                      logs.at ( r ).filterName.c_str(),
                      LogManager::logLevel2Str ( logs.at ( r ).level ),
                      ruleDesc.outputName.c_str() );
        }
    }

    fprintf ( toFile, "\n" );
}

void ConfigLogs::restoreDefaults()
{
    _value = _defaultValue;

    // We ALWAYS have both current and default values:
    assert ( _optFlags & FlagIsSet );
    assert ( _optFlags & FlagIsDefaultSet );
}

HashMap<String, StringList> ConfigLogs::getValues() const
{
    HashMap<String, StringList> ret;

    // We don't use getAdjustedDefaultLogLevel() here.
    // This is for printing help and sample configs and shouldn't be affected by verbosity command-line settings.

    ret[ String ( _prefix ).append ( DEFAULT_LVL ) ].append ( LogManager::logLevel2Str ( _value.defaultLogLevel ) );

    for ( HashMap<uint16_t, RuleDesc>::Iterator it ( _value.logConfig ); it.isValid(); it.next() )
    {
        String key ( _prefix );
        key.append ( String::number ( it.key() ) );

        const List<LogDesc> & vList = it.value().logs;
        String value ( it.value().outputName );

        value.append ( value.isEmpty() ? ":" : " :" );

        for ( size_t i = 0; i < vList.size(); ++i )
        {
            value.append ( " " );
            value.append ( vList.at ( i ).filterName );

            if ( vList.at ( i ).useLevel )
            {
                value.append ( "." );
                value.append ( LogManager::logLevel2Str ( vList.at ( i ).level ) );
            }
        }

        ret[ key ].clear();
        ret[ key ].append ( value );
    }

    return ret;
}

ERRCODE ConfigLogs::loadOption ( const String & name, const String & strValue, bool isDefault )
{
    if ( name.length() < _prefix.length() + 1 || strValue.length() < 1 )
        return Error::TooShortName;

    if ( String ( _prefix ).append ( DEFAULT_LVL ) == name )
    {
        if ( !LogManager::str2LogLevel ( strValue, _value.defaultLogLevel ) )
        {
            fprintf ( stderr, "Incorrect log level specified: '%s = %s'\n",
                      name.c_str(), strValue.c_str() );
            return Error::InvalidData;
        }

        if ( isDefault )
        {
            _defaultValue.defaultLogLevel = _value.defaultLogLevel;
        }

        return Error::Success;
    }

    bool ok = false;
    const uint16_t optId = name.substr ( _prefix.length() ).toUInt16 ( &ok );

    if ( !ok )
    {
        fprintf ( stderr, "Invalid log option name (option ID should be an unsigned, 16 bit value): "
                  "'%s = %s'\n", name.c_str(), strValue.c_str() );

        return Error::InvalidData;
    }

    const int outputSepIdx ( strValue.find ( ':' ) );

    if ( outputSepIdx < 0 )
    {
        fprintf ( stderr, "Log entry format error: '%s = %s'\n", name.c_str(), strValue.c_str() );
        return Error::ConfigError;
    }

    RuleDesc ruleDesc;

    ruleDesc.outputName = strValue.substr ( 0, outputSepIdx ).trimmed();
    const StringList logList ( strValue.substr ( outputSepIdx + 1 ).split ( " \t;," ) );

    for ( size_t i = 0; i < logList.size(); ++i )
    {
        LogDesc lDesc;

        if ( !lDesc.loadFromStr ( logList.at ( i ) ) )
        {
            fprintf ( stderr, "Invalid log specification string: "
                      "'%s = %s' (%s)\n", name.c_str(), strValue.c_str(), logList.at ( i ).c_str() );
            return Error::InvalidData;
        }

        ruleDesc.logs.append ( lDesc );
    }

    _value.logConfig.insert ( optId, ruleDesc );

    if ( isDefault )
    {
        _defaultValue = _value;
    }

    // We ALWAYS have both current and default values:
    assert ( _optFlags & FlagIsSet );
    assert ( _optFlags & FlagIsDefaultSet );

    return Error::Success;
}

bool ConfigLogs::LogDesc::loadFromStr ( const String & str )
{
    if ( str.length() < 1 )
        return false;

    const int dotIdx = str.findLastOf ( "." );

    if ( dotIdx < 0 )
    {
        filterName = str;
        useLevel = false;
        return true;
    }

    if ( dotIdx < 1 )
        return false;

    if ( !LogManager::str2LogLevel ( str.substr ( dotIdx + 1 ), level ) )
        return false;

    useLevel = true;
    filterName = str.substr ( 0, dotIdx );

    return true;
}

ERRCODE ConfigLogs::addLogSubscription ( const String & outputName, const ConfigLogs::LogDesc & logDesc )
{
    assert ( logDesc.level != Log::LogLevel::Invalid );

    if ( logDesc.level == Log::LogLevel::Invalid )
        return Error::InvalidParameter;

    String outName;

    if ( outputName.trimmed().isEmpty() || outputName.compare ( OUTPUT_STD, false ) == 0 )
    {
        // Default (empty) output, or standard output.

        if ( _daemonMode )
        {
            // In daemon mode we don't generate anything, except for Android where we use Android log output.

#ifdef PLATFORM_ANDROID
            outName = OUTPUT_ANDROID;
#else
            return Error::Success;
#endif
        }

        // In non-daemon mode we use standard output (Android or not).

        outName = OUTPUT_STD;
    }
    else if ( outputName.compare ( OUTPUT_ERR, false ) == 0 )
    {
        // Error output.

        if ( _daemonMode )
        {
            // In daemon mode we don't generate anything, except for Android where we use Android log output.

#ifdef PLATFORM_ANDROID
            outName = OUTPUT_ANDROID;
#else
            return Error::Success;
#endif
        }

        // In non-daemon mode we use standard error output (Android or not).

        outName = OUTPUT_ERR;
    }
    else if ( outputName.compare ( OUTPUT_ANDROID, false ) == 0 )
    {
        // 'Android' output. Only makes sense on Android - and we use Android log output.

#ifdef PLATFORM_ANDROID
        outName = OUTPUT_ANDROID;
#else
        // Not android, let's just silently skip this entry...
        return Error::Success;
#endif
    }
    else if ( outputName.compare ( OUTPUT_APPLE, false ) == 0 )
    {
        // 'Apple' output. Only makes sense on Apple - and we use Apple log output.

#ifdef SYSTEM_APPLE
        outName = OUTPUT_APPLE;
#else
        // Not apple, let's just silently skip this entry...
        return Error::Success;
#endif
    }
    else
    {
        // Otherwise we treat it as a log file path.

        outName = outputName;
    }

    TextLogOutput * logOutput = 0;

    if ( !_outputs.find ( outName, logOutput ) )
    {
        if ( outName == OUTPUT_STD )
        {
            TextLogFileOutput * out = new TextLogFileOutput ( STDOUT_FILENO );

            if ( out != 0 && !out->isOpen() )
            {
                delete out;
                out = 0;
            }
            else
            {
                logOutput = out;
            }
        }
        else if ( outName == OUTPUT_ERR )
        {
            TextLogFileOutput * out = new TextLogFileOutput ( STDERR_FILENO );

            if ( out != 0 && !out->isOpen() )
            {
                delete out;
                out = 0;
            }
            else
            {
                logOutput = out;
            }
        }
        else if ( outName == OUTPUT_ANDROID )
        {
#ifdef PLATFORM_ANDROID
            logOutput = new TextLogAndroidOutput;
#else
            return Error::Success;
#endif
        }
        else if ( outName == OUTPUT_APPLE )
        {
#ifdef SYSTEM_APPLE
            logOutput = new TextLogAppleOutput;
#else
            return Error::Success;
#endif
        }
        else
        {
            TextLogFileOutput * out = new TextLogFileOutput ( outName );

            if ( out != 0 && !out->isOpen() )
            {
                delete out;
                out = 0;
            }
            else
            {
                logOutput = out;
            }
        }

        if ( !logOutput )
            return Error::OpenFailed;

        _outputs.insert ( outName, logOutput );
    }

    if ( LogManager::get().subscribe ( logDesc.filterName, logDesc.level, logOutput ) )
    {
        return Error::Success;
    }

    return Error::NotFound;
}

Log::LogLevel ConfigLogs::getAdjustedDefaultLogLevel() const
{
    Log::LogLevel ret = _value.defaultLogLevel;

    if ( !_verbositySwitch )
        return ret;

    uint32_t vLevel = _verbositySwitch->value();

    while ( vLevel > 0 )
    {
        --vLevel;

        switch ( ret.value() )
        {
            case Log::LogLevel::FatalError:
                ret = Log::LogLevel::Error;
                break;

            case Log::LogLevel::Error:
                ret = Log::LogLevel::Warning;
                break;

            case Log::LogLevel::Warning:
                ret = Log::LogLevel::Info;
                break;

            case Log::LogLevel::Info:
                ret = Log::LogLevel::Debug;
                break;

            case Log::LogLevel::Debug:
                ret = Log::LogLevel::Debug2;
                break;

            case Log::LogLevel::Debug2:
                ret = Log::LogLevel::Debug3;
                break;

            case Log::LogLevel::Debug3:
                ret = Log::LogLevel::Debug4;
                break;

            case Log::LogLevel::Debug4:
            default:
                vLevel = 0;
                break;
        }
    }

    return ret;
}
