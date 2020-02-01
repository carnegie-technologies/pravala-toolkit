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

// We need those includes to include Windows headers in correct order...

#include "basic/IpAddress.hpp"

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
}
#else
extern "C"
{
#include <netinet/in.h>
}
#endif

// Only Windows doesn't have the ip.h header, but for simplicity let's use it everywhere...
extern "C"
{
struct ip
{
    unsigned int ip_hl : 4;             /* header length */
    unsigned int ip_v : 4;              /* version */
    uint8_t ip_tos;                     /* type of service */
    uint16_t ip_len;                    /* total length */
    uint16_t ip_id;                     /* identification */
    uint16_t ip_off;                    /* fragment offset field */
#define IP_OFFMASK    0x1fff            /* mask for fragmenting bits */
    uint8_t ip_ttl;                     /* time to live */
    uint8_t ip_p;                       /* protocol */
    uint16_t ip_sum;                    /* checksum */
    struct in_addr ip_src, ip_dst;      /* source and dest address */
};
}

// Some platforms don't have ip6.h header, for simplicity let's use it everywhere...

extern "C"
{
struct ip6_hdr
{
    union
    {
        struct ip6_hdrctl
        {
            uint32_t ip6_un1_flow;       /* 4 bits version, 8 bits TC, 20 bits flow-ID */
            uint16_t ip6_un1_plen;       /* payload length */
            uint8_t ip6_un1_nxt;         /* next header */
            uint8_t ip6_un1_hlim;        /* hop limit */
        } ip6_un1;

        uint8_t ip6_un2_vfc;           /* 4 bits version, top 4 bits tclass */
    } ip6_ctlun;

    struct in6_addr ip6_src;          /* source address */

    struct in6_addr ip6_dst;          /* destination address */
};

#define ip6_vfc     ip6_ctlun.ip6_un2_vfc
#define ip6_flow    ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen    ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt     ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim    ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops    ip6_ctlun.ip6_un1.ip6_un1_hlim
}
