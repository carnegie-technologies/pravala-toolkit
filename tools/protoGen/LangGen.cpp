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

#include "LangGen.hpp"
#include "Error.hpp"
#include "Symbol.hpp"

#include <cstdio>
#include <ctime>
#include <cerrno>
#include <cassert>

extern "C"
{
#include <sys/stat.h>
#include <sys/types.h>
}

#ifdef SYSTEM_WINDOWS

extern "C"
{
#include <direct.h> // for _mkdir
}

#ifdef mkdir
#undef mkdir
#endif

#define mkdir( a, b )    _mkdir ( a )
#endif

using namespace Pravala;

/// @todo TODO Files should be renamed to match class names.

LanguageGenerator::LanguageGenerator ( ProtocolSpec & proto ):
    _proto ( proto ), _singleIndent ( "  " ), _overwriteFiles ( false )
{
}

void LanguageGenerator::init()
{
}

void LanguageGenerator::canRun()
{
    // If we can overwrite files, we don't need to check flag files.
    if ( _overwriteFiles )
        return;

    struct stat st;

    if ( !_flagPath.isEmpty() && stat ( _flagPath.c_str(), &st ) == 0 )
        throw Error ( "Flag file '", _flagPath, "' already exists!" );

    if ( !_timeFlagPath.isEmpty() && stat ( _timeFlagPath.c_str(), &st ) == 0 )
        throw Error ( "Flag file '", _timeFlagPath, "' already exists!" );
}

String LanguageGenerator::getHelpText()
{
    String text
        = "      -o output_dir, --output=output_dir\n"
          "          Use given path as the output directory.\n\n"
          "      -i indent, --indent=indent\n"
          "          Use the string provided as a single indentation level.\n\n"
          "      --namespace-prefix=prefix\n"
          "          All symbols will be generated inside given namespace/package prefix.\n\n"
          "      --skip-leading-dirs=dirs\n"
          "          List of directories that will be skipped at the beginning of generated file paths.\n"
          "          Separated with commas or spaces.\n\n";

    text.append ( getBasicHelpText() );

    return text;
}

String LanguageGenerator::getBasicHelpText()
{
    return "      -O, --overwrite\n"
           "          When enabled, existing files will be overwritten.\n\n"
           "      --flag-file=file_path\n"
           "          When enabled, a file_path will be created after a successful generation.\n"
           "          Existing file will NOT be overwritten, unless --overwrite option is used as well.\n"
           "          When --overwrite is not used and the flag file already exists, it will prevent\n"
           "          the generator from running at all!\n\n"
           "      --time-flag-file=file_path\n"
           "          Just like set-flag option, but current date and time will be stored in file_path.\n\n";
}

LanguageGenerator::SetOptResult LanguageGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    if ( shortName == 'o' || longName == "output" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _baseOutDir = value;
    }
    else if ( shortName == 'i' || longName == "indent" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _singleIndent = value;
    }
    else if ( longName == "namespace-prefix" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _namespacePrefix = value;
    }
    else if ( longName == "skip-leading-dirs" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        // This will also get rid of any extra " or '
        _skipLeadingDirs = value.split ( " ,;\t\"'" );
    }
    else
    {
        return setBasicOption ( shortName, longName, value );
    }

    return OptOkValueConsumed;
}

LanguageGenerator::SetOptResult LanguageGenerator::setBasicOption (
        char shortName, const String & longName, const String & value )
{
    if ( longName == "flag-file" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _flagPath = value;
    }
    else if ( longName == "time-flag-file" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _timeFlagPath = value;
    }
    else if ( shortName == 'O' || longName == "overwrite" )
    {
        _overwriteFiles = true;
        return OptOkValueIgnored;
    }
    else
    {
        return OptErrUnknownOption;
    }

    return OptOkValueConsumed;
}

void LanguageGenerator::addFile ( FileObject * fileObject )
{
    if ( !fileObject )
    {
        assert ( false );
        return;
    }

    if ( _fileObjects.contains ( fileObject->getPath() ) )
    {
        assert ( false );
        throw Error ( "File '", fileObject->getPath(), "' has already been added" );
    }

    _fileObjects[ fileObject->getPath() ] = fileObject;
}

void LanguageGenerator::run()
{
    if ( _baseOutDir.isEmpty() )
        throw Error ( "An output directory path is needed" );

    createDirs ( _baseOutDir, false );

    procSymbol ( _proto.getRoot() );

    for ( HashMap<String, FileObject * >::Iterator it ( _fileObjects ); it.isValid(); it.next() )
    {
        FileObject * const fileObj = it.value();

        assert ( fileObj != 0 );

        writeFile ( getOutputFilePath ( fileObj ), fileObj );
    }

    generateFlagFiles();
}

