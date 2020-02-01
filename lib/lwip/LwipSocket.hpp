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

#include <cerrno>

#include "basic/NoCopy.hpp"
#include "basic/IpAddress.hpp"
#include "basic/SockAddr.hpp"
#include "basic/MemHandle.hpp"
#include "log/TextLog.hpp"

struct ip_addr;
struct ip_pcb;

typedef int8_t err_t;
typedef struct ip_addr ip_addr_t;
typedef struct ip_pcb * IpPcbPtr;

namespace Pravala
{
class LwipTcpSocket;
class LwipUdpSocket;

/// @brief Abstract base class for an lwIP TCP or UDP socket
class LwipSocket: public NoCopy
{
    public:
        /// @brief Determines whether the socket is bound to some local address.
        static const uint16_t SocketFlagIsBound = ( 1 << 0 );

        /// @brief Determines whether the socket is connecting. This is only applicable to TCP sockets.
        static const uint16_t SocketFlagIsConnecting = ( 1 << 1 );

        /// @brief Determines whether the socket is connected to some remote address.
        static const uint16_t SocketFlagIsConnected = ( 1 << 2 );

        /// @brief Set in sockets that were specifically created as IPv4 sockets.
        /// @note This flag should not be used together with SocketFlagIpV6Only flag.
        static const uint16_t SocketFlagIpV4Only = ( 1 << 3 );

        /// @brief Determines whether IPV6_V6ONLY option has been set on this socket.
        /// @note This flag should not be used together with SocketFlagIpV4Only flag.
        static const uint16_t SocketFlagIpV6Only = ( 1 << 4 );

        /// @brief Set in TCP sockets that have been disconnected.
        static const uint16_t SocketFlagIsDisconnected = ( 1 << 5 );

        /// @brief Destructor
        virtual ~LwipSocket();

        /// @brief Convenience function that casts this socket to a TCP socket.
        /// @return The LwipTcpSocket if this is a TCP socket, 0 otherwise.
        virtual LwipTcpSocket * getTCP();

        /// @brief Convenience function that casts this socket to a UDP socket.
        /// @return The LwipUdpSocket if this is a UDP socket, 0 otherwise.
        virtual LwipUdpSocket * getUDP();

        /// @brief Returns the name of socket's type (for logging).
        /// @return The name of this socket's type.
        virtual const char * getTypeName() const;

        /// @brief Checks if this socket is open.
        /// If the socket is open, then all socket operations will function normally.
        /// If the socket is NOT open:
        ///   - all socket operations will return an error
        ///   - getTag will always return 0, and setTag won't do anything
        ///   - the IpV4Only and IpV6Only flags are still valid
        /// @return True if this socket is open; False otherwise.
        inline bool isOpen() const
        {
            return ( _pcb != 0 );
        }

        /// @brief Checks if this socket has been bound to some address.
        /// @return True if this socket has been bound to some address; False otherwise.
        inline bool isBound() const
        {
            return getFlag ( SocketFlagIsBound );
        }

        /// @brief Checks if this socket is connected.
        /// @return True if this socket is connected; False otherwise.
        inline bool isConnected() const
        {
            return getFlag ( SocketFlagIsConnected );
        }

        /// @brief Checks if this socket is connecting.
        /// @return True if this socket is connecting; False otherwise.
        inline bool isConnecting() const
        {
            return getFlag ( SocketFlagIsConnecting );
        }

        /// @brief Checks if this socket is an IPv4-only socket.
        /// @return True if this socket is an IPv4-only socket; False otherwise.
        inline bool isIpV4Only() const
        {
            return getFlag ( SocketFlagIpV4Only );
        }

        /// @brief Checks if this socket has IPV6_V6ONLY option set.
        /// @return True if this socket has IPV6_V6ONLY option set; False otherwise.
        inline bool isIpV6Only() const
        {
            return getFlag ( SocketFlagIpV6Only );
        }

        /// @brief Checks if this socket is an IPv6 socket.
        /// @return True if this socket is an IPv6 socket; False otherwise.
        inline bool isIpV6() const
        {
            return !isIpV4Only();
        }

        /// @brief Get a flag
        /// @param [in] flag Flag to get (see SocketFlag* constants)
        /// @return True if the flag is set, false otherwise
        virtual bool getFlag ( uint16_t flag ) const
        {
            return ( ( _flags & flag ) == flag );
        }

        /// @brief Set/unset a flag
        /// @param [in] flag Flag to set (see SocketFlag* constants)
        /// @param [in] value True to set this flag, false to unset this flag
        virtual void setFlag ( uint16_t flag, bool value )
        {
            if ( value )
            {
                _flags |= flag;
            }
            else
            {
                _flags &= ~flag;
            }
        }

        /// @brief Get the value of a socket option.
        /// The default implementation gets options that are common to both UDP and TCP sockets.
        /// @param [in] level The protocol level
        /// @param [in] optName The option name
        /// @param [out] optVal The buffer to return the value of the socket option
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t getOption ( int32_t level, int32_t optName, MemHandle & optVal );

