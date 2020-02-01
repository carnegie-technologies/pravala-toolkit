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

extern "C"
{
#include <sys/socket.h>
#include <sys/errno.h>
#include <fcntl.h>
}

#include "basic/Math.hpp"
#include "basic/MemHandle.hpp"
#include "event/EventManager.hpp"
#include "sys/SocketApi.hpp"

#include "NetlinkCore.hpp"

// Number of bytes by which we try to increase the send buffer.
#define SNDBUF_SIZE_INCREASE    ( 16 * 1024 )

// Number of bytes by which we try to increase the receive buffer.
#define RCVBUF_SIZE_INCREASE    ( 32 * 1024 )

using namespace Pravala;

ConfigNumber<uint16_t> NetlinkCore::optMaxRequestBusyTries (
        ConfigOpt::FlagInitializeOnly,
        "os.netlink.max_req_busy_tries",
        "The max number of times we want to retry requests that resulted in EBUSY Netlink erorr response. "
        "Note that value '1' means there will be no retries, just the original attempt. 0 means 'no limit'.",
        10
);

ConfigNumber<uint16_t> NetlinkCore::optMaxRequestRespErrorTries (
        ConfigOpt::FlagInitializeOnly,
        "os.netlink.max_req_resp_error_tries",
        "The max number of times we want to retry requests that resulted in a socket error while receiving the reply. "
        "Note that value '1' means there will be no retries, just the original attempt. 0 means 'no limit'.",
        3
);

TextLog NetlinkCore::_log ( "netlink" );

NetlinkCore::NetlinkCore ( NetlinkFamily family, uint32_t mcastGroups ):
    _family ( family ), _mcastGroups ( mcastGroups ), _sock ( -1 ), _sockPID ( 0 ), _sndBufSize ( 0 ), _rcvBufSize ( 0 )
{
    reinitializeSocket();
}

NetlinkCore::~NetlinkCore()
{
    if ( _sock >= 0 )
    {
        // In case there were some event handlers registered...
        EventManager::closeFd ( _sock );
        _sock = -1;
        _sockPID = 0;
    }
}

bool NetlinkCore::reinitializeSocket ( int sndSizeIncrease, int rcvSizeIncrease )
{
    if ( _sock >= 0 )
    {
        // In case there were some event handlers registered...
        EventManager::closeFd ( _sock );
        _sock = -1;
        _sockPID = 0;
    }

    /// @note This will need to be updated once the list of supported families changes
    assert ( _family == Route );

    _sock = socket ( AF_NETLINK, SOCK_RAW, _family );

    if ( _sock < 0 )
    {
        LOG ( L_ERROR, "socket() failed: " << strerror ( errno ) );

        return false;
    }

    struct sockaddr_nl saddrnl;

    memset ( &saddrnl, 0, sizeof ( saddrnl ) );

    saddrnl.nl_family = AF_NETLINK;
    saddrnl.nl_groups = _mcastGroups;

    if ( ::bind ( _sock, ( struct sockaddr * ) &saddrnl, sizeof ( saddrnl ) ) < 0 )
    {
        LOG ( L_ERROR, "bind() failed: " << strerror ( errno ) );

        ::close ( _sock );
        _sock = -1;

        return false;
    }

    socklen_t sockLen = sizeof ( saddrnl );

    memset ( &saddrnl, 0, sizeof ( saddrnl ) );

    if ( ::getsockname ( _sock, ( struct sockaddr * ) &saddrnl, &sockLen ) < 0 )
    {
        LOG ( L_FATAL_ERROR, "Could not obtain the Port ID assigned to the Netlink socket: "
              << strerror ( errno ) << "; Netlink socket will not be functional" );

        ::close ( _sock );
        _sock = -1;

        return false;
    }

    if ( sockLen < ( int ) sizeof ( saddrnl ) )
    {
        LOG ( L_FATAL_ERROR, "Could not obtain the Port ID assigned to the Netlink socket; "
              "Received invalid address length: " << sockLen << ", expected at least "
              << sizeof ( saddrnl ) << "; Netlink socket will not be functional" );

        ::close ( _sock );
        _sock = -1;

        return false;
    }

    _sockPID = saddrnl.nl_pid;

    LOG ( L_DEBUG, "Successfully (re)initialized a Netlink socket; Family: " << _family
          << "; MCastGroups: " << _mcastGroups << "; PortID: " << _sockPID );

    // If we previously modified buffer sizes or want to increase them now, let's set them in the new socket too:

    modifySocketBufSize ( SendBuffer, sndSizeIncrease );
    modifySocketBufSize ( ReceiveBuffer, rcvSizeIncrease );

    netlinkSockReinitialized();
    return true;
}

