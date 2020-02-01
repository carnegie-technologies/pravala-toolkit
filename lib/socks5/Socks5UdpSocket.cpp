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

#include "socket/PacketDataStore.hpp"

#include "Socks5TcpSocket.hpp"
#include "Socks5UdpSocket.hpp"

#include "internal/Socks5UdpRequestHeader.hpp"

using namespace Pravala;

namespace Pravala
{
/// @brief Wrapper around Socks5TcpSocket that makes it as a control socket for UDP.
/// Instead of requesting 'TCP connect' it sends 'UDP associate' request.
class Socks5TcpSocketUdpWrapper: public Socks5TcpSocket
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] proxyAddr The address of SOCKS5 proxy server to use.
        Socks5TcpSocketUdpWrapper ( Socks5UdpSocket * owner, const SockAddr & proxyAddr ):
            Socks5TcpSocket ( owner, proxyAddr )
        {
        }

        virtual String getLogId ( bool extended = false ) const
        {
            if ( extended )
            {
                return String ( "S5-UDP_TCP:[%2-%3(%4),%1]" )
                       .arg ( _sockFd ).arg ( _localAddr.toString(), _remoteAddr.toString(), _proxyAddr.toString() );
            }

            return String ( "S5-UDP_TCP:%1(%2)" ).arg ( _remoteAddr.toString(), _proxyAddr.toString() );
        }

        /// @brief Returns the UDP address on which the proxy waits for data from us.
        /// @return The UDP address on which the proxy waits for data from us.
        inline const SockAddr & getProxyUdpSockAddr() const
        {
            return _proxyUdpAddr;
        }

    protected:
        SockAddr _proxyUdpAddr; ///< The UDP address on which the proxy will wait for data from us.

        virtual bool sendSocks5Request()
        {
            if ( !_remoteAddr.hasIpAddr() )
            {
                LOG ( L_ERROR, getLogId() << ": Remote address is invalid: " << _remoteAddr );
                return false;
            }

            // We are sending a 'zero' address, but it needs to be either IPv4 or IPv6 type
            // that matches the destination address.

            return Socks5TcpSocket::sendSocks5Request (
                Socks5RequestMessage::CmdUDPAssociate,
                _remoteAddr.isIPv6() ? Ipv6ZeroSockAddress : Ipv4ZeroSockAddress );
        }

        virtual void socks5RequestSucceeded ( const SockAddr & addr )
        {
            _proxyUdpAddr = addr;

            // All good! We are now "connected"!
            setFlags ( SockSocks5TcpBaseFlagHandshakeCompleted );

            // We don't need to use this socket for anything, but we would like to know if it gets closed!
            // To be notified, we need read events to be enabled.

            EventManager::setFdEvents ( _sockFd, EventManager::EventRead );

            LOG ( L_DEBUG, getLogId()
                  << ": Successfully completed SOCKS5 negotiation; Proxy's UDP address: " << _proxyUdpAddr );

            doSockConnected();
        }
};
}

Socks5UdpSocket::Socks5UdpSocket ( SocketOwner * owner, const SockAddr & proxyAddr ):
    UdpFdSocket ( owner ),
    _proxyAddr ( proxyAddr ),
    _tcpSocket ( new Socks5TcpSocketUdpWrapper ( this, proxyAddr ) )
{
}

Socks5UdpSocket::~Socks5UdpSocket()
{
    if ( _tcpSocket != 0 )
    {
        _tcpSocket->unrefOwner ( this );
        _tcpSocket = 0;
    }
}

String Socks5UdpSocket::getLogId ( bool extended ) const
{
    if ( extended )
    {
        return String ( "S5-UDP:[%2-%3(%4),%1]" )
               .arg ( _sockFd )
               .arg ( _localAddr.toString(), _remoteAddr.toString(), getProxySockAddr().toString() );
    }

    return String ( "S5-UDP:%1(%2)" ).arg ( _remoteAddr.toString(), getProxySockAddr().toString() );
}

