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

#include "UdpListenerSocket.hpp"
#include "UdpListener.hpp"

using namespace Pravala;

UdpListenerSocket::UdpListenerSocket ( UdpListener & listener, SocketOwner * owner, const SockAddr & remoteAddr ):
    UdpSocket ( owner ),
    IsListeningSock ( !remoteAddr.hasIpAddr() ),
    _listener ( listener ),
    _remoteAddr ( remoteAddr )
{
    _listener.ref();

    if ( _listener.isValid() )
    {
        setFlags ( SockFlagValid );

        if ( !IsListeningSock )
            setFlags ( SockFlagConnected );
    }
}

UdpListenerSocket::~UdpListenerSocket()
{
    _listener.unregisterUdpSocket ( this );
    _listener.unref();
}

String UdpListenerSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "UDP(L):[%1-%2]" ).arg ( getLocalSockAddr().toString(), _remoteAddr.toString() );
    }

    return String ( "UDP(L):" ).append ( _remoteAddr.toString() );
}

void UdpListenerSocket::notifyClosed()
{
    scheduleEvents ( SockEventClosed );
}

ERRCODE UdpListenerSocket::bind ( const SockAddr & )
{
    return Error::Unsupported;
}

ERRCODE UdpListenerSocket::bindToIface ( const String &, IpAddress::AddressType )
{
    return Error::Unsupported;
}

ERRCODE UdpListenerSocket::connect ( const SockAddr & )
{
    return Error::Unsupported;
}

int UdpListenerSocket::increaseRcvBufSize ( int size )
{
    return _listener.increaseRcvBufSize ( size );
}

int UdpListenerSocket::increaseSndBufSize ( int size )
{
    return _listener.increaseSndBufSize ( size );
}

const SockAddr & UdpListenerSocket::getLocalSockAddr() const
{
    return _listener.getLocalAddr();
}

const SockAddr & UdpListenerSocket::getRemoteSockAddr() const
{
    return _remoteAddr;
}

UdpSocket * UdpListenerSocket::generateConnectedSock ( SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode )
{
    return _listener.generateConnectedSock ( owner, remoteAddr, errCode );
}

ERRCODE UdpListenerSocket::sendTo ( const SockAddr & addr, const char * data, size_t dataSize )
{
    if ( !isValid() )
        return Error::NotInitialized;

    // It's either an error, or all the data is accepted - no need to modify dataSize.
    return _listener.send ( addr, data, dataSize );
}

ERRCODE UdpListenerSocket::sendTo ( const SockAddr & addr, MemHandle & data )
{
    if ( !isValid() )
        return Error::NotInitialized;

    const ERRCODE eCode = _listener.send ( addr, data );

    if ( IS_OK ( eCode ) )
    {
        // It's either an error, or all the data is accepted - we should clear the handle.
        data.clear();
    }

    return eCode;
}

ERRCODE UdpListenerSocket::sendTo ( const SockAddr & addr, MemVector & data )
{
    if ( !isValid() )
        return Error::NotInitialized;

    const ERRCODE eCode = _listener.send ( addr, data );

    if ( IS_OK ( eCode ) )
    {
        // It's either an error, or all the data is accepted - we should clear the handle.
        data.clear();
    }

    return eCode;
}

ERRCODE UdpListenerSocket::send ( const char * data, size_t & dataSize )
{
    if ( !isValid() )
        return Error::NotInitialized;

    // It's either an error, or all the data is accepted - no need to modify dataSize.
    return _listener.send ( _remoteAddr, data, dataSize );
}

ERRCODE UdpListenerSocket::send ( MemHandle & data )
{
    if ( !isValid() )
        return Error::NotInitialized;

    const ERRCODE eCode = _listener.send ( _remoteAddr, data );

    if ( IS_OK ( eCode ) )
    {
        // It's either an error, or all the data is accepted - we should clear the handle.
        data.clear();
    }

    return eCode;
}

ERRCODE UdpListenerSocket::send ( MemVector & data )
{
    if ( !isValid() )
        return Error::NotInitialized;

    const ERRCODE eCode = _listener.send ( _remoteAddr, data );

    if ( IS_OK ( eCode ) )
    {
        // It's either an error, or all the data is accepted - we should clear the handle.
        data.clear();
    }

    return eCode;
}
