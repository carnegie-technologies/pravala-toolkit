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

#include "basic/Math.hpp"
#include "IpPacket.hpp"

namespace Pravala
{
/// @brief Contains TCP-related functions
class TcpPacket: public IpPacket
{
    public:
        static const Proto::Number ProtoNumber = Proto::TCP; ///< The protocol number of TCP

        static const uint8_t FlagFin = ( 0x01U ); ///< The FIN flag
        static const uint8_t FlagSyn = ( 0x02U ); ///< The SYN flag
        static const uint8_t FlagRst = ( 0x04U ); ///< The RST flag
        static const uint8_t FlagPsh = ( 0x08U ); ///< The PSH flag
        static const uint8_t FlagAck = ( 0x10U ); ///< The ACK flag
        static const uint8_t FlagUrg = ( 0x20U ); ///< The URG flag
        static const uint8_t FlagEce = ( 0x40U ); ///< The ECE flag
        static const uint8_t FlagCwr = ( 0x80U ); ///< The CWR flag

        static const uint8_t OptEnd = 0;      ///< End of options list
        static const uint8_t OptNop = 1;      ///< Padding option (NOP)
        static const uint8_t OptMss = 2;      ///< Maximum segment size
        static const uint8_t OptWScale = 3;   ///< Window scale option
        static const uint8_t OptSAckPerm = 4; ///< Selective acknowledgements permitted
        static const uint8_t OptSAck = 5;     ///< Selective acknowledgement
        static const uint8_t OptTStamp = 8;   ///< Timestamp

#pragma pack(push, 1)
        /// @brief TCP packet header structure
        /// @note The fields are in this specific order to match a TCP header on the wire!
        struct Header
        {
            public:
                uint16_t source_port; ///< source port
                uint16_t dest_port; ///< destination port
                uint32_t seq_num;   ///< sequence number
                uint32_t ack_num;   ///< acknowledgement number (if ACK flag is set)

            private:
                // This one is private.
                // It really should NOT be modified (except by TcpPacket while constructing new packets).

                uint8_t data_off_res; ///< Data offset (TCP header size); upper 4 bits reserved

            public:
                uint8_t flags;      ///< Flags bitmask
                uint16_t window;    ///< Window size
                uint16_t checksum;  ///< Checksum of TCP header+data
                uint16_t urgent_ptr; ///< Urgent pointer - seq number of last urgent data packet

                /// @brief Returns the size of the header including all TCP options.
                /// @return The header size (in bytes).
                inline uint8_t getHeaderSize() const
                {
                    // In number of 32bit words: ( data_off_res >> 4 ) & 0xF;
                    // In number of bytes (same as above * 4): ( data_off_res >> 2 ) & 0x3C;
                    // Also, in case the header is not initialized properly yet, we want to return sizeof(Header):
                    return max<uint8_t> ( sizeof ( Header ), ( data_off_res >> 2 ) & 0x3C );
                }

                /// @brief Checks if the header has ACK flag set.
                /// @return True if the header has ACK flag set; False otherwise.
                inline bool isACK() const
                {
                    return ( ( flags & FlagAck ) != 0 );
                }

                /// @brief Checks if the header has RST flag set.
                /// @return True if the header has RST flag set; False otherwise.
                inline bool isRST() const
                {
                    return ( ( flags & FlagRst ) != 0 );
                }

                /// @brief Checks if the header has SYN flag set.
                /// @return True if the header has SYN flag set; False otherwise.
                inline bool isSYN() const
                {
                    return ( ( flags & FlagSyn ) != 0 );
                }

                /// @brief Checks if the header has FIN flag set.
                /// @return True if the header has FIN flag set; False otherwise.
                inline bool isFIN() const
                {
                    return ( ( flags & FlagFin ) != 0 );
                }

                /// @brief Return the source port stored in the header.
                /// @return The source port stored in the header (using host endianness).
                inline uint16_t getSrcPort() const
                {
                    return ntohs ( source_port );
                }

                /// @brief Return the destination port stored in the header.
                /// @return The destination port stored in the header (using host endianness).
                inline uint16_t getDestPort() const
                {
                    return ntohs ( dest_port );
                }

                /// @brief Return the data sequence number stored in the header.
                /// @return The data sequence number stored in the header (using host endianness).
                inline uint32_t getSeqNum() const
                {
                    return ntohl ( seq_num );
                }

                /// @brief Return the data acknowledgement number stored in the header.
                /// @return The data acknowledgement number stored in the header (using host endianness).
                inline uint32_t getAckNum() const
                {
                    return ntohl ( ack_num );
                }

