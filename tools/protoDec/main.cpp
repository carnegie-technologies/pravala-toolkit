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

#include "basic/Buffer.hpp"
#include "app/StdApp.hpp"
#include "sys/File.hpp"

#include "config/ConfigSwitch.hpp"
#include "config/ConfigString.hpp"

#include "ProtoDec.hpp"

using namespace Pravala;

static ConfigSwitch swHexMode ( "hex", 'x', "Use the hex mode instead of detecting it automatically" );
static ConfigSwitch swBinMode ( "binary", 'b', "Use the binary mode instead of detecting it automatically" );
static ConfigSwitch swDecMode ( "decimal", 'd', "Use the decimal mode instead of detecting it automatically" );
static ConfigSwitch swLineMode ( "line-input", 'l', "Each line of input will be treated as a separate object" );

static ConfigString optOutput ( 0, "output", 'o', "", "Output file; If not specified, standard output is used" );

ERRCODE decodeBinaryBuf ( const MemHandle & buf, StringList & output )
{
    ProtoDec pDec ( buf );

    output.clear();

    return pDec.decode ( output );
}

ERRCODE decodeBuf ( const MemHandle & buf, StringList & output )
{
    output.clear();

    if ( swBinMode.isSet() )
    {
        return decodeBinaryBuf ( buf, output );
    }

    bool canBin = true;
    bool canHex = true;
    bool canDec = true;

    if ( swHexMode.isSet() )
    {
        canBin = false;
        canDec = false;
    }

    if ( swDecMode.isSet() )
    {
        canBin = false;
        canHex = false;
    }

    // Let's see what makes sense...
    for ( size_t i = 0; ( canHex || canDec ) && i < buf.size(); ++i )
    {
        const char c = buf.get ( i )[ 0 ];

        if ( ( c >= '0' && c <= '9' ) || c == ' ' || c == '\t' || c == '\r' || c == '\n' )
        {
            continue;
        }

        if ( c == '-' )
        {
            canHex = false;
            continue;
        }

        canDec = false;

        if ( ( c >= 'A' && c <= 'F' ) || ( c >= 'a' && c <= 'f' ) || c == 'x' || c == 'X' )
        {
            continue;
        }

        canHex = false;
        break;
    }

    Buffer decBuf;

    if ( canDec )
    {
        const StringList list = buf.toStringList ( " \t\r\n" );

        for ( size_t i = 0; i < list.size(); ++i )
        {
            int16_t value = 0;

            // It can be either signed 0:255, or unsigned -128:127
            if ( !list.at ( i ).toNumber ( value ) || value < -128 || value > 255 )
            {
                canDec = false;
                decBuf.clear();
                break;
            }

            const char c = ( value & 0xFF );

            decBuf.appendData ( &c, 1 );
        }
    }

    Buffer hexBuf;

    if ( canHex )
    {
        const StringList list = buf.toStringList ( " \t\r\n" );

        for ( size_t i = 0; canHex && i < list.size(); ++i )
        {
            String s = list.at ( i ).toLower();

            if ( s.startsWith ( "0x" ) )
            {
                // Remove '0x' prefix
                s = s.substr ( 2 );
            }

            if ( s.isEmpty() )
            {
                canHex = false;
                hexBuf.clear();
                break;
            }

            if ( s.length() % 2 != 0 )
            {
                // If it has odd number of letters, add '0' at the beginning
                s = String ( "0%1" ).arg ( s );
            }

            for ( int j = 0; j < s.length(); j += 2 )
            {
                uint8_t value;

                if ( !s.substr ( j, 2 ).toNumber ( value, 16 ) )
                {
                    canHex = false;
                    hexBuf.clear();
                    break;
                }

                hexBuf.appendData ( ( const char * ) &value, 1 );
            }
        }
    }

    if ( swHexMode.isSet() && !canHex )
    {
        fprintf ( stderr, "Hex mode selected, but the data is not in hex format.\n" );
        return Error::InvalidData;
    }

    if ( swDecMode.isSet() && !canDec )
    {
        fprintf ( stderr, "Decimal mode selected, but the data is not in decimal format.\n" );
        return Error::InvalidData;
    }

    if ( !canBin && !canHex && !canDec )
    {
        // We can't decode it at all!
        // !canBin will be set only if we have swHexMode or swDecMode set.

        fprintf ( stderr, "Could not decode data - it is not in %s format.\n",
                  swHexMode.isSet() ? "hex" : ( swDecMode.isSet() ? "decimal" : "unknown" ) );

        return Error::InvalidData;
    }

    // Maybe we have only one possible type at this point?

    if ( canBin && !canHex && !canDec )
    {
        return decodeBinaryBuf ( buf, output );
    }
    else if ( !canBin && canHex && !canDec )
    {
        return decodeBinaryBuf ( hexBuf, output );
    }
    else if ( !canBin && !canHex && canDec )
    {
        return decodeBinaryBuf ( decBuf, output );
    }

    // We have multiple potential types. Let's try them all.

    int sucCount = 0;

    StringList tmpOutput;

    if ( canBin && IS_OK ( decodeBinaryBuf ( buf, tmpOutput ) ) )
    {
        ++sucCount;
        output = tmpOutput;
    }

    if ( canDec && IS_OK ( decodeBinaryBuf ( decBuf, tmpOutput ) ) )
    {
        ++sucCount;
        output = tmpOutput;
    }

    if ( canHex && IS_OK ( decodeBinaryBuf ( hexBuf, tmpOutput ) ) )
    {
        ++sucCount;
        output = tmpOutput;
    }

    if ( sucCount > 1 )
    {
        fprintf ( stderr, "Data can be decoded properly using multiple different formats. "
                  "Please use one of the 'bdx' switches.\n" );

        output.clear();
        return Error::InvalidData;
    }
    else if ( sucCount < 1 )
    {
        fprintf ( stderr, "Data cannot be decoded properly using any of the formats. "
                  "Please use one of the 'bdx' switches.\n" );

        output.clear();
        return Error::InvalidData;
    }

    return Error::Success;
}

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs,
            "This program takes one additional, optional, argument: the name of the file to read the data from.\n"
            "If no file is provided, data will be read from the standard input.\n" );
    app.init();

    if ( app.getExtraCmdLineArgs().size() > 1 )
    {
        fprintf ( stderr, "At most one input file can be used\n" );
        return EXIT_FAILURE;
    }

    const String inFile = ( app.getExtraCmdLineArgs().size() == 1 ) ? ( app.getExtraCmdLineArgs().first() ) : "";

    int swCount = 0;

    if ( swHexMode.isSet() ) ++swCount;
    if ( swBinMode.isSet() ) ++swCount;
    if ( swDecMode.isSet() ) ++swCount;

    if ( swCount > 1 )
    {
        fprintf ( stderr, "Only one of 'xbd' options can be used at a time.\n" );
        return EXIT_FAILURE;
    }

    if ( swBinMode.isSet() && swLineMode.isSet() )
    {
        fprintf ( stderr, "Line mode does not make sense with binary mode.\n" );
        return EXIT_FAILURE;
    }

    Buffer buf;
    FILE * outFile = 0;

    int inputI = EOF;

    ERRCODE eCode = Error::Success;

    do
    {
        MemHandle data;

        if ( !inFile.isEmpty() )
        {
            eCode = File::read ( inFile, data );

            if ( NOT_OK ( eCode ) )
            {
                fprintf ( stderr, "Error reading from file '%s': %s\n", inFile.c_str(), eCode.toString() );
                return EXIT_FAILURE;
            }

            if ( data.isEmpty() )
            {
                fprintf ( stderr, "File '%s' is empty\n", inFile.c_str() );
                return EXIT_FAILURE;
            }
        }
        else
        {
            inputI = fgetc ( stdin );

            if ( inputI != EOF && ( !swLineMode.isSet() || inputI != '\n' ) )
            {
                // Not EOF and not EOL (if we are running in line mode)

                const char c = ( char ) inputI;
                buf.appendData ( &c, 1 );
                continue;
            }

            data = buf;
            buf.clear();
        }

        // Decoding time!

        StringList output;

        eCode = decodeBuf ( data, output );

        if ( swLineMode.isSet() && NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Error processing input: %s\n", eCode.toString() );

            // We don't quit on error in line mode.
            eCode = Error::Success;
        }

        if ( !output.isEmpty() )
        {
            if ( !outFile && optOutput.isNonEmpty() )
            {
                outFile = fopen ( optOutput.value().c_str(), "w" );

                if ( !outFile )
                {
                    fprintf ( stderr, "Error opening output file '%s' for writing: %s\n",
                              optOutput.value().c_str(), strerror ( errno ) );
                    return EXIT_FAILURE;
                }
            }

            for ( size_t i = 0; i < output.size(); ++i )
            {
                fprintf ( ( outFile != 0 ) ? outFile : stdout, "%s\n", output.at ( i ).c_str() );
            }
        }
    }
    while ( IS_OK ( eCode ) && inputI != EOF );

    if ( outFile != 0 )
    {
        fclose ( outFile );
        outFile = 0;
    }

    if ( NOT_OK ( eCode ) )
    {
        if ( inFile.isEmpty() )
        {
            fprintf ( stderr, "Error processing input: %s\n", eCode.toString() );
        }
        else
        {
            fprintf ( stderr, "Error processing input file '%s': %s\n", inFile.c_str(), eCode.toString() );
        }

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
