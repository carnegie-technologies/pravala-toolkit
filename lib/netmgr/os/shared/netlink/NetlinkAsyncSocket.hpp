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

#include "event/EventManager.hpp"
#include "NetlinkCore.hpp"

namespace Pravala
{
/// @brief A Netlink socket wrapper for asynchronous operations.
class NetlinkAsyncSocket: public NetlinkCore, public EventManager::FdEventHandler
{
    public:
        /// @brief Creates a new Socket object with a Netlink socket fd set with parameters
        /// @param [in] family Netlink family to use
        /// @param [in] mcastGroups ORed Bitmask of multicast groups to listen to (default 0 = none)
        ///                         See linux/rtnetlink.h RTMGRP_*
        NetlinkAsyncSocket ( NetlinkFamily family, uint32_t mcastGroups );

        /// @brief Destructor
        ~NetlinkAsyncSocket();

        /// @brief Removes all requests from the write queue.
        void clearRequestQueue();

    protected:
        virtual void receiveFdEvent ( int fd, short events );
        virtual void netlinkSockReinitialized();

        /// @brief Sets the message sequence number
        /// It sets the message sequence number
        /// @param [in] msg  The message to be configured
        /// @return Sequence number used
        uint32_t setSeqNum ( NetlinkMessage & msg );

        /// @brief Adds a netlink message to the outgoing queue to be sent asynchronously.
        /// It will call setSeqNum() - nlmsg_seq field will be overwritten.
        /// @param [in] msg The message to send.
        /// @return Sequence number used for the request (or 0 if the message is incorrect)
        uint32_t sendMessage ( NetlinkMessage & msg );

        /// @brief Called when Netlink data is received.
        /// @warning This can be called multiple times in a loop,
        ///           so the socket should NOT be destroyed inside this callback!
        /// @note If there is a request in flight, the messages may be delivered in a different order than
        ///        they were actually read from the socket. All asynchronous messages will be delivered
        ///        after the response to the request, even if they were actually read before the response
        ///        (or during, if the response is a multipart message).
        /// @param [in] messages The list of message parts received. It can be modified inside this callback.
        virtual void netlinkReceived ( List<NetlinkMessage> & messages ) = 0;

        /// @brief Called when a Netlink request fails and is dropped - either due to error while sending or receiving.
        /// @param [in] reqSeqNum The sequence number of the request that failed.
        /// @param [in] errorCode The code of the error.
        virtual void netlinkReqFailed ( uint32_t reqSeqNum, ERRCODE errorCode ) = 0;

        /// @brief Called when there is an error on a socket that is configured to receive multicast updates.
        /// This is called to notify that this socket failed (and has been reinitialized),
        /// but there may be some updates that were lost. If the owner cares about the total state of the system,
        // then it should perform a complete refresh.
        virtual void netlinkMCastSocketFailed() = 0;

    private:
        List<NetlinkMessage> _writeQueue; ///< Write queue
        List<NetlinkMessage> _readRegParts; ///< Regular message parts read.
        List<NetlinkMessage> _readMultiParts; ///< Multipart message parts read.

        /// @brief The last sequence number used on outgoing messages
        /// 0 is reserved
        /// We don't expect to have more than 2^32-1 messages in flight, so this should be fine =)
        uint32_t _lastSeqNum;

        /// @brief The number of write attempts we performed while getting 'EBUSY' error.
        uint32_t _busyRetries;
};
}
