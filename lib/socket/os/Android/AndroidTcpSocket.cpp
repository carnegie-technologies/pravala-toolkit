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

#include "sys/os/Android/AndroidSocketApi.hpp"
#include "AndroidTcpSocket.hpp"

using namespace Pravala;

AndroidTcpSocket::AndroidTcpSocket ( SocketOwner * owner, int64_t networkId ):
    TcpFdSocket ( owner ),
    _netId ( networkId )
{
}

AndroidTcpSocket::~AndroidTcpSocket()
{
}

bool AndroidTcpSocket::sockInitFd ( SocketApi::SocketType sockType, int & sockFd )
{
    if ( sockFd >= 0 )
        return true;

    if ( !TcpFdSocket::sockInitFd ( sockType, sockFd ) )
        return false;

    // Android uses netId=0 to mean unset, otherwise it expects the netId to match a 'magic' value, see:
    //  - http://androidxref.com/9.0.0_r3/xref/frameworks/base/core/java/android/net/Network.java#414
    //  - http://androidxref.com/9.0.0_r3/xref/frameworks/base/native/android/net.c#26
    // We have to allow 0 because we use the netId in a different way, so we just use negative to mean unset.
    if ( _netId < 0 )
        return true;

    const ERRCODE eCode = AndroidSocketApi::bindSocketToNetwork ( sockFd, _netId );

    if ( IS_OK ( eCode ) )
        return true;

    LOG_ERR ( L_ERROR, eCode, getLogId() << ": Could not bind " << SocketApi::getSockTypeName ( sockType )
              << " socket to network with ID " << _netId << "; Closing the socket" );

    SocketApi::close ( sockFd );
    sockFd = -1;

    return false;
}
