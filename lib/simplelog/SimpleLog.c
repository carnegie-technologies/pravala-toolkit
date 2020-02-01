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

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "SimpleLog.h"

/// @brief Internal flag controlling whether SimpleLogs are enabled or not.
static int SimpleLogsEnabledFlag = 0;

int simpleLogsEnabled()
{
    return SimpleLogsEnabledFlag;
}

void simpleLogsSetEnabled ( int enabled )
{
    // We don't control access to this flag using a mutex because it would be costly.
    // Also, having it not apply right away is not a big deal anyway.
    SimpleLogsEnabledFlag = enabled;
}

const char * simpleLogAddrDesc ( const struct sockaddr * addr, socklen_t addrLen, struct SimpleLogAddrDescBuf * buf )
{
    if ( !addr || addrLen < 1 )
    {
        return "EMPTY";
    }

    if ( !buf )
    {
        return "INVALID-BUF";
    }

    buf->data[ 0 ] = 0;

    const char * ret = 0;
    unsigned port = 0;

    if ( addr->sa_family == AF_INET && addrLen >= sizeof ( struct sockaddr_in ) )
    {
        ret = inet_ntop (
            AF_INET, &( ( ( const struct sockaddr_in * ) addr )->sin_addr ), buf->data, sizeof ( buf->data ) );

        port = ntohs ( ( ( const struct sockaddr_in * ) addr )->sin_port );
    }
    else if ( addr->sa_family == AF_INET6 && addrLen >= sizeof ( struct sockaddr_in6 ) )
    {
        ret = inet_ntop (
            AF_INET6, &( ( ( const struct sockaddr_in6 * ) addr )->sin6_addr ), buf->data, sizeof ( buf->data ) );

        port = ntohs ( ( ( const struct sockaddr_in6 * ) addr )->sin6_port );
    }

    if ( !ret )
    {
        return "INVALID";
    }

    size_t i;

    for ( i = 0; i < sizeof ( buf->data ); ++i )
    {
        if ( buf->data[ i ] == 0 )
        {
            if ( i + 1 < sizeof ( buf->data ) )
            {
                snprintf ( buf->data + i, sizeof ( buf->data ) - i, ":%u", port );
            }

            break;
        }
    }

    buf->data[ sizeof ( buf->data ) - 1 ] = 0;
    return ret;
}
