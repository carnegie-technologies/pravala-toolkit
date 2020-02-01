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

#include "basic/MemVector.hpp"
#include "basic/IpAddress.hpp"
#include "basic/List.hpp"
#include "basic/HashSet.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief Represents a DNS packet
///
/// It contains several DNS-related defines and functions,
/// and also basic support for understanding DNS packets.
/// It stores the entire DNS packet's memory and checks all the records,
/// but exposes only a few basic data elements that we need.
class DnsPacket
{
    public:

        static const uint8_t FlagARecursionDesired = ( 0x01U ); ///< Recursion Desired flag
        static const uint8_t FlagATruncated = ( 0x02U ); ///< Truncation flag
        static const uint8_t FlagAAuthorativeAnswer = ( 0x04U ); ///< Authoritative Answer flas
        static const uint8_t FlagAResponse = ( 0x80U ); ///< Response flag

        static const uint8_t FlagBRecursionAvailable = ( 0x80U ); ///< Recursion Available flag

        /// @brief Possible operation codes
        enum OpCodes
        {
            OpCodeQuery = 0, ///< A standard query
            OpCodeStatus = 2, ///< Server status request
            OpCodeNotify = 4, ///< Notify message
            OpCodeUpdate = 5 ///< Update message
        };

        /// @brief Possible response codes
        enum RespCodes
        {
            RespNoError = 0, ///< No error
            RespFormatError = 1, ///< Query was incorrectly constructed
            RespServerFailure = 2, ///< Server failed
            RespNameError = 3, ///< The name in the query does not exist in the domain
            RespNotImplemented = 4, ///< This query type is not supported
            RespRefused = 5, ///< Server refused to process the query
            RespYXDomain = 6, ///< A name exists when it should not
            RespYXRRSet = 7, ///< A resource record set exists that should not
            RespNXRRSet = 8, ///< A resource record set that should exist does not
            RespNotAuth = 9, ///< The server is not authoritative for the zone specified
            RespNotZone = 10 ///< A name specified in the message is not within the zone specified in the message
        };

        /// @brief Possible record types (and q-types)
        enum RRTypes
        {
            RRTypeA = 1, ///< RecordType A
            RRTypeAAAA = 28, ///< RecordType AAAA
            RRTypeCNAME = 5, ///< RecordType CNAME
            RRTypeLOC = 29, ///< RecordType LOC
            RRTypeMX = 15, ///< RecordType MX
            RRTypeNS = 2, ///< RecordType NS
            RRTypePTR = 12, ///< RecordType PTR
            RRTypeSOA = 6, ///< RecordType SOA
            RRTypeSRV = 33, ///< RecordType SRV
            RRTypeTXT = 16, ///< RecordType TXT

            // Q-TYPES (not really record types, can only be used in queries):
            RRTypeQAll = 255 ///< Record Q-Type: request for all records
        };

        /// @brief Possible class types (and q-class)
        enum ClassTypes
        {
            ClassIN = 1, ///< The Internet

            // Q-CLASS values (not really classes, can only be used in queries):
            ClassQAny = 255 ///< Q-class: Any class
        };

#pragma pack(push, 1)
        /// @brief Describes fixed DNS header
        struct Header
        {
            uint16_t identifier; ///< Identifier of the query (network endianness)

            uint8_t flagsA; ///< Flags (group A)
            uint8_t flagsB; ///< Flags (group B)

            /// @brief Number of queries in the the message
            /// @note It's in network endianness
            uint16_t numberQuestions;

            /// @brief Number of answer records in this message
            /// @note It's in network endianness
            uint16_t numberAnswerRecords;

            /// @brief Number of authority records in this message
            /// @note It's in network endianness
            uint16_t numberAuthorityRecords;

            /// @brief Number of additional records in this message
            /// @note It's in network endianness
            uint16_t numberAdditionalRecords;

            /// @brief This is a convenience function to check if this packet is a DNS response
            /// @return True if this is a DNS response packet; False otherwise
            inline bool isResponse() const
            {
                return ( flagsA & FlagAResponse ) != 0;
            }

            /// @brief Returns the identifier stored in the header
            /// @return the identifier stored in the header
            inline uint16_t getIdentifier() const
            {
                return ntohs ( identifier );
            }

