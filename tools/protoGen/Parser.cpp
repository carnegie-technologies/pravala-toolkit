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
#include <cerrno>
#include <cctype>

#include "basic/MemHandle.hpp"
#include "md5/Md5.hpp"
#include "Parser.hpp"
#include "Error.hpp"

using namespace Pravala;

Parser::Parser ( const String & inputFile ): _lineNumber ( 0 ), _inputPath ( inputFile )
{
    int lastSlash = -1;
    int tmp = 0;

    while ( ( tmp = inputFile.find ( "/", lastSlash + 1 ) ) >= 0 )
    {
        lastSlash = tmp;
    }

    if ( lastSlash >= 0 )
    {
        _inputDir = inputFile.substr ( 0, lastSlash + 1 );
        _inputFile = inputFile.substr ( lastSlash + 1 );
    }
    else
    {
        _inputFile = inputFile;
    }
}

bool Parser::isBasicChar ( char c )
{
    return ( isalnum ( c ) || c == '_' || c == '.' );
}

bool Parser::isSpecialChar ( char c )
{
    return ( !isspace ( c ) && !isBasicChar ( c ) );
}

bool Parser::isFile ( const String & path )
{
    if ( path.isEmpty() )
        return false;

    FILE * file = fopen ( path.c_str(), "r" );

    if ( !file )
        return false;

    fclose ( file );
    file = 0;

    return true;
}

bool Parser::run ( ParserRoot & parserRoot, ProtocolSpec & protoSpec )
{
    _lineNumber = 0;

    bool isOk = false;
    const MemHandle buf ( _inputPath, &isOk );

    if ( !isOk )
    {
        fprintf ( stderr, "Error reading from file '%s'\n", _inputPath.c_str() );
        return false;
    }

    const String fileMd5 ( Md5::generate ( buf ) );

    if ( parserRoot._parsedFiles.contains ( fileMd5 ) )
    {
        return true;
    }

    parserRoot._parsedFiles.insert ( fileMd5 );

    const bool generateOutput = parserRoot._generateOutputForFiles.contains ( fileMd5 );

    printf ( "Parsing protocol file '%s'%s\n", _inputPath.c_str(), generateOutput ? " [OUTPUT]" : "" );

    StringList lines = buf.toStringList ( "\n", true );

    bool wasOk = false;

    try
    {
        wasOk = doRun ( lines, generateOutput, parserRoot, protoSpec );
    }
    catch ( const Error & err )
    {
        fprintf ( stderr, "Error parsing %s:%lu: %s\n", _inputFile.c_str(), ( long unsigned ) _lineNumber,
                  err.getMessage().c_str() );
        return false;
    }

    if ( wasOk && !protoSpec.isClosed() )
    {
        fprintf ( stderr, "Error parsing %s:%lu: Missing '}'\n", _inputFile.c_str(), ( long unsigned ) _lineNumber );
        return false;
    }

    return wasOk;
}

