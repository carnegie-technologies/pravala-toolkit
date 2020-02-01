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

#include <cerrno>

extern "C"
{
#include <sys/types.h>

#ifdef SYSTEM_WINDOWS
#include <winsock2.h>
#else
#include <netinet/tcp.h>
#endif
}

#include "config/ConfigCore.hpp"
#include "log/LogManager.hpp"
#include "log/Diagnostics.hpp"
#include "sys/OsUtils.hpp"
#include "sys/SocketApi.hpp"

#include "CtrlLink.hpp"
#include "CtrlSubHandler.hpp"

#include "auto/ctrl/Ctrl/Ping.hpp"
#include "auto/ctrl/Ctrl/Pong.hpp"
#include "auto/ctrl/Ctrl/SimpleResponse.hpp"
#include "auto/ctrl/Ctrl/SubscriptionRequest.hpp"
#include "auto/ctrl/Ctrl/SimpleSubscriptionResponse.hpp"
#include "auto/ctrl/Ctrl/LogMessage.hpp"
#include "auto/ctrl/Ctrl/GetLogList.hpp"
#include "auto/ctrl/Ctrl/LogListResp.hpp"
#include "auto/ctrl/Ctrl/LogSubscribe.hpp"
#include "auto/ctrl/Ctrl/LogUnsubscribe.hpp"
#include "auto/ctrl/Ctrl/GetDiagnostics.hpp"
#include "auto/ctrl/Ctrl/DiagnosticsResponse.hpp"
#include "auto/ctrl/Ctrl/LoadConfig.hpp"
#include "auto/ctrl/Ctrl/LoadConfigResponse.hpp"

using namespace Pravala;

TextLog CtrlLink::_log ( "ctrl_link" );

CtrlLink::Owner::~Owner()
{
    // The Iterator will create a copy and iterate over it.
    HashMap<uint32_t, CtrlSubHandler *>::Iterator it ( _subHandlers );

    _subHandlers.clear();

    for ( ; it.isValid(); it.next() )
    {
        if ( it.value() != 0 && it.value()->AutoDelete )
        {
            delete it.value();
        }
    }
}

void CtrlLink::Owner::ctrlAddSubHandler ( uint32_t subId, CtrlSubHandler * handler )
{
    CtrlSubHandler * prevHandler = 0;

    if ( _subHandlers.find ( subId, prevHandler ) )
    {
        LOG ( L_WARN, "Removing existing handler for subscription requests with type " << subId );

        _subHandlers.remove ( subId );

        if ( prevHandler != 0 && prevHandler->AutoDelete )
        {
            delete prevHandler;
            prevHandler = 0;
        }
    }

    if ( handler != 0 )
    {
        _subHandlers.insert ( subId, handler );
    }
}

void CtrlLink::Owner::ctrlRemoveSubHandler ( uint32_t subId, CtrlSubHandler * handler )
{
    if ( !handler || _subHandlers.isEmpty() )
        return;

    CtrlSubHandler * prevHandler = 0;

    if ( _subHandlers.find ( subId, prevHandler ) && prevHandler == handler )
    {
        _subHandlers.remove ( subId );
    }
}

ERRCODE CtrlLink::Owner::ctrlSubscriptionResponseReceived (
        int linkId, Ctrl::SimpleSubscriptionResponse & msg, List< int > & receivedFds )
{
    LOG ( L_DEBUG2, "Received a SimpleSubscriptionResponse message (type " << msg.getType()
          << "); Delivering all (" << msg.getUpdates().size() << ") one by one" );

    ERRCODE eCode = Error::Success;

    for ( size_t i = 0; i < msg.getUpdates().size(); ++i )
    {
        // We need to create a copy, so we could pass a writable reference!

        Ctrl::Message update = msg.getUpdates().at ( i );

        const ERRCODE tmpCode = ctrlPacketReceived ( linkId, update, receivedFds );

        LOG_ERR ( IS_OK ( tmpCode ) ? L_DEBUG2 : L_ERROR, tmpCode,
                  "Calling ctrlPacketReceived() for internal subscription update with type "
                  << msg.getUpdates().at ( i ).getObject().getType() );

        if ( NOT_OK ( tmpCode )
             && ( IS_OK ( eCode ) || eCode == Error::ResponseSent || eCode == Error::ResponsePending ) )
        {
            eCode = tmpCode;
        }
    }

    return eCode;
}

