//
// Note: this file is always copied from the original one in:
//    3rdparty/lwip/lwipopts.h
// to:
//    <your-build-dir>/3rdparty/lwip/lwip/src/include/lwipopts.h
//
// Any changes should only be made to the original one, not the copy.
//

#pragma once

/**
 * NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
 * mboxes). This means threaded APIs cannot be used (socket, netconn,
 * i.e. everything in the 'api' folder), only the callback-style raw API is
 * available (and you have to watch out for yourself that you don't access
 * lwIP functions/structures from more than one context at a time!)
 */
#define NO_SYS                          1

/**
 * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
 * protection) for certain critical regions during buffer allocation, deallocation
 * and memory allocation and deallocation.
 * ATTENTION: This is required when using lwIP from more than one context! If
 * you disable this, you must be sure what you are doing!
 */
#define SYS_LIGHTWEIGHT_PROT            0

/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define MEM_ALIGNMENT 4
 *    2 byte alignment -> #define MEM_ALIGNMENT 2
 */
#define MEM_ALIGNMENT                   4

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 */
#define MEM_SIZE                        (32*1024*1024)

/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEM_LIBC_MALLOC                 1

/**
 * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
 * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
 * from interrupt)!
 * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
 * not only for internal pools defined in memp_std.h)!
 */
#define MEMP_MEM_MALLOC                 1

/**
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#define MEMP_NUM_PBUF                   (32*1024)

/**
 * MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
 * per active UDP "connection".
 * (requires the LWIP_UDP option)
 */
#define MEMP_NUM_UDP_PCB                1024

/**
 * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_PCB                1024

/**
 * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_SEG                (32*1024)

/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
#define PBUF_POOL_SIZE                  (32*1024)

/**
 * LWIP_ARP==1: Enable ARP functionality.
 */
#define LWIP_ARP                        0

/**
 * LWIP_IPV4==1: Enable IPv4
 */
#define LWIP_IPV4                       1

/**
 * LWIP_IPV6==1: Enable IPv6
 */
#define LWIP_IPV6                       1

/**
 * LWIP_IPV6_MLD==1: Enable multicast listener discovery protocol.
 */
#define LWIP_IPV6_MLD                   0

/**
 * IP6_FRAG_COPYHEADER==1: for platforms where sizeof(void*) > 4, "struct
 * ip6_reass_helper" is too large to be stored in the IPv6 fragment header, and
 * will bleed into the header before it, which may be the IPv6 header or an
 * extension header. This means that for each first fragment packet, we need to
 * 1) make a copy of some IPv6 header fields (src+dest) that we need later on,
 * just in case we do overwrite part of the IPv6 header, and 2) make a copy of
 * the header data that we overwrote, so that we can restore it before either
 * completing reassembly or sending an ICMPv6 reply. The last part is true even
 * if this setting is disabled, but if it is enabled, we need to save a bit
 * more data (up to the size of a pointer) because we overwrite more. */
#define IPV6_FRAG_COPYHEADER            1

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#define TCP_MSS                         1460

/**
 * TCP_WND: The size of a TCP window.  This must be at least
 * (2 * TCP_MSS) for things to work well
 */
#define TCP_WND                         (4*1024*1024)

/**
 * LWIP_WND_SCALE and TCP_RCV_SCALE:
 * Set LWIP_WND_SCALE to 1 to enable window scaling.
 * Set TCP_RCV_SCALE to the desired scaling factor (shift count in the
 * range of [0..14]).
 * When LWIP_WND_SCALE is enabled but TCP_RCV_SCALE is 0, we can use a large
 * send window while having a small receive window only.
 */
#define LWIP_WND_SCALE                  1
#define TCP_RCV_SCALE                   7

/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#define TCP_SND_BUF                     65535

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
#define TCP_SND_QUEUELEN                512

/**
 * LWIP_TCP_SACK_OUT==1: TCP will support sending selective acknowledgements (SACKs).
 */
#define LWIP_TCP_SACK_OUT               1

/**
 * LWIP_TCP_MAX_SACK_NUM: The maximum number of SACK values to include in TCP segments.
 * Must be at least 1, but is only used if LWIP_TCP_SACK_OUT is enabled.
 * NOTE: Even though we never send more than 3 or 4 SACK ranges in a single segment
 * (depending on other options), setting this option to values greater than 4 is not pointless.
 * This is basically the max number of SACK ranges we want to keep track of.
 * As new data is delivered, some of the SACK ranges may be removed or merged.
 * In that case some of those older SACK ranges may be used again.
 * The amount of memory used to store SACK ranges is LWIP_TCP_MAX_SACK_NUM * 8 bytes for each TCP PCB.
 */
#define LWIP_TCP_MAX_SACK_NUM           8

/**
 * PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet.
 */
#define PBUF_LINK_HLEN                  16

/**
 * LWIP_NETCONN==1: Enable Netconn API (required to use api_lib.c)
 */
#define LWIP_NETCONN                    0

/**
 * LWIP_SOCKET==1: Enable Socket API (required to use sockets.c)
 */
#define LWIP_SOCKET                     0

/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#define SO_REUSE                        1

/**
 * LWIP_STATS==1: Enable statistics collection in lwip_stats.
 */
#define LWIP_STATS                      0

/**
 * Define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS to prevent lwIP
 * from defining hton* and ntoh* compatibility macros.
 */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#ifndef NDEBUG
#define LWIP_DEBUG                      1
#else
#define LWIP_DEBUG                      0

// This disables all messages:
#define LWIP_DBG_TYPES_ON               0
#endif

// #define PBUF_DEBUG       LWIP_DBG_ON
// #define TAPIF_DEBUG      LWIP_DBG_ON
// #define TUNIF_DEBUG      LWIP_DBG_ON
// #define UNIXIF_DEBUG     LWIP_DBG_ON
// #define DELIF_DEBUG      LWIP_DBG_ON
// #define SIO_FIFO_DEBUG   LWIP_DBG_ON
// #define TCPDUMP_DEBUG    LWIP_DBG_ON
// #define API_LIB_DEBUG    LWIP_DBG_ON
// #define API_MSG_DEBUG    LWIP_DBG_ON
// #define TCPIP_DEBUG      LWIP_DBG_ON
// #define NETIF_DEBUG      LWIP_DBG_ON
// #define SOCKETS_DEBUG    LWIP_DBG_ON
// #define DEMO_DEBUG       LWIP_DBG_ON
// #define IP_DEBUG         LWIP_DBG_ON
// #define IP6_DEBUG        LWIP_DBG_ON
// #define IP_REASS_DEBUG   LWIP_DBG_ON
// #define RAW_DEBUG        LWIP_DBG_ON
// #define ICMP_DEBUG       LWIP_DBG_ON
// #define UDP_DEBUG        LWIP_DBG_ON
// #define TCP_DEBUG        LWIP_DBG_ON
// #define TCP_INPUT_DEBUG  LWIP_DBG_ON
// #define TCP_OUTPUT_DEBUG LWIP_DBG_ON
// #define TCP_RTO_DEBUG    LWIP_DBG_ON
// #define TCP_CWND_DEBUG   LWIP_DBG_ON
// #define TCP_WND_DEBUG    LWIP_DBG_ON
// #define TCP_FR_DEBUG     LWIP_DBG_ON
// #define TCP_QLEN_DEBUG   LWIP_DBG_ON
// #define TCP_RST_DEBUG    LWIP_DBG_ON

// #define TIMERS_DEBUG     LWIP_DBG_ON
// #define LWIP_DEBUG_TIMERNAMES 1
