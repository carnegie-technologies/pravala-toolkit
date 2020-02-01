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

#include <cstring>
#include <cassert>

extern "C"
{
// lwip hijacks ntohs, ntohl, etc.
// To avoid "different exception specifier" compiler error with the new gcc-6.1.1
// netinet/in.h should be included before some lwip headers.
#include <netinet/in.h>

#include "lwip/netif.h"
#include "lwip/ip.h"
}

#include "basic/Math.hpp"
#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"
#include "net/TcpPacket.hpp"
#include "net/UdpPacket.hpp"

#include "internal/LwipEventPoller.hpp"
#include "internal/pbuf_custom_MemHandle.hpp"
#include "internal/PBufMemHandle.hpp"
#include "LwipInterface.hpp"

#define LOG_IFACE( LOG_LEVEL, X ) \
    LOG ( LOG_LEVEL, "LwipInterface [" << this << "]; " << X )

// This is the minimum MTU required for IPv6 to work
#define MIN_MTU_IPV6    1280

using namespace Pravala;

TextLog LwipInterface::_log ( "lwip_iface" );

LwipInterface::LwipInterface():
    _interface ( 0 ),
    _hasV4Addr ( false ),
    _numV6Addrs ( 0 )
{
}

LwipInterface::~LwipInterface()
{
    stop();
}

ERRCODE LwipInterface::init()
{
    if ( _interface != 0 )
        return Error::AlreadyInitialized;

    _interface = new netif;

    if ( !_interface )
    {
        LOG_IFACE ( L_ERROR, "Could not allocate memory for new lwIP netif structure" );
        return Error::MemoryError;
    }

    memset ( _interface, 0, sizeof ( *_interface ) );

    // This initializes and adds the lwIP interface into lwIP network stack.
    // The ifaceInitFunc is a required parameter (cannot be null), but it doesn't need to do anything.
    // The ip_input is a parameter required for any non-ethernet interface. See lwip/doc/rawapi.txt
    if ( !netif_add ( _interface, 0, 0, 0, 0, &ifaceInitFunc, &ip_input ) )
    {
        LOG_IFACE ( L_ERROR, "Could not initialize lwIP netif structure" );

        delete _interface;
        _interface = 0;
        return Error::CouldNotInitialize;
    }

    _interface->state = this;
    _interface->output = &ifaceIPv4OutputFunc;
    _interface->output_ip6 = &ifaceIPv6OutputFunc;

    /// @todo TODO For now we use this fixed MTU until we have a better system in place.
    _interface->mtu = MIN_MTU_IPV6;

    LOG_IFACE ( L_DEBUG2, "lwIP interface initialized" );

    // If lwIP currently has no default interface, set it to this one
    if ( !netif_default )
    {
        markDefault();
    }

    // This will start lwIP event polling if it is the first reference
    LwipEventPoller::get().ref();

    return Error::Success;
}

void LwipInterface::stop()
{
    if ( !_interface )
        return;

    EventManager::loopEndUnsubscribe ( this );

    const bool wasDefault = isDefault();

    netif_set_down ( _interface );
    netif_set_link_down ( _interface );

    // This unsets the default interface if this interface is the default
    netif_remove ( _interface );

    // If lwIP has no default interface now, pick the first one on the interface list as default
    if ( wasDefault && !netif_default && netif_list )
    {
        netif_set_default ( netif_list );
    }

    // This will stop lwIP event polling if it is the last reference
    LwipEventPoller::get().unref();

    delete _interface;
    _interface = 0;

    LOG_IFACE ( L_DEBUG2, "lwIP interface removed" );
}

void LwipInterface::markDefault()
{
    if ( !_interface )
        return;

    netif_set_default ( _interface );

    LwipEventPoller::get().checkLwipTimer();
}

bool LwipInterface::isDefault() const
{
    return ( _interface != 0 && netif_default == _interface );
}

bool LwipInterface::isUp() const
{
    return ( _interface != 0 && netif_is_up ( _interface ) );
}

void LwipInterface::setIpV4Address ( const IpAddress & addr )
{
    if ( !_interface )
        return;

    if ( !addr.isIPv4() )
    {
        LOG_IFACE ( L_DEBUG2, "Clearing single interface V4 address" );

        // Clear the IPv4 addresses if we are provided with a non-V4 address
        ip_addr_set_zero_ip4 ( &_interface->ip_addr );
        ip_addr_set_zero_ip4 ( &_interface->netmask );
        ip_addr_set_zero_ip4 ( &_interface->gw );

        _hasV4Addr = false;
    }
    else
    {
        LOG_IFACE ( L_DEBUG2, "Setting single interface V4 address: " << addr );

        netif_set_addr ( _interface,
                         ( const ip4_addr_t * ) &addr.getV4(),
                         ( const ip4_addr_t * ) &IpAddress::Ipv4HostNetmask.getV4(),
                         ( const ip4_addr_t * ) &addr.getV4() );

        _hasV4Addr = true;
    }

    updateIfaceState();
}