CtrlLink::CtrlLink ( CtrlLink::Owner & owner, int linkId ):
    LinkId ( linkId ), _linkFd ( -1 ), _owner ( owner ), _fdCleanupTimer ( *this ), _localPort ( 0 ), _remotePort ( 0 )
{
}

CtrlLink::~CtrlLink()
{
    setup ( -1 );
    cleanupFds();
}

int CtrlLink::takeFd()
{
    const int fd = _linkFd;
    _linkFd = -1;

    setup ( -1 );
    cleanupFds();

    return fd;
}

void CtrlLink::setup ( int linkFd, const String & sockName )
{
    setup ( linkFd );

    if ( _linkFd >= 0 )
    {
        _localSockName = sockName;
    }
}

void CtrlLink::setup (
        int linkFd, const IpAddress & localAddr, uint16_t localPort, const IpAddress & remoteAddr, uint16_t remotePort )
{
    setup ( linkFd );

    if ( _linkFd >= 0 )
    {
        _localAddr = localAddr;
        _remoteAddr = remoteAddr;
        _localPort = localPort;
        _remotePort = remotePort;
    }
}

void CtrlLink::setup ( int linkFd )
{
    _localSockName.clear();
    _localAddr.clear();
    _remoteAddr.clear();
    _localPort = 0;
    _remotePort = 0;

    if ( linkFd < 0 )
    {
        for ( HashMap<uint32_t, CtrlSubHandler *>::Iterator it ( _owner._subHandlers ); it.isValid(); it.next() )
        {
            if ( it.value() != 0 )
            {
                it.value()->ctrlLinkRemoved ( this );
            }
        }
    }

    // We CANNOT clear the buffer here!
    // CtrlLinkConnector may have appended something to it before calling setup()!

    if ( _linkFd >= 0 && _linkFd != linkFd )
    {
        EventManager::closeFd ( _linkFd );
        _linkFd = -1;
    }

    _linkFd = linkFd;

    if ( _linkFd >= 0 )
    {
        // We only want write events if there already is something in the write queue.
        EventManager::setFdHandler ( _linkFd,
                                     this,
                                     EventManager::EventRead
                                     | ( _writeQueue.isEmpty() ? ( 0 ) : ( EventManager::EventWrite ) ) );

        const int iVal = 1;

        SocketApi::setOption ( _linkFd, IPPROTO_TCP, TCP_NODELAY, iVal );
    }
}

void CtrlLink::cleanupFds()
{
    for ( size_t i = 0; i < _readFds.size(); ++i )
    {
        if ( _readFds[ i ] >= 0 )
        {
            LOG ( L_DEBUG2, "Closing FD: " << _readFds[ i ] );

            SocketApi::close ( _readFds[ i ] );
        }
    }

    _readFds.clear();
    _fdCleanupTimer.stop();
}

ERRCODE CtrlLink::serializePacket ( Ctrl::Message & packet, MemHandle & memHandle )
{
    ProtoError eCode;

    const size_t orgSize = memHandle.size();

    ExtProtoError extError;

    memHandle = packet.serializeWithLength ( &eCode, &extError );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Could not serialize a packet of type " << packet.getType()
                  << "; Error data: '" << extError.toString() << "'" );

        return eCode;
    }

    if ( memHandle.size() <= orgSize )
    {
        LOG ( L_ERROR, "Packet of type " << packet.getType()
              << " serialized to an empty string. Not sending" );

        return Error::EmptyWrite;
    }

    return Error::Success;
}

void CtrlLink::sendData ( Ctrl::Message & packet, MemHandle & data, bool atLoopEnd )
{
    ( void ) packet;

    LOG ( ( packet.getType() == Ctrl::LogMessage::DEF_TYPE ) ? L_DEBUG3 : L_DEBUG2,
          "Adding a packet to the " << ( atLoopEnd ? "end-of-loop " : "" )
          << "write queue of a link with ID " << LinkId << "; PacketType: " << packet.getType() );

    if ( _linkFd < 0 )
    {
        LOG ( L_FATAL_ERROR, "Calling sendData() on a control link with negative FD" );

        assert ( false );

        return;
    }

    if ( atLoopEnd )
    {
        _loopEndWriteQueue.append ( data );
        EventManager::loopEndSubscribe ( this );
    }
    else
    {
        _writeQueue.append ( data );
        EventManager::enableWriteEvents ( _linkFd );
    }
}

