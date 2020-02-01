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

#include "Socks5VersionMessage.hpp"
#include "Socks5MethodSelectionMessage.hpp"
#include "Socks5ReplyMessage.hpp"

#include "Socks5ServerSocket.hpp"

#include "../Socks5Server.hpp"

using namespace Pravala;

Socks5ServerSocket::Socks5ServerSocket ( Socks5Server * owner, TcpSocket * clientSock ):
    Socks5TcpBaseSocket ( owner, clientSock ),
    _cancelTimer ( *this ),
    _socks5Server ( owner )
{
}

Socks5ServerSocket::~Socks5ServerSocket()
{
    close();
}

ERRCODE Socks5ServerSocket::bind ( const SockAddr & )
{
    return Error::Unsupported;
}

ERRCODE Socks5ServerSocket::connect ( const SockAddr & )
{
    return Error::Unsupported;
}

void Socks5ServerSocket::ownerChanged()
{
    _socks5Server = 0;
}

void Socks5ServerSocket::setOwner ( Socks5Server * owner )
{
    Socks5TcpBaseSocket::setOwner ( owner );

    _socks5Server = owner;
}

void Socks5ServerSocket::refOwner ( Socks5Server * owner )
{
    Socks5TcpBaseSocket::refOwner ( owner );

    if ( owner != 0 )
    {
        _socks5Server = owner;
    }
}

String Socks5ServerSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "S5_SRV-TCP:[%2-%3,%1]" )
               .arg ( _sockFd ).arg ( _localAddr.toString(), _remoteAddr.toString() );
    }

    return String ( "S5_SRV-TCP:%1" ).arg ( _remoteAddr.toString() );
}

void Socks5ServerSocket::readyToSendSocks5Data()
{
    const uint16_t state = getState();

    if ( state == SockSocks5ServerStateValueCancelled )
    {
        // We have been cancelled - we can now safely close the socket!

        if ( IS_OK ( _cancelReason ) )
        {
            _cancelReason = Error::Unknown;
        }

        LOG ( L_DEBUG, getLogId()
              << ": No more data to send in 'cancelled' state; Sending 'connect failed' event with code: "
              << _cancelReason );

        doSockConnectFailed ( _cancelReason );
        return;
    }

    if ( state == SockSocks5ServerStateValueTcpConnected )
    {
        LOG ( L_DEBUG3, getLogId()
              << ": No more SOCKS5 data to send in 'TCP connected' state; Sending 'connected' event" );

        // We can send this event right away, nothing will happen after readyToSendSocks5Data() anyway.

        setFlags ( SockSocks5TcpBaseFlagHandshakeCompleted );
        doSockConnected();
    }
}

void Socks5ServerSocket::cancelSocks5 ( ERRCODE reason )
{
    if ( getState() == SockSocks5ServerStateValueCancelled || isSocks5Connected() )
    {
        // We don't want to do anything if we are already cancelled, or SOCKS5 handshake has been completed.
        return;
    }

    // SOCKS5 RFC specifies that the socket should be closed no more than 10 seconds after detecting
    // the condition that caused a failure.

    LOG_ERR ( L_DEBUG, reason, getLogId()
              << ": SOCKS5 connection has been cancelled; The socket is scheduled to close in 10 seconds" );

    _cancelTimer.start ( 10 * 1000 );
    _cancelReason = reason;
    _readBuf.clear();

    if ( _sockFd >= 0 )
    {
        EventManager::disableReadEvents ( _sockFd );
    }

    setState ( SockSocks5ServerStateValueCancelled );
}

void Socks5ServerSocket::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer == &_cancelTimer );

    LOG ( L_DEBUG, getLogId() << ": Timing-out a socket; Current state: " << getStateName ( getState() ) );

    doSockConnectFailed ( IS_OK ( _cancelReason ) ? ( Error::Unknown ) : _cancelReason );
}

void Socks5ServerSocket::close()
{
    _cancelTimer.stop();

    Socks5TcpBaseSocket::close();
}

void Socks5ServerSocket::doSockClosed ( ERRCODE reason )
{
    _cancelTimer.stop();

    Socks5TcpBaseSocket::doSockClosed ( reason );
}

void Socks5ServerSocket::doSockConnectFailed ( ERRCODE reason )
{
    _cancelTimer.stop();

    Socks5TcpBaseSocket::doSockConnectFailed ( reason );
}