                /// @brief Return the checksum stored in the header.
                /// @return The checksum stored in the header (using host endianness).
                inline uint16_t getChecksum() const
                {
                    return ntohs ( checksum );
                }

                /// @brief Return the size of the receive window stored in the header.
                /// @return The size of the receive window stored in the header (using host endianness).
                inline uint16_t getWindow() const
                {
                    return ntohs ( window );
                }

                /// @brief Sets the data offset in the header. In TCP packets, the dataOffset field holds the
                /// size of the TCP header, in other words the number of bytes offset from the beginning of the TCP
                /// packet to get to the payload of the TCP packet.
                /// @param [in] dataOffset The size of the header, in words! Not bytes!
                /// @note It will adjust the checksum when run.
                void setDataOffsetInWords ( uint8_t dataOffset );

                /// @brief Sets or unsets the ACK field in the header.
                /// @param [in] value Whether the ACK bit should be set or not.
                /// @note It will adjust the checksum when run.
                void setACK ( bool value );

                /// @brief Sets or unsets the RST field in the header.
                /// @param [in] value Whether the RST bit should be set or not.
                /// @note It will adjust the checksum when run.
                void setRST ( bool value );

                /// @brief Sets or unsets the SYN field in the header.
                /// @param [in] value Whether the SYN bit should be set or not.
                /// @note It will adjust the checksum when run.
                void setSYN ( bool value );

                /// @brief Sets or unsets the FIN field in the header.
                /// @param [in] value Whether the FIN bit should be set or not.
                /// @note It will adjust the checksum when run.
                void setFIN ( bool value );

                /// @brief Sets the source port in the header.
                /// @param [in] sourcePort The new source port.
                /// @note It will adjust the checksum when run.
                void setSrcPort ( uint16_t sourcePort );

                /// @brief Sets the destination port in the header.
                /// @param [in] destPort The new destination port.
                /// @note It will adjust the checksum when run.
                void setDestPort ( uint16_t destPort );

                /// @brief Sets the sequence number in the header.
                /// @param [in] seqNum The new sequence number.
                /// @note It will adjust the checksum when run.
                void setSeqNum ( uint32_t seqNum );

                /// @brief Sets the acknowledgement number in the header.
                /// @param [in] ackNum The new acknowledgement number.
                /// @note It will adjust the checksum when run.
                void setAckNum ( uint32_t ackNum );

                /// @brief Sets the checksum of the packet.
                /// @param [in] newChecksum The new checksum to set
                inline void setChecksum ( uint16_t newChecksum )
                {
                    checksum = newChecksum;
                    // Don't recalculate the checksum, since this is the value to use.
                }

                /// @brief Gets the value of MSS option from the packet.
                /// It only works if the packet has SYN flag set. It will also assume that the memory immediately
                /// after the header contains TCP options (up to payload offset), so it should not be called
                /// if it's not the case!
                /// @param [out] value Will be set to retrieved MSS option value (using host endianness).
                ///                    Only modified if this returns 'true'.
                /// @return True if the header has the SYN flag set, the option was present, and the value was set;
                ///         False if the option value could not be extracted.
                bool getOptMss ( uint16_t & value ) const;

                /// @brief Gets the value of window scale option from the packet.
                /// It only works if the packet has SYN flag set. It will also assume that the memory immediately
                /// after the header contains TCP options (up to payload offset), so it should not be called
                /// if it's not the case!
                /// @param [out] value Will be set to retrieved window scale option value (as a left shift value).
                ///                    Only modified if this returns 'true'.
                /// @return True if the header has the SYN flag set, the option was present, and the value was set;
                ///         False if the option value could not be extracted.
                bool getOptWindowScale ( uint8_t & value ) const;

                /// @brief Reads the content of TCP option.
                /// It assumes that the memory immediately after the header contains TCP options (up to payload offset),
                /// so it should not be called if it's not the case!
                /// @param [in] optType The type of the option to find.
                /// @param [out] data Will be set to point at the first byte of the option's value.
                ///                   Note that that data uses network endianness.
                ///                   Set to 0 if this function returns 'false', or when the option had no value.
                /// @param [out] dataLen Will be set to length of option's data (could be 0).
                ///                      Set to 0 if this function returns 'false', or when the option had no value.
                /// @return True if the option was present; False otherwise.
                ///         It is possible that this returns true, but data and dataLen are set to 0.
                ///         This means that the option was found, but it had no value stored.
                bool getOptData ( uint8_t optType, const char * & data, uint8_t & dataLen ) const;

