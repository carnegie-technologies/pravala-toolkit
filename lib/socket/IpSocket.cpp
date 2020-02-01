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

#ifndef SYSTEM_WINDOWS
extern "C"
{
#include <net/if.h>
}
#endif

#include "IpSocket.hpp"

using namespace Pravala;

IpSocket::IpSocket ( SocketOwner * owner ): Socket ( owner )
{
}

TcpSocket * IpSocket::getTcpSocket()
{
    return 0;
}

UdpSocket * IpSocket::getUdpSocket()
{
    return 0;
}

IpSocket * IpSocket::getIpSocket()
{
    return this;
}

String IpSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "%1-%2" ).arg ( getLocalDesc(), getRemoteDesc() );
    }

    return getRemoteDesc();
}

String IpSocket::getLocalDesc() const
{
    return getLocalSockAddr().toString();
}

String IpSocket::getRemoteDesc() const
{
    return getRemoteSockAddr().toString();
}

bool IpSocket::ipSockInitFd ( SocketApi::SocketType type, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr )
{
    if ( sockFd >= 0 )
        return true;

    if ( !sockInitFd ( type, sockFd ) || sockFd < 0 )
        return false;

    // In case this object was previously used (but is not anymore, since sockFd < 0),
    // we want to clear the addresses. We don't want to clear them in close() to allow the owner to check
    // the addresses inside sockClosed() callback.

    localAddr.clear();
    remoteAddr.clear();

    return true;
}

ERRCODE IpSocket::ipSockBind ( const SockAddr & addr, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr )
{
    if ( !addr.hasIpAddr() )
        return Error::InvalidParameter;

    if ( !ipSockInitFd ( ipSockGetType ( addr ), sockFd, localAddr, remoteAddr ) )
    {
        return Error::SocketFailed;
    }

    if ( localAddr.hasIpAddr() )
    {
        // If this is a newly-initialized socket, localAddr would have been cleared by sockInitFd().
        return Error::AlreadyInitialized;
    }

    if ( !SocketApi::bind ( sockFd, addr ) )
    {
        LOG ( L_ERROR, getLogId() << ": Error binding socket with FD " << sockFd
              << " to local address " << addr << "; Error: " << SocketApi::getLastErrorDesc() );

        return Error::BindFailed;
    }

    if ( !SocketApi::getName ( sockFd, localAddr ) || !localAddr.hasIpAddr() )
    {
        LOG ( L_ERROR, getLogId() << ": Error reading local name of socket with FD " << sockFd
              << " after binding it to address " << addr << "; Error: " << SocketApi::getLastErrorDesc() );

        return Error::BindFailed;
    }

    return Error::Success;
}

ERRCODE IpSocket::ipSockBindToIface (
        SocketApi::SocketType sockType, const String & ifaceName,
        int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr )
{
    if ( ifaceName.isEmpty() )
        return Error::InvalidParameter;

    if ( !ipSockInitFd ( sockType, sockFd, localAddr, remoteAddr ) )
    {
        return Error::SocketFailed;
    }

    const ERRCODE eCode = SocketApi::bindToIface ( sockFd, sockType, ifaceName );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId() << ": Error binding socket with FD " << sockFd
                  << " to IfaceName: '" << ifaceName << "': " << SocketApi::getLastErrorDesc() );
    }

    return eCode;
}

ERRCODE IpSocket::ipSockConnect ( const SockAddr & addr, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr )
{
    if ( !addr.hasIpAddr() || !addr.hasPort() )
        return Error::InvalidParameter;

    if ( !ipSockInitFd ( ipSockGetType ( addr ), sockFd, localAddr, remoteAddr ) )
    {
        return Error::SocketFailed;
    }

    if ( remoteAddr.hasIpAddr() )
    {
        // If this is a newly-initialized socket, remoteAddr would have been cleared by sockInitFd().
        return Error::AlreadyInitialized;
    }

    const ERRCODE connectCode = SocketApi::connect ( sockFd, addr );

    if ( NOT_OK ( connectCode ) )
    {
        LOG_ERR ( L_ERROR, connectCode, getLogId() << ": Error connecting socket with FD " << sockFd
                  << " to address " << addr << "; Error: " << SocketApi::getLastErrorDesc() );

        return connectCode;
    }

    if ( !localAddr.hasIpAddr() && ( !SocketApi::getName ( sockFd, localAddr ) || !localAddr.hasIpAddr() ) )
    {
        LOG ( L_ERROR, getLogId() << ": Error reading local name of socket with FD " << sockFd
              << " after connecting it to address " << addr << "; Error: " << SocketApi::getLastErrorDesc() );

        return Error::ConnectFailed;
    }

    remoteAddr = addr;
    setFlags ( SockFlagConnecting );

    return connectCode;
}
