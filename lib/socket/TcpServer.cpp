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
#include <sys/types.h>
}

#include "sys/OsUtils.hpp"
#include "sys/SocketApi.hpp"

#include "TcpFdSocket.hpp"
#include "TcpServer.hpp"

using namespace Pravala;

TextLog TcpServer::_log ( "tcp_server" );

void TcpServer::Owner::incomingUnixConnection (
        TcpServer * tcpServer, uint8_t extraData, int sockFd, const String & sockName )
{
    ( void ) tcpServer;
    ( void ) extraData;
    ( void ) sockName;

    LOG ( L_FATAL_ERROR, "Unimplemented UNIX connection callback; Name: '"
          << sockName << "'; FD: " << sockFd << "; Closing the socket" );

    SocketApi::close ( sockFd );
}

void TcpServer::Owner::incomingTcpConnection (
        TcpServer * tcpServer, uint8_t extraData, int sockFd,
        const IpAddress & localAddr, uint16_t localPort,
        const IpAddress & remoteAddr, uint16_t remotePort )
{
    ( void ) tcpServer;
    ( void ) extraData;
    ( void ) localAddr;
    ( void ) localPort;
    ( void ) remoteAddr;
    ( void ) remotePort;

    LOG ( L_FATAL_ERROR, "Unimplemented legacy TCP connection callback; Local: "
          << localAddr << ":" << localPort << "; Remote: " << remoteAddr << ":" << remotePort
          << "; FD: " << sockFd << "; Closing the socket" );

    SocketApi::close ( sockFd );
}

void TcpServer::Owner::incomingTcpConnection ( TcpServer * tcpServer, uint8_t extraData, TcpSocket * socket )
{
    assert ( socket != 0 );

    if ( !socket )
        return;

    const int sockFd = socket->stealSockFd();

    if ( sockFd < 0 )
        return;

    const SockAddr localAddr = socket->getLocalSockAddr();
    const SockAddr remoteAddr = socket->getRemoteSockAddr();

    incomingTcpConnection (
        tcpServer, extraData, sockFd,
        localAddr.getAddr(), localAddr.getPort(),
        remoteAddr.getAddr(), remoteAddr.getPort() );
}

TcpServer::TcpServer ( Owner & owner ):
    _owner ( owner )
{
}

TcpServer::~TcpServer()
{
    closeListeners();
}

void TcpServer::closeListeners()
{
    for ( HashMap<int, ListenerData>::Iterator it ( _listeners ); it.isValid(); it.next() )
    {
        int fd = it.key();

        if ( fd < 0 )
            continue;

#ifndef NO_LOGGING
        const ListenerData & lData = it.value();

        if ( _log.shouldLog ( L_DEBUG ) )
        {
            if ( lData.name.length() > 0 )
            {
                LOG ( L_DEBUG, "'" << lData.name << "': Closing listening UNIX socket" );
            }
            else
            {
                LOG ( L_DEBUG, lData.addr << ": Closing listening TCP socket" );
            }
        }
#endif

        EventManager::closeFd ( fd );
    }

    _listeners.clear();
}

ERRCODE TcpServer::addListener ( const SockAddr & localAddr, uint8_t extraData, int backlog )
{
    if ( !localAddr.hasIpAddr() || !localAddr.hasPort() )
        return Error::InvalidParameter;

    ERRCODE eCode;

    int sockFd = SocketApi::createListeningTcpSocket ( localAddr, backlog, &eCode );

    if ( sockFd < 0 )
    {
        LOG_ERR ( L_ERROR, eCode, localAddr
                  << ": Error creating a listening TCP socket: " << SocketApi::getLastErrorDesc() );

        return eCode;
    }

    if ( !SocketApi::setNonBlocking ( sockFd ) )
    {
        LOG ( L_ERROR, localAddr
              << ": Error setting listening TCP socket in non-blocking mode: " << SocketApi::getLastErrorDesc() );

        SocketApi::close ( sockFd );

        return Error::SetSockOptFailed;
    }

    ListenerData lData;

    lData.addr = localAddr;
    lData.extraData = extraData;

    _listeners.insert ( sockFd, lData );

    LOG ( L_DEBUG, localAddr << ": Created a listening TCP socket; FD: " << sockFd << "; ExtraData: " << extraData );

    // We don't need write events
    EventManager::setFdHandler ( sockFd, this, EventManager::EventRead );

    return Error::Success;
}

