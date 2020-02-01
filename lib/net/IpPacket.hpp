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

#include <cassert>

#include "basic/MemVector.hpp"
#include "log/TextLog.hpp"

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

namespace Pravala
{
class Buffer;
class IpAddress;
union FlowDesc;

/// @brief Represents an IP Packet.
class IpPacket
{
    public:
        /// @brief If the IP packet is fragmented, the protocol number from the IP header is bit-summed with this value.
        static const uint16_t ProtoBitNextIpFragment = ( 1 << 15 );

        /// @brief The size of the IPv4 header (without any options).
        static const size_t IPv4HeaderSize;

        /// @brief The size of the IPv6 header.
        static const size_t IPv6HeaderSize;

        /// @brief A structure used only to add 'Proto::' to protocol names
        struct Proto
        {
            /// @brief Well-known and supported protocol numbers by this class
            /// These are from http://www.iana.org/assignments/protocol-numbers
            enum Number
            {
                ICMP = 1, ///< Internet Control Message
                IGMP = 2, ///< Internet Group Management
                TCP = 6, ///< Transmission Control Protocol
                UDP = 17, ///< User Datagram Protocol
                RDP = 27, ///< Reliable Data Protocol
                IPv6Encaps = 41, ///< IPv6 encapsulation
                IPv6Route = 43, ///< Routing Header for IPv6
                IPv6Frag = 44, ///< Fragment Header for IPv6
                RSVP = 46, ///< Reservation Protocol
                GRE = 47, ///< General Routing Encapsulation
                ESP = 50, ///< Encap Security Payload
                AH = 51, ///< Authentication Header
                IPv6ICMP = 58, ///< ICMP for IPv6
                IPv6NoNxt = 59, ///< No Next Header for IPv6
                IPv6Opts = 60, ///< Destination Options for IPv6
                IPIP = 94, ///< IP-within-IP Encapsulation Protocol
                EtherIP = 97, ///< Ethernet-within-IP Encapsulation
                Encap = 98, ///< Encapsulation Header
                SCTP = 132, ///< Stream Control Transmission Protocol
                UDPLite = 136 ///< UDPLite
            };
        };

        /// @brief The type of the address for setAddress function
        enum AddressType
        {
            SourceAddress, ///< Source address
            DestAddress    ///< Destination address
        };

        /// @brief Used with setupFlowDesc() to determine whether the packet is sent by the client or by the server.
        enum PacketDirection
        {
            PacketFromClient, ///< Client is the source.
            PacketToClient    ///< Server is the source.
        };

        /// @brief Default constructor.
        /// It creates an empty (invalid) IP packet.
        IpPacket();

        /// @brief Constructor.
        /// If the data passed is invalid, the packet will also be invalid.
        /// @param [in] data The data of the IP packet.
        ///                  A new reference to the data will be created, so any non-const operation in IP packet
        ///                  (or TCP, UDP, etc. packets) will cause at least part of the data to be copied.
        ///                  To avoid this, the buffer passed to this function should be cleared (by calling clear())
        ///                  The memory in this buffer must be aligned properly ( data.get() % 4 should be equal to 0 ).
        IpPacket ( const MemHandle & data );

        /// @brief Checks if the packet is valid (contains any data).
        /// @return True if the packet is valid, false otherwise.
        inline bool isValid() const
        {
            return ( !_buffer.isEmpty() );
        }

        /// @brief Clears the packet and associated memory buffer
        inline void clear()
        {
            _buffer.clear();
        }

        /// @brief Returns the size of the entire IP packet.
        /// This includes IP headers, internal protocol's headers and its payload data.
        /// @return The size of the entire IP packet (in bytes).
        inline size_t getPacketSize() const
        {
            return _buffer.getDataSize();
        }

        /// @brief Exposes this packet's data.
        /// @note The memory is not copied, but if another reference to the memory is created (by copying the vector),
        ///       calling any method that modifies the content of this IP packet will cause
        ///       at least part of the memory to be copied.
        /// @return The const reference to this packet's buffer.
        inline const MemVector & getPacketData() const
        {
            return _buffer;
        }

