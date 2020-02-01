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

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <winsock2.h>
#include <ws2tcpip.h>
}
#else
extern "C"
{
#include <netinet/in.h>
}
#endif

#ifdef __SSE4_2__
extern "C"
{
#include <nmmintrin.h>
}
#endif

#include "log/TextMessage.hpp"

namespace Pravala
{
/// @brief Contains common (between IPv4 and IPv6) part of flow description
struct FlowDescCommon
{
    uint8_t type; ///< The type of the flow (4/6)
    uint8_t foo; ///< Not used at the moment, present for better data alignment)
    uint16_t heProto; ///< Internal protocol. This field (unlike other FlowDesc* fields) uses HOST endianness!

    union
    {
        struct
        {
            uint16_t client; ///< Client-side port
            uint16_t server; ///< Server-side port
        } port; ///< Protocols that use ports!

        // Maybe some other data for other protocols?
        // Also, we can't use ICMP code/type because request/response carry different values
    } u;
};

/// @brief Contains a flow description
union FlowDesc
{
    FlowDescCommon common; ///< Common part

    struct V4Data
    {
        FlowDescCommon common; ///< Common part
        in_addr clientAddr; ///< Client-side address
        in_addr serverAddr; ///< Server-side address
    } v4; ///< IPv4 version

    struct V6Data
    {
        FlowDescCommon common; ///< Common part
        in6_addr clientAddr; ///< Client-side address
        in6_addr serverAddr; ///< Server-side address
    } v6; ///< IPv6 version

    /// @brief Clears the content of this flow descriptor.
    inline void clear()
    {
        memset ( this, 0, sizeof ( *this ) );
    }

    /// @brief Checks if this flow descriptor is valid.
    /// This only checks whether the type is set to 4 or 6.
    /// @return True if this flow descriptor uses a valid type (4 or 6).
    inline bool isValid() const
    {
        return ( common.type == 4 || common.type == 6 );
    }

    /// @brief Compares two FlowDesc objects to determine equality
    /// @param [in] other The second FlowDesc object to compare
    /// @return A value indicating whether the two FlowDesc objects are equal
    inline bool operator== ( const FlowDesc & other ) const
    {
        return ( ( common.type == 4 )
                 ? ( memcmp ( &v4, &( other.v4 ), sizeof ( v4 ) ) == 0 )
                 : ( memcmp ( &v6, &( other.v6 ), sizeof ( v6 ) ) == 0 ) );
    }

    /// @brief A helper function that determines whether the flow is a UDP DNS
    /// @return True if this flow is a UDP DNS "flow"
    inline bool isUdpDns() const
    {
        static const uint16_t dnsPort ( htons ( 53 ) );

        // UDP: proto 17
        // DNS: server port 53
        return ( isValid() && common.heProto == 17 && common.u.port.server == dnsPort );
    }

    /// @brief Returns the hash of this FlowDesc object
    /// If support for SSE 4.2 is compiled in and available on this machine,
    /// CRC32C is used - it is by far the fastest possible hash function.
    /// On 64 bit machine the hash is created using only 2 calls for IPv4 and 5 for IPv6.
    /// If SSE 4.2 is not built-in or not available on this platform, the fast FNV hashing is used instead.
    /// @return the hash of this FlowDesc object
    inline uint32_t getHash() const
    {
#ifdef __SSE4_2__
        static const bool hasSse42 ( detectSse42() );

        if ( hasSse42 )
        {
#ifdef __x86_64__
            uint64_t crc = _mm_crc32_u64 ( ( uint64_t ) 0, ( ( const uint64_t * ) this )[ 0 ] );
            crc = _mm_crc32_u64 ( crc, ( ( const uint64_t * ) this )[ 1 ] );

            // If type is '4' we do 16 bytes (2*8)
            // If type is anything else, we do the entire thing - 40 bytes (5*8)

            if ( common.type != 4 )
            {
                crc = _mm_crc32_u64 ( crc, ( ( const uint64_t * ) this )[ 2 ] );
                crc = _mm_crc32_u64 ( crc, ( ( const uint64_t * ) this )[ 3 ] );
                crc = _mm_crc32_u64 ( crc, ( ( const uint64_t * ) this )[ 4 ] );
            }

            return ( uint32_t ) ( crc & 0xFFFFFFFFUL );
#else /* __x86_64__ */
            uint32_t crc = _mm_crc32_u32 ( ( uint32_t ) 0, ( ( const uint32_t * ) this )[ 0 ] );
            crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 1 ] );
            crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 2 ] );
            crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 3 ] );

            // If type is '4' we do 16 bytes (4*4)
            // If type is anything else, we do the entire thing - 40 bytes (10*4)

            if ( common.type != 4 )
            {
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 4 ] );
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 5 ] );
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 6 ] );
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 7 ] );
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 8 ] );
                crc = _mm_crc32_u32 ( crc, ( ( const uint32_t * ) this )[ 9 ] );
            }

            return crc;
#endif /* __x86_64__ */
        }
#endif /* __SSE4_2__ */

        //
        // FNV-1a hash (32bit)
        //
        // http://www.isthe.com/chongo/tech/comp/fnv/
        //
        // It is in public domain.
        //

        uint32_t hash = 2166136261U;

        for ( size_t i = ( ( common.type == 4 ) ? 16 : 40 ); i > 0; )
        {
            hash ^= ( ( const char * ) this )[ --i ];
            hash *= 16777619;
        }

        return hash;
    }

#ifdef __SSE4_2__

    private:
        /// @brief A helper function, that detects whether SSE 4.2 is supported or not
        /// @return True if SSE 4.2 instructions are supported; False otherwise
        static bool detectSse42();
#endif
};

/// @brief Streaming operator
/// Appends an FlowDesc's description to a TextMessage
/// @param [in] textMessage The TextMessage to append to
/// @param [in] value FlowDesc for which description should be appended
/// @return Reference to the TextMessage object
TextMessage & operator<< ( TextMessage & textMessage, const FlowDesc & value );

inline uint32_t getHash ( const FlowDesc & desc )
{
    return desc.getHash();
}
}