bool Parser::doRun (
        const StringList & lines, bool generateOutput,
        ParserRoot & parserRoot, ProtocolSpec & protoSpec )
{
    String lastLineComment;

    _lineNumber = 0;

    while ( _lineNumber < lines.size() )
    {
        String line = lines[ _lineNumber ].trimmed();

        ++_lineNumber;

        if ( line.isEmpty() )
            continue;

        String curToken;
        StringList tokens;
        bool quoteMode = false;

        // Init the current line comment to the last line comment (consuming it)
        String lineComment = lastLineComment;
        lastLineComment.clear();

        for ( int idx = 0; idx < line.length(); ++idx )
        {
            char c = line[ idx ];

            if ( quoteMode )
            {
                // This if statement MUST be first because in quote mode, every character is part
                // of the quoted string, except for the closing quote.

                if ( c == '"' )
                {
                    curToken.append ( c );
                    quoteMode = false;
                    tokens.append ( curToken );
                    curToken.clear();
                }
                else
                {
                    curToken.append ( c );
                }
            }
            else if ( c == '"' )
            {
                if ( curToken.length() > 0 )
                    tokens.append ( curToken );

                curToken.clear();
                curToken.append ( c );
                quoteMode = true;
            }
            else if ( c == '#' )
            {
                // Stop parsing this line
                break;
            }
            else if ( c == '/' && idx + 1 < line.length() && line[ idx + 1 ] == '/' )
            {
                while ( line[ idx ] == '/' )
                {
                    ++idx;
                }

                const String comment = line.substr ( idx ).trimmed();

                if ( comment.length() > 0 )
                {
                    // Append to the current line comment
                    if ( lineComment.length() > 0 )
                    {
                        lineComment.append ( " " );
                    }

                    lineComment.append ( comment );
                }

                // Stop parsing this line
                break;
            }
            else if ( isSpecialChar ( c ) )
            {
                if ( curToken.length() > 0 )
                    tokens.append ( curToken );

                curToken.clear();
                curToken.append ( c );

                tokens.append ( curToken );
                curToken.clear();
            }
            else if ( isspace ( c ) )
            {
                if ( curToken.length() > 0 )
                {
                    tokens.append ( curToken );
                    curToken.clear();
                }
            }
            else
            {
                curToken.append ( c );
            }
        }

        if ( curToken.length() > 0 )
        {
            tokens.append ( curToken );
            curToken.clear();
        }

        if ( tokens.isEmpty() )
        {
            // If there were no tokens, that means there were no symbols on this line,
            // so we should accumulate the line comment for later.
            lastLineComment = lineComment;
        }
        else if ( tokens.first() == ProtocolSpec::KW_PRAGMA )
        {
            fprintf ( stderr,
                      "Warning: %s:%lu: Pragma support has been removed; Ignoring line '%s'.\n",
                      _inputFile.c_str(), ( long unsigned ) _lineNumber, line.c_str() );

            continue;
        }
        else if ( tokens.first() == ProtocolSpec::KW_IMPORT )
        {
            if ( tokens.size() == 3
                 && tokens.last() == ";"
                 && tokens[ 1 ].length() > 2
                 && tokens[ 1 ].startsWith ( "\"" )
                 && tokens[ 1 ].endsWith ( "\"" ) )
            {
                const String importPath = tokens[ 1 ].substr ( 1, tokens[ 1 ].length() - 2 );

                String fPath;

                if ( importPath.startsWith ( "/" ) )
                {
                    // Absolute path.
                    if ( isFile ( importPath ) )
                    {
                        fPath = importPath;
                    }
                }
                else if ( !isFile ( ( fPath = String ( "%1%2" ).arg ( _inputDir, importPath ) ) ) )
                {
                    // We couldnt' find importPath under this parser's input directory.
                    // Let's try additional import directories:

                    bool found = false;

                    for ( size_t i = 0; i < parserRoot.getImportDirs().size(); ++i )
                    {
                        if ( isFile ( ( fPath = String ( "%1%2" )
                                                .arg ( parserRoot.getImportDirs().at ( i ), importPath ) ) ) )
                        {
                            found = true;
                            break;
                        }
                    }

                    if ( !found )
                    {
                        fPath.clear();
                    }
                }

                if ( fPath.isEmpty() )
                {
                    fprintf ( stderr, "Error: %s:%lu: Could not find file to %s: '%s'.\n",
                              _inputFile.c_str(), ( long unsigned ) _lineNumber,
                              ProtocolSpec::KW_IMPORT, importPath.c_str() );
                    return false;
                }

                Parser tmpParser ( fPath );

                if ( !tmpParser.run ( parserRoot, protoSpec ) )
                {
                    return false;
                }

                continue;
            }

            fprintf ( stderr, "Error: %s:%lu: Incorrect format of %s instruction."
                      " Correct format is: '%s \"file_name.proto\";'\n",
                      _inputFile.c_str(), ( long unsigned ) _lineNumber,
                      ProtocolSpec::KW_IMPORT, ProtocolSpec::KW_IMPORT );

            return false;
        }

        protoSpec.setGenerateMode ( generateOutput );
        protoSpec.setProtoFilePath ( _inputPath );

        for ( size_t tIdx = 0; tIdx < tokens.size(); ++tIdx )
        {
            protoSpec.appendToken ( tokens[ tIdx ], lineComment );
        }
    }

    return true;
}