ERRCODE CtrlLink::sendPacket ( Ctrl::Message & packet, bool atLoopEnd )
{
    if ( _linkFd < 0 )
    {
        LOG ( L_ERROR, "Received a packet for unconnected CtrlLink (ID: " << LinkId << ")" );

        return Error::NotConnected;
    }

    MemHandle mem;
    ERRCODE eCode = serializePacket ( packet, mem );

    if ( NOT_OK ( eCode ) )
        return eCode;

    sendData ( packet, mem, atLoopEnd );

    return Error::Success;
}

void CtrlLink::close()
{
    setup ( -1 );
    cleanupFds();
}

void CtrlLink::linkClosed()
{
    close();

    // No operations after the callback!
    _owner.ctrlLinkClosed ( LinkId );
    return;
}

void CtrlLink::receiveLoopEndEvent()
{
    if ( _linkFd < 0 )
    {
        LOG ( L_ERROR, "CtrlLink with ID " << LinkId
              << " is no longer connected. Not sending the end-of-loop packets" );
        return;
    }

    bool firstRun = true;

    while ( !_loopEndWriteQueue.isEmpty() )
    {
        if ( firstRun )
        {
            LOG ( L_DEBUG3, "Moving a packet from 'end-of-loop' queue to "
                  "write queue of a link with ID " << LinkId );

            if ( _linkFd < 0 )
            {
                LOG ( L_FATAL_ERROR, "Calling enableWriteEvents() with negative FD" );

                // It would assert anyway (in enableWriteEvents)
                assert ( false );

                return;
            }

            EventManager::enableWriteEvents ( _linkFd );

            firstRun = false;
        }

        _writeQueue.append ( _loopEndWriteQueue.first() );
        _loopEndWriteQueue.removeFirst();
    }
}

void CtrlLink::receiveFdEvent ( int fd, short int events )
{
    assert ( fd >= 0 );
    assert ( fd == _linkFd );

    if ( fd < 0 || fd != _linkFd )
    {
        return;
    }

    if ( events & EventManager::EventRead )
    {
        if ( !doCtrlRead() )
            return;
    }

    if ( events & EventManager::EventWrite )
    {
        if ( _writeQueue.isEmpty() )
        {
            LOG ( L_DEBUG3, "Write event received, but the write queue is empty. Disabling write events" );

            EventManager::disableWriteEvents ( _linkFd );
            return;
        }

        MemHandle packet = _writeQueue.first();

        LOG ( L_DEBUG3, "Writing the first packet from the queue; Data size: " << packet.size()
              << "; Current queue size: " << _writeQueue.size() );

        _writeQueue.removeFirst();

        const ssize_t ret = send ( _linkFd, packet.get(), packet.size(), 0 );

        LOG ( L_DEBUG4, "Writing " << packet.size() << " bytes over link with FD " << _linkFd
              << "; Wrote (result): " << ret << "; New queue size: " << _writeQueue.size() );

        if ( ret < 0 )
        {
#ifdef SYSTEM_WINDOWS
            LOG ( L_ERROR, "Error writing to control link: ["
                  << WSAGetLastError() << "]; Closing the link" );
#else
            LOG ( L_ERROR, "Error writing to control link: " << strerror ( errno )
                  << " [" << errno << "]; Closing the link" );
#endif

            // No operations after this!
            linkClosed();
            return;
        }

        if ( ( size_t ) ret < packet.size() )
        {
            MemHandle remData = packet.getHandle ( ret );
            _writeQueue.prepend ( remData );

            LOG ( L_DEBUG4, "Not all data written. Reinserting a packet with " << remData.size()
                  << " bytes into the queue; New queue size: " << _writeQueue.size() );
        }
    }
}

bool CtrlLink::doCtrlRead()
{
    if ( !internalRead() )
    {
        LOG ( L_ERROR, "Closing control link with ID " << LinkId );

        // No operations after this!
        linkClosed();
        return false;
    }

    return true;
}

