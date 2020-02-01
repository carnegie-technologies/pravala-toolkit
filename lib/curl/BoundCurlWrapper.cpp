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

#include "sys/SocketApi.hpp"
#include "BoundCurlWrapper.hpp"

using namespace Pravala;

/// @brief This function is a replacement for the cURL open socket function.
/// It opens a socket, binds it to a specific interface name, then tries to connect it to the given address.
/// @param [in] clientp Pointer to the iface name String object to bind to before connecting.
/// @param [in] purpose The purpose for which cURL is creating the socket. We only support
/// creating sockets for outbound connections (not accepting inbound connections), so if
/// this is not CURLSOCKTYPE_IPCXN, no socket is created and CURL_SOCKET_BAD is returned.
/// @param [in] address The address to which the socket should be connected.
/// @return The connected socket FD on success, or CURL_SOCKET_BAD on failure.
static curl_socket_t bcurl_opensocket_callback ( void * clientp, curlsocktype purpose, struct curl_sockaddr * address )
{
    if ( !clientp || purpose != CURLSOCKTYPE_IPCXN || !address )
    {
        return CURL_SOCKET_BAD;
    }

    SocketApi::SocketType sockType;

    if ( address->family == AF_INET )
    {
        sockType = ( address->socktype == SOCK_DGRAM ) ? SocketApi::SocketDgram4 : SocketApi::SocketStream4;
    }
    else if ( address->family == AF_INET6 )
    {
        sockType = ( address->socktype == SOCK_DGRAM ) ? SocketApi::SocketDgram6 : SocketApi::SocketStream6;
    }
    else
    {
        return CURL_SOCKET_BAD;
    }

    String ifaceName ( *( const String * ) clientp );

    const int sockFd = ::socket ( address->family, address->socktype, address->protocol );

    if ( sockFd < 0 )
    {
        return CURL_SOCKET_BAD;
    }

    if ( !ifaceName.isEmpty() && NOT_OK ( SocketApi::bindToIface ( sockFd, sockType, ifaceName ) ) )
    {
        SocketApi::close ( sockFd );

        return CURL_SOCKET_BAD;
    }

    const int result = ::connect ( sockFd, &address->addr, address->addrlen );

    if ( result < 0 )
    {
        SocketApi::close ( sockFd );

        return CURL_SOCKET_BAD;
    }

    return sockFd;
}

/// @brief This function is a replacement for the cURL sockopt function.
/// This function is called after cURL creates the socket but before cURL connects it, and
/// allows us to either set additional socket options before connecting, or to tell cURL
/// that the socket is already connected and not to try to connect it again.
/// See http://www.manpagez.com/man/3/CURLOPT_SOCKOPTFUNCTION/ for details.
/// Since the replacement open socket callback returns a socket that is already connected, this
/// function simply returns CURL_SOCKOPT_ALREADY_CONNECTED.
/// @param [in] clientp Unused.
/// @param [in] curlfd Unused.
/// @param [in] purpose Unused.
/// @return CURL_SOCKOPT_ALREADY_CONNECTED.
static int bcurl_sockopt_callback ( void * clientp, curl_socket_t curlfd, curlsocktype purpose )
{
    ( void ) clientp;
    ( void ) curlfd;
    ( void ) purpose;

    return CURL_SOCKOPT_ALREADY_CONNECTED;
}

BoundCurlWrapper::BoundCurlWrapper ( Receiver & receiver ):
    CurlWrapper ( receiver )
{
}

BoundCurlWrapper::BoundCurlWrapper ( Receiver & receiver, CurlMWrapper & mCurl ):
    CurlWrapper ( receiver, mCurl )
{
}

BoundCurlWrapper::~BoundCurlWrapper()
{
}

void BoundCurlWrapper::setBindIfaceName ( const String & ifaceName )
{
    _bindIfaceName = ifaceName;
}

CURL * BoundCurlWrapper::initCurlEasyHandle()
{
    CURL * handle = curl_easy_init();

    if ( !handle )
    {
        return 0;
    }

    CURLcode eCode = CURLE_OK;

    if ( !_bindIfaceName.isEmpty() )
    {
        // TODO use getaddrinfo implementation that can bind to a specific interface IP address
        // UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( handle, CURLOPT_GETADDRINFOFUNCTION, &getaddrinfo ) );
        // UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( handle, CURLOPT_FREEADDRINFOFUNCTION, &freeaddrinfo ) );
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( handle, CURLOPT_OPENSOCKETFUNCTION, &bcurl_opensocket_callback ) );
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( handle, CURLOPT_OPENSOCKETDATA, &_bindIfaceName ) );
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( handle, CURLOPT_SOCKOPTFUNCTION, &bcurl_sockopt_callback ) );
    }

    if ( eCode != CURLE_OK )
    {
        curl_easy_cleanup ( handle );
        return 0;
    }

    return handle;
}
