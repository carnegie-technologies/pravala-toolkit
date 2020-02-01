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

#include <cassert>
#include <cerrno>

#include "sys/OsUtils.hpp"

#include "SimpleSocket.hpp"

using namespace Pravala;

SimpleSocket::SimpleSocket(): _sock ( -1 ), _type ( SocketInvalid )
{
}

SimpleSocket::~SimpleSocket()
{
    close();
}

void SimpleSocket::close()
{
    if ( _sock >= 0 )
    {
        EventManager::closeFd ( _sock );
        _sock = -1;
    }

    _type = SocketInvalid;
}

ERRCODE SimpleSocket::init ( SocketType sockType )
{
    close();

    assert ( _sock < 0 );

    _sock = create ( sockType );

    if ( _sock < 0 )
    {
        return Error::SocketFailed;
    }

    _type = sockType;
    return Error::Success;
}

ERRCODE SimpleSocket::initListeningTcpSocket ( const IpAddress & localAddr, uint16_t localPort, int backLog )
{
    ERRCODE ret = Error::Success;

    close();

    _sock = createListeningTcpSocket ( localAddr, localPort, backLog, &ret );

    return ret;
}

ERRCODE SimpleSocket::accept ( SimpleSocket & acceptedSock, IpAddress & addr, uint16_t & port )
{
    const SocketType sType = getType();

    if ( !isInitialized() || ( sType != SocketStream4 && sType != SocketStream6 ) )
    {
        return Error::InvalidParameter;
    }

    const int newFd = accept ( addr, port );

    if ( newFd < 0 )
    {
        return Error::SocketFailed;
    }

    acceptedSock.close();
    acceptedSock._type = sType;
    acceptedSock._sock = newFd;

    return Error::Success;
}

ERRCODE SimpleSocket::accept ( SimpleSocket & acceptedSock, String & name )
{
    const SocketType sType = getType();

    if ( !isInitialized() || ( sType != SocketLocal && sType != SocketLocalSeq ) )
    {
        return Error::InvalidParameter;
    }

    const int newFd = accept ( name );

    if ( newFd < 0 )
    {
        return Error::SocketFailed;
    }

    acceptedSock.close();
    acceptedSock._type = sType;
    acceptedSock._sock = newFd;

    return Error::Success;
}
