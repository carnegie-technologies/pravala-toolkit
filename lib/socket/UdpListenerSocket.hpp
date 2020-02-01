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

#include "UdpSocket.hpp"

namespace Pravala
{
class UdpListener;

/// @brief Represents a UDP socket that uses separate UdpListener object.
///
/// To create a UDP socket of this type, first create a UdpListener,
/// then generate the appropriate UDP socket object from it.
///
/// Passing UdpListenerSocket objects between threads is NOT SUPPORTED.
class UdpListenerSocket: public UdpSocket
{
    public:
        /// @brief Set to true if this is a listening (unconnected) socket; False otherwise.
        /// If this is set to 'false', _remoteAddr will not be modified after creating this socket.
        const bool IsListeningSock;

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType );
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual const SockAddr & getLocalSockAddr() const;
        virtual const SockAddr & getRemoteSockAddr() const;

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual ERRCODE sendTo ( const SockAddr & addr, const char * data, size_t dataSize );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemHandle & data );
        virtual ERRCODE sendTo ( const SockAddr & addr, MemVector & data );

        virtual UdpSocket * generateConnectedSock (
            SocketOwner * owner, SockAddr & remoteAddr, ERRCODE * errCode );

        virtual String getLogId ( bool extended = false ) const;

        virtual int increaseRcvBufSize ( int size );
        virtual int increaseSndBufSize ( int size );

    protected:
        UdpListener & _listener; ///< UDP Listener this object sends data to/receives data from

        /// @brief For a connected socket, this is the address/port of the remote end.
        /// For a listening socket, this is the address/port of the last received packet.
        SockAddr _remoteAddr;

        /// @brief Constructor.
        /// @param [in] listener UDP Listener this object sends data to/receives data from.
        /// @param [in] owner The initial owner to set.
        /// @param [in] remoteAddr Remote address of this UDP socket, if invalid, this is a listening socket.
        UdpListenerSocket ( UdpListener & listener, SocketOwner * owner, const SockAddr & remoteAddr );

        /// @brief Destructor.
        virtual ~UdpListenerSocket();

        /// @brief Called by UdpListener when data was received on a listening UDP socket.
        /// Nothing happens if this is called on connected socket (the data will be dropped).
        /// @param [in] remoteAddr Remote address of data received.
        /// @param [in] data Data that was received. This handle will be cleared!
        inline void sockDataReceivedFrom ( const SockAddr & remoteAddr, MemHandle & data )
        {
            if ( !IsListeningSock )
                return;

            _remoteAddr = remoteAddr;

            doSockDataReceived ( data );
        }

        /// @brief To be called by the UdpListener to tell the socket that it is now closed.
        /// This will clear all the flags (marking socket as "invalid"), and schedule 'socket closed' event.
        /// It will NOT clear the remote address.
        void notifyClosed();

        friend class UdpListener;
};
}