const SockAddr & Socks5UdpSocket::getProxyUdpSockAddr() const
{
    return ( _tcpSocket != 0 ) ? ( _tcpSocket->getProxyUdpSockAddr() ) : ( EmptySockAddress );
}

void Socks5UdpSocket::close()
{
    if ( _tcpSocket != 0 )
    {
        _tcpSocket->close();
    }

    UdpFdSocket::close();
}

int Socks5UdpSocket::stealSockFd()
{
    // Because SOCKS5 uses special UDP prefix attached to each data transmission,
    // it doesn't make sense to allow anybody to take over our socket file descriptor.
    // This is the default behaviour - this function is only re-implemented to have the comment above!

    return -1;
}

UdpSocket * Socks5UdpSocket::generateConnectedSock ( SocketOwner *, SockAddr &, ERRCODE * errCode )
{
    // We could generate a 'connecting' socket, but not one that is connected right away...
    // Maybe we need to change that API, but for now it's unsupported.

    if ( errCode != 0 )
        *errCode = Error::Unsupported;

    return 0;
}

ERRCODE Socks5UdpSocket::bind ( const SockAddr & addr )
{
    if ( !_tcpSocket )
        return Error::WrongState;

    ERRCODE eCode = _tcpSocket->bind ( addr );

    if ( NOT_OK ( eCode ) )
        return eCode;

    return UdpFdSocket::bind ( addr );
}

ERRCODE Socks5UdpSocket::connect ( const SockAddr & addr )
{
    if ( addr.sa.sa_family == AF_UNSPEC )
    {
        // UDP socket is being disconnected.

        if ( _tcpSocket != 0 )
        {
            _tcpSocket->unrefOwner ( this );
            _tcpSocket = 0;
        }

        return UdpFdSocket::connect ( addr );
    }

    if ( !_tcpSocket )
        return Error::WrongState;

    // We first need to connect the TCP socket!
    // Once TCP socket tells us it's connected, we then will be able to connect the UDP socket.
    const ERRCODE eCode = _tcpSocket->connect ( addr );

    if ( IS_OK ( eCode ) )
    {
        _remoteAddr = addr;
    }

    return eCode;
}

void Socks5UdpSocket::doSockClosed ( ERRCODE reason )
{
    if ( _tcpSocket != 0 )
    {
        _tcpSocket->close();
    }

    UdpFdSocket::doSockClosed ( reason );
}

void Socks5UdpSocket::doSockConnectFailed ( ERRCODE reason )
{
    if ( _tcpSocket != 0 )
    {
        _tcpSocket->close();
    }

    UdpFdSocket::doSockConnectFailed ( reason );
}

void Socks5UdpSocket::socketClosed ( Socket * sock, ERRCODE reason )
{
    ( void ) sock;
    assert ( sock == _tcpSocket );

    LOG ( L_DEBUG, getLogId() << ": Our TCP socket got closed; Closing UDP socket" );

    UdpFdSocket::doSockClosed ( reason );
}

void Socks5UdpSocket::socketConnectFailed ( Socket * sock, ERRCODE reason )
{
    ( void ) sock;
    assert ( sock == _tcpSocket );

    LOG ( L_DEBUG, getLogId() << ": Our TCP socket failed to connect; Failing UDP socket" );

    doSockConnectFailed ( reason );
}

void Socks5UdpSocket::socketReadyToSend ( Socket * )
{
    LOG ( L_ERROR, getLogId() << ": Unexpected callback received" );
}

void Socks5UdpSocket::socketDataReceived ( Socket *, MemHandle & data )
{
    ( void ) data;

    LOG ( L_ERROR, getLogId() << ": Unexpected callback received; Data: " << data.getHexDump() );
}