void LwipInterface::setIpV6Addresses ( const List< IpAddress > & addrs )
{
    if ( !_interface )
        return;

    // Clear all IPv6 addresses first
    for ( size_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i )
    {
        ip_addr_set_zero_ip6 ( &_interface->ip6_addr[ i ] );
        netif_ip6_addr_set_state ( _interface, i, IP6_ADDR_INVALID );
    }

    _numV6Addrs = 0;

    for ( size_t i = 0; i < addrs.size(); ++i )
    {
        if ( !addrs[ i ].isIPv6() )
            continue;

        s8_t idx = 0;

        if ( netif_add_ip6_address ( _interface, ( const ip6_addr_t * ) &addrs[ i ].getV6(), &idx ) != ERR_OK )
        {
            LOG_IFACE ( L_WARN, "Failed to add interface address: " << addrs[ i ] );
        }
        else
        {
            netif_ip6_addr_set_state ( _interface, idx, IP6_ADDR_VALID );
            ++_numV6Addrs;

            LOG_IFACE ( L_DEBUG2, "Adding interface address: " << addrs[ i ] );
        }
    }

    updateIfaceState();
}

void LwipInterface::updateIfaceState()
{
    if ( !_hasV4Addr && _numV6Addrs == 0 && netif_is_up ( _interface ) )
    {
        netif_set_down ( _interface );
        netif_set_link_down ( _interface );

        LOG_IFACE ( L_DEBUG2, "Interface set to down" );
    }
    else if ( ( _hasV4Addr || _numV6Addrs > 0 ) && !netif_is_up ( _interface ) )
    {
        netif_set_link_up ( _interface );
        netif_set_up ( _interface );

        LOG_IFACE ( L_DEBUG2, "Interface set to up" );
    }

    LwipEventPoller::get().checkLwipTimer();
}

ERRCODE LwipInterface::sendPacket ( const IpPacket & packet )
{
    if ( !_interface )
        return Error::NotInitialized;

    if ( !packet.isValid() )
        return Error::InvalidParameter;

    //
    // We delay injecting IP packets into lwIP stack. This is especially important for TCP RST packets, because:
    //
    //  - An incorrect interface IP address + an invalid RST packet can cause infinite recursive
    //    calls to outputIpPacket/injectPacket during lwIP's TCP connect process.
    //    lwIP tries to send a SYN with bad src IP, Host sends a RST, lwIP will retry the SYN ...
    //
    //  - A valid RST packet can cause an lwIP TCP socket to be freed before it has completed
    //    an operation such as connect, which will trigger a segfault in lwIP's code.
    //
    // All other packets could be injected immediately.
    // However, this could cause lwIP to respond with some new packets (like ACK packets acknowledging
    // the data just received). This would cause us to generate a callback from inside a call in LwipInterface.
    // The code using this may, or may not be OK with that. To provide an API that's safer and easier to use,
    // we delay injecting ALL packets.
    //

    _inputQueue.append ( packet );

    LOG_IFACE ( L_DEBUG4, "Queueing packet for injection at loop-end [" << packet
                << "]; New queue size: " << _inputQueue.size() );

    if ( _inputQueue.size() == 1 )
    {
        EventManager::loopEndSubscribe ( this );
    }

    return Error::Success;
}

