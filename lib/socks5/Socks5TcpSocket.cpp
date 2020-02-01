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

#include "internal/Socks5VersionMessage.hpp"
#include "internal/Socks5MethodSelectionMessage.hpp"
#include "internal/Socks5ReplyMessage.hpp"

#include "Socks5TcpSocket.hpp"

using namespace Pravala;

Socks5TcpSocket::Socks5TcpSocket ( SocketOwner * owner, const SockAddr & proxyAddr ):
    Socks5TcpBaseSocket ( owner ),
    _proxyAddr ( proxyAddr )
{
}

String Socks5TcpSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "S5-TCP:[%2-%3(%4),%1]" )
               .arg ( _sockFd ).arg ( _localAddr.toString(), _remoteAddr.toString(), _proxyAddr.toString() );
    }

    return String ( "S5-TCP:%1(%2)" ).arg ( _remoteAddr.toString(), _proxyAddr.toString() );
}

ERRCODE Socks5TcpSocket::connect ( const SockAddr & addr )
{
    if ( !addr.hasIpAddr() || !addr.hasPort() )
    {
        return Error::InvalidParameter;
    }

    // Instead of connecting to 'addr' we connect to proxy address.
    //
    // We still want to pass _remoteAddr as the current remote address - to fail if socket FD already exists
    // and the remote address is valid (see IpSocket::ipSockConnect()).
    // On success _remoteAddr will be set to _proxyAddr, but we will simply replace it.

    const ERRCODE eCode = IpSocket::ipSockConnect ( _proxyAddr, _sockFd, _localAddr, _remoteAddr );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    // We succeeded. But we want our remote address to point at the final destination, not the proxy:
    _remoteAddr = addr;

    // Needs both read events (to determine connection failure) and write events (to determine connection success)
    EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead | EventManager::EventWrite );

    LOG ( L_DEBUG, getLogId ( true )
          << ": Successfully started connection attempt; Internal connect result: " << eCode );

    // We do NOT return the actual code, which could be 'connect in progress'.
    // This API always returns success. But this 'success' always means 'in progress' even if the
    // connection is already established. 'connected' callback will be generated regardless.

    return Error::Success;
}

void Socks5TcpSocket::readyToSendSocks5Data()
{
    if ( getFlagValue ( SockSocks5TcpStateFlagMask ) == SockSocks5TcpStateValueNotConnected )
    {
        LOG ( L_DEBUG2, getLogId() << ": Sending SOCKS5 version identifier / method selection" );

        List<uint8_t> authMethods;
        authMethods.append ( Socks5Message::AuthNoneRequired );

        Socks5VersionMessage msg ( authMethods );

        if ( !msg.isValid() )
        {
            LOG ( L_ERROR, getLogId() << ": Invalid version message generated" );
        }
        else if ( !sendSocks5Data ( msg.getData() ) )
        {
            LOG ( L_ERROR, getLogId() << ": Error sending SOCKS5 version identifier / method selection" );
        }
        else
        {
            setFlagValue ( SockSocks5TcpStateValueAwaitingMethod, SockSocks5TcpStateFlagMask );

            LOG ( L_DEBUG3, getLogId() << ": New state: Awaiting SOCKS5 method selection" );

            return;
        }

        // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
        doSockConnectFailed ( Error::ProtocolError );
    }
}

size_t Socks5TcpSocket::receiveSocks5Data()
{
    size_t reqBytes = 0;
    ERRCODE eCode = Error::Success;

    const uint16_t state = getFlagValue ( SockSocks5TcpStateFlagMask );

    switch ( state )
    {
        case SockSocks5TcpStateValueNotConnected:
            LOG ( L_ERROR, getLogId() << ": We are not expecting any SOCKS5 data yet" );
            eCode = Error::ProtocolError;
            break;

        case SockSocks5TcpStateValueAwaitingMethod:
            eCode = receiveMethodSelect ( reqBytes );
            break;

        case SockSocks5TcpStateValueAwaitingReply:
            eCode = receiveReply ( reqBytes );
            break;

        default:
            LOG ( L_ERROR, getLogId() << ": Invalid state (" << state << ")" );
            eCode = Error::InternalError;
            break;
    }

    if ( eCode == Error::IncompleteData )
    {
        LOG ( L_DEBUG4, getLogId() << ": We need " << reqBytes << " byte(s) of data more" );

        return reqBytes;
    }

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId() << ": Error processing incoming data; Closing the socket" );

        // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
        doSockConnectFailed ( eCode );
        return 0;
    }

    return 0;
}