bool CtrlLink::internalRead()
{
    int rcvdFds = 0;

    LOG ( L_DEBUG4, "Reading data from link (ID " << LinkId << "; FD " << _linkFd
          << "; Initial read buffer size: " << _readBuffer.size() );

    bool nonFatal = false;
    const ssize_t readRet = osInternalRead ( rcvdFds, nonFatal );

    if ( readRet < 0 )
    {
        if ( nonFatal )
        {
            // Not really an error, we just can't read at this time...
            return true;
        }

#ifdef SYSTEM_WINDOWS
        LOG ( L_ERROR, "Error reading from control link: ["
              << WSAGetLastError() << "]" );
#else
        LOG ( L_ERROR, "Error reading from control link: " << strerror ( errno )
              << " [" << errno << "]" );
#endif

        return false;
    }
    else if ( readRet == 0 )
    {
        LOG ( L_ERROR, "Link with FD " << _linkFd << " was closed. So far " << _readFds.size()
              << " passed FDs have been received (including " << rcvdFds
              << " during this read), that now will be closed" );

        // The link has been closed from the other side. Even if we read any FDs we ignore them
        // (and let the caller of doCtrlRead() ) close them.
        return false;
    }

    assert ( readRet > 0 );

    _readBuffer.markAppended ( readRet );

#ifndef NO_LOGGING
    if ( _log.shouldLog ( L_DEBUG4 ) )
    {
        String descs;

        for ( size_t idx = 0; idx < _readFds.size(); ++idx )
        {
            if ( !descs.isEmpty() )
                descs.append ( ", " );

            descs.append ( String::number ( _readFds[ idx ] ) );
        }

        LOG ( L_DEBUG4, "Read " << readRet << " bytes ( and " << rcvdFds
              << " passed FDs; So far: [" << descs << "]) from link with ID " << LinkId
              << " and FD " << _linkFd << "; Current read buffer size: " << _readBuffer.size() );
    }
#endif

    MemHandle dataToRead = _readBuffer.getHandle();

    _readBuffer.clear();

    ExtProtoError extError;

    while ( true )
    {
        size_t offset = 0;
        ERRCODE eCode = Error::Unknown;

        LOG ( L_DEBUG4, "Trying to deserialize " << dataToRead.size() << " bytes" );

        Ctrl::Message msgData;

        extError.clear();
        eCode = msgData.deserializeWithLength ( dataToRead, offset, 0, &extError );

        if ( NOT_OK ( eCode ) )
        {
            if ( eCode != Error::IncompleteData )
            {
                LOG_ERR ( L_ERROR, eCode, "Error reading from the link: '" << extError.toString() << "'" );

                return false;
            }

            assert ( _readBuffer.isEmpty() );

            _readBuffer.append ( dataToRead );

            LOG ( L_DEBUG4, "Incomplete message from the link; Waiting for more data; "
                  "Not closing the descriptor; New read buffer size: " << _readBuffer.size() );

            // No error, just wait for more data.
            // We keep the file descriptors read so far around, until we have the complete message!
            return true;
        }

        if ( !msgData.hasType() )
        {
            // This should not happen, as 'type' is marked as required.

            LOG ( L_ERROR, "Deserialized a message with no type; Closing the link!" );

            return false;
        }

        LOG ( L_DEBUG3, "Deserialized " << offset << " bytes; Message type: "
              << msgData.getType() << "; Number of FDs: " << _readFds.size() );

        ERRCODE ret;

        if ( msgData.getIsSubRequest() )
        {
            extError.clear();
            Ctrl::SubscriptionRequest subReq;

            ret = subReq.deserialize ( msgData, &extError );

            if ( NOT_OK ( ret ) )
            {
                LOG_ERR ( L_ERROR, eCode, "Error deserializing message (type " << msgData.getType()
                          << ") as a SubscriptionRequest: '" << extError.toString() << "'" );
            }
            else
            {
                CtrlSubHandler * handler = 0;

                if ( _owner._subHandlers.find ( msgData.getType(), handler ) && handler != 0 )
                {
                    ret = handler->ctrlProcessSubRequest ( this, subReq );

                    LOG_ERR ( L_DEBUG2, ret, "Called processSubRequest in CtrlSubHandler for a message type "
                              << msgData.getType() );
                }
                else
                {
                    LOG ( L_DEBUG, "Received a SubscriptionRequest message (type " << msgData.getType()
                          << "), but no CtrlSubHandler is registered for this type; "
                          "Calling generic ctrlPacketReceived() instead" );

                    ret = _owner.ctrlPacketReceived ( LinkId, subReq, _readFds );

                    LOG_ERR ( L_DEBUG2, ret, "Called owner's ctrlPacketReceived() with a "
                              << msgData.getType() << " message" );
                }
            }
        }
        else
        {
            switch ( msgData.getType() )
            {
                case Ctrl::Ping::DEF_TYPE:
                case Ctrl::Pong::DEF_TYPE:
                case Ctrl::LogSubscribe::DEF_TYPE:
                case Ctrl::LogUnsubscribe::DEF_TYPE:
                case Ctrl::GetLogList::DEF_TYPE:
                case Ctrl::GetDiagnostics::DEF_TYPE:
                case Ctrl::LoadConfig::DEF_TYPE:
                case Ctrl::SimpleSubscriptionResponse::DEF_TYPE:
                    ret = processBuiltInMessage ( msgData, _readFds );

                    LOG_ERR ( L_DEBUG2, ret, "Called processBuiltInMessage() with a "
                              << msgData.getType() << " message" );
                    break;

                default:
                    ret = _owner.ctrlPacketReceived ( LinkId, msgData, _readFds );

                    LOG_ERR ( L_DEBUG2, ret, "Called owner's ctrlPacketReceived() with a "
                              << msgData.getType() << " message" );
                    break;
            }
        }

        if ( msgData.hasIsRequest()
             && msgData.getIsRequest()
             && ret != Error::ResponseSent
             && ret != Error::ResponsePending )
        {
            // We got a request message that hasn't been responded to yet (error code different than ResponseSent),
            // and will not be responded to by the handler in the future (error code different than ResponsePending).

            Ctrl::Request reqMsg;
            extError.clear();

            const ERRCODE tmpCode = reqMsg.deserialize ( msgData, &extError );

            if ( NOT_OK ( tmpCode ) )
            {
                // Something is very bad. We can't deserialize the request message!

                LOG_ERR ( L_ERROR, tmpCode, "Error deserializing request message after it was processed: '"
                          << extError.toString() << "'" );

                return false;
            }

            if ( NOT_OK ( ret )
                 || reqMsg.hasRequestId()
                 || ( reqMsg.hasRequestResponse() && reqMsg.getRequestResponse() ) )
            {
                // Either we experienced an error (other than ResponseSent),
                // or this request has 'request response' bit set,
                // or it includes 'request ID'.
                // We HAVE to respond to it!

                Ctrl::SimpleResponse resp;

                sendResponse ( resp, reqMsg, ret );
            }
        }

        // We managed to deserialize a complete message and pass the FDs to the owner.
        // It is possible that we have some unused FDs. They may be used by the next message,
        // since it's possible that FDs arrive with a different message than they were sent with.
        // In that case we want to keep them around (until there is a message that can use them),
        // but not too long.

        if ( !_readFds.isEmpty() && !_fdCleanupTimer.isActive() )
            _fdCleanupTimer.start ( 500 );

        if ( offset >= dataToRead.size() )
        {
            dataToRead.clear();
            return true;
        }

        // We want the remaining data to be in this MemHandle.
        // If it is complete it will be processed during next loop run.
        // If it is incomplete it will be reinserted into the buffer.
        // This is done this way (instead of consuming bytes in RwBuffer)
        // to avoid creating extra data copies!

        dataToRead = dataToRead.getHandle ( offset );

        LOG ( L_DEBUG4, "There is more data in the buffer. Remaining bytes: " << dataToRead.size() );
    }

    return true;
}