bool NetlinkCore::modifySocketBufSize ( BufferType bufType, int sizeIncrease )
{
    if ( _sock < 0 || ( bufType != SendBuffer && bufType != ReceiveBuffer ) )
    {
        return false;
    }

    const int sockOpt = ( bufType == SendBuffer ) ? SO_SNDBUF : SO_RCVBUF;

    int & desiredSize = ( bufType == SendBuffer ) ? _sndBufSize : _rcvBufSize;

    if ( desiredSize <= 0 && sizeIncrease <= 0 )
    {
        // Size not set and no need to increase it.

        return false;
    }

    int prevBufSize = 0;

    if ( !SocketApi::getOption ( _sock, SOL_SOCKET, sockOpt, prevBufSize ) || prevBufSize <= 0 )
    {
        // We can't get the current size. Let's unset the desired size too.
        desiredSize = 0;
        return false;
    }

    // We got a valid buffer size.

    // Just in case:
    if ( sizeIncrease < 0 )
    {
        sizeIncrease = 0;
    }

    // Our new "desired" buffer size should be 'desired + increase' or 'current + increase',
    // whichever is greater.

    desiredSize = max ( desiredSize + sizeIncrease, prevBufSize + sizeIncrease );

    if ( desiredSize <= prevBufSize )
    {
        // No need to increase the size, but let's store the current one as the desired one

        desiredSize = prevBufSize;
        return false;
    }

    // Buffer should be bigger.

    SocketApi::setOption ( _sock, SOL_SOCKET, sockOpt, desiredSize );

    int newBufSize = 0;

    if ( !SocketApi::getOption ( _sock, SOL_SOCKET, sockOpt, newBufSize ) || newBufSize <= 0 )
    {
        // We can't get the current (new) size. Let's unset the desired size too.
        desiredSize = 0;
        return false;
    }

    // We, again, were able to get socket's buffer size.
    // Whatever the new value is, let's make it the new desired one
    // (we might not be able to increase it beyond certain point).

    desiredSize = newBufSize;

    if ( newBufSize > prevBufSize )
    {
        LOG ( L_DEBUG, "Increased the size of Netlink socket's buffer for "
              << ( ( bufType == SendBuffer ) ? "sending" : "receiving" )
              << " : " << prevBufSize << " -> " << newBufSize << " bytes" );

        return true;
    }

    // It is possible that it hasn't changed (due to max buffer size setting).
    return false;
}

void NetlinkCore::netlinkSockReinitialized()
{
}

ERRCODE NetlinkCore::writeMessage ( const NetlinkMessage & msg )
{
    if ( !msg.isValid() )
    {
        return Error::InvalidParameter;
    }

    if ( _sock < 0 )
    {
        LOG ( L_FATAL_ERROR, "Writing to uninitialized Netlink socket" );

        return Error::NotInitialized;
    }

    const MemHandle data = msg.getHandle();
    const size_t size = data.size();

    assert ( size > 0 );

    const ssize_t wRet = sendto ( _sock, data.get(), size, 0, 0, 0 );

    if ( wRet <= 0 )
    {
        // Error or closed.

        if ( wRet == 0 )
        {
            // Closed.

            LOG ( L_FATAL_ERROR, "Netlink socket has been closed" );
        }
        else if ( errno == ENOBUFS )
        {
            LOG ( L_DEBUG, "Writing to Netlink socket failed with ENOBUFS" );

            // Let's try to increase the socket buffer size for sending.

            if ( modifySocketBufSize ( SendBuffer, SNDBUF_SIZE_INCREASE ) )
            {
                // We increased the size so we should try again.
                // However, we may no longer be able to write (if we are used in async mode).
                // So let's pretend we got a temporary write error:
                return Error::SoftFail;
            }

            return Error::TooMuchData;
        }
        else if ( errno == EAGAIN || errno == EWOULDBLOCK )
        {
            // EAGAIN error - no need to close the socket!

            LOG ( L_DEBUG, "Writing to Netlink socket failed with EAGAIN" );

            return Error::SoftFail;
        }
        else
        {
            // Some other (fatal) error.

            LOG ( L_FATAL_ERROR, "Error writing to the Netlink socket: " << strerror ( errno ) );
        }

        reinitializeSocket();

        return Error::WriteFailed;
    }
    else if ( ( size_t ) wRet != size )
    {
        // This should not happen, if it does then something is seriously wrong...

        LOG ( L_FATAL_ERROR, "Incomplete write to to a socket; Expected size: "
              << size << "; Actually written: " << wRet << "; Re-initializing the socket" );

        reinitializeSocket();

        return Error::IncompleteWrite;
    }
    else
    {
        return Error::Success;
    }
}

