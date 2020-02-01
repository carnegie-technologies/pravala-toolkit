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

#include "basic/IpAddress.hpp"
#include "error/Error.hpp"

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
}
#else
extern "C"
{
#include <sys/socket.h>
#include <sys/un.h>
}
#endif

namespace Pravala
{
/// @brief A wrapper around POSIX socket API
class SocketApi
{
    public:
        /// @brief The type of the socket
        enum SocketType
        {
            SocketInvalid,     ///< Invalid

            SocketLocal,     ///< AF_LOCAL + SOCK_STREAM
            SocketLocalSeq,     ///< AF_LOCAL + SOCK_SEQPACKET

            SocketStream4,     ///< AF_INET + SOCK_STREAM
            SocketStream6,     ///< AF_INET6 + SOCK_STREAM

            SocketDgram4,     ///< AF_INET + SOCK_DGRAM
            SocketDgram6     ///< AF_INET6 + SOCK_DGRAM
        };

        /// @brief Max buffer size supported by increase*BufSize methods (in bytes).
        static const int MaxBufferSize = 512 * 1024 * 1024;

        /// @brief A helper function that returns the name of SocketType value.
        /// @param [in] sType The type to check.
        /// @return The name of the given type (or "Invalid").
        static const char * getSockTypeName ( SocketType sType );

        /// @brief Closes socket descriptor in socket-specific way
        /// It should be used instead of calling ::close() directly!
        /// @param [in] sockFd The socket descriptor to close
        /// @return True on success; False on failure
        static bool close ( int sockFd );

        /// @brief Modifies the 'blocking' mode of the socket
        /// @param [in] sockFd The socket descriptor
        /// @param [in] nonBlocking If true (default), the descriptor will be set in 'non-blocking' mode.
        ///                         If false, it will be set in 'blocking' mode.
        /// @return True on success, false on error.
        static bool setNonBlocking ( int sockFd, bool nonBlocking = true );

        /// @brief Binds the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The local IPv4 address and port to bind to
        /// @return True on success, false on error.
        static bool bind ( int sockFd, const sockaddr_in & addr );

        /// @brief Binds the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The local IPv6 address and port to bind to
        /// @return True on success, false on error.
        static bool bind ( int sockFd, const sockaddr_in6 & addr );

        /// @brief Binds the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The SockAddr address to bind to
        /// @return True on success, false on error.
        static bool bind ( int sockFd, const SockAddr & addr );

        /// @brief Binds the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The local IP address to bind to
        /// @param [in] port The local port number to bind to
        /// @return True on success, false on error.
        inline static bool bind ( int sockFd, const IpAddress & addr, uint16_t port )
        {
            return SocketApi::bind ( sockFd, SockAddr ( addr, port ) );
        }

        /// @brief Binds the local socket to the given name
        /// @param [in] sockFd The socket descriptor
        /// @param [in] name The name to bind the socket to. If the first character is '@' then
        ///                   the abstract namespace is used (and the first character is removed).
        ///                  To use abstract namespace with a name starting with '@', use it twice.
        /// @return True on success, false on error.
        static bool bind ( int sockFd, const String & name );

        /// @brief Binds a socket to a physical network interface.
        /// @param [in] sockFd The socket descriptor.
        /// @param [in] sockType The socket type, used to determine whether to bind in V4 or V6 mode.
        /// @param [in] ifaceName The physical interface name to bind the socket to.
        /// @return Standard error code.
        static ERRCODE bindToIface ( int sockFd, SocketType sockType, const String & ifaceName );

        /// @brief Connects the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The local IPv4 address and port to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been started.
        static ERRCODE connect ( int sockFd, const sockaddr_in & addr );

        /// @brief Connects the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The local IPv6 address and port to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been started.
        static ERRCODE connect ( int sockFd, const sockaddr_in6 & addr );

        /// @brief Connects the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The SockAddr address and port to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been started.
        static ERRCODE connect ( int sockFd, const SockAddr & addr );

        /// @brief Connects the socket to the address
        /// @param [in] sockFd The socket descriptor
        /// @param [in] addr The IP address to connect to
        /// @param [in] port The port number to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been started.
        inline static ERRCODE connect ( int sockFd, const IpAddress & addr, uint16_t port )
        {
            return SocketApi::connect ( sockFd, SockAddr ( addr, port ) );
        }