        /// @brief Checks if this IP packet carries specific protocol data.
        /// @param [in] protoNum The protocol number.
        /// @return True if this IP packet contains specified protocol's data, false otherwise.
        inline bool is ( Proto::Number protoNum ) const
        {
            PacketDesc pDesc;

            return ( examinePacket ( pDesc ) && pDesc.protoType == protoNum );
        }

        /// @brief Returns the internal protocol type.
        /// @return The protocol number used by this IP packet.
        inline uint16_t getProtoType() const
        {
            PacketDesc pDesc;

            return examinePacket ( pDesc ) ? pDesc.protoType : 0;
        }

        /// @brief Returns the IP version of this IP packet
        /// @return the IP version of this IP packet
        uint8_t getIpVersion() const;

        /// @brief Reads source and destination addresses from this IP packet
        /// @param [out] srcAddr Source address
        /// @param [out] dstAddr Destination address
        /// @return True if the addresses were read correctly, false otherwise
        bool getAddr ( IpAddress & srcAddr, IpAddress & dstAddr ) const;

        /// @brief Configures a FlowDesc to represent this packet
        /// @param [out] flowDesc The FlowDesc to setup
        /// @param [in] direction Direction in which this packet was sent.
        /// @return True if the FlowDesc was properly initialized;
        ///         False otherwise (if there is a problem with this IpPacket)
        bool setupFlowDesc ( FlowDesc & flowDesc, PacketDirection direction ) const;

        /// @brief Returns a "seed" of this packet.
        /// For UDP and TCP packets, this seed is simply XOR of source and destination port numbers.
        /// For all other packets, it is generated based on source and destination IP addresses.
        /// @return The "seed" of this IP packet
        uint16_t getSeed() const;

        /// @brief Change the IP packet's source address to newSrcAddr
        /// @note This also recalculates the IP and TCP/UDP checksum as appropriate
        /// @param [in] whichAddr Selects which address to set. Either source or destination.
        /// @param [in] newAddress New address to set
        bool setAddress ( AddressType whichAddr, const IpAddress & newAddress );

        /// @brief Appends the description of this packet to the buffer.
        /// @param [in,out] toBuffer The buffer to append the description to. It is not cleared first.
        void describe ( Buffer & toBuffer ) const;

        /// @brief Calculates the checksum for upper protocols (like UDP or TCP)
        /// It uses the appropriate pseudo header, and all the data stored in the buffer as the IP's data
        /// (so any internal protocol's headers and payload). So for this to work properly the data part of
        /// the buffer has to be initialized before calling this function. Also, the 'checksum' field
        /// in payload protocol's header should be set to 0.
        /// @return Calculated checksum.
        uint16_t calcPseudoHeaderPayloadChecksum() const;

        /// @brief Returns a pointer to internal protocol's header.
        /// @tparam ProtoType The type of the protocol to use.
        /// @return Pointer to internal protocol's header, or 0 if the packet is invalid,
        ///         or if ProtoType does not match internal protocol type.
        template<typename ProtoType> const typename ProtoType::Header * getProtoHeader() const
        {
            PacketDesc pDesc;

            return ( examinePacket ( pDesc )
                     && pDesc.protoType == ProtoType::ProtoNumber
                     && pDesc.protoHeaderSize >= sizeof ( typename ProtoType::Header )
                     && pDesc.protoHeaderSize
                     >= reinterpret_cast<const typename ProtoType::Header *> ( pDesc.protoHeader )->getHeaderSize() )
                   ? reinterpret_cast<const typename ProtoType::Header *> ( pDesc.protoHeader )
                   : 0;
        }

