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

#include "ctrl/CtrlCommandExecutor.hpp"

#include "CommandLogs.hpp"

#include "auto/ctrl/Ctrl/GetLogList.hpp"
#include "auto/ctrl/Ctrl/LogListResp.hpp"

using namespace Pravala;

CommandLogs::CommandLogs ( CtrlLink & ctrlLink, const StringList & args ): CtrlCommandObject ( ctrlLink, args )
{
}

void CommandLogs::registerCommand ( CtrlCommandExecutor & cmdExecutor, const String & ctrlAddr, const String & cmdName )
{
    cmdExecutor.registerCommand ( cmdName, "Displays the list of logs", generate, ctrlAddr );
}

CtrlCommandObject * CommandLogs::generate ( CtrlLink & ctrlLink, const StringList & args )
{
    if ( !args.isEmpty() )
    {
        fprintf ( stderr, "This command does not accept any arguments.\n" );
        return 0;
    }

    Ctrl::GetLogList msg;

    ERRCODE eCode = ctrlLink.sendPacket ( msg );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error sending Ctrl::GetLogList message: %s.\n", eCode.toString() );
        return 0;
    }

    return new CommandLogs ( ctrlLink, args );
}

CtrlCommandObject::CommandStatus CommandLogs::processCtrlMessage ( Ctrl::Message & msgData )
{
    if ( msgData.getType() != Ctrl::LogListResp::DEF_TYPE )
    {
        fprintf ( stderr, "Received unexpected message (type %d).\n", msgData.getType() );

        return returnStatus ( CommandError );
    }

    Ctrl::LogListResp msg;

    ERRCODE eCode = msg.deserialize ( msgData );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error deserializing Ctrl::LogListResp message: %s\n", eCode.toString() );

        return returnStatus ( CommandError );
    }

    printf ( "\nAvailable text logs: \n\n" );

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

    printf ( "\n" );

    return returnStatus ( CommandSucceeded );
}

CtrlCommandObject::CommandStatus CommandLogs::processInitialSubUpdate (
        uint32_t /*reqId*/, uint32_t /*reqType*/, Ctrl::Message & msg )
{
    fprintf ( stderr, "Received unexpected message (type %d).\n", msg.getType() );

    return returnStatus ( CommandError );
}

CtrlCommandObject::CommandStatus CommandLogs::endOfInitialSubUpdates ( uint32_t /*reqId*/, uint32_t /*reqType*/ )
{
    return returnStatus ( CommandError );
}