ERRCODE Socks5TcpSocket::receiveMethodSelect ( size_t & reqBytes )
{
    LOG ( L_DEBUG3, getLogId() << ": Receiving SOCKS5 method selection" );

    Socks5MethodSelectionMessage msg;

    ERRCODE eCode = msg.parseAndConsume ( _readBuf, reqBytes );

    if ( NOT_OK ( eCode ) )
        return eCode;

    // Current implementation only supports no authentication required
    if ( Socks5Message::AuthNoneRequired != msg.getAuthenticationMethod() )
    {
        LOG ( L_ERROR, getLogId()
              << ": Unsupported SOCKS5 method selected: " << msg.getAuthenticationMethod() );

        return Error::ProtocolError;
    }

    LOG ( L_DEBUG2, getLogId() << ": Using " << msg.getAuthenticationMethod() << " for authentication" );

    if ( !sendSocks5Request() )
    {
        LOG ( L_ERROR, getLogId() << ": Could not send 'SOCKS5 Request' message" );

        return Error::InternalError;
    }

    return Error::Success;
}

ERRCODE Socks5TcpSocket::receiveReply ( size_t & reqBytes )
{
    LOG ( L_DEBUG3, getLogId() << ": Receiving SOCKS5 reply" );

    Socks5ReplyMessage msg;

    ERRCODE eCode = msg.parseAndConsume ( _readBuf, reqBytes );

    if ( NOT_OK ( eCode ) )
        return eCode;

    switch ( msg.getReply() )
    {
        case Socks5ReplyMessage::ReplySuccess:
            {
                assert ( _readBuf.isEmpty() );

                if ( !_readBuf.isEmpty() )
                {
                    // Read buffer should really be empty at this point...
                    LOG ( L_ERROR, getLogId() << ": Read buffer not empty after parsing SOCKS5 reply" );
                    return Error::InternalError;
                }

                const uint8_t aType = msg.getAddressType();

                socks5RequestSucceeded ( ( aType == Socks5Message::AddrIPv4 || aType == Socks5Message::AddrIPv6 )
                                         ? SockAddr ( msg.getAddress(), msg.getPort() )
                                         : EmptySockAddress );
                return Error::Success;
            }
            break;

        case Socks5ReplyMessage::ReplyCommandNotSupported:
            LOG ( L_ERROR, getLogId() << ": Requested command not supported by the proxy server" );
            return Error::Unsupported;
            break;

        case Socks5ReplyMessage::ReplyAddressTypeNotSupported:
            LOG ( L_ERROR, getLogId()
                  << ": Address type (of " << _remoteAddr << ") is not supported by the proxy server" );
            return Error::InvalidAddress;
            break;
    }

    LOG ( L_ERROR, getLogId()
          << ": Proxy server could not complete the request; Reply code: " << msg.getReply() );

    return Error::ProtocolError;
}

void Socks5TcpSocket::socks5RequestSucceeded ( const SockAddr & addr )
{
    ( void ) addr;

    // All good! We are now "connected"!
    setFlags ( SockSocks5TcpBaseFlagHandshakeCompleted );

    EventManager::setFdEvents ( _sockFd, EventManager::EventRead | EventManager::EventWrite );

    // We don't use the 'bind address' received from the server at the moment...

    LOG ( L_DEBUG, getLogId()
          << ": Successfully completed SOCKS5 negotiation; Bind address: " << addr );

    doSockConnected();
}

bool Socks5TcpSocket::sendSocks5Request ( Socks5RequestMessage::Command reqCommand, const SockAddr & addr )
{
    if ( getFlagValue ( SockSocks5TcpStateFlagMask ) != SockSocks5TcpStateValueAwaitingMethod )
    {
        LOG ( L_ERROR, getLogId() << ": Wrong state: " << getFlagValue ( SockSocks5TcpStateFlagMask ) );
        return false;
    }

    LOG ( L_DEBUG2, getLogId() << ": Sending SOCKS5 request; Command: " << reqCommand << "; Address: " << addr );

    Socks5RequestMessage msg ( reqCommand, addr );

    if ( !msg.isValid() )
    {
        LOG ( L_ERROR, "Invalid request generated; Command: " << reqCommand << "; Address: " << addr );
        return false;
    }

    if ( !sendSocks5Data ( msg.getData() ) )
        return false;

    setFlagValue ( SockSocks5TcpStateValueAwaitingReply, SockSocks5TcpStateFlagMask );

    LOG ( L_DEBUG3, getLogId()
          << ": New state: Awaiting SOCKS5 reply; Command: " << reqCommand << "; Address: " << addr );

    return true;
}

bool Socks5TcpSocket::sendSocks5Request()
{
    if ( !_remoteAddr.hasIpAddr() || !_remoteAddr.hasPort() )
    {
        LOG ( L_ERROR, getLogId() << ": Remote address is invalid: " << _remoteAddr );
        return false;
    }

    return sendSocks5Request ( Socks5RequestMessage::CmdTcpConnect, _remoteAddr );
}