        /// @brief Connects the local socket to the given name
        /// @param [in] sockFd The socket descriptor
        /// @param [in] name The name to connect to. If the first character is '@' then
        ///                   the abstract namespace is used (and the first character is removed).
        ///                  To use abstract namespace with a name starting with '@', use it twice.
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been started.
        static ERRCODE connect ( int sockFd, const String & name );

        /// @brief Starts listening on the socket for incoming connections
        /// @param [in] sockFd The socket descriptor
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        /// @return True on success, false on error.
        static bool listen ( int sockFd, int backlog );

        /// @brief Sets the socket option
        /// @param [in] sockFd the socket descriptor
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to set
        /// @param [in] value the value to set
        /// @param [in] length the length of the value to set
        /// @return True on success, false on error.
        static bool setOption ( int sockFd, int level, int name, const void * value, socklen_t length );

        /// @brief Sets the socket option
        /// @param [in] sockFd the socket descriptor
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to set
        /// @param [in] value the value to set
        /// @return True on success, false on error.
        template<typename T> static bool setOption ( int sockFd, int level, int name, const T & value )
        {
            return setOption ( sockFd, level, name, ( const void * ) &value, sizeof ( value ) );
        }

        /// @brief Gets the socket option
        /// @param [in] sockFd the socket descriptor
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to get
        /// @param [in,out] value Location to store the value read.
        ///                       If not empty, the size of the memory will be used as option value size.
        ///                       Otherwise an automatic mode will be used (which will try sizes up to 1MB).
        ///                       In the automatic mode multiple calls to getsockopt() may be made.
        /// @return True on success, false on error.
        static bool getOption ( int sockFd, int level, int name, MemHandle & value );

        /// @brief Gets the socket option
        /// @param [in] sockFd the socket descriptor
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to get
        /// @param [out] value the value to get
        /// @param [in,out] length the length of the value to get (the actual value will be stored there)
        /// @return True on success, false on error.
        static bool getOption ( int sockFd, int level, int name, void * value, socklen_t * length );

        /// @brief Gets the socket option
        /// @param [in] sockFd the socket descriptor
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to get
        /// @param [out] value the value to get
        /// @return True on success, false on error.
        template<typename T> static bool getOption ( int sockFd, int level, int name, T & value )
        {
            socklen_t len = sizeof ( value );

            /// @todo Maybe we want to allow the different length?

            return ( getOption ( sockFd, level, name, ( void * ) &value, &len ) && len == sizeof ( value ) );
        }

        /// @brief Tries to increase receive buffer size of the socket.
        /// This function changes SO_RCVBUF option of the socket.
        /// If the currently used socket's buffer size is greater (or equal) to the size requested,
        /// it will NOT be modified (this function newer shrinks the buffer).
        /// Otherwise the buffer will be increased up to the size requested, if possible.
        /// If it cannot be increased to the requested size, it will be increased as much as possible.
        /// @param [in] sockFd The socket descriptor.
        /// @param [in] size The requested size (in bytes); Should be > 0; Will be limited to SocketApi::MaxBufferSize.
        /// @return New receive buffer size in bytes (even if it was not modified, can be larger than size requested);
        ///         -1 on error.
        static int increaseRcvBufSize ( int sockFd, int size );

        /// @brief Tries to increase send buffer size of the socket.
        /// This function changes SO_SNDBUF option of the socket.
        /// If the currently used socket's buffer size is greater (or equal) to the size requested,
        /// it will NOT be modified (this function newer shrinks the buffer).
        /// Otherwise the buffer will be increased up to the size requested, if possible.
        /// If it cannot be increased to the requested size, it will be increased as much as possible.
        /// @param [in] sockFd The socket descriptor.
        /// @param [in] size The requested size (in bytes); Should be > 0; Will be limited to SocketApi::MaxBufferSize.
        /// @return New send buffer size in bytes (even if it was not modified, can be larger than size requested);
        ///         -1 on error.
        static int increaseSndBufSize ( int sockFd, int size );

        /// @brief Reads the socket name (using getsockname) from the socket
        /// @param [in] sockFd The socket descriptor
        /// @param [out] sockAddr The address read
        /// @return True on success; False on failure
        static bool getName ( int sockFd, SockAddr & sockAddr );