void CtrlLink::timerExpired ( Timer * timer )
{
    if ( timer == &_fdCleanupTimer )
    {
        LOG ( L_DEBUG3, "Running cleanupFds()" );

        cleanupFds();
    }
}

ERRCODE CtrlLink::sendResponse ( Ctrl::Update & resp, const Ctrl::Request & request, ERRCODE respErrCode )
{
    resp.setCode ( respErrCode );
    resp.setRequestType ( request.getType() );

    if ( request.hasRequestId() )
        resp.setRequestId ( request.getRequestId() );

    const ERRCODE eCode = sendPacket ( resp );

    if ( request.hasRequestId() )
    {
        LOG_ERR ( IS_OK ( eCode ) ? L_DEBUG2 : L_ERROR, eCode,
                  "Sending a response message with code " << respErrCode.toString()
                  << " as a response to request with type " << request.getType()
                  << " and ID " << request.getRequestId() << " to link with ID " << LinkId );
    }
    else
    {
        LOG_ERR ( IS_OK ( eCode ) ? L_DEBUG2 : L_ERROR, eCode,
                  "Sending a response message with code " << respErrCode.toString()
                  << " as a response to request with type " << request.getType()
                  << " and no ID to link with ID " << LinkId );
    }

    // We already sent the response - this way rcvCtrlPacket() (or function called by it)
    // can simply do: "return sendResponse(...);"
    return Error::ResponseSent;
}

