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

#include "../../SocketApi.hpp"

#include <cerrno>

extern "C"
{
#include <unistd.h>
#include <fcntl.h>
}

using namespace Pravala;

bool SocketApi::setNonBlocking ( int sockFd, bool nonBlocking )
{
    if ( sockFd < 0 )
    {
        return false;
    }

    int flags = fcntl ( sockFd, F_GETFL, 0 );

    if ( nonBlocking && ( flags < 0 || ( flags & O_NONBLOCK ) == 0 ) )
    {
        if ( flags < 0 ) flags = 0;

        // It should be non-blocking, but it's not (or we don't know)

        return ( fcntl ( sockFd, F_SETFL, flags | O_NONBLOCK ) >= 0 );
    }

    if ( !nonBlocking && ( flags < 0 || ( flags & O_NONBLOCK ) != 0 ) )
    {
        if ( flags < 0 ) flags = 0;

        // It shouldn't be non-blocking, but it is (or we don't know)

        return ( fcntl ( sockFd, F_SETFL, flags & ~O_NONBLOCK ) >= 0 );
    }

    return true;
}

bool SocketApi::bind ( int sockFd, const String & name )
{
    const int len = name.length();
    struct sockaddr_un addr;

    // sun_path should fit the string + \0
    if ( sockFd < 0 || len < 1 || ( size_t ) len >= sizeof ( addr.sun_path ) )
    {
        return false;
    }

    memset ( &addr, 0, sizeof ( addr ) );

    addr.sun_family = AF_LOCAL;

// BSD systems define sun_len as a field; POSIX (including Linux) doesn't.
// Historical reason; neither defines its existence or lack of existence since each thinks they are canonical.
// (POSIX doesn't include sin_len, but doesn't prevent other implementations from extending sockaddr_in to include it).
#ifdef SYSTEM_UNIX
    addr.sun_len = sizeof ( addr );
#endif

    assert ( len > 0 );

    // sun_path should fit the string + \0
    assert ( ( size_t ) len < sizeof ( addr.sun_path ) );

    strncpy ( addr.sun_path, name.c_str(), len );

    // Let's check what the first character is. If it is '@', let's change it to \0 to use the abstract namespace
    if ( addr.sun_path[ 0 ] == '@' )
        addr.sun_path[ 0 ] = 0;

    return ( ::bind ( sockFd, ( const struct sockaddr * ) &addr, sizeof ( addr ) ) == 0 );
}

ERRCODE SocketApi::connect ( int sockFd, const String & name )
{
    const int len = name.length();
    struct sockaddr_un addr;

    if ( sockFd < 0 )
    {
        return Error::NotInitialized;
    }

    // sun_path should fit the string + \0
    if ( len < 1 || ( size_t ) len >= sizeof ( addr.sun_path ) )
    {
        return Error::InvalidParameter;
    }

    memset ( &addr, 0, sizeof ( addr ) );

    addr.sun_family = AF_LOCAL;

// See above.
#ifdef SYSTEM_UNIX
    addr.sun_len = sizeof ( addr );
#endif

    assert ( len > 0 );

    // sun_path should fit the string + \0
    assert ( ( size_t ) len < sizeof ( addr.sun_path ) );

    strncpy ( addr.sun_path, name.c_str(), len );

    // Let's check what the first character is. If it is '@', let's change it to \0 to use the abstract namespace
    if ( addr.sun_path[ 0 ] == '@' )
        addr.sun_path[ 0 ] = 0;

    int ret = ::connect ( sockFd, ( const struct sockaddr * ) &addr, sizeof ( addr ) );

    if ( ret == 0 )
        return Error::Success;

    if ( errno == EINPROGRESS || errno == EALREADY )
        return Error::ConnectInProgress;

    return Error::ConnectFailed;
}

int SocketApi::accept ( int sockFd, String & name )
{
    if ( sockFd < 0 )
        return -1;

    union LocalSockAddr
    {
        struct sockaddr sa;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa_in6;
        struct sockaddr_un sa_un;
    } sockAddr;

    socklen_t addrLen = sizeof ( sockAddr );

    memset ( &sockAddr, 0, addrLen );

    int ret = ::accept ( sockFd, ( struct sockaddr * ) &sockAddr, &addrLen );

    if ( ret < 0 )
        return -1;

    if ( sockAddr.sa.sa_family != AF_LOCAL )
    {
        // This could mean that we tried to call this version of accept on AF_INET or AF_INET6 socket!
        SocketApi::close ( ret );
        return -1;
    }

    name.clear();

    if ( sockAddr.sa_un.sun_path[ 0 ] == '@' )
    {
        // This is unlikely, but we don't want to have any problems...
        // In case the name was not in abstract namespace, but it just had '@' at the beginning,
        // let's say it was an error!
        SocketApi::close ( ret );
        return -1;
    }
    else if ( sockAddr.sa_un.sun_path[ 0 ] == 0 )
    {
        // This is the abstract namespace. Let's change the first 0 to '@'
        sockAddr.sa_un.sun_path[ 0 ] = '@';
    }

    // addrLen is the length of the entire address returned, including the sun_family and the path.
    // So the number of 'name' characters is addrLen minus the size of sun_family.
    name.append ( sockAddr.sa_un.sun_path, addrLen - sizeof ( sockAddr.sa_un.sun_family ) );

    return ret;
}