void LanguageGenerator::createDirs ( const String & path, bool skipLast )
{
    StringList pParts = path.split ( "/" );
    String tmp;

    if ( path.startsWith ( "/" ) )
        tmp.append ( "/" );

    size_t pSize = pParts.size();

    if ( skipLast && pSize > 0 )
        --pSize;

    for ( size_t i = 0; i < pSize; ++i )
    {
        tmp.append ( pParts[ i ] );

        if ( mkdir ( tmp.c_str(), 0700 ) != 0 )
        {
            if ( errno != EEXIST )
            {
                throw Error ( "Could not create directory '", tmp, "': '", strerror ( errno ), "'" );
            }

            struct stat st;

            if ( stat ( tmp.c_str(), &st ) == 0 )
            {
                if ( ( st.st_mode & S_IFDIR ) != S_IFDIR )
                {
                    throw Error ( "Could not create directory '", tmp,
                                  "'; It already exists and is not a directory" );
                }
            }
            else
            {
#ifdef SYSTEM_WINDOWS
                // On Windows, the first element of the path might be the drive letter.
                // stat() doesn't work on that...
                if ( errno != ENOENT || i > 0 )
#endif
                {
                    // stat() failed
                    throw Error ( "Could not create directory '", tmp,
                                  "' and we could not examine the path: '", strerror ( errno ), "'" );
                }
            }
        }

        tmp.append ( "/" );
    }
}

String LanguageGenerator::getOutputFilePath ( FileObject * file )
{
    if ( !file )
        return String::EmptyString;

    return String ( "%1/%2" ).arg ( _baseOutDir, file->getPath() );
}

void LanguageGenerator::generateFlagFiles()
{
    if ( !_flagPath.isEmpty() )
    {
        TextFile * const file = new TextFile ( _flagPath );

        if ( !file )
            throw Error ( "Could not create an object for the flag file: ", _flagPath );

        writeFile ( _flagPath, file );

        _flagPath.clear();
        delete file;
    }

    if ( !_timeFlagPath.isEmpty() )
    {
        TextFile * const file = new TextFile ( _timeFlagPath );

        if ( !file )
            throw Error ( "Could not create an object for the flag file: ", _timeFlagPath );

        char tmpBuf[ 101 ];

        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        strftime ( tmpBuf, 100, "%Y-%m-%d %H:%M:%S %Z", timeinfo );
        tmpBuf[ 100 ] = 0;

        // The last EOL will be added automatically.
        file->a ( tmpBuf );

        writeFile ( _timeFlagPath, file );

        _timeFlagPath.clear();
        delete file;
    }
}

void LanguageGenerator::writeFile ( const String & path, FileObject * file )
{
    if ( !file )
        throw Error ( "No file to generate!" );

    if ( !_overwriteFiles )
    {
        struct stat st;

        if ( stat ( path.c_str(), &st ) == 0 )
            throw Error ( "File '", path, "' already exists!" );
    }

    FILE * fout = fopen ( path.c_str(), "w" );

    if ( !fout )
    {
        createDirs ( path, true );
    }

    fout = fopen ( path.c_str(), "w" );

    if ( !fout )
        throw Error ( "Could not open file '", path, "' for writing: '", strerror ( errno ), "'" );

    file->finalizeFile();
    file->writeOutput ( fout );

    fclose ( fout );
}

void LanguageGenerator::procSymbol ( Symbol * symbol )
{
    if ( !symbol )
        return;

    if ( symbol->isNamespace() )
    {
        procNamespaceSymbol ( symbol );
    }
    else if ( symbol->isBasic() )
    {
        procBasicSymbol ( symbol );
    }
    else
    {
        procRegularSymbol ( symbol );
    }
}

void LanguageGenerator::procNamespaceSymbol ( Symbol * symbol )
{
    assert ( symbol != 0 );
    assert ( !symbol->isEnum() );
    assert ( !symbol->isBasic() );
    assert ( !symbol->isMessageOrStruct() );
    assert ( symbol->isNamespace() );

    procInternalSymbols ( symbol );
}

void LanguageGenerator::procInternalSymbols ( Symbol * symbol )
{
    assert ( symbol != 0 );

    const StringList & intSymbols = symbol->getOrdInternalSymbols();

    for ( size_t i = 0; i < intSymbols.size(); ++i )
    {
        Symbol * const s = symbol->getInternalSymbols().value ( intSymbols[ i ] );

        assert ( s != 0 );

        procSymbol ( s );
    }
}

void LanguageGenerator::procBasicSymbol ( Symbol * /* symbol */ )
{
}
