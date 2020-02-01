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

#pragma once

#include "SslSocket.hpp"

namespace Pravala
{
class TlsContext;
class TcpSocket;

/// @brief A class representing a TLS socket
class TlsSocket: public SslSocket, protected EventManager::FdEventHandler
{
    public:
        /// @brief Constructor.
        /// Creates a TlsSocket whose underlying TCP data connection is already established.
        /// @param [in] owner The initial owner to set.
        /// @param [in] tlsContext The TlsContext to use for the internal SSL object.
        /// @param [in] sockFd Socket descriptor of an established TCP connection.
        ///                    This socket will take ownership of this FD and will, eventually, close it.
        ///                    Local and remote socket names will be retrieved using this descriptor.
        TlsSocket ( SocketOwner * owner, TlsContext & tlsContext, int sockFd );

        /// @brief Constructor.
        /// Creates a TlsSocket whose underlying TCP data connection is already established.
        /// @param [in] owner The initial owner to set.
        /// @param [in] tlsContext The TlsContext to use for the internal SSL object.
        /// @param [in] sockFd Socket descriptor of an established TCP connection.
        ///                    This socket will take ownership of this FD and will, eventually, close it.
        /// @param [in] localAddr Local socket address to assume is being used.
        /// @param [in] remoteAddr Remote socket address to assume is being used.
        TlsSocket (
            SocketOwner * owner, TlsContext & tlsContext, int sockFd,
            const SockAddr & localAddr, const SockAddr & remoteAddr );

        /// @brief Constructor.
        /// Creates a TlsSocket whose underlying TCP data connection is initialized, but may still be connecting.
        /// @param [in] owner The initial owner to set.
        /// @param [in] tlsContext The TlsContext to use for the internal SSL object.
        /// @param [in] tcpSocket The TcpSocket object to connect over.
        ///                       It should be in either 'connected' or 'connecting' state.
        TlsSocket ( SocketOwner * owner, TlsContext & tlsContext, TcpSocket * tcpSocket );

        /// @brief Constructor.
        /// Creates an uninitialized TlsSocket. It will be possible to call bind and/or connect on it.
        /// @param [in] owner The initial owner to set.
        /// @param [in] tlsContext The TlsContext to use for the internal SSL object.
        TlsSocket ( SocketOwner * owner, TlsContext & tlsContext );

        /// @brief Tries to detect network MTU based on the underlying TCP socket.
        /// It only works on platforms that expose that data.
        /// @return Detected MTU value, or 0 if it could not be calculated.
        uint16_t getDetectedMtu() const;

        virtual const MemHandle & getReadBuffer() const;
        virtual void consumeReadBuffer ( size_t size );

        virtual TlsSocket * getTlsSocket();

        virtual String getLogId ( bool extended = false ) const;
        virtual void close();

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType );
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual const SockAddr & getLocalSockAddr() const;
        virtual const SockAddr & getRemoteSockAddr() const;

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual bool runEvents ( uint16_t events );

        virtual void receiveFdEvent ( int fd, short int events );

    protected:

        /// @brief This event is used to perform a delayed 'read' operation.
        /// This is because SSL may buffer data internally, which we won't know about until we try reading again.
        /// Which normally would happen the next time we get a new packet and read event.
        static const uint16_t SockTlsEventDelayedRead = ( 1 << ( SockSslNextEventShift + 0 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTlsNextEventShift = SockSslNextEventShift + 1;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockTlsNextFlagShift = SockSslNextFlagShift;

        SockAddr _localAddr; ///< The local address of the socket.
        SockAddr _remoteAddr; ///< The address of the remote host.

        MemHandle _readBuf; ///< Buffer for incoming data.

        TcpSocket * _tcpSocket; ///< Connecting TCP socket that we are waiting for.

        /// @brief Destructor
        ~TlsSocket();

        /// @brief Helper function that calls SSL_write.
        /// @param [in] sockFd The FD for the write. Just to avoid having to extract it from SSL state. Must be valid.
        /// @param [in] delayCallbacks If true, error callbacks will be delayed.
        /// @param [in] data Pointer to the data to send.
        /// @param [in,out] dataSize The size of the data to send. SSL_write is called even when it's 0.
        ///                          On success it is set to the number of bytes sent (or queued).
        /// @return Standard error code.
        ERRCODE tlsWrite ( int sockFd, bool delayCallbacks, const char * data, size_t & dataSize );

        /// @brief A helper function that calls SSL_read
        /// @param [in] sockFd The FD for the read. Just to avoid having to extract it from SSL state. Must be valid.
        void tlsRead ( int sockFd );

        /// @brief Helper function called when there is a "regular" write FD event that can be used for sending data.
        /// This will not get called if write event is used for doing regular SSL operations after "want write" error.
        /// @param [in] sockFd The FD for the write. Just to avoid having to extract it from SSL state. Must be valid.
        virtual void tlsHandleRegularWriteEvent ( int sockFd );

        /// @brief Helper function that configures underlying FD.
        /// This should only be called when the socket is not initialized yet.
        /// @param [in] sockFd The new socket FD to set.
        virtual void setSslSockFd ( int sockFd );

        virtual void socketConnected ( Socket * sock );

        virtual SocketApi::SocketType ipSockGetType ( const SockAddr & forAddr );
};
}
