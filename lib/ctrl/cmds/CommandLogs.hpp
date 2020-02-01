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

#include "ctrl/CtrlCommandObject.hpp"

namespace Pravala
{
class CtrlCommandExecutor;

/// @brief An object for running remote commands over the control channel.
class CommandLogs: public CtrlCommandObject
{
    public:
        /// @brief Constructor.
        /// @param [in] ctrlLink Control link to use.
        /// @param [in] args List of command arguments (without the name of the command).
        CommandLogs ( CtrlLink & ctrlLink, const StringList & args );

        /// @brief Registers this command object.
        /// @param [in] cmdExecutor The command executor to register this command with.
        /// @param [in] ctrlAddr The control address to connect to. This address is only used when the '-c/--connect'
        ///                      command line option is NOT set (or not created at all).
        /// @param [in] cmdName The name of the command.
        static void registerCommand (
            CtrlCommandExecutor & cmdExecutor,
            const String & ctrlAddr = String::EmptyString,
            const String & cmdName = "logs" );

    protected:
        virtual CommandStatus processInitialSubUpdate ( uint32_t reqId, uint32_t reqType, Ctrl::Message & msg );
        virtual CommandStatus endOfInitialSubUpdates ( uint32_t reqId, uint32_t reqType );
        virtual CommandStatus processCtrlMessage ( Ctrl::Message & msg );

    private:

        /// @brief Generates a command object for this command.
        /// @param [in] ctrlLink Control link to use.
        /// @param [in] args List of command arguments (without the name of the command).
        /// @return Pointer to the CommandObject, or 0 on error.
        static CtrlCommandObject * generate ( CtrlLink & ctrlLink, const StringList & args );
};
}
