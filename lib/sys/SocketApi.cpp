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

#ifndef _MSC_VER
#pragma GCC diagnostic push
#if ( __GNUC__ >= 6 ) || ( defined __clang__ )
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

#include <cstdio>
#include <cassert>
#include <cerrno>

#include "OsUtils.hpp"

#include "SocketApi.hpp"

// On Linux these files store max allowed sizes for send/receive socket buffers.
#define WMEM_MAX_PATH    "/proc/sys/net/core/wmem_max"
#define RMEM_MAX_PATH    "/proc/sys/net/core/rmem_max"

using namespace Pravala;

String SocketApi::getLastErrorDesc()
{
#ifdef SYSTEM_WINDOWS
    return String ( "[%1]" ).arg ( WSAGetLastError() );
#else
    const int e = errno;

    return String ( "%1 [%2]" ).arg ( strerror ( e ) ).arg ( e );
#endif
}

const char * SocketApi::getSockTypeName ( SocketApi::SocketType sType )
{
    switch ( sType )
    {
        case SocketInvalid:
            break;

        case SocketLocal:
            return "LocalStream";
            break;

        case SocketLocalSeq:
            return "LocalSeqPacket";
            break;

        case SocketStream4:
            return "TCPv4";
            break;

        case SocketStream6:
            return "TCPv6";
            break;

        case SocketDgram4:
            return "UDPv4";
            break;

        case SocketDgram6:
            return "UDPv6";
            break;
    }

    return "Invalid";
}

int SocketApi::create ( SocketApi::SocketType sockType )
{
#ifdef SYSTEM_WINDOWS
    static bool wsaStarted = false;

    if ( !wsaStarted )
    {
        wsaStarted = true;

        WSADATA wsaData;

        const int err = WSAStartup ( MAKEWORD ( 2, 2 ), &wsaData );

        if ( err != 0 )
        {
            fprintf ( stderr, "SocketApi: Error calling WSAStartup: %d\n", err );
            return -1;
        }
    }
#endif

    // On Linux >= 2.6.27 we could pass SOCK_NONBLOCK and/or O_NONBLOCK to socket() call.
    // For now, however, we don't do that!

    int sockRet = -1;

    switch ( sockType )
    {
        case SocketLocal:
#ifdef SYSTEM_WINDOWS
            return -1;
#else
            sockRet = ::socket ( AF_LOCAL, SOCK_STREAM, 0 );
#endif
            break;

        case SocketLocalSeq:
#ifdef SYSTEM_WINDOWS
            return -1;
#else
            sockRet = ::socket ( AF_LOCAL, SOCK_SEQPACKET, 0 );
#endif
            break;

        case SocketStream4:
            sockRet = ::socket ( AF_INET, SOCK_STREAM, 0 );
            break;

        case SocketStream6:
            sockRet = ::socket ( AF_INET6, SOCK_STREAM, 0 );
            break;

        case SocketDgram4:
            sockRet = ::socket ( AF_INET, SOCK_DGRAM, 0 );
            break;

        case SocketDgram6:
            sockRet = ::socket ( AF_INET6, SOCK_DGRAM, 0 );
            break;

        case SocketInvalid:
            sockRet = -1;
            break;
    }

    if ( sockRet < 0 )
    {
        fprintf ( stderr, "SocketApi: Error creating a socket of type %d (%s): %s [%d]\n",
                  sockType, getSockTypeName ( sockType ), strerror ( errno ), errno );
    }

    return sockRet;
}

bool SocketApi::close ( int sockFd )
{
    if ( sockFd < 0 )
        return false;

#ifdef SYSTEM_WINDOWS
    // On windows calling regular close() on socket crashes.
    // Calling closesocket on regular descriptor just causes an error.
    // Also, most (all on windows?) of the descriptors are sockets anyway.
    // So let's try to treat it like a socket first:
    if ( ::closesocket ( sockFd ) == 0 )
        return true;
#endif

    return ( ::close ( sockFd ) == 0 );
}

