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

#include <cstdio>
#include <cerrno>

#include "sys/OsUtils.hpp"

#include "SimpleSocket.hpp"
#include "SocketPair.hpp"
#include "EventManager.hpp"

#define SEND_CHAR    'Q'

using namespace Pravala;

SocketPair::SocketPair(): _sockA ( -1 ), _sockB ( -1 )
{
}

SocketPair::~SocketPair()
{
    close();
}

void SocketPair::close()
{
    if ( _sockA >= 0 )
    {
        EventManager::closeFd ( _sockA );
        _sockA = -1;
    }

    if ( _sockB >= 0 )
    {
        EventManager::closeFd ( _sockB );
        _sockB = -1;
    }
}

ERRCODE SocketPair::init()
{
    if ( _sockA >= 0 || _sockB >= 0 )
        return Error::AlreadyInitialized;

#ifndef SYSTEM_WINDOWS
    int socks[ 2 ] = { -1, -1 };

    // Let's try 'socketpair' call (not available on Windows).
    // If it fails, we will try creating a pair of connectedTCP sockets.

    if ( socketpair ( AF_UNIX, SOCK_STREAM, 0, socks ) == 0 )
    {
        _sockA = socks[ 0 ];
        _sockB = socks[ 1 ];

        return Error::Success;
    }
#endif

    SimpleSocket listSock;
    SimpleSocket conSock;
    SockAddr listAddr;

    ERRCODE eCode = listSock.initListeningTcpSocket ( "127.0.0.1", 0, 4 );

    UNTIL_ERROR ( eCode, listSock.getName ( listAddr ) );
    UNTIL_ERROR ( eCode, conSock.init ( SocketApi::SocketStream4 ) );
    UNTIL_ERROR ( eCode, conSock.setNonBlocking ( true ) );

    if ( NOT_OK ( eCode ) )
        return eCode;

    eCode = conSock.connect ( listAddr.getAddr(), listAddr.getPort() );

    if ( eCode != Error::Success && eCode != Error::ConnectInProgress )
        return eCode;

    IpAddress accAddr;
    uint16_t accPort;

    ( void ) accPort;

    int acSock = listSock.accept ( accAddr, accPort );

    if ( acSock < 0 )
        return Error::SocketFailed;

    char c = SEND_CHAR;

    if ( ::send ( acSock, &c, 1, 0 ) != 1 )
    {
        SocketApi::close ( acSock );
        return Error::WriteFailed;
    }

    int count = 0;
    int readRet = -1;

    while ( ( readRet = ::recv ( conSock.getSock(), &c, 1, 0 ) ) != 1 )
    {
        // After a second we really should give up...
        if ( ++count > 10 )
            break;

        // Sleep for 100ms:

#ifdef SYSTEM_WINDOWS
        Sleep ( 100 );
#else
        usleep ( 100000 );
#endif
    }

    if ( readRet != 1 || c != SEND_CHAR )
    {
        SocketApi::close ( acSock );
        return Error::ReadFailed;
    }

    _sockA = acSock;
    _sockB = conSock.takeSock();

    return Error::Success;
}
