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
#include <cstdlib>
#include <cerrno>
#include <ctime>

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>

#ifndef SYSTEM_WINDOWS
#include <dirent.h>
#else
#include <winsock2.h>
#include <direct.h>
#endif
}

#include "EcfgGen.hpp"
#include "GitInfo.hpp"
#include "config/ConfigCore.hpp"
#include "app/StdApp.hpp"
#include "event/Process.hpp"
#include "sys/OsUtils.hpp"
#include "json/Json.hpp"

#define OUT_INDENT    "    "

using namespace Pravala;

ConfigString EcfgGenerator::optProductName (
        ConfigOpt::FlagReqNonEmpty,
        "product", 'p', "",
        "The name of the product.\n"
        "It is used as value for 'build_info.product' option." );

ConfigString EcfgGenerator::optProjectDir (
        ConfigOpt::FlagReqNonEmpty,
        "directory", 'd', "",
        "The path to the root directory of the project." );

ConfigString EcfgGenerator::optInputFile ( "input", 'i',
        "The path to the ECFG file to use for generating the ECFG content (optional).\n"
        "If it is used, it should be specific product's '.ecfg' file." );

ConfigString EcfgGenerator::optOutputFile (
        ConfigOpt::FlagReqNonEmpty,
        "output", 'o', "",
        "The path to the output file. This file will be used for storing "
        "the generated ECFG initialization code. Use '-' for standard output." );

ConfigString EcfgGenerator::optUsername ( "user", 'u',
        "The value to be placed in 'build_info.build_user' option." );

ConfigString EcfgGenerator::optHostname ( "host", 'm',
        "The value to be placed in 'build_info.build_host' option." );

ConfigString EcfgGenerator::optBuildType ( "type", 'b',
        "The value to be placed in 'build_info.build_type' option." );

ConfigString EcfgGenerator::optRevSuffix ( "revision", 'r',
        "The value used as a revision suffix to be placed in 'build_info.src_rev' option. "
        "The revision is read from GIT and suffixed by this string." );

ConfigString EcfgGenerator::optVersionJson ( "version-json", 0,
        "The file from which to read the version string to be placed in 'build_info.version' option. "
        "Format: 'file_path,json_field'. Provided file will be parsed as a JSON object "
        "and 'json_field' will be read to obtain the version string." );

ConfigString EcfgGenerator::optVersionFile ( "version-file", 0,
        "The file from which to read the version string to be placed in 'build_info.version' option. "
        "Overrides 'version-json' option." );

ConfigString EcfgGenerator::optVersion ( "version", 'v',
        "The value used as a version string to be placed in 'build_info.version' option. "
        "Overrides 'version-file' and 'version-json' options. "
        "If no version information is provided, 'git:[branch name]' will be used." );

ConfigStringMultiSetList EcfgGenerator::optTags ( "tag", 't',
        "A tag ( one of the tags ) to be used while interpreting ECFG files. "
        "There can be several tags provided ( by using ' - t tag' several times ). "
        "The lines in ECFG files that start with '@tag' will be interpreted only "
        "if the 'tag' used is one of the tags provided in the command line." );

EcfgGenerator::EcfgGenerator()
{
}

bool EcfgGenerator::readEcfgFile()
{
    bool isOk = false;
    const MemHandle input ( optInputFile.value(), &isOk );

    if ( !isOk )
    {
        fprintf ( stderr, "Error reading file '%s'\n", optInputFile.value().c_str() );

        return false;
    }

    StringList lines = input.toString().split ( "\n" );

    for ( size_t i = 0; i < lines.size(); ++i )
    {
        String sLine = lines[ i ].trimmed();

        if ( sLine.isEmpty() )
            continue;

        // Lines starting with '#' are comments
        if ( sLine[ 0 ] == '#' )
            continue;

        bool skipLine = false;

        // '@' character means an 'optional, tag-based file entry'.
        // The line that follows '@tag' entry will be interpreted only if that
        // 'tag' is on the list of current tag (specified in program's command line)
        while ( !sLine.isEmpty() && sLine[ 0 ] == '@' )
        {
            String tagName;
            int optBeg = -1;
            bool endOfTag = false;

            for ( int j = 1; j < sLine.length(); ++j )
            {
                if ( String::isSpace ( sLine[ j ] ) )
                {
                    endOfTag = true;
                }
                else if ( endOfTag )
                {
                    optBeg = j;
                    break;
                }

                if ( !endOfTag )
                    tagName.append ( sLine[ j ] );
            }

            if ( tagName.isEmpty() || optBeg < 3 )
            {
                fprintf ( stderr, "Incorrect line format in '%s': '%s'\n",
                          optInputFile.value().c_str(), lines[ i ].c_str() );
                return false;
            }

            tagName = tagName.trimmed().toLower();

            if ( !_tags.contains ( tagName ) )
            {
                skipLine = true;
                break;
            }

            sLine = sLine.substr ( optBeg );
        }

        if ( skipLine )
            continue;

        int eqPos = sLine.find ( '=' );

        // < 1, we don't want empty option names!
        if ( eqPos < 1 )
        {
            fprintf ( stderr, "Incorrect line format in '%s': '%s'\n",
                      optInputFile.value().c_str(), lines[ i ].c_str() );
            return false;
        }

        String optName = sLine.substr ( 0, eqPos ).simplified().toLower();
        String optValue = sLine.substr ( eqPos + 1 ).trimmed();

        // It doesn't matter whether this option is forced or not.
        // If something set it as 'forced' already we have a problem!
        if ( _automaticOptions.contains ( optName ) )
        {
            fprintf ( stderr, "Option '%s' cannot be set using the ECFG file '%s'\n",
                      optName.c_str(), optInputFile.value().c_str() );
            return false;
        }

        _options[ optName ].append ( optValue );
    }

    return true;
}