                friend class TcpPacket;
        };

#pragma pack(pop)

        /// @brief Helper structure for passing options to new TcpPackets.
        struct Option
        {
            /// @brief Pointer to option's data.
            /// Depending on the option type it might be ignored.
            /// The data should be in network endianness since it is NOT converted automaticaly.
            const void * data;

            uint8_t type; ///< Option type.
            uint8_t dataLength; ///< Length of option's data. Could be 0.

            /// @brief Default constructor.
            /// It configures this option as a "NOP" option.
            inline Option(): data ( 0 ), type ( OptNop ), dataLength ( 0 )
            {
            }
        };

        /// @brief Default constructor.
        /// It creates an empty (invalid) TCP packet
        TcpPacket();

        /// @brief Creates a new TCP packet
        /// This packet should be valid for sending.
        /// It uses the IpPacket(srcAddr, destAddr, Proto::TCP, ...) constructor,
        /// so for the description of the IP fields set look at that constructor's comment.
        /// After configuring this TCP packet the checksum is calculated, so this packet should be ready
        /// for sending.
        /// Any other modifications (through Header's set*() functions) will adjust the checksum accordingly.
        /// @param [in] srcAddr The source address to set
        /// @param [in] srcPort The source port to set
        /// @param [in] destAddr The destination address to set. Has to be of the same size as the source address
        /// @param [in] destPort The destination port to set
        /// @param [in] flagsToSet The bitsum of Flag* values (see above) that determines which flags should be set
        /// @param [in] seqNum The sequence number to set.
        /// @param [in] ackNum The optional ACK number to set; Only used if FlagAck is set in flagsToSet.
        /// @param [in] winSize The receive window size to set.
        /// @param [in] payload The data to be used as this packet's payload.
        /// @param [in] options Pointer to the array of Option values.
        ///                     Options will be padded. If there are too many options, or something is wrong
        ///                     (like invalid option length), then an invalid packet will be generated.
        ///                     Note: End-of-options option should not be used and will be treated as an error.
        /// @param [in] optCount The number of entries in options array.
        TcpPacket (
            const IpAddress & srcAddr, uint16_t srcPort,
            const IpAddress & destAddr, uint16_t destPort,
            uint8_t flagsToSet,
            uint32_t seqNum, uint32_t ackNum, uint16_t winSize = 0,
            const MemVector & payload = MemVector::EmptyVector,
            Option * options = 0, uint8_t optCount = 0 );

        /// @brief A helper function that generates a TCP RESET packet in response to given TCP packet.
        /// It only generates a valid packet if the input IP packet was a TCP packet,
        /// and it was NOT a TCP RESET packet (it is against RFC 793 to respond to a reset with a reset).
        /// @param [in] packet The packet to respond to.
        /// @return A TCP response packet on success; An empty (invalid) packet if the response should not be sent.
        static TcpPacket generateResetResponse ( const IpPacket & packet );

        /// @brief Appends the description of a packet to the buffer.
        /// @param [in] ipPacket The IP packet to describe.
        /// @param [in,out] toBuffer The buffer to append the description to. It is not cleared first.
        static void describe ( const IpPacket & ipPacket, Buffer & toBuffer );

        /// @brief Modifies MSS in TCP packets
        /// @param [in] modifMSS If > 0 it describes the max allowed MSS.
        ///   if < 0 it means by how many bytes the actual MSS should be decreased.
        /// @param [out] oldMSS Contains MSS read from the packet before the change
        /// @param [out] newMSS Contains MSS that was set in the packet
        /// @return True if the packet was modified. False otherwise (for example this was not SYN packet)
        static bool fixMSS ( IpPacket & ipPacket, int modifMSS, int & oldMSS, int & newMSS );

        /// @brief Helper function that calculates the size of memory needed to store options.
        /// Returned value includes all needed padding, and won't be greater than 40 bytes.
        /// @param [in] options Pointer to the array of Option values.
        ///                     Note: End-of-options option should not be used and will be treated as an error.
        /// @param [in] optCount The number of entries in options array.
        /// @return The number of bytes needed to store options provided.
        ///         This value will include needed padding and won't be greater than 40 bytes.
        static uint8_t getOptLen ( const Option * options, uint8_t optCount );
};

/// @brief Streaming operator
/// Appends an TcpPacket's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value TcpPacket for which description should be appended
/// @return Reference to the TextMessage object
inline TextMessage & operator<< ( TextMessage & textMessage, const TcpPacket & value )
{
    return operator<< ( textMessage, ( const IpPacket & ) value );
}
}
