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

#include "NetlinkCore.hpp"

namespace Pravala
{
/// @brief A Netlink socket wrapper for synchronous operations.
class NetlinkSyncSocket: public NetlinkCore
{
    public:
        /// @brief Creates a new NetlinkSyncSocket object
        /// @param [in] family The netlink family to use.
        NetlinkSyncSocket ( NetlinkFamily family );

        /// @brief Destructor
        ~NetlinkSyncSocket();

    protected:

        /// @brief 'Executes' a NetlinkMessage.
        /// It sends the message over the netlink socket and reads the response.
        /// This function will keep reading until all parts of a multi-part message are received.
        /// It will also keep retrying to 'execute' the message if EBUSY is received in response.
        /// @param [in] msg The netlink message to 'execute'
        /// @param [out] response The list of message parts received in response. It is cleared first.
        ///                        In case multi-part message is received, all parts will be received
        ///                        before this method returns.
        /// @param [out] netlinkError Set to the error message received from Netlink.
        ///                            Only set if this function returns ErrorResponse code.
        /// @return Standard error code.
        ///          Success       - The request was executed properly and Netlink didn't return any errors
        ///                           or returned 'success' error code.
        ///          ErrorResponse - The request was executed properly, but Netlink responded with an error message.
        ///                           That message is stored in netlinkError.
        ///          NotAvailable  - The request failed due to 'EBUSY' Netlink error and was retried,
        ///                           but the max number of retry attempts has been reached.
        ///
        ///          Also any of the codes returned by NetlinkCore::writeMessage() and NetlinkCore::readMessage().
        ERRCODE execMessage ( NetlinkMessage & msg, List<NetlinkMessage> & response, struct nlmsgerr & netlinkError );

        /// @brief 'Executes' a NetlinkMessage.
        /// This is a helper wrapper around the other execMessage() method.
        /// It doesn't return the response, just the result of the operation and, optionally, netlink error code.
        /// @param [in] msg The netlink message to 'execute'
        /// @param [out] errorCode If used, the error code received from Netlink will be stored there
        ///                         (even if it's 0 = no error)
        /// @return Standard error code. It returns the same error codes as the other execMessage().
        ERRCODE execMessage ( NetlinkMessage & msg, int * errorCode );
};
}
