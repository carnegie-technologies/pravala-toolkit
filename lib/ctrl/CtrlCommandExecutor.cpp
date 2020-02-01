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

#include "ctrl/ConfigCtrlSock.hpp"
#include "app/StdApp.hpp"
#include "log/ConfigLogs.hpp"

#include "CtrlCommandExecutor.hpp"

#include "auto/ctrl/Ctrl/SimpleSubscriptionResponse.hpp"

using namespace Pravala;

CtrlCommandExecutor::CtrlCommandExecutor ( bool createCmdLineOption ):
    _link ( *this, 0 ),
    _configOpt ( createCmdLineOption
            ? ( new ConfigCtrlSock ( "connect", 'c', "", "Remote address for the control command" ) ) : 0 ),
    _cmdObj ( 0 ),
    _flags ( 0 )
{
}

CtrlCommandExecutor::~CtrlCommandExecutor()
{
    delete _configOpt;
    _configOpt = 0;

    delete _cmdObj;
    _cmdObj = 0;
}

void CtrlCommandExecutor::registerCommand (
        const String & cmdName,
        const String & cmdHelp,
        CtrlCommandObject::GenerateCmdObjectFuncType cmdGenerator,
        const String & ctrlAddr )
{
    if ( cmdName.isEmpty() || cmdHelp.isEmpty() || !cmdGenerator )
        return;

    const String name = cmdName.toLower();

    _cmds[ name ].help = cmdHelp;
    _cmds[ name ].generateFunc = cmdGenerator;

    if ( _configOpt != 0 && _configOpt->isSet() && !_configOpt->value().isEmpty() )
    {
        _cmds[ name ].ctrlAddr = _configOpt->value();
    }
    else
    {
        _cmds[ name ].ctrlAddr = ctrlAddr;
    }
}

int CtrlCommandExecutor::initAndRun ( const StdApp & stdApp, ConfigLogs * logs, int timeout )
{
    return initAndRun ( stdApp.getExtraCmdLineArgs(), ( !logs ) ? stdApp.getLogs() : logs, timeout );
}

int CtrlCommandExecutor::initAndRun ( const StringList & cmd, ConfigLogs * logs, int timeout )
{
    if ( cmd.isEmpty() )
        return EXIT_FAILURE;

    ERRCODE eCode;

    if ( logs != 0 )
    {
        eCode = logs->init ( false );

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Could not initialize logging system: %s\n", eCode.toString() );
            return EXIT_FAILURE;
        }
    }

    eCode = EventManager::init();

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Could not initialize EventManager: %s\n", eCode.toString() );
        return EXIT_FAILURE;
    }

    return run ( cmd, timeout );
}

int CtrlCommandExecutor::run ( const StringList & cmd, int timeout )
{
    _cmd.clear();
    _args = cmd;

    if ( !_args.isEmpty() )
    {
        _cmd = _args.first().toLower();
        _args.removeFirst();
    }

    if ( _cmd.isEmpty() )
    {
        fprintf ( stderr, "No command provided...\n" );
        return EXIT_FAILURE;
    }

    if ( _cmd == "help" )
    {
        int maxLen = 0;
        StringList names;

        for ( HashMap<String, CtrlCommandObject::CommandEntry>::Iterator it ( _cmds ); it.isValid(); it.next() )
        {
            if ( it.value().generateFunc != 0 )
            {
                if ( it.key().length() > maxLen )
                {
                    maxLen = it.key().length();
                }

                names.append ( it.key() );
            }
        }

        names.sortAscending();

        printf ( "Available commands:\n" );

        for ( size_t i = 0; i < names.size(); ++i )
        {
            printf ( " - %-*s : %s\n", maxLen, names.at ( i ).c_str(), _cmds[ names.at ( i ) ].help.c_str() );
        }

        printf ( "\n" );

        return EXIT_SUCCESS;
    }

    if ( !_ctrlAddr.isEmpty() )
    {
        fprintf ( stderr, "This object has already been used (to connect to '%s').\n", _ctrlAddr.c_str() );

        return EXIT_FAILURE;
    }

    if ( !_cmds.contains ( _cmd ) )
    {
        fprintf ( stderr, "Unknown command '%s'; Use 'help' for help.\n", _cmd.c_str() );

        return EXIT_FAILURE;
    }

    _ctrlAddr = _cmds[ _cmd ].ctrlAddr;

    if ( _ctrlAddr.isEmpty() )
    {
        fprintf ( stderr, "No control address to connect to.\n" );
        return EXIT_FAILURE;
    }

    const ERRCODE eCode = _link.connect ( _ctrlAddr, -1, timeout * 1000 );

    if ( NOT_OK ( eCode ) )
    {
        fprintf ( stderr, "Error connecting to '%s': %s.\n", _ctrlAddr.c_str(), eCode.toString() );
        return EXIT_FAILURE;
    }

    // The ctrl link connection is in progress, and the ctrlLinkConnected callback will be generated later

    _flags = 0;

    EventManager::run();

    return getExitCode();
}

