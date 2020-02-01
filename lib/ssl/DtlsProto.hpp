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

extern "C"
{
#include <stdint.h>
}

namespace Pravala
{
#pragma pack(push, 1)

/// @brief 'Mini' DTLS header - the common part of DTLS headers
/// @warning DTLS-related headers use weird alignment, so all operations on 2 or 4 byte fields
///           have to use memory copy instead of just assigning values!
struct DtlsMiniHeader
{
    static const uint8_t TypeChangeCipherSpec = 20; ///< DTLS 'change_cipher_spec' packet type
    static const uint8_t TypeAlert = 21; ///< DTLS 'handshake' packet type
    static const uint8_t TypeHandshake = 22; ///< DTLS 'handshake' packet type
    static const uint8_t TypeAppData = 23; ///< DTLS 'application data' packet type

    static const uint8_t VersionMajorDtls = 0xFE; ///< DTLS major version
    static const uint8_t VersionMinorDtls_1_0 = 0xFF; ///< DTLS 1.0 minor version
    static const uint8_t VersionMinorDtls_1_2 = 0xFD; ///< DTLS 1.2 minor version

    uint8_t type; ///< The type of the packet (handshake, data, etc.)
    uint8_t versionMajor; ///< The 'major' part of the protocol version. For DTLS it should be 0xFE.
    uint8_t versionMinor; ///< The 'minor' part of the protocol version. For DTLS it should be 0xFF.

    /// @brief Checks if the packet is a DTLS packet
    /// @return True if this is a header of a DTLS packet
    inline bool isDtls() const
    {
        return ( VersionMajorDtls == versionMajor
                 && ( VersionMinorDtls_1_0 == versionMinor || VersionMinorDtls_1_2 == versionMinor ) );
    }

    /// @brief Checks if the packet is a DTLS handshake packet
    /// @return True if this is a header of a DTLS handshake packet
    inline bool isDtlsHandshake() const
    {
        return ( isDtls() && type == TypeHandshake );
    }
};

#pragma pack(pop)

#pragma pack(push, 1)

/// @brief Structure that represents a DTLS header
/// @warning DTLS-related headers use weird alignment, so all operations on 2 or 4 byte fields
///           have to use memory copy instead of just assigning values!
struct DtlsHeader: public DtlsMiniHeader
{
    /// @brief DTLS epoch number.
    /// @note It uses a byte array because it may not be aligned properly.
    uint8_t epoch[ 2 ];

    /// @brief DTLS sequence number.
    uint8_t sequenceNumber[ 6 ];

    /// @brief The length of the payload that follows (doesn't include the header).
    /// @note It uses a byte array because it may not be aligned properly.
    uint8_t length[ 2 ];

    /// @brief Return the two least significant bytes of the DTLS sequence number
    /// @return The part of the DTLS sequence number
    inline uint16_t getPartialSeqNum() const
    {
        return ( ( ( uint16_t ) sequenceNumber[ 4 ] ) << 8 ) | sequenceNumber[ 5 ];
    }
};

#pragma pack(pop)

#pragma pack(push, 1)

/// @brief Structure that represents a DTLS handshake packet's header
/// @warning DTLS-related headers use weird alignment, so all operations on 2 or 4 byte fields
///           have to use memory copy instead of just assigning values!
struct DtlsHandshakeHeader: public DtlsHeader
{
    static const uint8_t HandshakeTypeClientHello = 1; ///< 'ClientHello' handshake packet
    static const uint8_t HandshakeTypeServerHello = 2; ///< 'ServerHello' handshake packet
    static const uint8_t HandshakeTypeHelloVerifyReq = 3; ///< 'HelloVerifyRequest' handshake packet
    static const uint8_t HandshakeTypeCertificate = 11; ///< 'Certificate' handshake packet
    static const uint8_t HandshakeTypeCertificateReq = 13; ///< 'CertificateRequest' handshake packet
    static const uint8_t HandshakeTypeServerHelloDone = 14; ///< 'ServerHelloDone' handshake packet
    static const uint8_t HandshakeTypeCertificateVerify = 15; ///< 'CertificateVerify' handshake packet
    static const uint8_t HandshakeTypeClientKeyExchange = 16; ///< 'ClientKeyExchange' handshake packet

    uint8_t handshakeType; ///< The type of the handshake packet

    /// @brief Returns the handshake type of the DTLS packet.
    /// @return The handshake type of the packet. If it is not a handshake packet (or it's too short), 0 is returned.
    inline uint8_t getHandshakeType() const
    {
        // The packet should be a DTLS handshake packet, and the payload length should be greater than 0.
        // 'length' is in network byte order, but we only need one byte of payload (the handshake type).
        // So we only care whether it's 0 or not - no need to convert.
        if ( !isDtlsHandshake() || ( length[ 0 ] == 0 && length[ 1 ] == 0 ) )
        {
            return 0;
        }

        // To make sure, just in case, that everything is in the right order in the memory...
        assert ( ( const char * ) &handshakeType == ( ( const char * ) &length ) + sizeof ( length ) );
        assert ( ( const char * ) &handshakeType == ( ( const char * ) &type ) + sizeof ( DtlsHeader ) );

        // The first byte AFTER the 'length' field is the handshake type
        return handshakeType;
    }

    /// @brief Checks if the packet is a DTLS handshake 'ClientHello' packet
    /// @return True if this is a header of a DTLS handshake 'ClientHello' packet
    inline bool isDtlsHandshakeClientHello() const
    {
        return ( getHandshakeType() == HandshakeTypeClientHello );
    }
};

#pragma pack(pop)

}
