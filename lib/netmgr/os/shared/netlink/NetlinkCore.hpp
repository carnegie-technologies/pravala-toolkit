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

extern "C"
{
#include <net/if.h>
#include <linux/netlink.h>
#include <sys/socket.h>
}

#include "basic/IpAddress.hpp"
#include "error/Error.hpp"
#include "config/ConfigNumber.hpp"
#include "log/TextLog.hpp"

#include "NetlinkTypes.hpp"
#include "NetlinkMessage.hpp"

namespace Pravala
{
/// @brief The core functionality of Netlink-related classes
class NetlinkCore
{
    public:
        /// @brief The max number of times we want to try requests
        /// that resulted in EBUSY error in NLMSG_ERROR response.
        static ConfigNumber<uint16_t> optMaxRequestBusyTries;

        /// @brief The max number of times we want to try requests
        /// that resulted in a socket error while receiving the reply.
        static ConfigNumber<uint16_t> optMaxRequestRespErrorTries;

        /// @brief Netlink message family
        /// See: linux/netlink.h for constants
        enum NetlinkFamily
        {
            Unknown = -1, ///< Unknown (invalid) family
            Route = NETLINK_ROUTE ///< NETLINK_ROUTE family

                    // Firewall = NETLINK_FIREWALL,
                    // InetDiag = NETLINK_INET_DIAG,
                    // ForwardTable = NETLINK_FIB_LOOKUP,
                    // Netfilter = NETLINK_NETFILTER,
                    // Firewall6 = NETLINK_IP6_FW,
                    // UserSock = NETLINK_USERSOCK,
                    // Generic = NETLINK_GENERIC
        };

        /// @brief Return the length of the payload in Netlink message.
        /// @param [in] nlmsghdr pointer to nlmsghdr
        /// @param [in] amhdrLen length of the ancillary message header
        /// @return Total length of the rtattr payloads
        static inline ssize_t getMsgPayloadLength ( const struct nlmsghdr * nlh, int amhdrLen )
        {
            assert ( nlh != 0 );

            return NLMSG_PAYLOAD ( nlh, amhdrLen );
        }

    protected:
        static TextLog _log; ///< Log stream

        /// @brief Family
        const NetlinkFamily _family;

        /// ORed Bitmask of multicast groups to listen to. See linux/rtnetlink.h RTMGRP_*
        const uint32_t _mcastGroups;

        int _sock; ///< Socket file descriptor.
        uint32_t _sockPID; ///< Netlink's Port ID of the socket.

        int _sndBufSize; ///< Desired socket's send buffer. 0 - unknown.
        int _rcvBufSize; ///< Desired socket's receive buffer. 0 - unknown.

        /// @brief Creates a new Socket object with a Netlink socket fd set with parameters.
        /// It also initializes the socket, but netlinkSockReinitialized() in inheriting class will not be called.
        /// This is because when this constructor runs, netlinkSockReinitialized is not configured to be run
        /// in the inheriting class yet.
        /// @param [in] family Netlink family to use
        /// @param [in] mcastGroups ORed Bitmask of multicast groups to listen to (default 0 = none)
        ///                         See linux/rtnetlink.h RTMGRP_*
        NetlinkCore ( NetlinkFamily family, uint32_t mcastGroups = 0 );

        /// @brief Destructor
        ~NetlinkCore();

        /// @brief Initializes the netlink socket.
        /// If a valid socket already exists, it's closed first
        /// (and the FD event subscriber, if it is set, is unsubscribed).
        /// This function calls netlinkSockReinitialized() callback on success.
        /// @param [in] sndSizeIncrease The number of bytes new socket's send buffer should be increased
        ///                              beyond the current or desired size (whichever is greater).
        /// @param [in] rcvSizeIncrease The number of bytes new socket's receive buffer should be increased
        ///                              beyond the current or desired size (whichever is greater).
        /// @return True if the socket has been (re)-initialized properly; False otherwise.
        bool reinitializeSocket ( int sndSizeIncrease = 0, int rcvSizeIncrease = 0 );