/// @brief Tries to increase buffer size of the socket.
/// If the currently used socket's buffer size is greater (or equal) to the size requested,
/// it will NOT be modified (this function newer shrinks the buffer).
/// Otherwise the buffer will be increased up to the size requested, if possible.
/// If it cannot be increased to the requested size, it will be increased as much as possible.
/// @param [in] sockFd The socket descriptor.
/// @param [in] size The requested size (in bytes); Should be > 0; Will be limited to SocketApi::MaxBufferSize.
/// @param [in] optName The option name (must be either SO_RCVBUF or SO_SNDBUF).
/// @param [in] maxPath The path to the file with the max size.
/// @return New buffer size in bytes (even if it was not modified, can be larger than size requested);
///         -1 on error.
static int increaseBufSize ( int sockFd, int size, int optName, const char * maxPath )
{
    if ( size > SocketApi::MaxBufferSize )
    {
        size = SocketApi::MaxBufferSize;
    }

    if ( sockFd < 0 || size < 1 )
    {
        return -1;
    }

    if ( optName != SO_RCVBUF && optName != SO_SNDBUF )
    {
        assert ( false );

        return -1;
    }

    int curSize = -1;

    if ( SocketApi::getOption ( sockFd, SOL_SOCKET, optName, curSize ) && curSize >= size )
    {
        return curSize;
    }

    assert ( size > 0 );

    if ( SocketApi::setOption ( sockFd, SOL_SOCKET, optName, size ) )
    {
        // It worked!
        // The actual value, however, may be lower than what we set.
        // This happens on Linux when we try to use too large value - it sets the max possible value instead.
        // If 'set' succeeded, but 'get' (somehow) failed, we assume that 'size' was set.
        return ( SocketApi::getOption ( sockFd, SOL_SOCKET, optName, curSize ) ) ? curSize : size;
    }

    // We can't set the size requested.
    // Everything below assumes that setOption fails on values that are too large (otherwise the above call would work).
    // Let's see what we can do!
    // First, let's determine the MAX value allowed (which may not work on some platforms).

    const int maxSize = MemHandle ( maxPath ).toString().toInt32();

    if ( maxSize > 0
         && maxSize > curSize
         && maxSize < size
         && SocketApi::setOption ( sockFd, SOL_SOCKET, optName, maxSize ) )
    {
        // We managed to read the max allowed value which is better than current size.
        // If it is greater than size requested, then there is something wrong and we don't want to use that value.
        // Finally, if we successfully set it, we are done.
        return maxSize;
    }

    // We couldn't get the max, or it didn't make sense.
    // Let's try to set the largest possible value using binary search.

    int min = ( curSize > 1 ) ? curSize : 1;
    int max = size;

    assert ( min > 0 );
    assert ( size > 0 );

    while ( min <= max )
    {
        assert ( max <= SocketApi::MaxBufferSize );
        assert ( ( max + min + 1 ) > max );

        const int s = ( max + min + 1 ) / 2;

        assert ( s >= min );
        assert ( s <= max );

        if ( SocketApi::setOption ( sockFd, SOL_SOCKET, optName, s ) )
        {
            // It worked!
            // The next one to try is larger:
            min = s + 1;

            // That's also our new current size.
            curSize = s;
        }
        else
        {
            // It didn't work.
            max = s - 1;
        }
    }

    return curSize;
}

int SocketApi::increaseRcvBufSize ( int sockFd, int size )
{
    return increaseBufSize ( sockFd, size, SO_RCVBUF, RMEM_MAX_PATH );
}

int SocketApi::increaseSndBufSize ( int sockFd, int size )
{
    return increaseBufSize ( sockFd, size, SO_SNDBUF, WMEM_MAX_PATH );
}

bool SocketApi::setOption ( int sockFd, int level, int name, const void * value, socklen_t length )
{
    // We cast void* to char* because Windows functions use that instead.
    // On posix systems we can pass char* as void* anyway.

    return ( sockFd >= 0 && setsockopt ( sockFd, level, name, ( const char * ) value, length ) == 0 );
}

