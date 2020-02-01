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
#include <linux/version.h>

#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
}

#include <cerrno>

#include "basic/Math.hpp"

#include "LwipStateConversion.hpp"
#include "../../LwipTcpSocket.hpp"

using namespace Pravala;

MemHandle LwipTcpSocket::getTcpInfo()
{
    MemHandle ret ( sizeof ( struct tcp_info ) );
    struct tcp_pcb * const pcb = _lwipSock.tcp;
    struct tcp_info * const info = ( struct tcp_info * ) ret.getWritable();

    if ( !pcb || !info )
    {
        return MemHandle();
    }

    memset ( info, 0, sizeof ( *info ) );

    // The TCP state enum for Linux is only available in <netinet/tcp.h>, not <linux/tcp.h>
    // Although both headers define struct tcp_info, the definition in linux/ has added new
    // members to the end, including tcpi_segs_out which is needed for calculating the
    // retransmission rate. These two headers have many conflicting definitions, so they can't
    // both be included. The state conversion needs to be done in a separate file.

    info->tcpi_state = convertLwipToNativeTcpState ( pcb->state );
    info->tcpi_snd_cwnd = pcb->cwnd;
    info->tcpi_retrans = pcb->nrtx; // This is the number of retransmits since data was last ACKed.

    if ( pcb->flags & TF_SACK )
        info->tcpi_options |= TCPI_OPT_SACK;

    if ( pcb->flags & TF_WND_SCALE )
        info->tcpi_options |= TCPI_OPT_WSCALE;

    info->tcpi_total_retrans = pcb->rtx_segs_total;

    // The tcpi_segs_out and tcpi_bytes_acked members were added in 4.2.
#if LINUX_VERSION_CODE >= KERNEL_VERSION ( 4, 2, 0 )
    info->tcpi_segs_out = pcb->tx_segs_total;
    info->tcpi_bytes_acked = pcb->bytes_acked;
#endif

    info->tcpi_snd_wscale = min<uint8_t> ( pcb->snd_scale, 0xF );
    info->tcpi_rcv_wscale = min<uint8_t> ( pcb->rcv_scale, 0xF );

    // pcb->rto is in units of TCP_SLOW_INTERVAL, which is in ms. tcpi_rto is in usecs
    info->tcpi_rto = pcb->rto * TCP_SLOW_INTERVAL * 1000;
    info->tcpi_rcv_mss = info->tcpi_snd_mss = pcb->mss;
    info->tcpi_rtt = pcb->rttest * 500000; // rtt is in usecs, rtttest is in 500ms ticks
    info->tcpi_snd_ssthresh = pcb->ssthresh;

    // None of the timestamp fields in tcp_info are set since Lwip doesn't track
    // the time packets are actually sent and received. The timestamp fields in
    // the tcp_pcb are related to the timestamp option for TCP.

    return ret;
}