ERRCODE LwipInterface::injectPacket ( const IpPacket & packet )
{
    if ( !_interface )
        return Error::NotInitialized;

    if ( !packet.isValid() )
        return Error::InvalidParameter;

    //
    // We want to pass the IP packet to lwip, but we are trying to avoid copying the data.
    // Unfortunately lwip needs to modify headers (TCP, UDP, likely others too) in the packets it is getting.
    // It changes port numbers, as well as sequence/ack numbers to local host's endianness.
    // So for the known packet types we want to copy just the headers (IP+transport headers),
    // and reference the actual payload.
    // This depends on specific transport protocol, so we cannot do this for all packets
    // without having support for each individual transport protocol. So we do this only for TCP and UDP
    // packets, and copy the entire packet in all other cases.
    // Most of the big packets should be TCP or UDP anyway.
    //
    // Note that if the packet is fragmented, we will only do this to the first part. The rest of the packet
    // will be copied, even if it is TCP or UDP.
    //

    MemVector payloadData;

    switch ( packet.getProtoType() )
    {
        case IpPacket::Proto::TCP:
            packet.getProtoPayload<TcpPacket> ( payloadData );
            break;

        case IpPacket::Proto::UDP:
            packet.getProtoPayload<UdpPacket> ( payloadData );
            break;
    }

    const MemVector & packetData = packet.getPacketData();

    // If payloadData is not empty, it means that the packet uses protocol we understand and contains payload.
    // We need to copy everything else.

    assert ( packetData.getDataSize() > payloadData.getDataSize() );

    const size_t copySize = packetData.getDataSize() - payloadData.getDataSize();

    // We use PBUF_RAW instead of PBUF_IP, because our data already includes IP headers
    // (no need to allocate additional memory for the IP header).

    struct pbuf * packetBuffer = pbuf_alloc ( PBUF_RAW, copySize, PBUF_RAM );

    if ( !packetBuffer )
    {
        LOG_IFACE ( L_ERROR, "Could not allocate an lwIP pbuf for IP packet of size " << packet.getPacketSize()
                    << " bytes (data to be copied: " << copySize << " bytes)" );
        return Error::MemoryError;
    }

    {
        // Let's copy first 'copySize' bytes of the packet  (which could be the entire packet):

        const struct iovec * const chunks = packetData.getChunks();
        char * bufMem = static_cast<char *> ( packetBuffer->payload );

        for ( size_t toCopy = copySize, idx = 0; toCopy > 0; ++idx )
        {
            assert ( idx < packetData.getNumChunks() );

            if ( toCopy <= chunks[ idx ].iov_len )
            {
                assert ( toCopy > 0 );
                assert ( toCopy <= chunks[ idx ].iov_len );

                memcpy ( bufMem, chunks[ idx ].iov_base, toCopy );
                break;
            }

            memcpy ( bufMem, chunks[ idx ].iov_base, chunks[ idx ].iov_len );

            bufMem += chunks[ idx ].iov_len;
            toCopy -= chunks[ idx ].iov_len;
        }
    }

    // We copied first 'copySize' bytes from the packet to packetBuffer.
    // Now, if there is anything in payloadData we need to attach it after that.
    if ( !payloadData.isEmpty() )
    {
        // We add chunks in reverse order.
        // This is because when appending buffer B to buffer A using pbuf_cat we give away our reference in buffer B.
        // This means we cannot use buffer B to append anything to it anymore. But in that scenario we would still
        // own the reference in A, which means we could use that for appending.
        // So to achieve A->B->C we first append C to B (and lose reference in C), and then append B to A.
        // Note: We could always append B to A, and then C to A and lwip would place C after B,
        //       but it would have to traverse the buffer chain, which would be slower.

        struct pbuf * bufHead = 0;

        for ( size_t idx = payloadData.getNumChunks(); idx > 0; --idx )
        {
            pbuf_custom_MemHandle * const newBuf = new pbuf_custom_MemHandle ( payloadData.getChunk ( idx - 1 ) );

            assert ( newBuf != 0 );

            if ( newBuf->getData().isEmpty() )
            {
                LOG_IFACE ( L_ERROR, "Could not generate a custom PBuf based on IP packet's payload" );

                pbuf_free ( reinterpret_cast<struct pbuf *> ( newBuf ) );
                pbuf_free ( packetBuffer );

                if ( bufHead != 0 )
                {
                    pbuf_free ( bufHead );
                }

                return Error::MemoryError;
            }

            if ( bufHead != 0 )
            {
                // Append the previous head to the new buffer, and switch the head.
                // We use pbuf_cat instead of pbuf_chain to transfer our reference (in bufHead),
                // instead of adding a new one.
                pbuf_cat ( reinterpret_cast<struct pbuf *> ( newBuf ), bufHead );
            }

            bufHead = reinterpret_cast<struct pbuf *> ( newBuf );
        }

        assert ( bufHead != 0 );

        // And now we append the entire list to packetBuffer.
        // We use pbuf_cat instead of pbuf_chain to transfer our reference (in bufHead),
        // instead of adding a new one.
        pbuf_cat ( packetBuffer, bufHead );
    }

    assert ( packetData.getDataSize() == copySize + payloadData.getDataSize() );

    LOG_IFACE ( L_DEBUG4, "Injecting IP packet [" << packet << "]; Bytes copied: "
                << copySize << "; Bytes referenced: " << payloadData.getDataSize() );

    // On success, lwIP will free the packetBuffer when it is no longer used.
    // Also, we cannot use packetBuffer after successful input().

    const err_t err = _interface->input ( packetBuffer, _interface );

    if ( err != ERR_OK )
    {
        pbuf_free ( packetBuffer );
        packetBuffer = 0;

        LOG_IFACE ( L_ERROR, "Failed to inject IP packet [" << packet << "] due to lwIP error: ["
                    << err << "] " << lwip_strerr ( err ) );

        return mapLwipErrorToERRCODE ( err );
    }

    // It is invalid now - input() took over our reference.
    packetBuffer = 0;

    LOG_IFACE ( L_DEBUG4, "Successfully injected IP packet [" << packet << "]" );

    return Error::Success;
}

