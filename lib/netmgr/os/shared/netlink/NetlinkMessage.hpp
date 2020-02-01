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

#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"
#include "log/TextLog.hpp"

extern "C"
{
#include <linux/netlink.h>
}

/// @brief Equivalent to NLMSG_DATA, but operates on const pointer
#define NLMSG_CONST_DATA( nlh )    ( ( const void * ) ( ( ( const char * ) nlh ) + NLMSG_LENGTH ( 0 ) ) )

/// @brief Equivalent to NLMSG_NEXT, but operates on const pointer
#define NLMSG_CONST_NEXT( nlh, len ) \
    ( ( len ) -= NLMSG_ALIGN ( ( nlh )->nlmsg_len ),  \
      ( const struct nlmsghdr * ) ( ( ( const char * ) ( nlh ) ) + NLMSG_ALIGN ( ( nlh )->nlmsg_len ) ) )

namespace Pravala
{
/// @brief The Netlink Message core class.
/// @note This can only be used to store a single part of a Netlink message.
///        Multipart Netlink messages have to be stored in several NetlinkMessage objects.
class NetlinkMessage
{
    public:
        /// @brief Appends an RTA parameter
        /// @param [in] type The type of the parameter
        /// @param [in] data The pointer to parameter's payload
        /// @param [in] dataSize The size of the data
        /// @return True if it was appended properly, false if there is not enough space in the
        ///          underlying buffer, or the message has not been configured properly.
        bool appendRta ( int type, const void * data, const uint16_t dataSize );

        /// @brief Sets the sequence number in the message
        void setSeqNum ( uint32_t seqNum );

        /// @brief Returns the sequence number set in the message.
        /// @return the sequence number set in the message.
        uint32_t getSeqNum() const;

        /// @brief Returns the Netlink's Port ID carried in the message.
        /// @return the Netlink's Port ID set in the message.
        uint32_t getPid() const;

        /// @brief Checks if the message is valid.
        /// @return True if the message was marked as valid and the internal data makes sense; False otherwise.
        inline bool isValid() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            return ( _isValid
                     && ( hdr != 0 )
                     && ( hdr->nlmsg_len >= NLMSG_LENGTH ( 0 ) )
                     && ( _buf.size() >= hdr->nlmsg_len ) );
        }

        /// @brief Marks whether the message is 'valid' or not
        /// @param [in] valid The new 'is valid' flag
        inline void setValid ( bool valid = true )
        {
            _isValid = valid;
        }

        /// @brief Returns the MemHandle with the message's internal data
        inline MemHandle getHandle() const
        {
            if ( !isValid() )
                return MemHandle();

            return _buf.getHandle ( 0, getNlmsghdr()->nlmsg_len );
        }

        /// @brief Returns a pointer to the nlmsghdr stored in the buffer.
        /// @return A pointer to the nlmsghdr stored in the buffer or 0 if the buffer is too small.
        inline struct nlmsghdr * getNlmsghdr()
        {
            return ( _buf.size() < sizeof ( struct nlmsghdr ) ) ? ( 0 ) : ( ( struct nlmsghdr * ) _buf.getWritable() );
        }

        /// @brief Returns a pointer to the nlmsghdr stored in the buffer.
        /// @return A pointer to the nlmsghdr stored in the buffer or 0 if the buffer is too small.
        inline const struct nlmsghdr * getNlmsghdr() const
        {
            return ( _buf.size() < sizeof ( struct nlmsghdr ) ) ? ( 0 ) : ( ( const struct nlmsghdr * ) _buf.get() );
        }

