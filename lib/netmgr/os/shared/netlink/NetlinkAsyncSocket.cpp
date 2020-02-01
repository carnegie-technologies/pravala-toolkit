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
#include <unistd.h>
#include <fcntl.h>
}

#include <cerrno>

#include "NetlinkAsyncSocket.hpp"
#include "NetlinkMsgCreator.hpp"

/// @brief Don't read anything larger than this
#define MAX_READ_BUFFER    65536

using namespace Pravala;

NetlinkAsyncSocket::NetlinkAsyncSocket ( NetlinkFamily family, uint32_t mcastGroups ):
    NetlinkCore ( family, mcastGroups ), _lastSeqNum ( 0 ), _busyRetries ( 0 )
{
    if ( _sock >= 0 )
    {
        netlinkSockReinitialized();
    }
}

NetlinkAsyncSocket::~NetlinkAsyncSocket()
{
    if ( _sock < 0 )
        return;

    // reset the fd to blocking mode
    int flags = fcntl ( _sock, F_GETFL, 0 );

    if ( flags < 0 )
        flags = 0;

    fcntl ( _sock, F_SETFL, flags & ~O_NONBLOCK );

    // dump our write queue
    while ( !_writeQueue.isEmpty() )
    {
        const NetlinkMessage & msg = _writeQueue.first();

        const ERRCODE eCode = writeMessage ( msg );

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_FATAL_ERROR, eCode, "Error sending a Netlink message while flushing the queue" );
        }

        _writeQueue.removeFirst();
    }

    EventManager::closeFd ( _sock );
    _sock = -1;
}

uint32_t NetlinkAsyncSocket::setSeqNum ( NetlinkMessage & msg )
{
    if ( ++_lastSeqNum == 0 )
    {
        _lastSeqNum = 1;
    }

    msg.setSeqNum ( _lastSeqNum );

    return _lastSeqNum;
}

void NetlinkAsyncSocket::clearRequestQueue()
{
    _busyRetries = 0;
    _writeQueue.clear();

    if ( _sock >= 0 )
    {
        EventManager::disableWriteEvents ( _sock );
    }
}

void NetlinkAsyncSocket::netlinkSockReinitialized()
{
    _busyRetries = 0;
    _readMultiParts.clear();
    _readRegParts.clear();

    if ( _sock >= 0 )
    {
        if ( _writeQueue.isEmpty() )
        {
            EventManager::setFdHandler ( _sock, this, EventManager::EventRead );
        }
        else
        {
            EventManager::setFdHandler ( _sock, this, EventManager::EventRead | EventManager::EventWrite );
        }
    }
}