        /// @brief Called by reinitializeSocket() after the socket has been (re)initialized properly.
        /// Default implementation does nothing.
        virtual void netlinkSockReinitialized();

        /// @brief Writes data to the socket.
        /// @param [in] msg The message to write.
        /// @return Standard error code. Possible codes:
        ///          Success, NotInitialized, InvalidParameter,
        ///          TooMuchData, SoftFail, WriteFailed, IncompleteWrite
        ///
        ///          Success             - The data has been written properly. Note that this status is returned on
        ///                                 success, and NOT the number of bytes written to the socket
        ///                                (like standard write/send functions do).
        ///          NotInitialized      - Netlink connection is not established.
        ///          InvalidParameter    - The Netlink message is invalid and was not sent.
        ///          TooMuchData         - There was an ENOBUFS error while writing.
        ///          SoftFail            - Writing failed with EAGAIN error and can be retried later.
        ///                                 This code is also used when the sending fails with ENOBUFS error
        ///                                 and the socket's buffer is increased - this is possible even in blocking
        ///                                 mode and means that the write should be retried.
        ///          WriteFailed         - Writing to the Netlink socket failed (the socket has been re-initialized).
        ///          IncompleteWrite     - Incomplete data has been written (the socket has been re-initialized).
        ERRCODE writeMessage ( const NetlinkMessage & msg );

        /// @brief Reads data from a netlink socket and appends the result to the list of message parts.
        /// @note If the data read contains several message parts it is split and added as multiple message parts.
        /// @note This function separates multipart and regular messages, but doesn't care whether all parts
        ///        of a multipart message have been received or not.
        /// @param [in,out] multipartMessages The list to append multipart message parts. Those are either messages
        ///                                    that have the 'multipart' flag set, or have NLMSG_DONE type.
        /// @param [in,out] messages The list to append non-multipart messages read from the socket to.
        /// @return Standard error code. Possible codes:
        ///          Success, NotInitialized, SoftFail, ReadFailed, IncompleteData, MemoryError.
        ///
        ///          Success             - The data has been read properly. This doesn't mean the data is correct,
        ///                                 only that it has been correctly read.
        ///          NotInitialized      - Netlink connection is not established.
        ///          SoftFail            - Reading failed with EAGAIN error and can be retried later.
        ///          ReadFailed          - Reading from the Netlink socket failed (the socket has been re-initialized).
        ///          IncompleteData      - The data received is incomplete (the socket has been re-initialized).
        ///                                 This means a partial datagram was received, not a multipart message.
        ///          MemoryError         - Error allocating memory for reading.
        ERRCODE readMessages ( List<NetlinkMessage> & multipartMessages, List<NetlinkMessage> & messages );

    private:
        /// @brief The type of the buffer for modifySocketBufSize()
        enum BufferType
        {
            SendBuffer = 0, ///< Buffer for sending.
            ReceiveBuffer = 1 ///< Buffer for receiving.
        };

        /// @brief Modifies Netlink socket's buffers' sizes.
        /// It sets socket's buffer size to "desired" size stored in _sndBufSize or _rcvBufSize
        /// increased by sizeIncrease.
        /// If there is no "desired" size set, and sizeIncrease <= 0, nothing happens.
        /// Otherwise the current buffer size will be read.
        /// If there is no "desired" size, the current size + sizeIncrease will be set as the new "desired" size.
        /// If the current size is less than "desired" size, it will attempt to set it to "desired" size.
        /// At the end, desired size will be set to the new, modified size  (even if it actually hasn't changed).
        /// If, at any point, current buffer size cannot be obtained, "desired" size is set to 0.
        /// @param [in] bufType The type of the buffer to modify.
        /// @param [in] sizeIncrease The number of bytes by which the size should be increased beyond
        ///                           current or desired size (whichever is greater).
        /// @return True if the new size of the buffer is bigger than before; False otherwise.
        bool modifySocketBufSize ( BufferType bufType, int sizeIncrease = 0 );
};
}