size_t Socks5ServerSocket::receiveSocks5Data()
{
    size_t reqBytes = 0;
    const uint16_t state = getState();

    Socks5ReplyMessage::Reply replyCode = Socks5ReplyMessage::ReplyGeneralSocksServerFailure;

    switch ( state )
    {
        case SockSocks5ServerStateValueReceivingVersion:
            replyCode = receiveVersion ( reqBytes );
            break;

        case SockSocks5ServerStateValueReceivingRequest:
            replyCode = receiveRequest ( reqBytes );
            break;

        case SockSocks5ServerStateValueAwaitingTcpConnection:
        case SockSocks5ServerStateValueAwaitingUdpAssociation:
        case SockSocks5ServerStateValueAwaitingBoundTcpListener:
        case SockSocks5ServerStateValueAwaitingInboundTcpConnection:
        case SockSocks5ServerStateValueTcpConnected:
        case SockSocks5ServerStateValueUdpAssociated:
        case SockSocks5ServerStateValueTcpBound:
            LOG ( L_ERROR, getLogId() << ": Wrong state to receive SOCKS5 data: " << getStateName ( state ) );

            replyCode = Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
            break;

        case SockSocks5ServerStateValueCancelled:
            LOG ( L_ERROR, getLogId() << ": Socket has been cancelled; Ignoring SOCKS5 data" );
            return 0;
            break;

        default:
            LOG ( L_ERROR, getLogId() << ": Unsupported state: " << getStateName ( state ) );
            break;
    }

    if ( replyCode == Socks5ReplyMessage::ReplySuccess )
    {
        if ( reqBytes > 0 )
        {
            LOG ( L_DEBUG4, getLogId() << ": We need " << reqBytes << " more byte(s) of data; Current state: "
                  << getStateName ( state ) );
        }

        return reqBytes;
    }

    LOG ( L_ERROR, getLogId() << ": Error processing incoming data (current state: "
          << getStateName ( state ) << "); ReplyCode: " << replyCode
          << "; Sending error response and cancelling the socket" );

    if ( !sendSocks5Reply ( replyCode ) )
    {
        LOG ( L_ERROR, getLogId() << ": Could not send error reply (with code " << replyCode << ")" );
    }

    cancelSocks5 ( Error::ProtocolError );
    return 0;
}

Socks5ReplyMessage::Reply Socks5ServerSocket::receiveVersion ( size_t & reqBytes )
{
    LOG ( L_DEBUG3, getLogId() << ": Receiving SOCKS5 version identifier / method selection message" );

    Socks5VersionMessage msg;

    const ERRCODE eCode = msg.parseAndConsume ( _readBuf, reqBytes );

    if ( eCode == Error::IncompleteData )
    {
        return Socks5ReplyMessage::ReplySuccess;
    }
    else if ( NOT_OK ( eCode ) )
    {
        return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
    }

    Socks5Message::AuthenticationMethod method;

    // Current implementation only supports 'no authentication required'.
    if ( msg.containsAuthMethod ( Socks5Message::AuthNoneRequired ) )
    {
        method = Socks5Message::AuthNoneRequired;

        LOG ( L_DEBUG2, getLogId() << ": Using 'no-auth' method" );
    }
    else
    {
        method = Socks5Message::AuthNoneAcceptable;

        LOG ( L_DEBUG2, getLogId() << ": No acceptable authentication methods" );
    }

    if ( !sendSocks5MethodSelect ( method ) )
    {
        LOG ( L_ERROR, getLogId() << ": Could not send SOCKS5 method select message; Method: " << method );

        return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
    }

    if ( method == Socks5Message::AuthNoneAcceptable )
    {
        // We need to 'cancel' socket here since we handled the error internally and we are returning 'success'
        // status code.
        cancelSocks5 ( Error::ProtocolError );
    }
    else
    {
        LOG ( L_DEBUG3, getLogId() << ": New state: Waiting for client to send SOCKS5 request" );

        setState ( SockSocks5ServerStateValueReceivingRequest );
    }

    return Socks5ReplyMessage::ReplySuccess;
}

Socks5ReplyMessage::Reply Socks5ServerSocket::receiveRequest ( size_t & reqBytes )
{
    LOG ( L_DEBUG3, getLogId() << ": Receiving SOCKS5 request message" );

    Socks5RequestMessage msg;

    const ERRCODE eCode = msg.parseAndConsume ( _readBuf, reqBytes );

    if ( eCode == Error::IncompleteData )
    {
        return Socks5ReplyMessage::ReplySuccess;
    }
    else if ( NOT_OK ( eCode ) )
    {
        return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
    }

    const uint8_t cmd = msg.getCommand();

    const SockAddr destAddr ( msg.getAddress(), msg.getPort() );

    switch ( ( Socks5RequestMessage::Command ) cmd )
    {
        case Socks5RequestMessage::CmdTcpConnect:
            if ( !_socks5Server )
            {
                LOG ( L_ERROR, getLogId() << ": SOCKS5 proxy server is not set" );

                return Socks5ReplyMessage::ReplyGeneralSocksServerFailure;
            }
            else
            {
                const Socks5ReplyMessage::Reply replyCode = _socks5Server->socks5TcpConnectRequested ( this, destAddr );

                if ( replyCode != Socks5ReplyMessage::ReplySuccess )
                {
                    LOG ( L_ERROR, getLogId() << ": Error handling a request to open a TCP connection to "
                          << destAddr << "; Reply code: " << replyCode );

                    return replyCode;
                }

                LOG ( L_DEBUG2, getLogId()
                      << ": Waiting for proxy server to open a TCP connection to " << destAddr );

                setState ( SockSocks5ServerStateValueAwaitingTcpConnection );
                return replyCode;
            }
            break;

        case Socks5RequestMessage::CmdUDPAssociate:
            LOG ( L_ERROR, getLogId() << ": SOCKS5 client requested associated UDP socket; We do not support it" );

            setState ( SockSocks5ServerStateValueAwaitingUdpAssociation );

            return Socks5ReplyMessage::ReplyCommandNotSupported;
            break;

        case Socks5RequestMessage::CmdTcpBind:
            LOG ( L_ERROR, getLogId() << ": SOCKS5 client requested listening TCP socket; We do not support it" );

            setState ( SockSocks5ServerStateValueAwaitingBoundTcpListener );

            return Socks5ReplyMessage::ReplyCommandNotSupported;
            break;
    }

    LOG ( L_DEBUG, getLogId() << ": Received invalid command:" << cmd );

    return Socks5ReplyMessage::ReplyCommandNotSupported;
}