            /// @brief Returns the operation code stored in the header
            /// @return the operation code stored in the header
            inline uint8_t getOperationCode() const
            {
                return ( flagsA >> 3 ) & 0x0F;
            }

            /// @brief Returns the response code stored in the header
            /// @return the response code stored in the header
            inline uint8_t getResponseCode() const
            {
                return flagsB & 0x0F;
            }

            /// @brief Returns the number of questions in the packet
            /// @return the number of questions in the packet
            inline uint16_t getNumQuestions() const
            {
                return ntohs ( numberQuestions );
            }

            /// @brief Returns the number of answer records in the packet
            /// @return the number of answer records in the packet
            inline uint16_t getNumAnswerRecords() const
            {
                return ntohs ( numberAnswerRecords );
            }

            /// @brief Returns the number of authority records in the packet
            /// @return the number of authority records in the packet
            inline uint16_t getNumAuthorityRecords() const
            {
                return ntohs ( numberAuthorityRecords );
            }

            /// @brief Returns the number of additional records in the packet
            /// @return the number of additional records in the packet
            inline uint16_t getNumAdditionalRecords() const
            {
                return ntohs ( numberAdditionalRecords );
            }
        };

#pragma pack(pop)

        /// @brief Represents a single 'query' record
        struct Query
        {
#pragma pack(push, 1)
            /// @brief Fixed part of the Query
            struct Data
            {
                uint16_t rType; ///< The type of the query. In host's endianness
                uint16_t rClass; ///< The class of the query. In host's endianness

                /// @brief Default constructor
                Data(): rType ( 0 ), rClass ( 0 )
                {
                }
            };

#pragma pack(pop)

            String name; ///< The 'name' of the query. Converted into dot notation (from internal DNS' format)
            Data data; ///< The actual data

            /// @brief Compares two Query objects to determine equality.
            /// @param [in] other The second Query object to compare
            /// @return A value indicating whether the two Query objects are equal
            bool operator== ( const Query & other ) const;

            /// @brief Compares two Query objects to determine inequality.
            /// @param [in] other The second Query object to compare
            /// @return A value indicating whether the two Query objects are NOT equal
            bool operator!= ( const Query & other ) const;
        };

        /// @brief Represents a single resource record
        struct ResRecord
        {
#pragma pack(push, 1)
            /// @brief Fixed part of the Resource Record
            struct Data: public Query::Data
            {
                uint32_t rTTL;     ///< TTL, in host's endianness
                uint16_t rDataSize;     ///< Size of the record's data

                /// @brief Default constructor
                Data(): rTTL ( 0 ), rDataSize ( 0 )
                {
                }
            };

#pragma pack(pop)

            String name;     ///< The 'name' of the record. Converted into dot notation (from internal DNS' format)
            Data data;     ///< The actual data

            /// @brief Offset of the record's data in the packet's original memory
            /// We can't store just this record's data, since it may contain compressed domain names
            /// and to decode them we will need the entire packet.
            uint16_t rDataOffset;

            /// @brief Default constructor
            ResRecord(): rDataOffset ( 0 )
            {
            }
        };

        /// @brief Returns the list of queries
        /// @return The list of queries
        inline const List<Query> & getQueries() const
        {
            return _queries;
        }

        /// @brief Returns the list of CNAME answers
        /// @return The list of CNAME answers
        inline const StringList & getCnames() const
        {
            return _cnames;
        }

        /// @brief Returns the set of A and AAAA answers
        /// @return The set of A and AAAA answers
        inline const HashSet<IpAddress> & getAddresses() const
        {
            return _addrs;
        }

        /// @brief Returns the shortest TTL value
        /// @return the shortest TTL value
        inline uint32_t getShortestTtl() const
        {
            return _shortestTtl;
        }

        /// @brief Returns the original data
        /// @return The original data of this DNS packet
        inline const MemHandle & getData() const
        {
            return _data;
        }

        /// @brief Returns a pointer to this packet's DNS header
        /// @return a pointer to this packet's DNS header, or 0 if packet is incorrect
        const Header * getHeader() const;

        /// @brief Configures the packet based on the MemHandle provided
        /// It will create a reference to the same memory.
        /// @param [in] data The memory to use
        /// @return True if the packet has been parser properly; False otherwise
        bool setupPacket ( const MemHandle & data );