void LwipInterface::receiveLoopEndEvent()
{
    // We would be risking an infinite loop if lwIP keeps outputting packets in response to injected packets,
    // and those packets trigger new reset packets. So here we only inject the original contents of the queue
    // and postpone processing of any new reset packets until the next end-of-loop event.

    List<IpPacket> inputQueue = _inputQueue;
    _inputQueue.clear();

    if ( !inputQueue.isEmpty() )
    {
        LOG_IFACE ( L_DEBUG4, "Injecting queued IP packets" );
    }

    while ( !inputQueue.isEmpty() )
    {
        injectPacket ( inputQueue.first() );

        // Stop processing the rest of the queue if we are now stopped.
        // Furthermore this interface should not be deallocated as the result of a callback from lwIP.
        if ( !_interface )
        {
            LOG_IFACE ( L_WARN, "Interface stopped before all packets have been injected; Discarding queue of "
                        << inputQueue.size() << " packets" );
            return;
        }

        inputQueue.removeFirst();
    }

    LwipEventPoller::get().checkLwipTimer();
}

err_t LwipInterface::ifaceInitFunc ( netif * /* interface */ )
{
    // lwIP always calls this function when adding the interface,
    // it doesn't actually need to do anything, just needs to return OK.
    return ERR_OK;
}

err_t LwipInterface::ifaceIPv4OutputFunc ( netif * interface, pbuf * buffer, const ip4_addr_t * ipaddr )
{
    assert ( interface != 0 );
    assert ( interface->state != 0 );
    assert ( buffer != 0 );
    assert ( ipaddr != 0 );
    assert ( ( ( LwipInterface * ) interface->state )->_interface == interface );
    ( void ) ipaddr;

    LOG ( L_DEBUG4, "Outputting packet of " << buffer->tot_len << " bytes to " << ip4addr_ntoa ( ipaddr ) );

    return ( ( LwipInterface * ) interface->state )->outputIpPacket ( buffer );
}

err_t LwipInterface::ifaceIPv6OutputFunc ( netif * interface, pbuf * buffer, const ip6_addr_t * ipaddr )
{
    assert ( interface != 0 );
    assert ( interface->state != 0 );
    assert ( buffer != 0 );
    assert ( ipaddr != 0 );
    assert ( ( ( LwipInterface * ) interface->state )->_interface == interface );
    ( void ) ipaddr;

    LOG ( L_DEBUG4, "Outputting packet of " << buffer->tot_len << " bytes to " << ip6addr_ntoa ( ipaddr ) );

    return ( ( LwipInterface * ) interface->state )->outputIpPacket ( buffer );
}

err_t LwipInterface::outputIpPacket ( pbuf * buffer )
{
    assert ( buffer != 0 );

    MemHandle mh = PBufMemHandle::getPacket ( buffer );

    if ( mh.isEmpty() )
    {
        LOG_IFACE ( L_ERROR, "Could not create a MemHandle with pbuf's content; pbuf's size: "
                    << buffer->tot_len << "; Dropping" );
        return ERR_MEM;
    }

    // It could destroy us, so we need to return right after calling this.
    packetReceived ( mh, buffer->tag );
    return ERR_OK;
}

ERRCODE LwipInterface::mapLwipErrorToERRCODE ( err_t error )
{
    switch ( error )
    {
        case ERR_OK:         // No error, everything OK.
            return Error::Success;

        case ERR_MEM:        // Out of memory error.
            return Error::MemoryError;

        case ERR_BUF:        // Buffer error.
            return Error::TooMuchData;

        case ERR_TIMEOUT:    // Timeout.
            return Error::Timeout;

        case ERR_WOULDBLOCK: // Operation would block.
            return Error::NotAvailable;

        case ERR_RTE:        // Routing problem.
            return Error::RoutingError;

        case ERR_VAL:        // Illegal value.
        case ERR_ARG:        // Illegal argument.
            return Error::InvalidParameter;

        case ERR_USE:        // Address in use.
            return Error::AddrInUse;

        case ERR_INPROGRESS: // Operation in progress
        case ERR_ALREADY:    // Already connecting.
            return Error::ConnectInProgress;

        case ERR_ISCONN:     // Conn already established.
            return Error::AlreadyInitialized;

        case ERR_CLSD:       // Connection closed.
        case ERR_ABRT:       // Connection aborted.
        case ERR_RST:        // Connection reset.
            return Error::Closed;

        case ERR_CONN:       // Not connected.
            return Error::NotConnected;

        case ERR_IF:         // Low-level netif error
            return Error::InterfaceError;
    }

    return Error::Unknown;
}
