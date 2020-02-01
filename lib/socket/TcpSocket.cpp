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

#include "TcpSocket.hpp"

using namespace Pravala;

TcpSocket::TcpSocket ( SocketOwner * owner ): IpSocket ( owner )
{
}

TcpSocket::TcpSocket ( SocketOwner * owner, const SockAddr & localAddr, const SockAddr & remoteAddr ):
    IpSocket ( owner ),
    _localAddr ( localAddr ),
    _remoteAddr ( remoteAddr )
{
}

TcpSocket * TcpSocket::getTcpSocket()
{
    return this;
}

uint16_t TcpSocket::getDetectedMtu() const
{
    return 0;
}

const SockAddr & TcpSocket::getLocalSockAddr() const
{
    return _localAddr;
}

const SockAddr & TcpSocket::getRemoteSockAddr() const
{
    return _remoteAddr;
}

const MemHandle & TcpSocket::getReadBuffer() const
{
    return _readBuf;
}

SocketApi::SocketType TcpSocket::ipSockGetType ( const SockAddr & forAddr )
{
    if ( forAddr.isIPv4() )
    {
        return SocketApi::SocketStream4;
    }
    else if ( forAddr.isIPv6() )
    {
        return SocketApi::SocketStream6;
    }

    return SocketApi::SocketInvalid;
}