        /// @brief Returns a pointer to internal protocol's header.
        /// @tparam ProtoType The type of the protocol to use.
        /// @return Pointer to internal protocol's header, or 0 if the packet is invalid,
        ///         or if ProtoType does not match internal protocol type.
        template<typename ProtoType> typename ProtoType::Header * getWritableProtoHeader()
        {
            PacketDesc pDesc;

            if ( examinePacket ( pDesc )
                 && pDesc.protoType == ProtoType::ProtoNumber
                 && pDesc.protoHeaderSize >= sizeof ( typename ProtoType::Header ) )
            {
                const uint16_t fullHdrSize
                    = reinterpret_cast<const typename ProtoType::Header *> ( pDesc.protoHeader )->getHeaderSize();

                if ( pDesc.protoHeaderSize >= fullHdrSize )
                {
                    // We ask the buffer to generate a writable pointer to the first IP_HDR + PROTO_HDR bytes,
                    // and then we skip the IP header size.

                    char * const buf = _buffer.getContinuousWritable ( pDesc.ipHeaderSize + fullHdrSize );

                    return ( buf != 0 )
                           ? reinterpret_cast<typename ProtoType::Header *> ( buf + pDesc.ipHeaderSize )
                           : 0;
                }
            }

            return 0;
        }

        /// @brief Retrieves the payload protocol's data (everything after payload protocol's header).
        /// @param [out] vec The MemVector to which the data will be appended (it is NOT cleared in advance).
        /// @tparam ProtoType The type of the protocol to use.
        /// @return True if the protocol is valid, even if there is no payload.
        template<typename ProtoType> bool getProtoPayload ( MemVector & vec ) const
        {
            PacketDesc pDesc;

            if ( examinePacket ( pDesc )
                 && pDesc.protoType == ProtoType::ProtoNumber
                 && pDesc.protoHeaderSize >= sizeof ( typename ProtoType::Header ) )
            {
                const uint16_t fullHdrSize
                    = reinterpret_cast<const typename ProtoType::Header *> ( pDesc.protoHeader )->getHeaderSize();

                return vec.append ( _buffer, pDesc.ipHeaderSize + fullHdrSize );
            }

            return false;
        }

        /// @brief Returns payload protocol's data size (not including payload protocol's header).
        /// @tparam ProtoType The type of the protocol to use.
        /// @return The size of the payload data; 0 on error.
        template<typename ProtoType> size_t getProtoPayloadSize() const
        {
            PacketDesc pDesc;

            if ( examinePacket ( pDesc )
                 && pDesc.protoType == ProtoType::ProtoNumber
                 && pDesc.protoHeaderSize >= sizeof ( typename ProtoType::Header ) )
            {
                size_t totalHdrSize
                    = pDesc.ipHeaderSize
                      + reinterpret_cast<const typename ProtoType::Header *> ( pDesc.protoHeader )->getHeaderSize();

                if ( totalHdrSize < _buffer.getDataSize() )
                {
                    return ( _buffer.getDataSize() - totalHdrSize );
                }
            }

            return 0;
        }

        /// @brief Returns the name of given IP protocol (UDP, TCP, ICMP, etc.)
        /// @param [in] protoNum The protocol number.
        /// @return The name of the given protocol.
        static String getProtoName ( uint16_t protoNum );

        /// @brief Adjust the checksum of packet/header for a change of rdiff
        /// @note Both values should be in the same byte order, but it doesn't matter which one
        /// @param [in,out] checksum Pointer to a 16-bit IP packet checksum
        /// @param [in] diff Difference (old value - new value)
        /// @note Yes, it really is "old value - new value" and not the other way around!!
        static void adjustChecksum ( uint16_t & checksum, int32_t diff );

        /// @brief Adjust the checksum of packet/header for a change from oldValue to newValue
        /// @note All three values should be in the same byte order, but it doesn't matter which one
        /// @param [in,out] checksum Pointer to a 16-bit IP packet checksum
        /// @param [in] oldValue Old value in the packet
        /// @param [in] newValue New value in the packet
        static void adjustChecksum ( uint16_t & checksum, uint32_t oldValue, uint32_t newValue );

