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

#include "basic/MemHandle.hpp"
#include "sys/OsUtils.hpp"

#include "LangGen.hpp"
#include "Error.hpp"
#include "Parser.hpp"
#include "ProtoSpec.hpp"

#include "NullGen.hpp"
#include "cpp/PravCppGen.hpp"
#include "java/PravJavaGen.hpp"
#include "csharp/CSharpGen.hpp"
#include "html/HtmlGen.hpp"

using namespace Pravala;

static HashMap<String, LanguageGenerator *> langGenerators;

void showHelp ( FILE * toFile, const char * binName )
{
    fprintf ( toFile,
              "\nUsage: %s [generator] [options] [input_files]\n\n"
              " First argument is a mandatory name of the generator.\n"
              " Each additional argument that does not start with '-' "
              "will be treated as another input file to process.\n\n"
              " Options:\n\n"
              "  -h, --help\n"
              "      Print this help and exit.\n\n"
              "  -i file, --input-files=file\n"
              "      Read the list of input files from 'file', one file per line.\n"
              "      File paths will be treated as if they were passed in the command line.\n"
              "      This means that if they are relative, they will be relative to the directory in which\n"
              "      the generator is run, and NOT to the location of the 'file'.\n\n"
              "  -I dir, --import-dir=dir\n"
              "      Adds 'dir' to the list of directories to check while looking for files\n"
              "      specified using 'import' keyword.\n\n"
              "  -s scope, --id-scope=scope\n"
              "      Use specified field ID scope. Possible values:\n"
              "        branch - a field cannot use the same ID as any other field in the same object,\n"
              "                 or in any object inherited by it (directly or indirectly).\n"
              "        tree   - a field cannot use the same ID as any other field in the same object,\n"
              "                 or in any object that shares the same ancestor in the inheritance tree.\n"
              "                 This is the default mode.\n"
              "        global - a field cannot use the same ID as any other field.\n\n"
              " Available generators:\n\n",
              binName );

    for ( HashMap<String, LanguageGenerator *>::Iterator it ( langGenerators ); it.isValid(); it.next() )
    {
        fprintf ( toFile, "  %s\n", it.key().c_str() );
        fprintf ( toFile, "%s", it.value()->getHelpText().c_str() );
    }
}