ERRCODE NetlinkCore::readMessages ( List<NetlinkMessage> & multipartMessages, List<NetlinkMessage> & messages )
{
    if ( _sock < 0 )
    {
        LOG ( L_FATAL_ERROR, "Reading from uninitialized Netlink socket" );

        return Error::NotInitialized;
    }

    // do a pre-read to see how much memory we need

    char c;

    ssize_t rRet = recvfrom ( _sock, &c, 1, MSG_PEEK | MSG_TRUNC, 0, 0 );

    MemHandle data;

    if ( rRet > 0 )
    {
        data = MemHandle ( rRet );

        // do the real read

        char * const buf = data.getWritable();

        if ( !buf )
        {
            LOG ( L_FATAL_ERROR, "Error allocating memory for reading from Netlink socket" );

            return Error::MemoryError;
        }

        rRet = recvfrom ( _sock, buf, data.size(), 0, 0, 0 );
    }

    LOG ( L_DEBUG4, "recvfrom read: " << rRet << " bytes" );

    if ( rRet <= 0 )
    {
        // Error or closed.

        int rcvSizeIncrease = 0;

        if ( rRet == 0 )
        {
            // Closed.

            LOG ( L_FATAL_ERROR, "Netlink socket has been closed" );
        }
        else if ( errno == EAGAIN || errno == EWOULDBLOCK )
        {
            // EAGAIN error - no need to close the socket!

            LOG ( L_DEBUG, "Reading from Netlink socket failed with EAGAIN" );

            return Error::SoftFail;
        }
        else
        {
            // Some other (fatal) error.

            LOG ( L_FATAL_ERROR, "Error reading from the Netlink socket: " << strerror ( errno ) );

            if ( errno == ENOBUFS )
            {
                rcvSizeIncrease = RCVBUF_SIZE_INCREASE;
            }
        }

        reinitializeSocket ( 0, rcvSizeIncrease );

        return Error::ReadFailed;
    }

    data.truncate ( rRet );

    size_t offset = 0;

    const struct nlmsghdr * nlHdr = ( const struct nlmsghdr * ) data.get();

    while ( nlHdr != 0
            && offset < data.size()
            && offset + sizeof ( struct nlmsghdr ) < data.size()
            && nlHdr->nlmsg_len >= sizeof ( struct nlmsghdr )
            && offset + nlHdr->nlmsg_len <= data.size() )
    {
        const NetlinkMessage curMsg ( data.getHandle ( offset, nlHdr->nlmsg_len ) );

        offset += NLMSG_ALIGN ( nlHdr->nlmsg_len );

        nlHdr = ( const struct nlmsghdr * ) data.get ( offset );

        // According to man 7 netlink:
        //
        // In multipart messages (multiple nlmsghdr headers with associated payload in one byte stream)
        // the first and all following headers have the NLM_F_MULTI flag set,
        // except for the last header which has the type NLMSG_DONE.
        //
        // In reality, the NLMSG_DONE also has NLM_F_MULTI set.
        // So even if it was marked as multipart, we need to check if it's 'done':
        //
        // We don't want to rely on undocumented behaviour, so we treat both flagged multipart as well as DONE
        // messages as multipart:
        //
        if ( curMsg.isMultipart() || curMsg.isDone() )
        {
            multipartMessages.append ( curMsg );
        }
        else
        {
            messages.append ( curMsg );
        }
    }

    if ( offset != data.size() )
    {
        LOG ( L_FATAL_ERROR, "Partial Netlink datagram read" );

        reinitializeSocket();

        return Error::IncompleteData;
    }
    else
    {
        return Error::Success;
    }
}