        /// @brief Set the value of a socket option
        /// The default implementation sets options that are common to both UDP and TCP sockets.
        /// @param [in] level The protocol level
        /// @param [in] optName The option name
        /// @param [in] optVal The buffer that contains the value to set
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t setOption ( int32_t level, int32_t optName, const MemHandle & optVal );

        /// @brief Get the value of the socket tag
        /// @return The socket tag
        int32_t getTag() const;

        /// @brief Set the value of the socket tag
        /// @param [in] tag The socket tag
        void setTag ( int32_t tag );

        /// @brief Get the local bound address of the socket, invalid if the socket is not bound
        /// @return The local socket address, invalid if the socket is not bound
        inline const SockAddr & getLocalAddr() const
        {
            return _localAddr;
        }

        /// @brief Get the remote connected address of the socket, invalid if the socket is not connected
        /// @return The remote socket address, invalid if the socket is not connected
        inline const SockAddr & getRemoteAddr() const
        {
            return _remoteAddr;
        }

        /// @brief Bind the socket to a local address.
        /// Returns an error if the socket is already bound.
        /// @param [in] address The local address to bind to
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t bind ( const SockAddr & address ) = 0;

        /// @brief Connect the socket to a remote address.
        /// Returns an error if the socket is already connecting or connected.
        /// @param [in] address The remote address to connect to
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t connect ( const SockAddr & address ) = 0;

        /// @brief Send data to the remote connected address. Returns an error if the socket is not connected.
        /// @param [in] data The data to send
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t send ( const MemHandle & data ) = 0;

        /// @brief Send data to the specified remote address. The remote sockAddr is ignored for TCP sockets.
        /// @param [in] data The data to send
        /// @param [in] sockAddr The remote socket address to send to
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t sendTo ( const MemHandle & data, const SockAddr & sockAddr ) = 0;

    protected:
        static TextLog _log; ///< Log stream.

        /// @brief The local socket address
        SockAddr _localAddr;

        /// @brief The remote socket address
        SockAddr _remoteAddr;

        /// @brief Constructor
        /// @param pcb Reference to the pcb pointer in the inheriting class
        LwipSocket ( IpPcbPtr & pcb );

        /// @brief Prepare the socket to bind and/or connect to the given IP address.
        ///
        /// If the IpV4Only flag is set, only V4 addresses are valid.
        /// If the IpV6Only flag is set, only V6 addresses are valid.
        ///
        /// If neither flag is set:
        ///  - If bound to the V4 or V6 any address, then it can connect to both V4 or V6 address.
        ///  - If bound to a non-any V4 address, then it can connect only to a V4 address.
        ///  - If bound to a non-any V6 address, then it can connect only to a V6 address.
        ///
        /// Then if and only if the IP address is valid, the lwIP socket type is changed to match the address.
        /// If not valid, then the internal socket type is unchanged.
        ///
        /// @param [in] addr The IP address to use as the target
        ///
        /// @return True if the IP address is a valid target, and this socket is now prepared.
        ///         False if the socket is not open,
        ///            or if the IP address is not a valid target, and this socket is unchanged.
        bool prepareSocketForAddr ( const ip_addr_t & addr );

        /// @brief Configures timer for handling internal lwip events.
        /// If it needs to be run or changed, it will get scheduled.
        /// It should be called after calling lwip functions that could potentially
        /// change the internal lwIP state resulting in scheduling new events.
        void checkLwipTimer();

        /// @brief Map an lwIP error to an errno code
        /// @param error lwIP error
        /// @return The errno code
        static int32_t mapLwipErrorToErrno ( err_t error );

        /// @brief Convert from a SockAddr to ip_addr_t.
        /// @param [in] addr The SockAddr to be converted.
        /// @param [out] lwipAddr The converted lwIP IP address; this is only assigned on success.
        /// @return True if the conversion was successful, false otherwise.
        /// The conversion will fail if the provided IP address is not a valid V4 or V6 address.
        static bool convertFromSockAddr ( const SockAddr & addr, ip_addr_t & lwipAddr );

        /// @brief Convert an ip_addr_t and port number to a SockAddr.
        /// @param [in] lwipAddr The lwIP IP address to be converted.
        /// @param [in] port The port number to be converted.
        /// @param [out] sa The converted SockAddr; this is only valid on success.
        /// @return True if the conversion was successful, false otherwise.
        /// The conversion will fail if the provided IP address type is neither V4 nor V6.
        static bool convertToSockAddr ( const ip_addr_t & lwipAddr, uint16_t port, SockAddr & sa );

    private:
        /// @brief Pointer to the common lwIP socket type, 0 if the socket is closed
        IpPcbPtr & _pcb;

        /// @brief Various flags (see SocketFlag* constants)
        uint16_t _flags;
};
}