bool SocketApi::getOption ( int sockFd, int level, int name, MemHandle & value )
{
    // The list of sizes to try in auto mode.
    // We don't know what is the largest possible one, so let's try a couple different sizes, just to be safe...
    // The last one MUST be 0.
    static const size_t autoSizes[] = { 16, 4 * 1024, 32 * 1024, 128 * 1024, 1024 * 1024, 0 };

    if ( sockFd < 0 )
    {
        return false;
    }

    const bool autoMode = value.isEmpty();

    for ( int i = 0; autoSizes[ i ] != 0; ++i )
    {
        if ( !autoMode )
        {
            value = MemHandle ( autoSizes[ i ] );
        }

        socklen_t vLen = value.size();

        if ( !SocketApi::getOption ( sockFd, level, name, value.getWritable(), &vLen ) )
        {
            return false;
        }

        if ( !autoMode || vLen < ( socklen_t ) value.size() )
        {
            value.truncate ( vLen );
            return true;
        }

        // The size read is the same as requested.
        // The data may be truncated...
        // Let's try again, with the next size from the list.
    }

    return true;
}

bool SocketApi::getOption ( int sockFd, int level, int name, void * value, socklen_t * length )
{
    // We cast void* to char* because Windows functions use that instead.
    // On posix systems we can pass char* as void* anyway.

    return ( sockFd >= 0 && getsockopt ( sockFd, level, name, ( char * ) value, length ) == 0 );
}

bool SocketApi::getName ( int sockFd, SockAddr & sockAddr )
{
    if ( sockFd < 0 )
        return false;

    socklen_t addrLen = sizeof ( sockAddr );

    return ( getsockname ( sockFd, &( sockAddr.sa ), &addrLen ) == 0 );
}

bool SocketApi::getPeerName ( int sockFd, SockAddr & sockAddr )
{
    if ( sockFd < 0 )
        return false;

    socklen_t addrLen = sizeof ( sockAddr );

    return ( getpeername ( sockFd, &( sockAddr.sa ), &addrLen ) == 0 );
}

bool SocketApi::bind ( int sockFd, const struct sockaddr_in & addr )
{
    if ( sockFd < 0 || addr.sin_family != AF_INET )
    {
        return false;
    }

// BSD systems define sin_len (and sin6_len) as a field; POSIX (including Linux) doesn't.
// Historical reason; neither defines its existence or lack of existence since each thinks they are canonical.
// (POSIX doesn't include sin_len, but doesn't prevent other implementations from extending sockaddr_in to include it).
#ifdef SYSTEM_UNIX
    if ( addr.sin_len != sizeof ( addr ) )
    {
        // On UNIX we also need sin_len field to be set.
        // If it isn't, we need to create a version of the address that has it set.

        struct sockaddr_in tmpAddr;

        tmpAddr = addr;
        tmpAddr.sin_len = sizeof ( tmpAddr );

        return ( 0 == ::bind ( sockFd, ( struct sockaddr * ) &tmpAddr, ( socklen_t ) sizeof ( tmpAddr ) ) );
    }
#endif

    return ( 0 == ::bind ( sockFd,
                           reinterpret_cast<struct sockaddr *> ( const_cast<struct sockaddr_in *> ( &addr ) ),
                           ( socklen_t ) sizeof ( addr ) ) );
}

bool SocketApi::bind ( int sockFd, const struct sockaddr_in6 & addr )
{
    if ( sockFd < 0 || addr.sin6_family != AF_INET6 )
    {
        return false;
    }

// See above.
#ifdef SYSTEM_UNIX
    if ( addr.sin6_len != sizeof ( addr ) )
    {
        // On UNIX we also need sin6_len field to be set.
        // If it isn't, we need to create a version of the address that has it set.

        struct sockaddr_in6 tmpAddr;

        tmpAddr = addr;
        tmpAddr.sin6_len = sizeof ( tmpAddr );

        return ( 0 == ::bind ( sockFd, ( struct sockaddr * ) &tmpAddr, ( socklen_t ) sizeof ( tmpAddr ) ) );
    }
#endif

    return ( 0 == ::bind ( sockFd,
                           reinterpret_cast<struct sockaddr *> ( const_cast<struct sockaddr_in6 *> ( &addr ) ),
                           ( socklen_t ) sizeof ( addr ) ) );
}