        /// @brief Adjust the checksum of packet/header for a change from oldValue to newValue
        /// @note All three values should be in the same byte order, but it doesn't matter which one
        /// @param [in,out] checksum Pointer to a 16-bit IP packet checksum
        /// @param [in] oldValue Old value in the packet
        /// @param [in] newValue New value in the packet
        static void adjustChecksum ( uint16_t & checksum, uint16_t oldValue, uint16_t newValue );

    protected:
        /// @brief A helper function for describing packet properties.
        struct PacketDesc
        {
            const char * protoHeader; ///< Pointer to the first byte of payload protocol's header.

            /// @brief The size of memory pointed to by protoHeader.
            /// @note This is the max possible header size, the size actually used by the header may be smaller.
            uint16_t protoHeaderSize;

            uint16_t ipHeaderSize; ///< The size of the IP header.

            uint16_t protoType; ///< The type of the payload protocol. This could include ProtoBitNextIpFragment.
        };

        static TextLog _log; ///< Log stream

        /// @brief Configures the IP packet for a specific payload protocol.
        ///
        /// This packet will be well-structured enough to be treated as a valid IpPacket by internal methods.
        /// For this packet to be valid for sending, internal protocol's header needs to be configured.
        /// This constructor will allocate data for payload protocol's header, but not set it in any way.
        ///
        /// If this creates an IPv4 packet, following fields will be set:
        ///    version = 4
        ///    header_length = 5
        ///    length = 20 + payloadHdrSize + the size of the payload data
        ///    payload_proto = payloadProto
        ///    src_address = srcAddr
        ///    dest_address = destAddr
        ///    checksum - calculated
        ///   Also, following fields are initialized to 0: ID, FragOffset.
        ///
        /// If this creates an IPv6 packet, following fields will be set:
        ///    version = 6
        ///    payload_length = payloadHdrSize + the size of the payload data
        ///    next_header = payloadProto
        ///   Also, following fields are initialized to 0: FlowLabel
        ///
        /// In both cases the buffer allocated for payload protocol's header will NOT be initialized to 0.
        ///
        /// @note If the buffer contains any data when this is called, it will be replaced with new memory.
        ///
        /// @param [in] srcAddr The source address to set.
        /// @param [in] destAddr The destination address to set. Has to be of the same type as the source address.
        /// @param [in] payloadProto The payload protocol to set.
        /// @param [in] payloadHdrSize The number of bytes to reserve for payload protocol's header.
        ///                            That data will be reserved in the first chunk, after IP header.
        /// @param [in] payloadData The payload data to append to the buffer.
        /// @param [in] tos Value for the DSCP/ECN (IPv4) or Traffic Class (IPv6) field in the IpPacket.
        /// @param [in] ttl Value for the TTL (IPv4) or Hop Limit (IPv6) field in the IpPacket.
        /// @return Pointer to the memory where payload protocol's header should be stored on success,
        ///         or 0 if the packet could not be constructed (because, for example, source and destination
        ///         addresses were not of the same type).
        ///         This points to payloadHdrSize bytes immediately after IP header.
        ///         Those bytes are NOT initialized to 0.
        char * initProtoPacket (
            const IpAddress & srcAddr, const IpAddress & destAddr,
            Proto::Number payloadProto, uint16_t payloadHdrSize, const MemVector & payloadData,
            uint8_t tos = 0, uint8_t ttl = 255 );

        /// @brief Examines the packet.
        /// @param [out] pDesc Decoded packet's parameters. May not be modified on error.
        /// @return True if the packet makes sense; False otherwise (also if there is no payload protocol's header).
        bool examinePacket ( PacketDesc & pDesc ) const;

    private:
        /// @brief The internal buffer that contains the data
        /// If it's not empty, it means that it contains data that has been verified to contain a correct IP packet.
        /// @note If the data is stored in multiple chunks, the IP header must NOT be split between multiple chunks.
        ///       The same applies to the entire header of the internal protocol (TCP, UDP, etc.).
        ///       However, that protocol's header does not need to be in the same chunk as IP header.
        MemVector _buffer;
};

/// @brief Streaming operator
/// Appends an IpPacket's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value IpPacket for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const IpPacket & value );
}
