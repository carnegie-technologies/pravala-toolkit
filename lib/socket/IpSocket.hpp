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

#include "basic/SockAddr.hpp"
#include "Socket.hpp"

namespace Pravala
{
class UdpSocket;
class TcpSocket;

/// @brief An abstract IP (IPv4/IPv6) socket that extends base socket with calls that use IP addresses.
class IpSocket: public Socket
{
    public:
        /// @brief Binds socket to a local address.
        /// @param [in] addr The address to bind the socket to.
        /// @return Standard error code.
        virtual ERRCODE bind ( const SockAddr & addr ) = 0;

        /// @brief Binds socket to a physical network interface.
        /// @param [in] ifaceName The physical interface name to bind the socket to.
        /// @param [in] addrType The address type to initialize the socket with.
        ///                      This may be ignored if the socket is already initialized.
        /// @return Standard error code.
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType ) = 0;

        /// @brief Starts connecting socket to a remote address.
        /// This call always only starts connection procedure.
        /// When it returns an error, it means that there is something wrong with the socket,
        /// or with the address used. In that case there will be no 'connected' or 'connect failed' callback.
        /// If this method succeeds, it means that there will be a 'connected' or 'connect failed' callback.
        /// Specifically, this method does NOT return ConnectInProgress code (and 'Success' code has this
        /// exact meaning). If the socket gets connected right away, the callback will
        /// be generated during the end of the event loop.
        /// @param [in] addr The address to connect the socket to.
        /// @return Standard error code.
        virtual ERRCODE connect ( const SockAddr & addr ) = 0;

        /// @brief Exposes the local SockAddr this socket is bound to.
        /// @return the local SockAddr this socket is bound to.
        virtual const SockAddr & getLocalSockAddr() const = 0;

        /// @brief Exposes the remote SockAddr this socket is associated with.
        /// In most cases this will be the remote address this socket is connected (or connecting) to,
        /// but if this is an unconnected socket that could receive data from different hosts,
        /// this method will return the last remote address this socket received data from.
        /// @return the remote SockAddr this socket is associated with.
        virtual const SockAddr & getRemoteSockAddr() const = 0;

        /// @brief Returns this object as a UdpSocket.
        /// @return This object as a UdpSocket pointer, or 0 if it is not a UdpSocket.
        virtual UdpSocket * getUdpSocket();

        /// @brief Returns this object as a TcpSocket.
        /// @return This object as a TcpSocket pointer, or 0 if it is not a TcpSocket.
        virtual TcpSocket * getTcpSocket();

        virtual IpSocket * getIpSocket();
        virtual String getLogId ( bool extended = false ) const;
        virtual String getLocalDesc() const;
        virtual String getRemoteDesc() const;

    protected:
        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockIpNextEventShift = SockNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockIpNextFlagShift = SockNextFlagShift;

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        IpSocket ( SocketOwner * owner );

        /// @brief A helper function that determines the socket type based on the address.
        /// This must be implemented by the relevant socket subclasses.
        /// @param [in] forAddr The address to determine the socket type for.
        /// @return Determined socket type (or Invalid).
        virtual SocketApi::SocketType ipSockGetType ( const SockAddr & forAddr ) = 0;

        /// @brief A helper function that initializes socket's FD.
        /// It does nothing if the socket is already initialized.
        /// If a new socket is created, this function will also enable non-blocking mode on it.
        /// It also sets 'valid' flag if the socket is initialized successfully,
        /// and clears both local and remote addresses.
        /// @param [in] sockType The socket type to initialize the socket with.
        /// @param [in,out] sockFd The FD of the socket to initialize. It is not modified if it's already >= 0.
        /// @param [out] localAddr The local address of the socket.
        ///                        If a new socket descriptor is created, localAddr will be cleared.
        /// @param [out] remoteAddr The remote address of the socket.
        ///                         If a new socket descriptor is created, remoteAddr will be cleared.
        /// @return True if the socket has been initialized properly (or was already initialized);
        ///         False if we failed to generate a new socket file descriptor or enable non-blocking mode.
        bool ipSockInitFd ( SocketApi::SocketType sockType, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr );

        /// @brief A helper function that binds a socket.
        /// It is also possible that 'valid' flag will be set, if sockInitFd() ends up initializing it.
        /// @param [in] addr The address to bind the socket to.
        /// @param [in,out] sockFd The FD of the socket. If this descriptor is not valid, both local and remote
        ///                        addresses will be cleared, and sockFd will be initialized using Socket::sockInitFd().
        /// @param [in,out] localAddr The local address the socket is bound to.
        ///                           If a new socket descriptor is created, localAddr will be cleared.
        ///                           If sockFd is valid and localAddr is also valid, this function will fail.
        ///                           If sockFd is invalid, a new sockFd is generated, and bind succeeds,
        ///                            localAddr will be overwritten with the new local address of the socket.
        /// @param [out] remoteAddr The remote address the socket is connected to.
        ///                         If a new socket descriptor is created, remoteAddr will be cleared.
        /// @return Standard error code.
        ERRCODE ipSockBind ( const SockAddr & addr, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr );

        /// @brief A helper function that binds a socket to a physical network interface.
        /// It is also possible that 'valid' flag will be set, if sockInitFd() ends up initializing it.
        /// @param [in] sockType The socket type to initialize the socket with.
        /// @param [in] ifaceName The physical interface name to bind the socket to.
        /// @param [in,out] sockFd The FD of the socket. If this descriptor is not valid, both local and remote
        ///                        addresses will be cleared, and sockFd will be initialized using Socket::sockInitFd().
        /// @param [in,out] localAddr The local address the socket is bound to.
        ///                           If a new socket descriptor is created, localAddr will be cleared.
        ///                           If sockFd is valid and localAddr is also valid, this function will fail.
        ///                           If sockFd is invalid, a new sockFd is generated, and bind succeeds,
        ///                            localAddr will be overwritten with the socket family type. The rest of
        ///                            the localAddr struct will be all zeroes in this case.
        /// @param [out] remoteAddr The remote address the socket is connected to.
        ///                         If a new socket descriptor is created, remoteAddr will be cleared.
        /// @return Standard error code.
        ERRCODE ipSockBindToIface (
            SocketApi::SocketType sockType, const String & ifaceName,
            int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr );

        /// @brief A helper function that connects a socket.
        /// On success (whether the result is 'success' or 'connect in progress') the 'connecting' flag
        /// will be set on this socket.
        /// It is also possible that 'valid' flag will be set, if sockInitFd() ends up initializing it.
        /// @param [in] addr The address to connect the socket to.
        /// @param [in,out] sockFd The FD of the socket. If this descriptor is not valid, both local and remote
        ///                        addresses will be cleared, and sockFd will be initialized using Socket::sockInitFd().
        /// @param [in,out] localAddr The local address the socket is bound to.
        ///                           If a new socket descriptor is created, localAddr will be cleared.
        ///                           If localAddr is invalid, or a new sockFd is generated, localAddr will
        ///                           be overwritten with the new local address of the socket.
        ///                           This is because connecting an unbound socket will implicitly bind it.
        /// @param [in,out] remoteAddr The remote address the socket is connected to.
        ///                            If a new socket descriptor is created, remoteAddr will be cleared.
        ///                            If sockFd is valid and remoteAddr is also valid, this function will fail.
        ///                            Otherwise remoteAddr will be set to 'addr' after successful connect.
        /// @return Standard error code. It returns the same code as SocketApi::connect,
        ///         so it is possible to get 'connect in progress' code.
        ERRCODE ipSockConnect ( const SockAddr & addr, int & sockFd, SockAddr & localAddr, SockAddr & remoteAddr );
};
}