        /// @brief Reads the remote peer's name (using getpeername) from the socket
        /// @param [in] sockFd The socket descriptor
        /// @param [out] sockAddr The address read
        /// @return True on success; False on failure
        static bool getPeerName ( int sockFd, SockAddr & sockAddr );

        /// @brief Creates a new socket (using socket() system call)
        /// @param [in] sockType The type of the socket
        /// @return The descriptor of the new socket, or -1 on error.
        static int create ( SocketType sockType );

        /// @brief Accepts a connection on the internet socket
        /// @param [in] sockFd The socket descriptor to accept() on
        /// @param [out] addr The SockAddr to store the address of the incoming socket in
        /// @return The descriptor of the new socket, or -1 on error.
        static int accept ( int sockFd, SockAddr & addr );

        /// @brief Accepts a connection on the internet socket
        /// @param [in] sockFd The socket descriptor to accept() on
        /// @param [out] addr The IP address of the incoming socket
        /// @param [out] port The port number of the incoming socket
        /// @return The descriptor of the new socket, or -1 on error.
        static int accept ( int sockFd, IpAddress & addr, uint16_t & port );

        /// @brief Accepts a connection on the local socket
        /// @param [in] sockFd The socket descriptor to accept() on
        /// @param [out] name The name on which it was accepted.
        ///                    If the abstract namespace was used, the name is prepended with '@' character.
        /// @return The descriptor of the new socket, or -1 on error.
        static int accept ( int sockFd, String & name );

        /// @brief Creates a UDP socket with some pre-set options
        /// It creates a socket and tries to set following options:
        /// SO_REUSEADDR, SO_REUSEPORT (if available), IPV6_V6ONLY (only for IPv6).
        /// Next it disables IP MTU discovery (which means that the "don't fragment" bit will not be used).
        /// @param [in] addrType The type of the address that the socket will be using
        /// @param [in] reuseAddrPort If set to true, SO_REUSEADDR and SO_REUSEPORT will be set (if available)
        /// @param [out] reason If used, the error code is put there.
        /// @return The file descriptor of the socket created, or -1 on error.
        static int createUdpSocket ( IpAddress::AddressType addrType, bool reuseAddrPort, ERRCODE * reason = 0 );

        /// @brief Creates a UDP socket with some pre-set options
        /// It creates a socket and tries to set following options:
        /// SO_REUSEADDR, SO_REUSEPORT (if available), IPV6_V6ONLY (only for IPv6).
        /// Next it disables IP MTU discovery (which means that the "don't fragment" bit will not be used).
        /// It also binds it to the local port and address.
        /// @param [in] localAddr Local address and port to use
        /// @param [in] reuseAddrPort If set to true, SO_REUSEADDR and SO_REUSEPORT will be set (if available)
        /// @param [out] reason If used, the error code is put there.
        /// @return The file descriptor of the socket created, or -1 on error.
        static int createUdpSocket ( const SockAddr & localAddr, bool reuseAddrPort, ERRCODE * reason = 0 );

        /// @brief Creates a UDP socket with some pre-set options
        /// It creates a socket and tries to set following options:
        /// SO_REUSEADDR, SO_REUSEPORT (if available), IPV6_V6ONLY (only for IPv6).
        /// Next it disables IP MTU discovery (which means that the "don't fragment" bit will not be used).
        /// It also binds it to the local port and address.
        /// @param [in] localAddr Local address to use
        /// @param [in] localPort Local port to use
        /// @param [in] reuseAddrPort If set to true, SO_REUSEADDR and SO_REUSEPORT will be set (if available)
        /// @param [out] reason If used, the error code is put there.
        /// @return The file descriptor of the socket created, or -1 on error.
        inline static int createUdpSocket (
            const IpAddress & localAddr, uint16_t localPort, bool reuseAddrPort,
            ERRCODE * reason = 0 )
        {
            return SocketApi::createUdpSocket ( SockAddr ( localAddr, localPort ), reuseAddrPort, reason );
        }

