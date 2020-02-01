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
#include <ctime>

#include "config/ConfigSwitch.hpp"
#include "sys/CalendarTime.hpp"

#include "RemoteLog.hpp"

#include "auto/ctrl/Ctrl/GetLogList.hpp"
#include "auto/ctrl/Ctrl/LogListResp.hpp"
#include "auto/ctrl/Ctrl/LogSubscribe.hpp"
#include "auto/ctrl/Ctrl/LogMessage.hpp"

using namespace Pravala;

static ConfigSwitch swTimeInSeconds (
        "time-in-seconds",
        'S',
        "Use seconds as a base for timestamps in log messages. "
        "Needed to properly interpret messages from old control servers." );

RemoteLog::RemoteLog ( const List<LogDesc> & lDescs ):
    _ctrlLink ( *this, 0 ),
    _logDescs ( lDescs )
{
}

ERRCODE RemoteLog::startCtrlLink ( const String & sockAddr )
{
    const ERRCODE eCode = _ctrlLink.connect ( sockAddr, 1000, 5000 );

    // Either 'Success' or 'ConnectInProgress':
    if ( IS_OK ( eCode ) )
    {
        printf ( "Starting control link succeeded; Code: %s\n", eCode.toString() );

        // The ctrl link connection is in progress, and the ctrlLinkConnected callback will be generated later
    }

    return eCode;
}

ERRCODE RemoteLog::ctrlPacketReceived ( int /* linkId */, Ctrl::Message & msgData, List<int> & /* receivedFds */ )
{
    // printf ( "Received packet; LinkId: %d; MessageType: %s; NumFds: %d\n",
    //       linkId, msgData.getType().toString(), ( int ) receivedFds.size() );

    ERRCODE eCode = Error::Unknown;

    if ( msgData.getType() == Ctrl::LogListResp::DEF_TYPE )
    {
        Ctrl::LogListResp msg;

        eCode = msg.deserialize ( msgData );

        if ( IS_OK ( eCode ) )
        {
            printf ( "LogListResp received; Available text logs: \n\n" );

            for ( int i = 0; i < ( int ) msg.getTextLogs().size(); ++i )
            {
                printf ( "  %d: %s\n", i + 1, msg.getTextLogs().at ( i ).c_str() );
            }

            printf ( "\nAvailable binary logs: \n\n" );

            for ( int i = 0; i < ( int ) msg.getBinLogs().size(); ++i )
            {
                printf ( "%d: %s [%s]\n", i + 1,
                         msg.getBinLogs().at ( i ).getName().c_str(),
                         msg.getBinLogs().at ( i ).getType().toString() );
            }

            printf ( "\nEnd of log list\n" );
        }
        else
        {
            printf ( "Error deserializing Ctrl::LogListResp message: %s\n", eCode.toString() );
        }
    }
    else if ( msgData.getType() == Ctrl::LogMessage::DEF_TYPE )
    {
        Ctrl::LogMessage msg;

        eCode = msg.deserialize ( msgData );

        if ( IS_OK ( eCode ) )
        {
            String timeDesc ( "no-time" );

            if ( msg.getLogMessage().hasTime() )
            {
                uint64_t msgTime = msg.getLogMessage().getTime();

                if ( swTimeInSeconds.isSet() )
                {
                    // We are expecting timestamp to be in seconds instead of milliseconds.
                    // So we need to scale it up!

                    msgTime *= 1000;
                }

                timeDesc = CalendarTime::getTimeDesc ( msgTime );
            }

            if ( msg.getLogMessage().getType() == Log::LogType::TextMessage )
            {
                Log::TextMessage textMsg;

                eCode = textMsg.deserialize ( msg.getLogMessage() );

                if ( IS_OK ( eCode ) )
                {
                    String str;

                    TextLogOutput::formatMessage ( textMsg, timeDesc, str );

                    printf ( "LOG: %s", str.c_str() );

                    if ( !str.endsWith ( "\n" ) )
                        printf ( "\n" );
                }
                else
                {
                    printf ( "Error deserializing Log::TextMessage message: %s\n", eCode.toString() );
                }
            }
            else
            {
                printf ( "BLOG: %s: %s [%s]\n", timeDesc.c_str(),
                         msg.getLogMessage().getName().c_str(),
                         msg.getLogMessage().getType().toString() );
            }
        }
        else
        {
            printf ( "Error deserializing Ctrl::LogMessage message: %s\n", eCode.toString() );
        }
    }
    else if ( msgData.getIsUpdate() )
    {
        Ctrl::Update msg;

        eCode = msg.deserialize ( msgData );

        if ( IS_OK ( eCode ) )
        {
            if ( msg.hasCode() )
            {
                printf ( "Update received; RequestType: %d; ErrorCode: %s\n",
                         msg.getRequestType(), msg.getCode().toString() );
            }
            else
            {
                printf ( "Update received; RequestType: %d\n",
                         msg.getRequestType() );
            }
        }
        else
        {
            printf ( "Error deserializing Ctrl::Update message: %s\n", eCode.toString() );
        }
    }

    return Error::Success;
}

void RemoteLog::ctrlLinkClosed ( int linkId )
{
    printf ( "Link with ID %d was closed\n", linkId );
}

void RemoteLog::ctrlLinkConnectFailed ( CtrlLinkConnector * link )
{
    assert ( link != 0 );

    printf ( "Link with ID %d failed to connect...\n", link->LinkId );
}

void RemoteLog::ctrlLinkConnected ( int linkId )
{
    printf ( "Link with ID %d connected; Sending 1 GetLogList and %d LogSubscribe message(s)\n",
             linkId, ( int ) _logDescs.size() );

    Ctrl::GetLogList listMsg;

    ERRCODE eCode = _ctrlLink.sendPacket ( listMsg );

    printf ( "Sending GetLogList setup message: %s\n", eCode.toString() );

    Ctrl::LogSubscribe subMsg;

    for ( size_t i = 0; i < _logDescs.size(); ++i )
    {
        subMsg.setLevel ( _logDescs.at ( i ).level );
        subMsg.setNamePattern ( _logDescs.at ( i ).name );

        eCode = _ctrlLink.sendPacket ( subMsg );

        printf ( "Sending LogSubscribe setup message: %s\n", eCode.toString() );
    }
}
