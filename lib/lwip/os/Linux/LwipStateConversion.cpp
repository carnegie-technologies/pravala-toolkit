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
#include <netinet/tcp.h>

// TCP_MSS is #defined by both headers. We don't care about it.
#undef TCP_MSS

#include "lwip/tcp.h"
}

#include "LwipStateConversion.hpp"

/// @def CASE_TCP_STATE
/// Convert the lwip state to native, by prepending TCP_ to state.
#define CASE_TCP_STATE( state ) \
    case state: \
        return TCP_ ## state;

// The TCP state enum for Linux is only available in <netinet/tcp.h>, not <linux/tcp.h>
// Although both headers define struct tcp_info, the definition in linux/ has added new
// members to the end, including tcpi_segs_out which is needed for calculating the
// retransmission rate. These two headers have many conflicting definitions, so they can't
// both be included. The state conversion needs to be done in a separate file.

uint8_t Pravala::convertLwipToNativeTcpState ( enum tcp_state lwipState )
{
    switch ( lwipState )
    {
        CASE_TCP_STATE ( LISTEN )
        CASE_TCP_STATE ( SYN_SENT )
        CASE_TCP_STATE ( ESTABLISHED )
        CASE_TCP_STATE ( CLOSE_WAIT )
        CASE_TCP_STATE ( CLOSING )
        CASE_TCP_STATE ( LAST_ACK )
        CASE_TCP_STATE ( TIME_WAIT )

        case CLOSED:
            return TCP_CLOSE;

        case SYN_RCVD:
            return TCP_SYN_RECV;

        case FIN_WAIT_1:
            return TCP_FIN_WAIT1;

        case FIN_WAIT_2:
            return TCP_FIN_WAIT2;

        default:
            return 0;
    }
}