ERRCODE CtrlLink::sendPing ( const String & payload )
{
    Ctrl::Ping msg;

    if ( !payload.isEmpty() )
    {
        msg.setPayload ( payload );
    }

    return sendPacket ( msg );
}

void CtrlLink::sendBinLog ( Log::LogMessage & logMessage )
{
    Ctrl::LogMessage ctrlMsg;
    ctrlMsg.setLogMessage ( logMessage );

    ERRCODE eCode = sendPacket ( ctrlMsg );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error sending Ctrl::LogMessage with LogMessage ("
                  << logMessage.getType().toString() << ")" );
        return;
    }
}

ERRCODE CtrlLink::processBuiltInMessage ( const Ctrl::Message & msg, List<int> & receivedFds )
{
    if ( !msg.hasType() )
        return Error::InvalidParameter;

    // CASE_CTRL_MSG_TYPE macro assumes this is available
    const int linkId = LinkId;

    switch ( msg.getType() )
    {
        CASE_CTRL_MSG_TYPE ( Ping );
        CASE_CTRL_MSG_TYPE ( Pong );
        CASE_CTRL_MSG_TYPE ( GetLogList );
        CASE_CTRL_MSG_TYPE ( LogSubscribe );
        CASE_CTRL_MSG_TYPE ( LogUnsubscribe );
        CASE_CTRL_MSG_TYPE ( GetDiagnostics );
        CASE_CTRL_MSG_TYPE ( LoadConfig );
        CASE_CTRL_MSG_TYPE_FDS ( SimpleSubscriptionResponse );

        default:
            LOG ( L_ERROR, "Received an unsupported '" << msg.getType() << "' control packet" );

            return Error::Unsupported;
            break;
    }
}

ERRCODE CtrlLink::handleCtrlMessage ( int /*linkId*/, const Ctrl::Ping & msg )
{
    Ctrl::Pong resp;

    if ( msg.hasPayload() )
    {
        resp.setPayload ( msg.getPayload() );
    }

    return sendResponse ( resp, msg, Error::Success );
}

ERRCODE CtrlLink::handleCtrlMessage ( int linkId, const Ctrl::Pong & msg )
{
    processPong ( linkId, msg );
    return Error::Success;
}

void CtrlLink::processPong ( int /*linkId*/, const Ctrl::Pong & /*msg*/ )
{
}

ERRCODE CtrlLink::handleCtrlMessage ( int /* clientId */, const Ctrl::GetLogList & msg )
{
    Ctrl::LogListResp resp;

    String pattern;

    if ( msg.hasNamePattern() )
        pattern = msg.getNamePattern();

    StringList textLogs = LogManager::get().getRegisteredTextLogs();

    if ( pattern.isEmpty() )
    {
        resp.modTextLogs() = textLogs;
    }
    else
    {
        for ( size_t idx = 0; idx < textLogs.size(); ++idx )
        {
            if ( textLogs.at ( idx ).matches ( pattern ) )
                resp.modTextLogs().append ( textLogs.at ( idx ) );
        }
    }

    List<LogManager::LogDesc> binLogs = LogManager::get().getRegisteredBinLogs();

    for ( size_t idx = 0; idx < binLogs.size(); ++idx )
    {
        if ( pattern.isEmpty() || binLogs.at ( idx ).logName.matches ( pattern ) )
        {
            Ctrl::LogListResp::LogDesc lDesc;

            lDesc.setName ( binLogs.at ( idx ).logName );
            lDesc.setType ( binLogs.at ( idx ).logType );

            resp.modBinLogs().append ( lDesc );
        }
    }

    return sendResponse ( resp, msg );
}

