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
#include "log/TextLog.hpp"

#include "../../WifiMgrTypes.hpp"

struct wpa_ctrl;

namespace Pravala
{
/// @brief The wpa_supplicant core logic; parses its output and handles the connect/disconnect procedure.
class WpaSuppCore: public NoCopy
{
    protected:
        static TextLog _log; ///< The log output stream

        struct wpa_ctrl * _conn; ///< The connection to the supplicant

        String _sockName; ///< The name of the supplicant socket connection (so we can reconnect if it goes away)

        /// @brief Constructor
        /// @param [in] ctrlName The name of the control socket for wpa supplicant
        WpaSuppCore ( const String & ctrlName );

        /// @brief Destructor
        virtual ~WpaSuppCore();

        /// @brief Synchronously send a command and wait for its response. This will connect if required, and may fail.
        /// @param [in] cmd The command to send.
        /// @param [out] result The output from the command; unparsed. If return is no Success, result isn't touched.
        /// @return standard error code
        ERRCODE executeCommand ( const String & cmd, String & result );

        /// @brief Retrieve the fd associated with this connection. Wrapper around wpa_ctrl_get_fd
        /// @return fd, or -1 if not connected
        int getFd() const;

        /// @brief Attempts to connect to the socket specified in the constructor.
        /// @return standard error code
        ERRCODE connect();

        /// @brief Parse the output from a Wi-Fi scan
        /// @param [in] buf The buffer read from the socket
        /// @param [out] results The parsed results
        /// @return standard error code
        static ERRCODE parseScanResults ( const String & buf, List<WifiMgrTypes::NetworkInstance> & results );

        /// @brief Parse the output from a 'list networks' command
        /// @param [in] buf The buffer read from the socket
        /// @param [out] results The parsed results
        /// @return standard error code
        static ERRCODE parseListNetworks ( const String & buf, List<WifiMgrTypes::NetworkConfiguration> & results );

        /// @brief Parse the output from a 'status' command
        /// @param [in] buf The buffer read from the socket
        /// @param [out] status The parsed results
        /// @return standard error code
        static ERRCODE parseStatus ( const String & buf, WifiMgrTypes::Status & status );

        /// @brief Parse the 'flags' field storing the network capabilities (i.e. [WPA-PSK-CCMP])
        /// @param [in] flags The flags
        /// @param [out] network The network we will be setting the flag fields on
        /// @return standard error code
        static ERRCODE parseNetworkFlags ( const String & flags, WifiMgrTypes::NetworkInstance & network );

        /// @brief Parse the 'flags' field storing the network status (i.e. [DISABLED])
        /// @param [in] flags The flags
        /// @param [out] network The network we will be setting the flag fields on
        /// @return standard error code
        static ERRCODE parseNetworkFlags ( const String & flags, WifiMgrTypes::NetworkConfiguration & network );

        /// @brief Convert a string to a cipher value
        /// @param [in] in The text representation
        /// @param [out] out The cipher
        /// @return true on success; false if no cipher match
        static bool strToCipher ( const String & in, WifiMgrTypes::Cipher & out );

        /// @brief Disconnect and clean up (remove fd from event manager if monitoring).
        virtual void reset();
};
}