        /// @brief Configures the packet based on the MemVector provided
        /// It will create a reference to the same memory.
        /// @param [in] data The memory to use
        /// @return True if the packet has been parser properly; False otherwise
        bool setupPacket ( const MemVector & data );

        /// @brief Generates a description of this DNS packet
        /// @param [in] toBuffer Buffer to append the description to
        void describe ( Buffer & toBuffer ) const;

        /// @brief Clears the content of this packet
        void clear();

        /// @brief Returns a description of given operation code value
        /// @warning It uses static buffer for unknown values!
        /// @param [in] code The code value to check
        /// @return A description of given operation code value
        static const char * opCode2Str ( uint8_t code );

        /// @brief Returns a description of given response code value
        /// @warning It uses static buffer for unknown values!
        /// @param [in] code The code value to check
        /// @return A description of given response code value
        static const char * respCode2Str ( uint8_t code );

        /// @brief Returns a description of given record type
        /// @warning It uses static buffer for unknown values!
        /// @param [in] code The record type to check
        /// @return A description of given record type
        static const char * rrType2Str ( uint16_t code );

        /// @brief Returns a description of given record class
        /// @warning It uses static buffer for unknown values!
        /// @param [in] code The record class to check
        /// @return A description of given record class
        static const char * class2Str ( uint16_t code );

    private:
        MemHandle _data; ///< Packet's data
        List<Query> _queries; ///< The list of queries
        StringList _cnames; ///< List of CNAME domains in 'answers' section
        HashSet<IpAddress> _addrs; ///< Set of A or AAAA values in 'answers' section
        uint32_t _shortestTtl; ///< The shortest TTL received

        /// @brief Reads queries from a DNS packet
        /// It will clear the list of queries first!
        /// @param [in] count Number of queries to read
        /// @param [in,out] index Index within the packet at which to start reading at;
        ///                        It is modified as the data is consumed.
        /// @return True if 'count' queries were read properly; False if there was an error (which is related to
        ///          incorrect size/offset, invalid data, or not enough elements read)
        bool readQueries ( uint16_t count, uint16_t & index );

        /// @brief Reads records from a DNS packet
        /// @param [in] records A list to put records in (it is cleared at the beginning).
        ///                     There are 3 different list of records in the DNS packet, and all are formatted
        ///                     in the same way. readRecords doesn't care what it's reading, but it needs to know
        ///                     were to put the result.
        /// @param [in] count Number of records to read
        /// @param [in,out] index Index within the packet at which to start reading at;
        ///                        It is modified as the data is consumed.
        /// @return True if 'count' records were read properly; False if there was an error (which is related to
        ///          incorrect size/offset, invalid data, incorrect recIndex, or not enough elements read)
        bool readRecords ( List<ResRecord> & records, uint16_t count, uint16_t & index );

        /// @brief Performs the actual data parsing
        /// It should be called with all data structures cleared, and _data set to packet's data
        /// @return True if everything was parsed properly; False otherwise
        bool parsePacket();

        /// @brief Reads a single domain name from a DNS packet
        /// It will convert label list format to a simple string using '.' as separators.
        /// It resolves pointers used in DNS packets for data compression.
        /// @param [in] packetData Pointer to the beginning of the packet's data. It HAS to point to the
        ///                         beginning, to be able to handle pointers properly.
        /// @param [in] packetSize Size of the entire packet
        /// @param [in] output The string to append the name to
        /// @param [in,out] index Index within the data at which to start reading the string;
        ///                        It is modified as the data is consumed, but will not reflect the data
        ///                        consumed after resolving pointers.
        /// @return True if the name has been read properly; False if there was an error (which is related to
        ///          incorrect size/offset, or invalid data - lack of \0 at the end, or incorrect pointers)
        static bool readDomainName ( const char * packetData, uint16_t packetSize, String & output, uint16_t & index );
};

/// @brief Streaming operator
/// Appends DnsPacket's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value DnsPacket for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const DnsPacket & value );

/// @brief Hash function needed for using DNS query objects in hashing containers
/// @param [in] key The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
size_t getHash ( const DnsPacket::Query & key );
}
