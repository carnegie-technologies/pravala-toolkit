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

#include <cstdlib>

#include "NetUtils.hpp"
#include "basic/MemHandle.hpp"

// Entries in /proc/net/[udp,tcp,udp6,tcp6]:
// sl: local_address:local_port rem_address:rem_port st tx_queue:rx_queue tr:tm->when retrnsmt uid ...
//   0                        1                    2  3                 4           5        6   7
#define FIELD_BEG          0
#define FIELD_LOC_ADDR     1
#define FIELD_REM_ADDR     2
#define FIELD_UID          7

#define Q_FILE             "/proc/net/wireless"
#define Q_FIELD_BEG        0
#define Q_FIELD_NAME       0
#define Q_FIELD_QUALITY    3

#define P_FILE             "/proc/net/dev"
#define P_FIELD_BEG        0
#define P_FIELD_NAME       0
#define P_FIELD_COUNT      2

using namespace Pravala;

List<NetUtils::FlowDesc> NetUtils::readFlows ( NetUtils::FlowType flowType )
{
    List<FlowDesc> list;

    const char * fileName;

    switch ( flowType )
    {
        case FlowUDP4:
            fileName = "/proc/net/udp";
            break;

        case FlowUDP6:
            fileName = "/proc/net/udp6";
            break;

        case FlowTCP4:
            fileName = "/proc/net/tcp";
            break;

        case FlowTCP6:
            fileName = "/proc/net/tcp6";
            break;

        default:
            return list;
            break;
    }

    MemHandle buf ( fileName );

    const size_t size = buf.size();

    if ( size < 1 )
        return list;

    const char * mem = buf.get();

    int field = FIELD_BEG;
    bool isWhitespace = true;
    size_t lastUnusable = 0;

    FlowDesc fDesc;

    // We say there were conversion errors at the beginning
    // The first line contains the header, which we don't need to try to convert
    // (it would fail anyway, but it saves us one conversion attempt)

    bool convErrors = true;

    for ( size_t i = 0; i < size; ++i )
    {
        switch ( mem[ i ] )
        {
            case '\n':
                // EOL - let's start the next line

                isWhitespace = true;
                field = FIELD_BEG;
                lastUnusable = i;

                if ( !convErrors
                     && ( list.isEmpty() || fDesc.isDifferent ( list.last() ) ) )
                {
                    list.append ( fDesc );
                }

                // Next, fresh line - no errors!
                convErrors = false;
                break;

            case ' ':
            case '\t':
                if ( !isWhitespace )
                {
                    // End of a field
                    isWhitespace = true;

                    if ( !convErrors )
                    {
                        // If there were errors in this line there is no point in trying to convert anything else!

                        char * endPtr = 0;

                        assert ( lastUnusable < i );

                        if ( field == FIELD_UID )
                        {
                            fDesc.uid = ( uint32_t ) strtoul ( mem + lastUnusable + 1, &endPtr, 10 );
                        }
                        else if ( field == FIELD_LOC_ADDR )
                        {
                            fDesc.localPort = ( uint16_t ) strtoul ( mem + lastUnusable + 1, &endPtr, 16 );
                        }
                        else if ( field == FIELD_REM_ADDR )
                        {
                            fDesc.remotePort = ( uint16_t ) strtoul ( mem + lastUnusable + 1, &endPtr, 16 );
                        }

                        if ( endPtr != 0 && endPtr != mem + i )
                        {
                            convErrors = true;
                        }
                    }

                    // Now we are in the next field
                    ++field;
                }

                // It's a whitespace, so it counts as 'unusable'
                lastUnusable = i;
                break;

            case ':':
                // ':' is not usable, but it's not a whitespace
                isWhitespace = false;
                lastUnusable = i;
                break;

            default:
                // Anything else is not a whitespace
                isWhitespace = false;
                break;
        }
    }

    return list;
}

long unsigned int NetUtils::readPacketCount ( const String & ifaceName )
{
    MemHandle buf ( P_FILE );

    const size_t size = buf.size();

    if ( size < 1 )
        return 0;

    const char * mem = buf.get();

    int field = P_FIELD_BEG;
    bool isWhitespace = true;
    size_t lastUnusable = 0;

    bool useLine = false;

    for ( size_t i = 0; i < size; ++i )
    {
        switch ( mem[ i ] )
        {
            case '\n':
                // EOL - let's start the next line

                isWhitespace = true;
                field = P_FIELD_BEG;
                useLine = false;
                lastUnusable = i;
                break;

            case ' ':
            case '\t':
            case ':':
            case '.':
            case '|':
                if ( !isWhitespace )
                {
                    // End of a field
                    isWhitespace = true;
                    char * endPtr = 0;

                    assert ( lastUnusable + 1 < i );

                    if ( field == P_FIELD_NAME )
                    {
                        if ( ifaceName.length() == ( int ) ( i - lastUnusable - 1 )
                             && strncmp ( ifaceName.c_str(), mem + lastUnusable + 1, ifaceName.length() ) == 0 )
                        {
                            useLine = true;
                        }
                    }

                    if ( useLine && field == P_FIELD_COUNT )
                    {
                        long unsigned int value = strtoul ( mem + lastUnusable + 1, &endPtr, 10 );

                        if ( endPtr != 0 && endPtr == mem + i )
                        {
                            return value;
                        }
                    }

                    // Now we are in the next field
                    ++field;
                }

                // It's a whitespace, so it counts as 'unusable'
                lastUnusable = i;
                break;

            default:
                // Anything else is not a whitespace
                isWhitespace = false;
                break;
        }
    }

    return 0;
}

uint16_t NetUtils::readWirelessQuality ( const String & ifaceName )
{
    MemHandle buf ( Q_FILE );

    const size_t size = buf.size();

    if ( size < 1 )
        return 0;

    const char * mem = buf.get();

    int field = Q_FIELD_BEG;
    bool isWhitespace = true;
    size_t lastUnusable = 0;

    bool useLine = false;

    for ( size_t i = 0; i < size; ++i )
    {
        switch ( mem[ i ] )
        {
            case '\n':
                // EOL - let's start the next line

                isWhitespace = true;
                field = Q_FIELD_BEG;
                useLine = false;
                lastUnusable = i;
                break;

            case ' ':
            case '\t':
            case ':':
            case '.':
            case '|':
                if ( !isWhitespace )
                {
                    // End of a field
                    isWhitespace = true;
                    char * endPtr = 0;

                    assert ( lastUnusable + 1 < i );

                    if ( field == Q_FIELD_NAME )
                    {
                        if ( ifaceName.length() == ( int ) ( i - lastUnusable - 1 )
                             && strncmp ( ifaceName.c_str(), mem + lastUnusable + 1, ifaceName.length() ) == 0 )
                        {
                            useLine = true;
                        }
                    }

                    if ( useLine && field == Q_FIELD_QUALITY )
                    {
                        uint16_t value = ( uint16_t ) strtoul ( mem + lastUnusable + 1, &endPtr, 10 );

                        if ( endPtr != 0 && endPtr == mem + i )
                        {
                            return value;
                        }
                    }

                    // Now we are in the next field
                    ++field;
                }

                // It's a whitespace, so it counts as 'unusable'
                lastUnusable = i;
                break;

            default:
                // Anything else is not a whitespace
                isWhitespace = false;
                break;
        }
    }

    return 0;
}
