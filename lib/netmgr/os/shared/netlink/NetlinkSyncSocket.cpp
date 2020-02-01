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

#include "NetlinkSyncSocket.hpp"

using namespace Pravala;

NetlinkSyncSocket::NetlinkSyncSocket ( NetlinkCore::NetlinkFamily family ):
    NetlinkCore ( family, 0 ) // We don't want to be receiving any multicast messages
{
}

NetlinkSyncSocket::~NetlinkSyncSocket()
{
}

ERRCODE NetlinkSyncSocket::execMessage ( NetlinkMessage & msg, int * errorCode )
{
    List<NetlinkMessage> response;
    struct nlmsgerr netlinkError;

    memset ( &netlinkError, 0, sizeof ( netlinkError ) );

    const ERRCODE ret = execMessage ( msg, response, netlinkError );

    if ( errorCode != 0 )
    {
        *errorCode = netlinkError.error;
    }

    return ret;
}

ERRCODE NetlinkSyncSocket::execMessage (
        NetlinkMessage & msg, List<NetlinkMessage> & response, struct nlmsgerr & netlinkError )
{
    if ( !msg.isValid() )
    {
        return Error::InvalidParameter;
    }

    if ( _sock < 0 )
    {
        return Error::NotInitialized;
    }

    // We want to receive the response code.
    msg.addNetlinkFlags ( NLM_F_ACK );

    const uint32_t maxBusyTries = optMaxRequestBusyTries.value();
    const uint32_t maxRespErrTries = optMaxRequestRespErrorTries.value();

    uint32_t busyRetries = 0;
    uint32_t respErrorRetries = 0;

    // This loop re-tries to execute the request.
    while ( true )
    {
        response.clear();

        ERRCODE eCode;

        do
        {
            eCode = writeMessage ( msg );

            // We are using the socket in synchronous mode, so writing to a socket should always block and never
            // return SoftFail. However, if the socket's sending buffer size is adjusted (due to ENOBUFS error),
            // SoftFail may be returned even in the synchronous mode.
        }
        while ( eCode == Error::SoftFail );

        if ( NOT_OK ( eCode ) )
        {
            // Write error, no point re-trying...
            // It should already be logged by writeMessage()

            return eCode;
        }

        List<NetlinkMessage> multiMsgs;
        List<NetlinkMessage> regMsgs;

        bool needParts = true;
        bool continueReq = false;

        while ( !continueReq && needParts )
        {
            needParts = false;

            LOG ( L_DEBUG4, "Reading Netlink message parts" );

            eCode = readMessages ( multiMsgs, regMsgs );

            if ( IS_OK ( eCode ) )
            {
                // Read successful.
                // We read some message parts.
                // We should either be getting only a single regular message, or only multipart messages.
                // If we get both, then something is really weird...
                // Also, if we get multipart messages, we may have to read more to get the last part.

                LOG ( ( ( !multiMsgs.isEmpty() && !regMsgs.isEmpty() ) ? L_FATAL_ERROR : L_DEBUG4 ),
                      "We have " << multiMsgs.size() << " multipart message parts, and "
                      << regMsgs.size() << " regular message parts" );

                if ( !multiMsgs.isEmpty() )
                {
                    if ( multiMsgs.last().isDone() )
                    {
                        // We received some multipart data, and the last part IS NLMSG_DONE.

                        LOG ( L_DEBUG3, "The last multipart message is a NLMSG_DONE - done reading parts" );

                        // Thanks to implicit sharing this is cheap:
                        response = multiMsgs;
                    }
                    else
                    {
                        // We received some multipart data, and the last part is not NLMSG_DONE.

                        LOG ( L_DEBUG3, "The last multipart message is not NLMSG_DONE; We need to read more" );

                        needParts = true;
                    }
                }
                else
                {
                    LOG ( L_DEBUG3, "No multipart data received; Using regular messages (number received: "
                          << regMsgs.size() << ")" );

                    // Thanks to implicit sharing this is cheap:
                    response = regMsgs;
                }
            }
            else if ( maxRespErrTries == 0 || ++respErrorRetries < maxRespErrTries )
            {
                // An error, but we should retry.

                LOG ( L_WARN, "Re-trying a request that a reply to failed due to a socket error" );

                // We can't 'continue' here - we're inside 'needParts' loop.
                // So let's set a flag to continue the main loop once we are outside of this one.
                continueReq = true;
            }
            else
            {
                // An error and we should not retry.

                LOG ( L_ERROR, "Too many response socket error retries: " << respErrorRetries
                      << "; Limit is: " << maxRespErrTries );

                return eCode;
            }
        }

        if ( continueReq )
        {
            continue;
        }

        LOG ( L_DEBUG3, "Reading Netlink message parts completed" );

        const struct nlmsgerr * errMsg = 0;

        // We are done reading all parts of the message.
        // Now let's see if there were any errors - let's find the first one!

        for ( size_t r = 0; r < response.size(); ++r )
        {
            if ( ( errMsg = response.at ( r ).getError() ) != 0 )
            {
                break;
            }
        }

        if ( !errMsg || errMsg->error == 0 )
        {
            // No errors, or 'success' error code.

            return Error::Success;
        }
        else if ( errMsg->error != EBUSY )
        {
            // Some other, unknown, error.

            netlinkError = *errMsg;

            return Error::ErrorResponse;
        }

        if ( maxBusyTries == 0 || ++busyRetries < maxBusyTries )
        {
            // We have 'EBUSY' and we should re-try the whole write-read procedure.

            LOG ( L_DEBUG, "Re-trying a request that failed with EBUSY Netlink error" );
        }
        else
        {
            // We have 'EBUSY' (again), but we have retried too many times - we give up!

            LOG ( L_ERROR, "Too many EBUSY Netlink retries: " << busyRetries
                  << "; Limit is: " << maxBusyTries );

            return Error::NotAvailable;
        }
    }

    assert ( false );

    return Error::InternalError;
}