ERRCODE CtrlLink::handleCtrlMessage ( int /* clientId */, const Ctrl::LogSubscribe & msg )
{
    if ( !msg.hasNamePattern() )
        return Error::InvalidParameter;

    if ( msg.hasLevel() )
    {
        if ( !LogManager::get().subscribe ( msg.getNamePattern(), msg.getLevel(), this ) )
            return Error::NotFound;
    }
    else
    {
        if ( !LogManager::get().subscribe ( msg.getNamePattern(), this ) )
            return Error::NotFound;
    }

    // We REALLY don't want to get updates about the CtrlLink itself!
    // Subscribing a CtrlLink to this log at DEBUG level would cause infinite recursion and crashes!
    // We can't unsubscribe from DEBUG (and lower) only, but having any logs from CtrlLink directed to CtrlLink
    // could cause problems anyway. So let's simply completely unsubscribe from CtrlLink's log.
    // We don't really know if we were subscribed, but if we weren't this simply won't do anything...

    _log.unsubscribeOutput ( this );

    return Error::Success;
}

ERRCODE CtrlLink::handleCtrlMessage ( int /* clientId */, const Ctrl::LogUnsubscribe & msg )
{
    String pattern;

    if ( msg.hasNamePattern() )
        pattern = msg.getNamePattern();

    LogManager::get().unsubscribe ( this, pattern );

    return Error::Success;
}

ERRCODE CtrlLink::handleCtrlMessage ( int /* clientId */, const Ctrl::GetDiagnostics & msg )
{
    const HashMap<String, Log::Diagnostic> & diags = Diagnostics::get().getDiagnostics();

    Ctrl::DiagnosticsResponse resp;

    for ( HashMap<String, Log::Diagnostic>::Iterator it ( diags ); it.isValid(); it.next() )
    {
        LOG ( L_DEBUG4, "Adding diagnostic response entry: " << it.value().getKey()
              << "='" << it.value().getValue() << "' [" << it.value().getTimestamp() << "]" );

        resp.modDiagnostics().append ( it.value() );
    }

    return sendResponse ( resp, msg, Error::Success );
}

ERRCODE CtrlLink::handleCtrlMessage ( int /* clientId */, const Ctrl::LoadConfig & msg )
{
    uint8_t opts = 0;

    if ( msg.getIgnoreErrors() )
    {
        opts |= ConfigCore::LoadOptIgnoreErrors;
    }

    if ( msg.getRestoreDefaultsFirst() )
    {
        opts |= ConfigCore::LoadOptRestoreDefaultsFirst;
    }

    const String & data = msg.getConfigData();

    LOG ( L_DEBUG4, "Loading config data; opts: 0x" << String::number ( opts, String::Int_HEX )
          << "; configData: '\n" << data << "\n'" );

    String errorMsg;
    const ERRCODE eCode = ConfigCore::get().loadConfigData ( data, opts, errorMsg );

    Ctrl::LoadConfigResponse resp;

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error loading config contents: " << errorMsg );

        resp.setErrorMessage ( errorMsg );
    }

    return sendResponse ( resp, msg, eCode );
}

ERRCODE CtrlLink::handleCtrlMessage ( int cId, Ctrl::SimpleSubscriptionResponse & msg, List<int> & rcvFds )
{
    const ERRCODE ret = _owner.ctrlSubscriptionResponseReceived ( cId, msg, rcvFds );

    LOG_ERR ( IS_OK ( ret ) ? L_DEBUG2 : L_ERROR, ret,
              "Called ctrlSubscriptionResponseReceived() in the owner; Link ID: " << cId );

    return ret;
}
