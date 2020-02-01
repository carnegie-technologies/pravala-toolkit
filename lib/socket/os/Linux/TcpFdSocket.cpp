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

extern "C"
{
#include <linux/tcp.h>
}

#include <cstdio>

#include "basic/Math.hpp"
#include "../../TcpFdSocket.hpp"

using namespace Pravala;

uint16_t TcpFdSocket::detectMtu ( int fd )
{
    if ( fd < 0 )
        return 0;

    struct tcp_info info;
    socklen_t infoLen = sizeof ( info );

    memset ( &info, 0, sizeof ( info ) );

    if ( getsockopt ( fd, IPPROTO_TCP, TCP_INFO, &info, &infoLen ) < 0 )
    {
        return 0;
    }

    uint32_t mss = info.tcpi_snd_mss;

    if ( mss < 1 )
    {
        mss = info.tcpi_rcv_mss;
    }
    else if ( info.tcpi_rcv_mss > mss )
    {
        // We want the larger of the two. We have seen that when MSS in SYN packets is lowered,
        // TCP drops one of these to much lower value.
        // Specifically, when MSS is decreased from 1460 to 1408 on our Wi-Fi network,
        // tcpi_snd_mss is set to 1396 and tcpi_rcv_mss to 978 (with timestamps enabled).
        // This may need to be further improved.
        mss = info.tcpi_rcv_mss;
    }

    // 0 or way too big.
    if ( mss < 1 || mss > 0x7FFF )
    {
        return 0;
    }

    if ( info.tcpi_options & TCPI_OPT_TIMESTAMPS )
    {
        // Timestamps cause the MSS to be lower.
        // Timestamp itself uses 10 bytes, but must be padded to 4-byte boundary (with NOP options).
        // Effectively they take up 12 bytes:
        mss += 12;
    }

    // MTU = MSS + 40 bytes (20 bytes for IPv4 header, and 20 bytes for TCP header)

    /// @todo TODO: Add support for IPv6!

    return ( uint16_t ) mss + 40;
}