        /// @brief Returns currently set nlmsg_flags value
        /// @return currently set nlmsg_flags value
        inline uint16_t getNetlinkFlags() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            return ( !hdr ) ? 0 : hdr->nlmsg_flags;
        }

        /// @brief Sets nlmsg_flags value
        /// @param [in] flags New flags to set
        inline void setNetlinkFlags ( uint16_t flags )
        {
            struct nlmsghdr * const hdr = getNlmsghdr();

            assert ( hdr != 0 );

            hdr->nlmsg_flags = flags;
        }

        /// @brief Modifies nlmsg_flags value, by adding (bit sum) the new flags
        /// @param [in] flags New flags to add
        inline void addNetlinkFlags ( uint16_t flags )
        {
            struct nlmsghdr * const hdr = getNlmsghdr();

            assert ( hdr != 0 );

            hdr->nlmsg_flags |= flags;
        }

        /// @brief Checks whether this Netlink message has 'multipart' flag set.
        /// @return True if this message has 'multipart' flag set; False otherwise.
        inline bool isMultipart() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            return ( hdr != 0 && ( hdr->nlmsg_flags & NLM_F_MULTI ) == NLM_F_MULTI );
        }

        /// @brief Checks whether this Netlink message is a 'NLMSG_DONE' message.
        /// According to the docs, NLMSG_DONE message is the last part of a multi-part message,
        /// but it is not marked as multi-part itself.
        /// In practice, NLMSG_DONE is also marked as a multi-part message, so we shouldn't relay on either behaviour.
        /// @return True if this message is a NLMSG_DONE; False otherwise.
        inline bool isDone() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            return ( hdr != 0 && hdr->nlmsg_type == NLMSG_DONE );
        }

        /// @brief Checks whether this Netlink message is a 'NLMSG_ERROR' message.
        /// @return True if this message is a NLMSG_ERROR; False otherwise.
        inline bool isError() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            return ( hdr != 0
                     && hdr->nlmsg_type == NLMSG_ERROR
                     && _buf.size() >= NLMSG_HDRLEN + NLMSG_ALIGN ( sizeof ( struct nlmsgerr ) )
                     && NLMSG_OK ( hdr, _buf.size() ) );
        }

        /// @brief Returns a pointer to nlmsgerr payload (if it is, in fact, a NLMSG_ERROR message).
        /// @return Pointer to nlmsgerr payload or 0 if this message is not an error message.
        inline const struct nlmsgerr * getError() const
        {
            const struct nlmsghdr * const hdr = getNlmsghdr();

            if ( hdr != 0
                 && hdr->nlmsg_type == NLMSG_ERROR
                 && _buf.size() >= NLMSG_HDRLEN + NLMSG_ALIGN ( sizeof ( struct nlmsgerr ) )
                 && NLMSG_OK ( hdr, _buf.size() ) )
            {
                return ( const struct nlmsgerr * ) NLMSG_CONST_DATA ( hdr );
            }

            return 0;
        }

        /// @brief Returns the pointer to the internal 'payload' data.
        /// @return Pointer to the internal payload data.
        inline char * getPayloadData()
        {
            // The payload data starts after aligned nlmsghdr
            return _buf.getWritable ( NLMSG_HDRLEN );
        }

        /// @brief Returns the pointer to the internal 'payload' data.
        /// @return Pointer to the internal payload data.
        inline const char * getPayloadData() const
        {
            // The payload data starts after aligned nlmsghdr
            return _buf.get ( NLMSG_HDRLEN );
        }

    protected:
        MemHandle _buf; ///< The buffer in which the message is (will be) stored

        /// @brief Constructor.
        /// Creates a NetlinkMessage wrapper around the data.
        /// @param [in] data The data that contains a single Netlink message.
        NetlinkMessage ( const MemHandle & data );

        /// @brief Constructor.
        /// @param [in] bufSize The size of the buffer to be allocated - the max possible total message size
        /// @param [in] payloadMsgSize The size of the payload message, doesn't need to be aligned
        ///                             (for example it could be  sizeof ( struct ifaddrmsg ).
        ///                             It will be used for setting the initial value of nlmsg_len field.
        /// @param [in] type Message content
        /// @param [in] flags Additional flags
        /// @param [in] seq Sequence number
        /// @param [in] pid Process port ID
        NetlinkMessage (
            size_t bufSize, uint32_t payloadMsgSize,
            uint16_t type, uint16_t flags, uint32_t seq = 0, uint32_t pid = 0 );

        friend class NetlinkCore;

    private:
        bool _isValid; ///< Flag for determining whether the message is valid or not
};

/// @brief The Netlink Message helper class
template<typename MessageType> class NetlinkPayloadMessage: public NetlinkMessage
{
    public:
        /// @brief Constructor
        /// Generates a buffer for the netlink message that consist of the netlink header
        /// configured using parameters provided here, additional message whose type is the
        /// parameter of the template, and additional size for RTA arguments (optional).
        /// @param [in] type Message content
        /// @param [in] flags Additional flags
        /// @param [in] rtaPayloadSize The extra payload size to reserve the memory for.
        ///                             The size of each RTA parameter that will be stored here should be padded
        ///                             using RTA_SPACE, added up with the others, and passed as rtaPayloadSize.
        /// @param [in] seq Sequence number. 0 by default
        /// @param [in] pid Process port ID. 0 by default (message to the kernel)
        NetlinkPayloadMessage (
            uint16_t type, uint16_t flags = 0, size_t rtaPayloadSize = 0,
            uint32_t seq = 0, uint32_t pid = 0 ):
            NetlinkMessage (
                    // The total size of the buffer required is:
                    // (aligned) nlmsghdr + (aligned) MessageType) + rtaPayloadSize (in which each RTA parameter is
                    // padded)
                    NLMSG_HDRLEN + NLMSG_ALIGN ( sizeof ( MessageType ) ) + rtaPayloadSize,
                    sizeof ( MessageType ), type, flags, seq, pid )
        {
            assert ( NLMSG_HDRLEN == NLMSG_ALIGN ( sizeof ( struct nlmsghdr ) ) );
            assert ( !isValid() );

            // Let's make sure our buffer is big enough
            assert ( _buf.size() >= NLMSG_HDRLEN + NLMSG_ALIGN ( sizeof ( MessageType ) ) + rtaPayloadSize );
        }

        /// @brief Returns the pointer to the internal 'payload' message
        /// @return Pointer to the internal message
        inline MessageType * getPayloadMessage()
        {
            MessageType * const ret = ( MessageType * ) getPayloadData();

            assert ( ret != 0 );

            return ret;
        }

        /// @brief Returns the pointer to the internal 'payload' message
        /// @return Pointer to the internal message
        inline const MessageType * getPayloadMessage() const
        {
            const MessageType * const ret = ( const MessageType * ) getPayloadData();

            assert ( ret != 0 );

            return ret;
        }
};
}