bool EcfgGenerator::run()
{
    fprintf ( stderr, "Product Name: '%s'\n", optProductName.c_str() );
    fprintf ( stderr, "Project Dir: '%s'\n", optProjectDir.c_str() );

    if ( optInputFile.isSet() )
    {
        fprintf ( stderr, "Input File: '%s'\n", optInputFile.c_str() );
    }
    else
    {
        fprintf ( stderr, "Input File: NOT USED\n" );
    }

    if ( optOutputFile.value() == "-" )
    {
        fprintf ( stderr, "Output File: STD OUTPUT\n" );
    }
    else
    {
        fprintf ( stderr, "Output File: '%s'\n", optOutputFile.c_str() );
    }

    if ( optUsername.isSet() )
        fprintf ( stderr, "User: '%s'\n", optUsername.c_str() );

    if ( optHostname.isSet() )
        fprintf ( stderr, "Hostname: '%s'\n", optHostname.c_str() );

    if ( optBuildType.isSet() )
        fprintf ( stderr, "Build Type: '%s'\n", optBuildType.c_str() );

    if ( optRevSuffix.isSet() )
        fprintf ( stderr, "Revision Suffix: '%s'\n", optRevSuffix.c_str() );

    _tags.clear();

    for ( size_t i = 0; i < optTags.value().size(); ++i )
    {
        String str = String ( optTags.value().at ( i ) ).simplified().toLower();

        if ( str.isEmpty() )
        {
            fprintf ( stderr, "Incorrect tag provided: '%s'\n", optTags.value().at ( i ).c_str() );
            return false;
        }

        fprintf ( stderr, "Adding Tag: '%s'\n", str.c_str() );

        _tags.insert ( str );
    }

    if ( !addBuildInfo() )
        return false;

    if ( optInputFile.isSet() && !readEcfgFile() )
        return false;

    FILE * fout = 0;

    if ( optOutputFile.value() == "-" )
    {
        fout = stdout;
    }
    else
    {
        fout = fopen ( optOutputFile.value().c_str(), "w" );

        if ( !fout )
        {
            fprintf ( stderr, "Could not open output file '%s'\n", optOutputFile.value().c_str() );
            perror ( "fopen()" );

            return false;
        }
    }

    StringList lines;

    for ( HashMap<String, String>::Iterator it ( _automaticOptions ); it.isValid(); it.next() )
    {
        const String val = it.value().simplified();

        lines.append ( String ( "%1 =%2%3" ).arg ( it.key().simplified(), val.isEmpty() ? "" : " ", val ) );
    }

    for ( HashMap<String, StringList>::Iterator it ( _options ); it.isValid(); it.next() )
    {
        const StringList & vals = it.value();

        if ( vals.isEmpty() )
        {
            lines.append ( String ( "%1 =" ).arg ( it.key() ) );
        }
        else
        {
            for ( size_t v = 0; v < vals.size(); ++v )
            {
                lines.append ( String ( "%1 =%2%3" )
                               .arg ( it.key(), vals.at ( v ).isEmpty() ? "" : " ", vals.at ( v ) ) );
            }
        }
    }

    fprintf ( fout, "//\n"
              "// This file has been automatically generated.\n"
              "// Do not edit!\n"
              "//\n"
              "\n"
              "#include \"config/ConfigCore.hpp\"\n\n"
              "static Pravala::ConfigCore::EcfgInit ecfgInit (\n" OUT_INDENT "\"#_ECFG_OPT_BEGIN_\\n\"\n" );

    while ( lines.size() > 0 )
    {
        size_t idx = 0;

        assert ( lines.size() > 0 );

        String line = lines[ 0 ];

        // Ugly, ugly way, but it's easy (we don't have sorted templates) and this code is not critical anyway!
        for ( size_t i = 0; i < lines.size(); ++i )
        {
            if ( lines[ i ] < line )
            {
                idx = i;
                line = lines[ i ];
            }
        }

        fprintf ( fout, OUT_INDENT "\"%s\\n\"\n", line.replace ( "\\", "\\\\" ).replace ( "\"", "\\\"" ).c_str() );

        assert ( idx < lines.size() );

        lines.removeIndex ( idx );
    }

    fprintf ( fout, OUT_INDENT "\"#_ECFG_OPT_END_\\n\"\n);\n" );

    if ( fout != stdout )
    {
        fclose ( fout );
    }

    fout = 0;

    return true;
}