bool SocketApi::bind ( int sockFd, const SockAddr & addr )
{
    switch ( addr.sa.sa_family )
    {
        case AF_INET:
            return SocketApi::bind ( sockFd, addr.sa_in );
            break;

        case AF_INET6:
            return SocketApi::bind ( sockFd, addr.sa_in6 );
            break;
    }

    return false;
}

bool SocketApi::listen ( int sockFd, int backlog )
{
    return ( sockFd >= 0 && backlog > 0 && ::listen ( sockFd, backlog ) == 0 );
}

ERRCODE SocketApi::connect ( int sockFd, const struct sockaddr_in & addr )
{
    if ( sockFd < 0 )
    {
        return Error::InvalidParameter;
    }
    else if ( addr.sin_family != AF_INET )
    {
        return Error::InvalidAddress;
    }

// See above.
#ifdef SYSTEM_UNIX
    if ( addr.sin_len != sizeof ( addr ) )
    {
        // On UNIX we also need sin_len field to be set.
        // If it isn't, we need to create a version of the address that has it set.

        struct sockaddr_in tmpAddr;

        tmpAddr = addr;
        tmpAddr.sin_len = sizeof ( tmpAddr );

        if ( 0 == ::connect ( sockFd, ( struct sockaddr * ) &tmpAddr, ( socklen_t ) sizeof ( tmpAddr ) ) )
        {
            return Error::Success;
        }
    }
    else
#endif

    if ( 0 == ::connect ( sockFd,
                          reinterpret_cast<struct sockaddr *> ( const_cast<struct sockaddr_in *> ( &addr ) ),
                          ( socklen_t ) sizeof ( addr ) ) )
    {
        return Error::Success;
    }

#ifdef SYSTEM_WINDOWS
    const int wsaErr = WSAGetLastError();

    if ( wsaErr == WSAEINPROGRESS || wsaErr == WSAEWOULDBLOCK || wsaErr == WSAEALREADY || wsaErr == WSAEINVAL )
#else
    if ( errno == EINPROGRESS || errno == EALREADY )
#endif
    {
        return Error::ConnectInProgress;
    }

    return Error::ConnectFailed;
}

ERRCODE SocketApi::connect ( int sockFd, const struct sockaddr_in6 & addr )
{
    if ( sockFd < 0 )
    {
        return Error::InvalidParameter;
    }
    else if ( addr.sin6_family != AF_INET6 )
    {
        return Error::InvalidAddress;
    }

// See above.
#ifdef SYSTEM_UNIX
    if ( addr.sin6_len != sizeof ( addr ) )
    {
        // On UNIX we also need sin6_len field to be set.
        // If it isn't, we need to create a version of the address that has it set.

        struct sockaddr_in6 tmpAddr;

        tmpAddr = addr;
        tmpAddr.sin6_len = sizeof ( tmpAddr );

        if ( 0 == ::connect ( sockFd, ( struct sockaddr * ) &tmpAddr, ( socklen_t ) sizeof ( tmpAddr ) ) )
        {
            return Error::Success;
        }
    }
    else
#endif

    if ( 0 == ::connect ( sockFd,
                          reinterpret_cast<struct sockaddr *> ( const_cast<struct sockaddr_in6 *> ( &addr ) ),
                          ( socklen_t ) sizeof ( addr ) ) )
    {
        return Error::Success;
    }

#ifdef SYSTEM_WINDOWS
    const int wsaErr = WSAGetLastError();

    if ( wsaErr == WSAEINPROGRESS || wsaErr == WSAEWOULDBLOCK || wsaErr == WSAEALREADY || wsaErr == WSAEINVAL )
#else
    if ( errno == EINPROGRESS || errno == EALREADY )
#endif
    {
        return Error::ConnectInProgress;
    }

    return Error::ConnectFailed;
}

