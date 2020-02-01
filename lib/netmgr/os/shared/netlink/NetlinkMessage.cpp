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
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
}

#include "NetlinkMessage.hpp"

using namespace Pravala;

NetlinkMessage::NetlinkMessage ( const MemHandle & data ): _buf ( data ), _isValid ( false )
{
    struct nlmsghdr * const hdr = getNlmsghdr();

    if ( hdr != 0 && hdr->nlmsg_len <= _buf.size() )
    {
        _buf.truncate ( hdr->nlmsg_len );
        _isValid = true;

        return;
    }

    _buf.clear();
}

NetlinkMessage::NetlinkMessage (
        size_t bufSize, uint32_t payloadMsgSize,
        uint16_t type, uint16_t flags, uint32_t seq, uint32_t pid ):
    _buf ( bufSize ), _isValid ( false )
{
    assert ( _buf.size() >= sizeof ( struct nlmsghdr ) );

    _buf.setZero();

    struct nlmsghdr * const hdr = getNlmsghdr();

    assert ( hdr != 0 );

    // The current size of the message is the aligned netlink header size plus the size of the specific message
    hdr->nlmsg_len = NLMSG_LENGTH ( payloadMsgSize );

    hdr->nlmsg_flags = flags;
    hdr->nlmsg_pid = pid;
    hdr->nlmsg_seq = seq;
    hdr->nlmsg_type = type;

    assert ( _buf.size() >= hdr->nlmsg_len );
}

void NetlinkMessage::setSeqNum ( uint32_t seqNum )
{
    struct nlmsghdr * const hdr = getNlmsghdr();

    assert ( hdr != 0 );

    if ( hdr != 0 )
    {
        hdr->nlmsg_seq = seqNum;
    }
}

uint32_t NetlinkMessage::getSeqNum() const
{
    const struct nlmsghdr * const hdr = getNlmsghdr();

    assert ( hdr != 0 );

    return ( hdr != 0 ) ? hdr->nlmsg_seq : 0;
}

uint32_t NetlinkMessage::getPid() const
{
    const struct nlmsghdr * const hdr = getNlmsghdr();

    assert ( hdr != 0 );

    return ( hdr != 0 ) ? hdr->nlmsg_pid : 0;
}

bool NetlinkMessage::appendRta ( int type, const void * data, const uint16_t dataSize )
{
    assert ( !_buf.isEmpty() );

    struct nlmsghdr * const hdr = getNlmsghdr();

    assert ( hdr != 0 );

    // The RTA field length for the dataSize - it includes the RTA-Aligned RTA header + dataSize (not aligned)
    const uint16_t length = RTA_LENGTH ( dataSize );

    // The total, new message size is the current size (aligned) + the length of the new RTA field
    const uint32_t newSize = NLMSG_ALIGN ( hdr->nlmsg_len ) + length;

    if ( newSize > _buf.size() )
    {
        assert ( false );

        return false;
    }

    // The new RTA parameter will start after the current total size of the message (aligned):
    struct rtattr * const rta = ( struct rtattr * ) ( ( ( char * ) hdr ) + NLMSG_ALIGN ( hdr->nlmsg_len ) );

    rta->rta_type = type;
    rta->rta_len = length;

    memcpy ( RTA_DATA ( rta ), data, dataSize );

    hdr->nlmsg_len = newSize;

    return true;
}