void Socks5UdpSocket::socketConnected ( Socket * sock )
{
    ( void ) sock;
    assert ( sock == _tcpSocket );

    if ( !_tcpSocket )
        return;

    LOG ( L_DEBUG, getLogId()
          << ": Our TCP socket connected; Connecting UDP socket to proxy's UDP address: "
          << _tcpSocket->getProxyUdpSockAddr() );

    const ERRCODE eCode
        = IpSocket::ipSockConnect ( _tcpSocket->getProxyUdpSockAddr(), _sockFd, _localAddr, _remoteAddr );

    if ( NOT_OK ( eCode ) )
    {
        LOG ( L_DEBUG, getLogId() << ": UDP socket failed to connect" );

        doSockConnectFailed ( eCode );
        return;
    }

    // _remoteAddr should be now set to proxy's UDP address.
    // Our final remote address is the address set in the TCP socket.
    _remoteAddr = _tcpSocket->getRemoteSockAddr();

    EventManager::setFdHandler ( _sockFd, this, EventManager::EventRead );

    LOG ( L_DEBUG, getLogId() << ": Successfully connected socket to " << _tcpSocket->getProxyUdpSockAddr() );

    // This is already an asynchronous callback. So we can do everything right away:
    setFlags ( SockUdpFlagConnected );
    doSockConnected();
}

ERRCODE Socks5UdpSocket::send ( MemHandle & data )
{
    // No need to check if _remoteAddr is valid, to send anything we have to be connected first anyway.
    return sendTo ( _remoteAddr, data );
}

ERRCODE Socks5UdpSocket::send ( MemVector & data )
{
    // No need to check if _remoteAddr is valid, to send anything we have to be connected first anyway.
    return sendTo ( _remoteAddr, data );
}

ERRCODE Socks5UdpSocket::send ( const char * data, size_t & dataSize )
{
    // No need to check if _remoteAddr is valid, to send anything we have to be connected first anyway.
    return sendTo ( _remoteAddr, data, dataSize );
}

ERRCODE Socks5UdpSocket::sendTo ( const SockAddr & addr, MemHandle & data )
{
    // Here we do some quick checks, before we try to construct the vector.

    if ( !isValid() )
        return Error::NotInitialized;

    if ( !hasFlag ( SockFlagConnected ) || !hasFlag ( SockUdpFlagConnected ) )
        return Error::NotConnected;

    if ( !addr.hasIpAddr() )
        return Error::InvalidAddress;

    if ( data.isEmpty() )
        return Error::Success;

    // All good, let's build the vector:

    Socks5UdpRequestHeader hdr ( addr );

    if ( !hdr.isValid() )
    {
        LOG ( L_ERROR, "Invalid UDP request header generated using address " << addr );
        return Error::MessageCreateFailed;
    }

    MemVector vec ( 2 );

    // We want to send UDP request header first, followed by the actual data.
    if ( !vec.append ( hdr.getData() ) || !vec.append ( data ) )
    {
        return Error::MemoryError;
    }

    return UdpFdSocket::send ( vec );
}

ERRCODE Socks5UdpSocket::sendTo ( const SockAddr & addr, MemVector & data )
{
    // SOCKS5 UDP is a bit funny. To use it at all, first we need to connect it.
    // Once connected, we can send data without using the address, in which case it goes to the address
    // we are connected to. But we also could send to any other address.

    if ( !isValid() )
        return Error::NotInitialized;

    if ( !hasFlag ( SockFlagConnected ) || !hasFlag ( SockUdpFlagConnected ) )
        return Error::NotConnected;

    if ( !addr.hasIpAddr() )
        return Error::InvalidAddress;

    if ( data.isEmpty() )
        return Error::Success;

    Socks5UdpRequestHeader hdr ( addr );

    if ( !hdr.isValid() )
    {
        LOG ( L_ERROR, "Invalid UDP request header generated using address " << addr );
        return Error::MessageCreateFailed;
    }

    // We want to send UDP request header first, followed by the actual data.

    if ( !data.prepend ( hdr.getData() ) )
    {
        return Error::MemoryError;
    }

    return UdpFdSocket::send ( data );
}