ERRCODE SocketApi::connect ( int sockFd, const SockAddr & addr )
{
    switch ( addr.sa.sa_family )
    {
        case AF_INET:
            return SocketApi::connect ( sockFd, addr.sa_in );
            break;

        case AF_INET6:
            return SocketApi::connect ( sockFd, addr.sa_in6 );
            break;
    }

    return Error::InvalidAddress;
}

int SocketApi::accept ( int sockFd, SockAddr & addr )
{
    if ( sockFd < 0 )
        return -1;

    socklen_t addrLen = sizeof ( addr );

    const int ret = ::accept ( sockFd, &addr.sa, &addrLen );

    if ( ret < 0 || addr.isIPv4() )
    {
        return ret;
    }

    if ( addr.isIPv6() )
    {
        if ( addr.isIPv6MappedIPv4() )
        {
            addr.convertToV4();
        }

        return ret;
    }

    // This could mean that we tried to call this version of accept on AF_LOCAL socket!
    SocketApi::close ( ret );
    return -1;
}

int SocketApi::accept ( int sockFd, IpAddress & addr, uint16_t & port )
{
    SockAddr sAddr;

    const int ret = SocketApi::accept ( sockFd, sAddr );

    if ( ret < 0 )
    {
        return -1;
    }

    if ( sAddr.isIPv4() )
    {
        addr = sAddr.sa_in.sin_addr;
    }
    else if ( sAddr.isIPv6() )
    {
        addr = sAddr.sa_in6.sin6_addr;
    }
    else
    {
        // Shouldn't happen, but doesn't hurt us to handle it...
        SocketApi::close ( ret );
        return -1;
    }

    port = sAddr.getPort();
    return ret;
}

int SocketApi::createUdpSocket ( IpAddress::AddressType addrType, bool reuseAddrPort, ERRCODE * reason )
{
    int sockFd = -1;

    if ( addrType == IpAddress::V4Address )
    {
        sockFd = SocketApi::create ( SocketApi::SocketDgram4 );
    }
    else if ( addrType == IpAddress::V6Address )
    {
        sockFd = SocketApi::create ( SocketApi::SocketDgram6 );
    }
    else
    {
        fprintf ( stderr, "SocketApi: Could not create a socket; Invalid type: %d\n", addrType );

        if ( reason != 0 )
            *reason = Error::InvalidParameter;

        return -1;
    }

    if ( sockFd < 0 )
    {
        fprintf ( stderr, "SocketApi: Error creating a socket: %s\n", getLastErrorDesc().c_str() );

        if ( reason != 0 )
            *reason = Error::SocketFailed;

        return -1;
    }

    if ( reuseAddrPort )
    {
        if ( !SocketApi::setOption ( sockFd, SOL_SOCKET, SO_REUSEADDR, ( int ) 1 ) )
        {
            fprintf ( stderr, "SocketApi: Error calling setsockopt(SO_REUSEADDR): %s\n", getLastErrorDesc().c_str() );
        }

#ifdef SO_REUSEPORT
        if ( !SocketApi::setOption ( sockFd, SOL_SOCKET, SO_REUSEPORT, ( int ) 1 ) )
        {
            fprintf ( stderr, "SocketApi: Error calling setsockopt(SO_REUSEPORT): %s\n", getLastErrorDesc().c_str() );
        }
#endif
    }

#ifdef IPV6_V6ONLY
    if ( addrType == IpAddress::V6Address && !SocketApi::setOption ( sockFd, IPPROTO_IPV6, IPV6_V6ONLY, ( int ) 0 ) )
    {
        fprintf ( stderr, "SocketApi: Error calling setsockopt(IPV6_V6ONLY): %s\n", getLastErrorDesc().c_str() );
    }
#endif

    if ( addrType == IpAddress::V4Address )
    {
#if defined( IP_MTU_DISCOVER ) && defined( IP_PMTUDISC_DONT )
        const long int optVal = IP_PMTUDISC_DONT;

        if ( !SocketApi::setOption ( sockFd, IPPROTO_IP, IP_MTU_DISCOVER, optVal ) )
        {
            fprintf ( stderr, "SocketApi: Error calling setsockopt(IP_MTU_DISCOVER): %s\n",
                      getLastErrorDesc().c_str() );
        }
#endif
    }
    else if ( addrType == IpAddress::V6Address )
    {
#if defined( IPV6_MTU_DISCOVER ) && defined( IPV6_PMTUDISC_DONT )
        const long int optVal = IPV6_PMTUDISC_DONT;

        if ( !SocketApi::setOption ( sockFd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, optVal ) )
        {
            fprintf ( stderr, "SocketApi: Error calling setsockopt(IPV6_MTU_DISCOVER): %s\n",
                      getLastErrorDesc().c_str() );
        }
#endif
    }

    if ( reason != 0 )
        *reason = Error::Success;

    return sockFd;
}

