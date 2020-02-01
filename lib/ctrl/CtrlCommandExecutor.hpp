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

#pragma once

#include "basic/NoCopy.hpp"
#include "CtrlLinkCon.hpp"
#include "CtrlCommandObject.hpp"

namespace Pravala
{
class StdApp;
class ConfigLogs;
class ConfigCtrlSock;

/// @brief An object for running remote commands over the control channel.
/// @note It should be created before StdApp::initConfig() if its built-in config option
/// ('-c' to specify the remote address) is to be registered properly.
class CtrlCommandExecutor: public NoCopy, protected CtrlLinkConnector::Owner
{
    public:
        /// @brief Constructor.
        /// @param [in] createCmdLineOption If true (default) a standard '-c/--connect' cmd line option will be created.
        CtrlCommandExecutor ( bool createCmdLineOption = true );

        /// @brief Destructor.
        virtual ~CtrlCommandExecutor();

        /// @brief Runs the command.
        /// @param [in] cmd List of command arguments (first being the name of the command).
        /// @param [in] timeout The connection timeout to use (in seconds).
        /// @return The exit code (can be returned from main() ).
        int run ( const StringList & cmd, int timeout = 30 );

        /// @brief Initializes the logs, EventManager and runs the command.
        /// @param [in] stdApp The StdApp to get the command from.
        /// @param [in] logs The ConfigLogs object to initialize.
        ///                   If not provided, ConfigLogs from the StdApp will be used.
        /// @param [in] timeout The connection timeout to use (in seconds).
        /// @return The exit code (can be returned from main() ).
        int initAndRun ( const StdApp & stdApp, ConfigLogs * logs = 0, int timeout = 30 );

        /// @brief Initializes the logs, EventManager and runs the command.
        /// @param [in] cmd List of command arguments (first being the name of the command).
        /// @param [in] logs The ConfigLogs object to initialize. Can be 0.
        /// @param [in] timeout The connection timeout to use (in seconds).
        /// @return The exit code (can be returned from main() ).
        int initAndRun ( const StringList & cmd, ConfigLogs * logs, int timeout = 30 );

        /// @brief Registers a command.
        /// @param [in] cmdName The name of the command. This will be changed to lower case.
        /// @param [in] cmdHelp The help text for the command.
        /// @param [in] cmdGenerator The generator of the command.
        /// @param [in] ctrlAddr The control address to connect to. This address is only used when the internal
        ///                      '-c/--connect' command line option is NOT set (or not created at all).
        void registerCommand (
            const String & cmdName,
            const String & cmdHelp,
            CtrlCommandObject::GenerateCmdObjectFuncType cmdGenerator,
            const String & ctrlAddr = String::EmptyString );

    protected:
        virtual void ctrlLinkClosed ( int linkId );
        virtual void ctrlLinkConnectFailed ( CtrlLinkConnector * link );
        virtual void ctrlLinkConnected ( int linkId );
        virtual ERRCODE ctrlPacketReceived ( int linkId, Ctrl::Message & msg, List< int > & receivedFds );
        virtual ERRCODE ctrlSubscriptionResponseReceived (
            int linkId, Ctrl::SimpleSubscriptionResponse & msg, List< int > & receivedFds );

    private:
        /// @brief Flag set when the command was successful (otherwise it failed).
        static const uint8_t FlagSucceeded = ( 1 << 0 );

        /// @brief Set when we open control connection.
        static const uint8_t FlagConnected = ( 1 << 1 );

        /// @brief Set when the command is finished.
        static const uint8_t FlagFinished = ( 1 << 2 );

        /// @brief Control link used by this command.
        CtrlLinkConnector _link;

        ConfigCtrlSock * _configOpt; ///< Configuration option for setting the remote address.

        /// @brief The list of arguments of the command (without the name of the command)
        StringList _args;

        /// @brief The lower case name of the command to use.
        String _cmd;

        /// @brief The control address we are connecting to.
        String _ctrlAddr;

        /// @brief All registered commands (name:command_entry).
        HashMap<String, CtrlCommandObject::CommandEntry> _cmds;

        /// @brief Command object used.
        CtrlCommandObject * _cmdObj;

        /// @brief Flag storage.
        uint8_t _flags;

        /// @brief Returns the exit code, based on the flags.
        /// @return The exit code based on flags.
        inline int getExitCode() const
        {
            return ( ( _flags & FlagSucceeded ) == FlagSucceeded ) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        /// @brief Stops the event manager and sets the 'finished' flag.
        inline void stop()
        {
            _flags |= FlagFinished;
            EventManager::stop();
        }
};
}