ERRCODE Socks5UdpSocket::sendTo ( const SockAddr & addr, const char * data, size_t dataSize )
{
    if ( !isValid() )
        return Error::NotInitialized;

    if ( !hasFlag ( SockFlagConnected ) || !hasFlag ( SockUdpFlagConnected ) )
        return Error::NotConnected;

    if ( !addr.hasIpAddr() )
        return Error::InvalidAddress;

    if ( dataSize < 1 )
        return Error::Success;

    if ( !data )
        return Error::InvalidParameter;

    // To include the header before the payload, when we get the payload as a memory pointer,
    // we need to copy the data anyway. So let's put it in a MemHandle, so it can be handled more efficiently.
    Socks5UdpRequestHeader hdr ( addr );

    if ( !hdr.isValid() )
    {
        LOG ( L_ERROR, "Invalid UDP request header generated using address " << addr );
        return Error::MessageCreateFailed;
    }

    const MemHandle & hdrData = hdr.getData();
    const size_t totalSize = hdrData.size() + dataSize;

    MemHandle mh = PacketDataStore::getPacket ( totalSize );

    char * mem = mh.getWritable();

    if ( !mem || mh.size() < totalSize )
    {
        return Error::TooMuchData;
    }

    memcpy ( mem, hdrData.get(), hdrData.size() );
    memcpy ( mem + hdrData.size(), data, dataSize );

    mh.truncate ( totalSize );

    return UdpFdSocket::send ( mh );
}

void Socks5UdpSocket::receiveFdEvent ( int fd, short int events )
{
    assert ( fd == _sockFd );

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        // We shouldn't be getting those, but in case we do:
        EventManager::disableWriteEvents ( _sockFd );
    }

    if ( ( events & EventManager::EventRead ) != EventManager::EventRead )
    {
        return;
    }

    // Get the data
    MemHandle mh = PacketDataStore::getPacket();

    char * const mem = mh.getWritable();

    if ( !mem || mh.isEmpty() )
    {
        LOG ( L_ERROR, getLogId() << ": Unable to receive data; Failed to allocate memory" );
        return;
    }

    const ssize_t ret = ::recv ( fd, mem, mh.size(), 0 );

    if ( ret == 0 )
    {
        LOG ( L_ERROR, getLogId() << ": Read 0 bytes; Closing the socket" );

        doSockClosed ( Error::Closed );
        return;
    }
    else if ( ret < 0 )
    {
        LOG ( L_ERROR, getLogId() << ": Error receiving data; Closing the socket; Error: "
              << SocketApi::getLastErrorDesc() );

        doSockClosed ( Error::ReadFailed );
        return;
    }
    else if ( ( size_t ) ret >= mh.size() )
    {
        // If we fill the entire buffer, it is likely that the message was truncated.
        // We could pass MSG_TRUNC to get the actual size, but that flag may not be supported
        // on all platforms. Since the messages should be smaller than buffers generated
        // by PacketDataStore, we should usually see less data generated. If we see a full
        // buffer, it's likely the message was too big.

        LOG ( L_WARN, getLogId() << ": recv() filled the entire data buffer (" << ret
              << " bytes were generated); Most likely the message was truncated; Discarding packet: "
              << mh.getHexDump() );
        return;
    }

    assert ( ret > 0 );
    assert ( ( size_t ) ret <= mh.size() );

    mh.truncate ( ret );

    Socks5UdpRequestHeader header;

    size_t reqSize = 0;

    const ERRCODE eCode = header.parseAndConsume ( mh, reqSize );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_WARN, eCode, getLogId()
                  << ": Invalid UDP request header; ReqSize: " << reqSize
                  << "; Discarding packet: " << mh.getHexDump() );
        return;
    }

    _remoteAddr = SockAddr ( header.getAddress(), header.getPort() );

    LOG ( L_DEBUG4, getLogId() << ": Received " << mh.size() << " bytes of data from " << _remoteAddr );

    doSockDataReceived ( mh );
    return;
}