int main ( int argc, char * argv[] )
{
    ProtocolSpec protoSpec;

    langGenerators[ "null" ] = new NullGenerator ( protoSpec );
    langGenerators[ "cpp" ] = new PravalaCppGenerator ( protoSpec );
    langGenerators[ "java" ] = new PravalaJavaGenerator ( protoSpec );
    langGenerators[ "csharp" ] = new CSharpGenerator ( protoSpec );
    langGenerators[ "html" ] = new HtmlGenerator ( protoSpec );

    for ( HashMap<String, LanguageGenerator *>::Iterator it ( langGenerators ); it.isValid(); it.next() )
    {
        if ( it.value() != 0 )
        {
            it.value()->init();
        }
    }

    if ( argc < 2 )
    {
        fprintf ( stderr, "At least one argument is needed.\n" );
        showHelp ( stderr, argv[ 0 ] );
        return EXIT_FAILURE;
    }

    const String genName = String ( argv[ 1 ] ).toLower();

    if ( genName == "-h" || genName == "--help" )
    {
        showHelp ( stdout, argv[ 0 ] );
        return EXIT_SUCCESS;
    }

    LanguageGenerator * const langGen = langGenerators.value ( genName );

    if ( !langGen )
    {
        fprintf ( stderr, "Unknown generator: '%s'\n", genName.c_str() );
        showHelp ( stderr, argv[ 0 ] );
        return EXIT_FAILURE;
    }

    ParserRoot parser;

    for ( int idx = 2; idx < argc; ++idx )
    {
        const String arg = String ( argv[ idx ] );

        if ( !arg.startsWith ( "-" ) )
        {
            if ( !parser.addInputFile ( arg ) )
            {
                fprintf ( stderr, "Invalid input file: '%s'\n", arg.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }

            continue;
        }

        char optChar = 0;
        String optName;
        String optEqValue; // Only set in '--name=value' format.

        if ( !arg.startsWith ( "--" ) )
        {
            if ( arg.length() != 2 )
            {
                fprintf ( stderr, "Invalid option: '%s'\n", argv[ idx ] );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }

            optChar = arg[ 1 ];
        }
        else
        {
            int tmp = arg.findFirstOf ( "=" );

            optName = arg.substr ( 2, tmp - 2 ).toLower();

            if ( tmp > 2 )
            {
                optEqValue = arg.substr ( tmp + 1 );
            }

            if ( optName.isEmpty() )
            {
                fprintf ( stderr, "Invalid option: '%s'\n", argv[ idx ] );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }
        }

        if ( optChar == 'h' || optName == "help" )
        {
            showHelp ( stdout, argv[ 0 ] );
            return EXIT_SUCCESS;
        }

        if ( optChar == 's' || optName == "id-scope" )
        {
            if ( optEqValue.isEmpty() )
            {
                if ( idx + 1 >= argc )
                {
                    fprintf ( stderr, "Option '%s' needs a value\n", argv[ idx ] );
                    showHelp ( stderr, argv[ 0 ] );
                    return EXIT_FAILURE;
                }

                optEqValue = argv[ ++idx ];
            }

            if ( !protoSpec.setIdScope ( optEqValue ) )
            {
                fprintf ( stderr, "Invalid field ID scope value: '%s'\n", optEqValue.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }

            continue;
        }

        if ( optChar == 'i' || optName == "input-files" )
        {
            if ( optEqValue.isEmpty() )
            {
                if ( idx + 1 >= argc )
                {
                    fprintf ( stderr, "Option '%s' needs a value\n", argv[ idx ] );
                    showHelp ( stderr, argv[ 0 ] );
                    return EXIT_FAILURE;
                }

                optEqValue = argv[ ++idx ];
            }

            bool isOk = false;
            const MemHandle buf ( optEqValue, &isOk );

            if ( !isOk )
            {
                fprintf ( stderr, "Could not read the list of input files from '%s'\n", optEqValue.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }

            const StringList tmpList = buf.toStringList ( "\r\n" );

            for ( size_t i = 0; i < tmpList.size(); ++i )
            {
                if ( !parser.addInputFile ( tmpList.at ( i ) ) )
                {
                    fprintf ( stderr, "Invalid input file: '%s'\n", tmpList.at ( i ).c_str() );
                    showHelp ( stderr, argv[ 0 ] );
                    return EXIT_FAILURE;
                }
            }

            continue;
        }

        if ( optChar == 'I' || optName == "import-dir" || optName == "import-directory" )
        {
            if ( optEqValue.isEmpty() )
            {
                if ( idx + 1 >= argc )
                {
                    fprintf ( stderr, "Option '%s' needs a non-empty value\n", argv[ idx ] );
                    showHelp ( stderr, argv[ 0 ] );
                    return EXIT_FAILURE;
                }

                optEqValue = argv[ ++idx ];
            }

            if ( !parser.addImportDir ( optEqValue ) )
            {
                fprintf ( stderr, "Invalid import directory: '%s'\n", optEqValue.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
            }

            continue;
        }

        String optVal = optEqValue;

        if ( optVal.isEmpty() && idx + 1 < argc )
        {
            optVal = argv[ idx + 1 ];
        }

        LanguageGenerator::SetOptResult setOptRet = LanguageGenerator::OptErrUnknownOption;

        try
        {
            setOptRet = langGen->setOption ( optChar, optName, optVal );
        }
        catch ( const Error & err )
        {
            fflush ( stdout );
            fprintf ( stderr, "Error: %s\n", err.getMessage().c_str() );
            return EXIT_FAILURE;
        }

        if ( optChar != 0 )
        {
            optName = String ( "-%1" ).arg ( optChar );
        }
        else
        {
            optName = String ( "--%1" ).arg ( optName );
        }

        switch ( setOptRet )
        {
            case LanguageGenerator::OptErrUnknownOption:
                fprintf ( stderr, "'%s' generator does not support '%s' option\n",
                          genName.c_str(), optName.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
                break;

            case LanguageGenerator::OptErrMissingValue:
                fprintf ( stderr, "Option '%s' requires a value\n", optName.c_str() );
                showHelp ( stderr, argv[ 0 ] );
                return EXIT_FAILURE;
                break;

            case LanguageGenerator::OptOkValueIgnored:
                if ( !optEqValue.isEmpty() )
                {
                    // Value not consumed, but was provided using 'option=value' format.

                    fprintf ( stderr, "Option '%s' does not use a value ('%s' was provided)\n",
                              optName.c_str(), optEqValue.c_str() );
                    showHelp ( stderr, argv[ 0 ] );
                    return EXIT_FAILURE;
                }
                break;

            case LanguageGenerator::OptOkValueConsumed:
                if ( optEqValue.isEmpty() )
                {
                    // Value consumed, and it was the next argument in the command line - skip it.

                    ++idx;
                }
        }
    }

    if ( parser.getInputFiles().isEmpty() )
    {
        fprintf ( stderr, "At least one input file is needed\n" );
        showHelp ( stderr, argv[ 0 ] );
        return EXIT_FAILURE;
    }

    if ( !parser.run ( protoSpec ) )
    {
        return EXIT_FAILURE;
    }

    try
    {
        protoSpec.checkGlobal();

        langGen->canRun();

        fprintf ( stdout, "\nGenerating output for '%s' language\n", genName.c_str() );

        langGen->run();
    }
    catch ( const Error & err )
    {
        fflush ( stdout );
        fprintf ( stderr, "Error: %s\n", err.getMessage().c_str() );
        return EXIT_FAILURE;
    }

    fprintf ( stdout, "\n Finished successfully!\n" );

    return EXIT_SUCCESS;
}