ERRCODE TcpServer::addListener ( const String & sockName, uint8_t extraData, int backlog )
{
    if ( sockName.length() < 1 )
        return Error::InvalidParameter;

    IpAddress ipAddr;
    uint16_t ipPort;

    if ( IpAddress::convertAddrSpec ( sockName, ipAddr, ipPort ) )
        return addListener ( ipAddr, ipPort, extraData, backlog );

    int sockFd = SocketApi::create ( SocketApi::SocketLocal );

    if ( sockFd < 0 )
    {
        LOG ( L_ERROR,
              "'" << sockName << "': Error creating a listening UNIX socket: " << SocketApi::getLastErrorDesc() );

        return Error::SocketFailed;
    }

    if ( !SocketApi::bind ( sockFd, sockName ) )
    {
        LOG ( L_ERROR,
              "'" << sockName << "': Error binding a listening UNIX socket: " << SocketApi::getLastErrorDesc() );

        SocketApi::close ( sockFd );
        return Error::BindFailed;
    }

    if ( !SocketApi::listen ( sockFd, backlog ) )
    {
        LOG ( L_ERROR,
              "'" << sockName << "': Error listening on a UNIX socket: " << SocketApi::getLastErrorDesc() );

        SocketApi::close ( sockFd );
        return Error::ListenFailed;
    }

    if ( !SocketApi::setNonBlocking ( sockFd ) )
    {
        LOG ( L_ERROR, "'" << sockName << "': Error setting a listening UNIX socket in non-blocking mode: "
              << SocketApi::getLastErrorDesc() );

        SocketApi::close ( sockFd );
        return Error::SetSockOptFailed;
    }

    ListenerData lData;

    lData.name = sockName;
    lData.extraData = extraData;

    _listeners.insert ( sockFd, lData );

    LOG ( L_DEBUG, "'" << sockName << "': Created a listening UNIX socket; FD: "
          << sockFd << "; ExtraData: " << extraData );

    // We don't need write events
    EventManager::setFdHandler ( sockFd, this, EventManager::EventRead );

    return Error::Success;
}

void TcpServer::receiveFdEvent ( int fd, short int /* events */ )
{
    assert ( fd >= 0 );

    ListenerData lData;

    if ( !_listeners.find ( fd, lData ) )
    {
        LOG ( L_ERROR, "Received an FD event for unknown FD (" << fd << "); Ignoring" );

        assert ( false );

        return;
    }

    int newFd = -1;

    if ( lData.name.length() > 0 )
    {
        String name;

        if ( ( newFd = SocketApi::accept ( fd, name ) ) < 0 )
        {
            LOG ( L_ERROR,
                  "'" << lData.name << "': Error accepting UNIX connection: " << SocketApi::getLastErrorDesc() );
            return;
        }

        LOG ( L_DEBUG2, "'" << lData.name << "': Accepted new UNIX connection from '" << name << "'" );

        _owner.incomingUnixConnection ( this, lData.extraData, newFd, lData.name );
        return;
    }

    SockAddr remoteAddr;

    if ( ( newFd = SocketApi::accept ( fd, remoteAddr ) ) < 0 )
    {
        LOG ( L_ERROR, lData.addr << ": Error accepting TCP connection: " << SocketApi::getLastErrorDesc() );
        return;
    }

    LOG ( L_DEBUG2, lData.addr << ": Accepted new TCP connection from " << remoteAddr );

    TcpFdSocket * tcpSock = new TcpFdSocket ( 0, newFd, lData.addr, remoteAddr );

    _owner.incomingTcpConnection ( this, lData.extraData, tcpSock );

    tcpSock->simpleUnref();

    return;
}