int SocketApi::createUdpSocket ( const SockAddr & localAddr, bool reuseAddrPort, ERRCODE * reason )
{
    int sockFd = -1;

    if ( localAddr.isIPv4() )
    {
        sockFd = SocketApi::createUdpSocket ( IpAddress::V4Address, reuseAddrPort, reason );
    }
    else if ( localAddr.isIPv6() )
    {
        sockFd = SocketApi::createUdpSocket ( IpAddress::V6Address, reuseAddrPort, reason );
    }
    else if ( reason != 0 )
    {
        *reason = Error::InvalidAddress;
        return -1;
    }

    if ( sockFd < 0 )
        return sockFd;

    if ( !SocketApi::bind ( sockFd, localAddr ) )
    {
        fprintf ( stderr, "SocketApi: Error calling bind(%s): %s\n",
                  localAddr.toString().c_str(), getLastErrorDesc().c_str() );

        SocketApi::close ( sockFd );

        if ( reason != 0 )
            *reason = Error::BindFailed;

        return -1;
    }

    if ( reason != 0 )
        *reason = Error::Success;

    return sockFd;
}

int SocketApi::createListeningTcpSocket ( const SockAddr & localAddr, int backLog, ERRCODE * reason )
{
    assert ( backLog > 0 );

    int sockFd = -1;

    if ( localAddr.isIPv4() )
    {
        sockFd = SocketApi::create ( SocketStream4 );
    }
    else if ( localAddr.isIPv6() )
    {
        sockFd = SocketApi::create ( SocketStream6 );
    }
    else if ( reason != 0 )
    {
        *reason = Error::InvalidAddress;
        return -1;
    }

    if ( sockFd < 0 )
    {
        if ( reason != 0 )
            *reason = Error::SocketFailed;

        return sockFd;
    }

    if ( !SocketApi::setOption ( sockFd, SOL_SOCKET, SO_REUSEADDR, ( int ) 1 ) )
    {
        fprintf ( stderr, "SocketApi: Error calling setsockopt(SO_REUSEADDR): %s\n", getLastErrorDesc().c_str() );
    }

#ifdef SO_REUSEPORT
    if ( !SocketApi::setOption ( sockFd, SOL_SOCKET, SO_REUSEPORT, ( int ) 1 ) )
    {
        fprintf ( stderr, "SocketApi: Error calling setsockopt(SO_REUSEPORT): %s\n", getLastErrorDesc().c_str() );
    }
#endif

    if ( !SocketApi::bind ( sockFd, localAddr ) )
    {
        fprintf ( stderr, "SocketApi: Error calling bind(%s): %s\n",
                  localAddr.toString().c_str(), getLastErrorDesc().c_str() );

        SocketApi::close ( sockFd );

        if ( reason != 0 )
            *reason = Error::BindFailed;

        return -1;
    }

    if ( !SocketApi::listen ( sockFd, backLog ) )
    {
        fprintf ( stderr, "SocketApi: Error calling listen(%s): %s\n",
                  localAddr.toString().c_str(), getLastErrorDesc().c_str() );

        SocketApi::close ( sockFd );

        if ( reason != 0 )
            *reason = Error::ListenFailed;

        return -1;
    }

    if ( reason != 0 )
        *reason = Error::Success;

    return sockFd;
}