void NetlinkAsyncSocket::receiveFdEvent ( int fd, short events )
{
    ( void ) fd;
    assert ( fd == _sock );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        assert ( _sock >= 0 );

        // We only want to have one request in flight.
        // If Netlink is busy processing a request, another concurrent request might result in an error.
        // We want to avoid it, so we only send one request at a time. This is still possible to happen,
        // but we don't want to increase the odds...

        EventManager::disableWriteEvents ( _sock );

        if ( !_writeQueue.isEmpty() )
        {
            const NetlinkMessage & msg = _writeQueue.first();
            const uint32_t reqSeqNum = msg.getSeqNum();
            const ERRCODE eCode = writeMessage ( msg );

            switch ( eCode.value() )
            {
                case Error::Success:
                    LOG ( L_DEBUG2, "A Netlink message with SeqNum " << msg.getSeqNum() << " successfully written" );
                    break;

                case Error::SoftFail:
                    LOG ( L_DEBUG, "A soft error while writing to a Netlink socket; We will retry later" );

                    if ( _sock >= 0 )
                    {
                        EventManager::enableWriteEvents ( _sock );
                    }
                    break;

                case Error::TooMuchData:
                case Error::InvalidParameter:
                    LOG_ERR ( L_FATAL_ERROR, eCode, "Could not write a message to a Netlink message; SeqNum: "
                              << msg.getSeqNum() << "; Dropping it" );

                    _busyRetries = 0;
                    _writeQueue.removeFirst();

                    if ( !_writeQueue.isEmpty() && _sock >= 0 )
                    {
                        EventManager::enableWriteEvents ( _sock );
                    }

                    netlinkReqFailed ( reqSeqNum, eCode );

                    // If there are read events we will get called again...
                    return;
                    break;

                default:
                    LOG_ERR ( L_FATAL_ERROR, eCode, "Error writing to a Netlink socket" );

                    // All other errors result in reinitializing the socket
                    // (or are caused by the socket not being initialized to begin with).
                    // If that happened, the events would already be configured in netlinkSockReinitialized().
                    // The potential read event in this function may not be valid anymore - we need to return.
                    //
                    // Also, if a multicast socket gets reinitialized, its state may no longer be correct.
                    // We need to tell the owner.

                    if ( _mcastGroups != 0 )
                    {
                        _busyRetries = 0;
                        _writeQueue.clear();
                        netlinkMCastSocketFailed();
                    }
                    return;
                    break;
            }
        }
    }

    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        const ERRCODE eCode = readMessages ( _readMultiParts, _readRegParts );

        if ( eCode == Error::SoftFail )
        {
            LOG ( L_DEBUG4, "Experienced 'soft fail' error while reading from the Netlink socket" );

            // Nothing to do...

            return;
        }
        else if ( NOT_OK ( eCode ) )
        {
            // Read error.
            // This could mean that the response to the request is too big, or we somehow failed to read it.
            // If we are configured to receive multicast updates, this may not have anything to do with our
            // request, but with one of the updates - for instance it could be too large - but we don't know
            // which one it is. Also, if we are listening to multicast updates, all the requests should
            // not modify the state of the system, just query it. So it is safe to retry them.
            //
            // However, if we are not supposed to be getting multicast updates, this error is a result of a request
            // which may have already modified the state of the system. In that case we cannot retry it.

            // Also, if we get an error here, it means that the underlying socket has already been re-initialized.

            LOG_ERR ( L_FATAL_ERROR, eCode, "Error reading from the Netlink socket" );

            // The response is not really valid anymore...

            _readMultiParts.clear();
            _readRegParts.clear();

            if ( _mcastGroups != 0 )
            {
                // We are configured to receive multicast updates.
                // If we get an error it means that our state may no longer be valid.

                _busyRetries = 0;
                _writeQueue.clear();
                netlinkMCastSocketFailed();
                return;
            }

            if ( _writeQueue.isEmpty() )
            {
                // Read failed, but there are no requests in the queue... weird, but doesn't hurt...
                return;
            }

            // Otherwise - let's drop the request!

            const uint32_t reqSeqNum = _writeQueue.first().getSeqNum();

            LOG_ERR ( L_FATAL_ERROR, eCode, "Error reading from the Netlink socket; "
                      "Dropping the request with SeqNum " << reqSeqNum );

            _busyRetries = 0;
            _writeQueue.removeFirst();

            if ( !_writeQueue.isEmpty() && _sock >= 0 )
            {
                EventManager::enableWriteEvents ( _sock );
            }

            netlinkReqFailed ( reqSeqNum, eCode );
            return;
        }

        if ( !_readMultiParts.isEmpty() && !_readMultiParts.last().isDone() )
        {
            LOG ( L_DEBUG3, "Incomplete multipart message received; Waiting for more parts" );

            return;
        }

        LOG ( L_DEBUG3, "About to process netlink message parts; Number of multipart messages: "
              << _readMultiParts.size() << "; Number of regular messages: " << _readRegParts.size() );

        if ( _readMultiParts.isEmpty() && _readRegParts.isEmpty() )
        {
            LOG ( L_ERROR, "No Netlink messages received" );

            return;
        }

        uint32_t reqSeqNum = 0;
        List<NetlinkMessage> responseParts;

        if ( !_writeQueue.isEmpty() )
        {
            reqSeqNum = _writeQueue.first().getSeqNum();

            assert ( reqSeqNum != 0 );

            // We are looking for a response to our request.
            // It could be stored in _readMultiParts, or in _readRegParts.
            // Let's check the multi-part first (since it should only contain responses).

            if ( !_readMultiParts.isEmpty() )
            {
                bool foundResponse = false;

                for ( size_t idx = 0; idx < _readMultiParts.size(); ++idx )
                {
                    const NetlinkMessage & msg = _readMultiParts.at ( idx );

                    if ( msg.getPid() != _sockPID || msg.getSeqNum() != reqSeqNum )
                    {
                        LOG ( L_FATAL_ERROR, "Received a multipart message with unexpected PID/SeqNum: "
                              << String::number ( msg.getPid(), String::Int_HEX ) << "/" << msg.getSeqNum()
                              << "; Expected: " << String::number ( _sockPID, String::Int_HEX ) << "/" << reqSeqNum );

                        // This should not happen!
                        // But to handle this properly we would have to inspect and clear the entire list
                        // and also detect whether the actual message we want to have is 'DONE'.
                        // For now we just ignore it...

                        continue;
                    }

                    foundResponse = true;

                    const struct nlmsgerr * errMsg = msg.getError();

                    if ( errMsg != 0 && errMsg->error == EBUSY )
                    {
                        if ( optMaxRequestBusyTries.value() == 0 || ++_busyRetries < optMaxRequestBusyTries.value() )
                        {
                            LOG ( L_DEBUG, "Received EBUSY error response to message with SeqNum "
                                  << reqSeqNum << "; This request message has been sent "
                                  << _busyRetries << " time(s); We will retry it again" );

                            _readMultiParts.clear();

                            if ( _sock >= 0 )
                            {
                                EventManager::enableWriteEvents ( _sock );
                            }

                            return;
                        }

                        LOG ( L_ERROR, "Received EBUSY error response to message with SeqNum "
                              << reqSeqNum << "; This request message has been sent "
                              << _busyRetries << " time(s) and we will NOT try again" );

                        break;
                    }
                }

                if ( foundResponse )
                {
                    LOG ( L_DEBUG3, "Received a multipart response we were waiting for; SeqNum: " << reqSeqNum );

                    responseParts = _readMultiParts;
                    _readMultiParts.clear();
                }
                else
                {
                    LOG ( L_FATAL_ERROR, "Received a multipart message that is NOT a response we are waiting for; "
                          "Request SeqNum: " << reqSeqNum << "; Clearing the whole multipart queue" );

                    _readMultiParts.clear();
                }
            }

            if ( responseParts.isEmpty() )
            {
                // The response was not in _readMultiParts.
                // Let's check the regular list.

                for ( size_t idx = 0; idx < _readRegParts.size(); ++idx )
                {
                    if ( _readRegParts.at ( idx ).getPid() != _sockPID
                         || _readRegParts.at ( idx ).getSeqNum() != reqSeqNum )
                    {
                        continue;
                    }

                    const struct nlmsgerr * errMsg = _readRegParts.at ( idx ).getError();

                    if ( errMsg != 0 && errMsg->error == EBUSY )
                    {
                        if ( optMaxRequestBusyTries.value() == 0 || ++_busyRetries < optMaxRequestBusyTries.value() )
                        {
                            LOG ( L_DEBUG, "Received EBUSY error response to message with SeqNum "
                                  << reqSeqNum << "; This request message has been sent "
                                  << _busyRetries << " time(s); We will retry it again" );

                            // Instead of clearing the entire list, we remove only this particular response.

                            _readRegParts.removeIndex ( idx );

                            if ( _sock >= 0 )
                            {
                                EventManager::enableWriteEvents ( _sock );
                            }

                            return;
                        }

                        LOG ( L_ERROR, "Received EBUSY error response to message with SeqNum "
                              << reqSeqNum << "; This request message has been sent "
                              << _busyRetries << " time(s) and we will NOT try again" );
                    }

                    // We break either way!

                    responseParts.append ( _readRegParts.at ( idx ) );
                    _readRegParts.removeIndex ( idx );

                    break;
                }
            }

            if ( responseParts.isEmpty() )
            {
                // We couldn't find the response.
                LOG ( L_DEBUG2, "Some messages were received (multipart: " << _readMultiParts.size()
                      << "; regular: " << _readRegParts.size()
                      << "), but we are still waiting for a response to request " << reqSeqNum );
                return;
            }

            // Doesn't matter if it was a Netlink success or a Netlink failure.
            // We only care about EBUSY (and only for a few times).

            LOG ( L_DEBUG2, "Received a response to message with SeqNum " << reqSeqNum << " - removing the request"
                  << ( ( !_writeQueue.isEmpty() && _sock >= 0 ) ? " and re-enabling write events" : "" ) );

            _busyRetries = 0;
            _writeQueue.removeFirst();

            if ( !_writeQueue.isEmpty() && _sock >= 0 )
            {
                EventManager::enableWriteEvents ( _sock );
            }
        }

        if ( !_readMultiParts.isEmpty() )
        {
            // These should only be received in response to a request,
            // in which case we would have cleared them already (and put in responseParts).

            LOG ( L_FATAL_ERROR, "Received unexpected multipart message; SeqNum: "
                  << _readMultiParts.first().getSeqNum()
                  << "; Number of parts: " << _readMultiParts.size() << "; Dropping" );

            _readMultiParts.clear();
        }

        // Here we have a bunch of messages. We could have some messages in responseParts, which means
        // that those are responses to our requests.
        // We could also have some messages in _readRegParts, which means there are asynchronous updates.
        // It is possible, that we have have both.
        // This means that some updates were received while waiting for the complete response
        // to a request (and were queued). We need to deliver the response first (if we have it),
        // followed by all the other messages.

        List<NetlinkMessage> rMsgs ( _readRegParts );

        _readRegParts.clear();

        if ( !responseParts.isEmpty() )
        {
            LOG ( L_DEBUG2, "Received a Netlink message response to request " << reqSeqNum << " consisting of "
                  << responseParts.size() << " part(s) - delivering" );

            netlinkReceived ( responseParts );
        }

        if ( !rMsgs.isEmpty() )
        {
            LOG ( L_DEBUG2, "Delivering " << rMsgs.size() << " single-part Netlink message(s)" );

            // Pass the messages individually to netlinkReceived since they're all single part messages
            List<NetlinkMessage> tmpList;

            while ( !rMsgs.isEmpty() )
            {
                tmpList.append ( rMsgs.first() );

                rMsgs.removeFirst();

                netlinkReceived ( tmpList );

                tmpList.clear();
            }
        }

        return;
    }
}

uint32_t NetlinkAsyncSocket::sendMessage ( NetlinkMessage & msg )
{
    if ( !msg.isValid() )
    {
        LOG ( L_FATAL_ERROR, "Could not send an invalid Netlink message" );

        return 0;
    }

    if ( _sock < 0 )
    {
        LOG ( L_FATAL_ERROR, "Trying to write to an uninitialized Netlink socket" );

        return 0;
    }

    // We want to receive the response code.
    msg.addNetlinkFlags ( NLM_F_ACK );

    const uint32_t ret = setSeqNum ( msg );

    if ( ret == 0 )
    {
        LOG ( L_FATAL_ERROR, "Could not set a sequence number in the message" );

        return 0;
    }

    if ( _writeQueue.isEmpty() )
    {
        _busyRetries = 0;

        EventManager::enableWriteEvents ( _sock );
    }

    _writeQueue.append ( msg );

    return ret;
}
