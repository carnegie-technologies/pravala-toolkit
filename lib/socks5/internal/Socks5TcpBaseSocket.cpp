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

#include "Socks5TcpBaseSocket.hpp"

using namespace Pravala;

Socks5TcpBaseSocket::Socks5TcpBaseSocket ( SocketOwner * owner ):
    TcpFdSocket ( owner )
{
}

Socks5TcpBaseSocket::Socks5TcpBaseSocket ( SocketOwner * owner, TcpSocket * tcpSock ):
    TcpFdSocket ( owner, tcpSock )
{
}

TcpFdSocket * Socks5TcpBaseSocket::generateTcpFdSock ( SocketOwner * owner )
{
    if ( !isSocks5Connected() )
    {
        LOG ( L_ERROR, getLogId() << ": Could not generate a basic socket, we are not SOCKS5-connected" );
        return 0;
    }
    else if ( !_readBuf.isEmpty() )
    {
        LOG ( L_ERROR, getLogId() << ": Could not generate a basic socket, read buffer still contains "
              << _readBuf.size() << " byte(s)" );
        return 0;
    }

    return TcpFdSocket::generateTcpFdSock ( owner );
}

void Socks5TcpBaseSocket::consumeReadBuffer ( size_t size )
{
    // Let's prevent others from touching the read buffer until SOCKS5 handshake is done.
    if ( isSocks5Connected() )
    {
        TcpFdSocket::consumeReadBuffer ( size );
    }
}

const MemHandle & Socks5TcpBaseSocket::getReadBuffer() const
{
    // Let's prevent others from seeing the read buffer until SOCKS5 handshake is done.
    return ( isSocks5Connected() ) ? ( TcpFdSocket::getReadBuffer() ) : ( MemHandle::EmptyHandle );
}

ERRCODE Socks5TcpBaseSocket::send ( MemHandle & data )
{
    return ( isSocks5Connected() ) ? ( TcpFdSocket::send ( data ) ) : ( Error::NotConnected );
}

ERRCODE Socks5TcpBaseSocket::send ( const char * data, size_t & dataSize )
{
    return ( isSocks5Connected() ) ? ( TcpFdSocket::send ( data, dataSize ) ) : ( Error::NotConnected );
}

bool Socks5TcpBaseSocket::sendSocks5Data ( const MemHandle & data )
{
    if ( isSocks5Connected() )
    {
        LOG ( L_ERROR, getLogId() << ": SOCKS5 TCP socket is already connected" );
        return false;
    }

    if ( !_sendBuffer.isEmpty() )
    {
        LOG ( L_ERROR, getLogId() << ": Send buffer is not empty" );
        return false;
    }

    _sendBuffer = data;

    if ( !_sendBuffer.isEmpty() )
    {
        EventManager::enableWriteEvents ( _sockFd );
    }

    return true;
}

void Socks5TcpBaseSocket::receiveFdEvent ( int fd, short int events )
{
    if ( isSocks5Connected() )
    {
        TcpFdSocket::receiveFdEvent ( fd, events );
        return;
    }

    assert ( fd >= 0 );
    assert ( fd == _sockFd );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        // The first write event tells us the TCP connection is complete
        if ( !hasFlag ( SockTcpFlagConnected ) )
        {
            setFlags ( SockTcpFlagConnected );

            LOG ( L_DEBUG3, getLogId() << ": Socket connected at TCP level" );
        }

        if ( _sendBuffer.isEmpty() )
        {
            // If needed, write events will be re-enabled inside the callback.
            EventManager::disableWriteEvents ( _sockFd );

            readyToSendSocks5Data();
            return;
        }

        assert ( !_sendBuffer.isEmpty() );

        const ssize_t ret = ::send ( fd, _sendBuffer.get(), _sendBuffer.size(), 0 );

        if ( ret > 0 )
        {
            LOG ( L_DEBUG4, getLogId()
                  << ": Successfully sent " << ret << " out of " << _sendBuffer.size() << " bytes" );

            _sendBuffer.consume ( ret );

            if ( _sendBuffer.isEmpty() )
            {
                EventManager::enableReadEvents ( fd );
            }

            return;
        }

        if ( SocketApi::isErrnoSoft() )
        {
            LOG ( L_DEBUG4, getLogId() << ": Send would block" );
            return;
        }

        LOG ( L_ERROR, getLogId ( true )
              << ": Error sending data; Closing the socket; Error: " << SocketApi::getLastErrorDesc() );

        // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
        doSockConnectFailed ( Error::WriteFailed );
        return;
    }

    // Read events are for data received
    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        // If we are not 'TCP connected' yet, read events mean that the connection attempt failed.
        if ( !hasFlag ( SockTcpFlagConnected ) )
        {
            LOG ( L_DEBUG3, getLogId() << ": Connection refused" );

            // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
            doSockConnectFailed ( Error::ConnectFailed );
            return;
        }

        // If we still have data to be sent, disable read events.
        // We will re-enable read events once all the data has been sent.
        if ( !_sendBuffer.isEmpty() )
        {
            LOG ( L_DEBUG3, getLogId() << ": We have data pending to be sent; Disabling read events for now" );

            EventManager::disableReadEvents ( fd );
            return;
        }

        Buffer buf;

        buf.append ( _readBuf );

        size_t reqBytes = 0;

        // In case something decides to destroy us:
        simpleRef();

        while ( ( reqBytes = receiveSocks5Data() ) > 0 )
        {
            char * const mem = buf.getAppendable ( reqBytes );

            if ( !mem )
            {
                LOG ( L_ERROR, getLogId() << ": Error allocating memory for remote data; Closing the socket" );

                // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
                doSockConnectFailed ( Error::MemoryError );
                break;
            }

            const ssize_t ret = ::recv ( fd, mem, reqBytes, 0 );

            if ( ret > 0 )
            {
                buf.markAppended ( ret );
                _readBuf = buf.getHandle();

                continue;
            }

            if ( ret == 0 )
            {
                LOG ( L_ERROR, getLogId() << ": Socket closed by remote" );

                // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
                doSockConnectFailed ( Error::Closed );
                break;
            }

            // ret < 0

            if ( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                // We need to wait for more data.
                break;
            }

            LOG ( L_ERROR, getLogId() << ": Error receiving data; Closing socket; Error: "
                  << SocketApi::getLastErrorDesc() );

            // We are here only until SOCKS5 negotiation is complete, so we are still 'connecting'.
            doSockConnectFailed ( Error::ReadFailed );
            break;
        }

        // receiveSocks5Data() returned 0, which means either it got a complete message, or there was some error.
        // In either case we should return.

        simpleUnref();
        return;
    }
}