void CtrlCommandExecutor::ctrlLinkClosed ( int /*linkId*/ )
{
    if ( ( _flags & FlagConnected ) == FlagConnected )
    {
        printf ( "Control link to '%s' has been closed.\n", _ctrlAddr.c_str() );
    }
    else
    {
        fprintf ( stderr, "Could not connect to '%s'.\n", _ctrlAddr.c_str() );
    }

    stop();
}

void CtrlCommandExecutor::ctrlLinkConnectFailed ( CtrlLinkConnector * )
{
    fprintf ( stderr, "Could not connect to '%s'.\n", _ctrlAddr.c_str() );
    stop();
}

void CtrlCommandExecutor::ctrlLinkConnected ( int /*linkId*/ )
{
    _flags |= FlagConnected;

    fprintf ( stderr, "Connected to '%s'.\n", _ctrlAddr.c_str() );

    if ( _cmd.isEmpty() )
    {
        fprintf ( stderr, "No command provided...\n" );

        stop();
        return;
    }

    if ( !_cmds[ _cmd ].generateFunc )
    {
        fprintf ( stderr, "Unknown command '%s'; Use 'help' for help.\n", _cmd.c_str() );

        stop();
        return;
    }

    if ( _cmdObj != 0 )
    {
        fprintf ( stderr, "Internal error; This command has already been started...\n" );

        stop();
        return;
    }

    _cmdObj = _cmds[ _cmd ].generateFunc ( _link, _args );

    if ( !_cmdObj )
    {
        fprintf ( stderr, "Could not initialize '%s' command; Use 'help' for help.\n", _cmd.c_str() );

        stop();
        return;
    }

    if ( _cmdObj->getStatus() == CtrlCommandObject::CommandSucceeded )
    {
        _flags |= FlagSucceeded;
        stop();
    }
}

ERRCODE CtrlCommandExecutor::ctrlPacketReceived (
        int /*linkId*/, Ctrl::Message & msgData, List<int> & /* receivedFds */ )
{
    if ( !_cmdObj )
    {
        fprintf ( stderr, "Received a control packet but the command is not initialized.\n" );
        stop();

        // This is code for the control link, not the result of running the remote command.
        return Error::Success;
    }

    const CtrlCommandObject::CommandStatus cmdStatus = _cmdObj->processCtrlMessage ( msgData );

    switch ( cmdStatus )
    {
        case CtrlCommandObject::CommandSucceeded:
            _flags |= FlagSucceeded;
            stop();
            break;

        case CtrlCommandObject::CommandError:
            _flags &= ~FlagSucceeded;
            stop();
            break;

        case CtrlCommandObject::CommandContinues:
            break;
    }

    // This is code for the control link, not the result of running the remote command.
    return Error::Success;
}

ERRCODE CtrlCommandExecutor::ctrlSubscriptionResponseReceived (
        int /*linkId*/, Ctrl::SimpleSubscriptionResponse & msg, List<int> & /*receivedFds*/ )
{
    if ( !_cmdObj )
    {
        fprintf ( stderr, "Received a control packet but the command is not initialized.\n" );
        stop();

        // This is code for the control link, not the result of running the remote command.
        return Error::Success;
    }

    for ( size_t i = 0; i <= msg.getUpdates().size(); ++i )
    {
        CtrlCommandObject::CommandStatus cmdStatus;

        if ( i < msg.getUpdates().size() )
        {
            Ctrl::Message update ( msg.getUpdates().at ( i ) );

            cmdStatus = _cmdObj->processInitialSubUpdate ( msg.getRequestId(), msg.getRequestType(), update );
        }
        else
        {
            cmdStatus = _cmdObj->endOfInitialSubUpdates ( msg.getRequestId(), msg.getRequestType() );
        }

        if ( cmdStatus == CtrlCommandObject::CommandSucceeded )
        {
            _flags |= FlagSucceeded;
            stop();

            // No break, in case it can do more when it receives more updates!
        }
        else if ( cmdStatus == CtrlCommandObject::CommandError )
        {
            _flags &= ~FlagSucceeded;
            stop();

            // This is code for the control link, not the result of running the remote command.
            return Error::Success;
        }
    }

    // This is code for the control link, not the result of running the remote command.
    return Error::Success;
}