bool Socks5ServerSocket::sendSocks5MethodSelect ( Socks5Message::AuthenticationMethod method )
{
    if ( getState() != SockSocks5ServerStateValueReceivingVersion )
    {
        LOG ( L_ERROR, getLogId() << ": Wrong state: " << getState() );
        return false;
    }

    LOG ( L_DEBUG2, getLogId() << ": Sending SOCKS5 method select message" );

    Socks5MethodSelectionMessage msg ( method );

    if ( !msg.isValid() )
    {
        LOG ( L_ERROR, getLogId() << ": Invalid method select message generated; Method: " << method );

        return false;
    }

    return sendSocks5Data ( msg.getData() );
}

bool Socks5ServerSocket::sendSocks5Reply ( Socks5ReplyMessage::Reply replyCode, const SockAddr & addr )
{
    LOG ( L_DEBUG2, getLogId()
          << ": Sending SOCKS5 reply message; Reply code: " << replyCode << "; Address: " << addr );

    Socks5ReplyMessage msg (
            replyCode, ( replyCode == Socks5ReplyMessage::ReplySuccess ) ? addr : Ipv4ZeroSockAddress );

    if ( !msg.isValid() )
    {
        LOG ( L_ERROR, getLogId()
              << ": Invalid reply message generated; Reply code: " << replyCode << "; Address: " << addr );

        return false;
    }

    return sendSocks5Data ( msg.getData() );
}

ERRCODE Socks5ServerSocket::handledTcpConnect ( Socks5ReplyMessage::Reply result, const SockAddr & addr )
{
    if ( getState() != SockSocks5ServerStateValueAwaitingTcpConnection )
    {
        LOG ( L_ERROR, getLogId() << ": Unexpected state: " << getState() );

        cancelSocks5 ( Error::WrongState );
        return Error::WrongState;
    }

    if ( result == Socks5ReplyMessage::ReplySuccess && !addr.isIPv4() && !addr.isIPv6() )
    {
        LOG ( L_ERROR, getLogId() << ": Invalid address received: " << addr );

        cancelSocks5 ( Error::InvalidAddress );
        return Error::InvalidAddress;
    }

    if ( !sendSocks5Reply ( result, addr ) )
    {
        cancelSocks5 ( Error::InternalError );
        return Error::InternalError;
    }

    if ( result == Socks5ReplyMessage::ReplySuccess )
    {
        LOG ( L_DEBUG3, getLogId()
              << ": Successfully handled SOCKS5 'TCP connect' request; Waiting for the reply to be sent" );

        setState ( SockSocks5ServerStateValueTcpConnected );
        return Error::Success;
    }

    cancelSocks5 ( Error::ConnectFailed );
    return Error::ConnectFailed;
}

#define CASE_STATE_NAME( s ) \
    case SockSocks5ServerStateValue ## s: \
        stateName = #s; break

String Socks5ServerSocket::getStateName ( uint16_t state )
{
    const char * stateName = "Invalid";

    switch ( state )
    {
        CASE_STATE_NAME ( ReceivingVersion );
        CASE_STATE_NAME ( ReceivingRequest );
        CASE_STATE_NAME ( AwaitingTcpConnection );
        CASE_STATE_NAME ( AwaitingUdpAssociation );
        CASE_STATE_NAME ( AwaitingBoundTcpListener );
        CASE_STATE_NAME ( AwaitingInboundTcpConnection );
        CASE_STATE_NAME ( TcpConnected );
        CASE_STATE_NAME ( UdpAssociated );
        CASE_STATE_NAME ( TcpBound );
        CASE_STATE_NAME ( Cancelled );
    }

    return String ( "%1 [%2]" ).arg ( stateName, String::number ( state ) );
}
