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

#include "ctrl/CtrlLinkCon.hpp"
#include "auto/log/Log/LogLevel.hpp"

namespace Pravala
{
/// @brief A remote log client
class RemoteLog: public CtrlLinkConnector::Owner
{
    public:
        /// @brief Contains data about a single log stream
        struct LogDesc
        {
            Log::LogLevel level; ///< Log level
            String name; ///< Log stream name
        };

        /// @brief Constructor
        /// @param [in] lDescs The list of log descriptions
        RemoteLog ( const List<LogDesc> & lDescs );

        /// @brief Starts the control link to be used by the RemoteLog.
        /// @param [in] sockAddr The address to connect to. If it "looks like" ip_addr:port,
        ///                       the IP link will be used. Otherwise this address is treated as a name/path
        ///                       for the local socket.
        /// @return Standard error code.
        ERRCODE startCtrlLink ( const String & sockAddr );

        virtual ERRCODE ctrlPacketReceived ( int linkId, Ctrl::Message & msg, List<int> & receivedFds );
        virtual void ctrlLinkClosed ( int linkId );
        virtual void ctrlLinkConnected ( int linkId );
        virtual void ctrlLinkConnectFailed ( CtrlLinkConnector * link );

    private:
        CtrlLinkConnector _ctrlLink; ///< The control link we use.
        List<LogDesc> _logDescs; ///< The logs we listen to
};
}