        /// @brief Creates a listening TCP socket
        /// @param [in] localAddr Local address and port to use
        /// @param [in] backLog Maximum connection backlog allowed
        /// @param [out] reason If used, the error code is put there.
        /// @return The file descriptor of the socket created, or -1 on error.
        static int createListeningTcpSocket ( const SockAddr & localAddr, int backLog, ERRCODE * reason = 0 );

        /// @brief Creates a listening TCP socket
        /// @param [in] localAddr Local address to use
        /// @param [in] localPort Local port to use
        /// @param [in] backLog Maximum connection backlog allowed
        /// @param [out] reason If used, the error code is put there.
        /// @return The file descriptor of the socket created, or -1 on error.
        inline static int createListeningTcpSocket (
            const IpAddress & localAddr, uint16_t localPort,
            int backLog, ERRCODE * reason = 0 )
        {
            return SocketApi::createListeningTcpSocket ( SockAddr ( localAddr, localPort ), backLog, reason );
        }

        /// @brief Returns the error message associated with the last operation.
        /// On Windows it uses WSAGetLastError(). Everywhere else it uses errno.
        /// @return The error message associated with the last operation.
        static String getLastErrorDesc();

        /// @brief Checks whether the errno value describes a 'soft' error
        /// If this returns true, the caller can retry this call (either immediately or later).
        /// @return True if the errno describes a soft/temporary error (like EAGAIN)
        static inline bool isErrnoSoft()
        {
#ifdef SYSTEM_WINDOWS
            return ( WSAGetLastError() == WSAEWOULDBLOCK );
#else
            return ( false
#ifdef EAGAIN
                     || errno == EAGAIN
#endif

#ifdef EWOULDBLOCK
                     || errno == EWOULDBLOCK
#endif

#ifdef EINTR
                     || errno == EINTR
#endif

#ifdef ENOBUFS
                     || errno == ENOBUFS
#endif

#ifdef ENOMEM
                     || errno == ENOMEM
#endif
                     // Do not treat EPERM as a soft error. See comment in isErrnoNonFatal().
            );
#endif
        }

        /// @brief Checks whether the errno value describes a condition where the FD can still be used.
        /// If this returns true, then the caller should not retry the call as it will likely fail again, but we also
        /// don't need to close the underlying socket.
        /// The caller may check if it can retry the call by calling isErrnoSoft().
        /// @note On Windows, this returns simply isErrnoSoft() - at least for the moment. It may need to be improved.
        /// @warning This treats EINVAL as a non-fatal error! On Android, when the OS closes socket on us,
        ///          that error will be returned while attempting to write to the UDP socket.
        ///          Because that depends on the context, it must be dealt with elsewhere.
        /// @return True if the errno is non-fatal (i.e. the errno is a soft-error, or another non-fatal error).
        static inline bool isErrnoNonFatal()
        {
#ifdef SYSTEM_WINDOWS
            return ( isErrnoSoft() );
#else
            return ( isErrnoSoft()
#ifdef EFAULT
                     || errno == EFAULT  // Bad memory address passed
#endif

#ifdef EACCES
                     || errno == EACCES  // Trying to send a packet to a destination without adequate permissions.
#endif

#ifdef ENETUNREACH
                     || errno == ENETUNREACH // Network unreachable
#endif

#ifdef EHOSTUNREACH
                     || errno == EHOSTUNREACH // Host unreachable
#endif

#ifdef EINVAL
                     || errno == EINVAL // Invalid argument - e.g. destination a blackhole route
#endif

#ifdef ENOPROTOOPT
                     || errno == ENOPROTOOPT // Bad flag
#endif

#ifdef EOPNOTSUPP
                     || errno == EOPNOTSUPP // Bad flag
#endif

#ifdef ENOENT
                     || errno == ENOENT // SIOCGSTAMP without packet
#endif

#ifdef EMSGSIZE
                     || errno == EMSGSIZE // Message too big
#endif

#ifdef EISCONN
                     || errno == EISCONN // Already connected. Also sendto() on connected socket on Apple.
#endif

#ifdef EPERM
                     || errno == EPERM // Operation not permitted - e.g. blocked by FW rules
                     // This seems to also appear in various netfilter error cases that would
                     // be "soft" errors, but there doesn't seem to be a way to determine
                     // whether it's "soft" error or one that may succeed if we retry.
#endif
            );
#endif
        }
};
}
