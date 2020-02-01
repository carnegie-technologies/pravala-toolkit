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

#include "CtrlLink.hpp"

#define CMD_CASE_CTRL_MSG( msg_type )  \
    case Ctrl::msg_type::DEF_TYPE: \
        { \
            ExtProtoError eErr; Ctrl::msg_type tmpMsg; ERRCODE tmpErrCode = tmpMsg.deserialize ( msg, &eErr ); \
            if ( NOT_OK ( tmpErrCode ) ) { \
                fprintf ( stderr, "Error deserializing Ctrl::" #msg_type " update: %s (%s)\n", \
                          tmpErrCode.toString(), eErr.toString().c_str() ); \
                return CommandError; } \
            processMessage ( tmpMsg ); } \
                                       break

namespace Pravala
{
/// @brief An object that runs a single remote command.
class CtrlCommandObject
{
    public:
        typedef CtrlCommandObject * (* GenerateCmdObjectFuncType) ( CtrlLink &, const StringList & args );

        /// @brief A structure for storing information about commands.
        struct CommandEntry
        {
            GenerateCmdObjectFuncType generateFunc; ///< Function used to generate the command.
            String help; ///< The help text.
            String ctrlAddr; ///< The control address to connect to.

            /// @brief Default constructor.
            CommandEntry(): generateFunc ( 0 )
            {
            }
        };

        /// @brief The status of the command.
        enum CommandStatus
        {
            CommandError = 0, ///< There was an error
            CommandContinues, ///< There was no error but the command is not done yet.
            CommandSucceeded ///< The command is done and it succeeded.
        };

        /// @brief Constructor.
        /// @param [in] ctrlLink Control link to use.
        /// @param [in] args List of command arguments (without the name of the command).
        CtrlCommandObject ( CtrlLink & ctrlLink, const StringList & args );

        /// @brief Destructor.
        virtual ~CtrlCommandObject();

        /// @brief Processes one of the updates included in the subscription response.
        /// @param [in] reqId The ID of the subscription request that caused this update.
        /// @param [in] reqType The type of the subscription request that caused this update.
        /// @param [in] msg The update received.
        /// @return The status of the command.
        virtual CommandStatus processInitialSubUpdate ( uint32_t reqId, uint32_t reqType, Ctrl::Message & msg ) = 0;

        /// @brief Called after all initial sub updates has been processed (by calling processInitialSubUpdate()).
        /// @param [in] reqId The ID of the subscription request that caused this update.
        /// @param [in] reqType The type of the subscription request that caused this update.
        virtual CommandStatus endOfInitialSubUpdates ( uint32_t reqId, uint32_t reqType ) = 0;

        /// @brief Processes a message from the control link.
        /// @param [in] msg The message received.
        /// @return The status of the command.
        virtual CommandStatus processCtrlMessage ( Ctrl::Message & msg ) = 0;

        /// @brief Returns the status of the command.
        /// @return The status of the command.
        inline CommandStatus getStatus() const
        {
            return _cmdStatus;
        }

    protected:
        CtrlLink & _link; ///< Reference to the link used.
        const StringList & _args; ///< The list of arguments of the command (without the name of the command).
        CommandStatus _cmdStatus; ///< The status of the command.

        /// @brief A helper function that sets the new command status and returns it.
        /// This way it is easy to modify the status and return it at the same time.
        /// @param [in] cmdStatus The new status to set.
        /// @return The command status value set.
        inline CommandStatus returnStatus ( CommandStatus cmdStatus )
        {
            _cmdStatus = cmdStatus;
            return cmdStatus;
        }
};
}