bool EcfgGenerator::addBuildInfo()
{
    String version = optVersion.value();

    if ( version.isEmpty()
         && optVersionFile.isNonEmpty()
         && ( version = MemHandle ( optVersionFile.value() ).toString().simplified() ).isEmpty() )
    {
        fprintf ( stderr, "Error reading version string from '%s' file\n", optVersionFile.c_str() );
        return false;
    }

    if ( version.isEmpty() && optVersionJson.isNonEmpty() )
    {
        const StringList strList ( optVersionJson.value().split ( ",", true ) );

        if ( strList.size() != 2 )
        {
            fprintf ( stderr, "Invalid value of '%s' option: '%s'; 'file_path,json_field' expected\n",
                      optVersionJson.optName.c_str(), optVersionJson.value().c_str() );
            return false;
        }

        Json json;

        if ( !json.decode ( MemHandle ( strList.at ( 0 ) ) ) )
        {
            fprintf ( stderr, "Error reading/parsing JSON file '%s'\n", strList.at ( 0 ).c_str() );
            return false;
        }

        if ( !json.get ( strList.at ( 1 ), version ) )
        {
            fprintf ( stderr, "Error reading field '%s' from JSON file '%s'\n",
                      strList.at ( 1 ).c_str(), strList.at ( 0 ).c_str() );
            return false;
        }
    }

    String gitRev;

    ERRCODE ret = GitInfo::readGitRev ( optProjectDir.value(), true, gitRev );

    if ( IS_OK ( ret ) && version.isEmpty() )
    {
        String gitBranch;

        if ( IS_OK ( ret = GitInfo::readGitBranch ( optProjectDir.value(), gitBranch ) ) )
        {
            version = String ( "git:" ).append ( gitBranch );
        }
    }

    if ( NOT_OK ( ret ) )
    {
        if ( ret == Error::CommandNotFound )
        {
            fprintf ( stderr, "Error running GIT: Could not find 'git' binary\n" );
        }
        else
        {
            fprintf ( stderr, "Error retrieving GIT data: %s\n", ret.toString() );
        }

        return false;
    }

    _automaticOptions.insert ( ConfigCore::optVersion.optName, version );
    _automaticOptions.insert ( ConfigCore::optProductName.optName, optProductName.value() );

    char tmpBuf[ 101 ];

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    strftime ( tmpBuf, 100, "%Y-%m-%d %H:%M:%S %Z", timeinfo );
    tmpBuf[ 100 ] = 0;

    _automaticOptions.insert ( ConfigCore::optBuildTime.optName, tmpBuf );

    if ( !optHostname.isEmpty() )
    {
        _automaticOptions.insert ( ConfigCore::optBuildHost.optName, optHostname.value() );
    }
    else
    {
#ifdef SYSTEM_WINDOWS
        static bool wsaStarted = false;

        if ( !wsaStarted )
        {
            wsaStarted = true;

            WSADATA wsaData;
            WSAStartup ( MAKEWORD ( 2, 2 ), &wsaData );
        }
#endif

        if ( gethostname ( tmpBuf, 100 ) != 0 )
            snprintf ( tmpBuf, 100, "unknown" );

        tmpBuf[ 100 ] = 0;

        _automaticOptions.insert ( ConfigCore::optBuildHost.optName, tmpBuf );
    }

    if ( !optUsername.isEmpty() )
    {
        _automaticOptions.insert ( ConfigCore::optBuildUser.optName, optUsername.value() );
    }
    else
    {
        String uName;

#if defined( SYSTEM_WINDOWS ) && !defined( UNICODE )
        DWORD len = 100;

        if ( GetUserName ( tmpBuf, &len ) )
        {
            tmpBuf[ 100 ] = 0;

            if ( len < 100 )
                tmpBuf[ len ] = 0;

            uName = tmpBuf;
        }
#else
        uName = Process::getEnvValue ( "USER" );
#endif

        if ( uName.isEmpty() )
            uName = "unknown";

        _automaticOptions.insert ( ConfigCore::optBuildUser.optName, uName );
    }

    String revStr ( gitRev );

    if ( !optRevSuffix.isEmpty() )
    {
        revStr.append ( "-" );
        revStr.append ( optRevSuffix.value() );
    }

    _automaticOptions.insert ( ConfigCore::optSrcRevision.optName, String ( "git:" ).append ( revStr ) );

    if ( !optBuildType.isEmpty() )
    {
        _automaticOptions.insert ( ConfigCore::optBuildType.optName, optBuildType.value() );
    }
    else
    {
        _automaticOptions.insert ( ConfigCore::optBuildType.optName, "devel" );
    }

    String strTags;

    for ( HashSet<String>::Iterator it ( _tags ); it.isValid(); it.next() )
    {
        if ( !strTags.isEmpty() )
            strTags.append ( "," );

        strTags.append ( it.value() );
    }

    _automaticOptions.insert ( ConfigCore::optEcfgTags.optName, strTags );

    return true;
}
