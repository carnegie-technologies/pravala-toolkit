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

#include "basic/Math.hpp"
#include "basic/Random.hpp"
#include "net/TcpPacket.hpp"
#include "socket/PacketDataStore.hpp"

#include "TcpTerminator.hpp"

// The time failed TCP flows stick around to respond to any IP packets (in milliseconds).
#define LINGER_TIME    30000

// The time TCP flows that had their FIN packets ACKed stick around (in milliseconds).
// This means the connection was torn down gracefully. We could stick around longer,
// but if the client tries to reuse the same source port for a new connection
// to the exact same destination, we should remove this very quickly.
#define ACKED_FIN_LINGER_TIME    1000

// The time to use for TCP retransmissions (in milliseconds).
#define RETRANS_TIME             500

// The MSS to use if not provided
#define DEFAULT_MSS              1300

// The number of bytes of IPv4/TCP overhead that needs to fit in the MTU used by the tunnel interface.
// The MSS is obtained by subtracting this value from interface's MTU.
#define TCP_IPv4_MTU_OVERHEAD    40

// The number of bytes of IPv6/TCP overhead that needs to fit in the MTU used by the tunnel interface.
// The MSS is obtained by subtracting this value from interface's MTU.
#define TCP_IPv6_MTU_OVERHEAD    60

// The minimum allowed MSS; 576 minimum IPv4 MTU minus the overhead.
// For IPv6 the minimum MTU is higher, but MSS could still be smaller, so let's not worry about it here.
#define MIN_MSS                  ( 576 - TCP_IPv4_MTU_OVERHEAD )

using namespace Pravala;

TextLogLimited TcpTerminator::_log ( "tcp_terminator" );

TcpTerminator::TcpTerminator ( const FlowDesc & flowDesc, uint16_t mtu ):
    IpFlow ( flowDesc ),
    ClientAddr ( ( flowDesc.common.type == 4 )
            ? IpAddress ( flowDesc.v4.clientAddr )
            : IpAddress ( flowDesc.v6.clientAddr ) ),
    ServerAddr ( ( flowDesc.common.type == 4 )
            ? IpAddress ( flowDesc.v4.serverAddr )
            : IpAddress ( flowDesc.v6.serverAddr ) ),
    ClientPort ( ntohs ( flowDesc.common.u.port.client ) ),
    ServerPort ( ntohs ( flowDesc.common.u.port.server ) ),
    _tcpTimer ( *this ),
    _tcpState ( TcpInit ),
    _unsentBufSize ( 0 ),
    _rcvBufSize ( 0 ),
    _maxSendBufSize ( 0 ),
    _nextRcvSeq ( 0 ),
    _sendDataSeq ( ( uint32_t ) Random::rand() ),
    _mtu ( mtu ),
    _mss ( 0 ),
    _clientWScale ( 0 ),
    _tcpFlags ( 0 )
{
    assert ( flowDesc.common.type == 4 || flowDesc.common.type == 6 );
    assert ( flowDesc.common.heProto == TcpPacket::ProtoNumber );

    LOG ( L_DEBUG, getLogId() << ": New TCP terminator created" );
}

TcpTerminator::~TcpTerminator()
{
}

String TcpTerminator::getLogId() const
{
    return String ( "[%1:%2-%3:%4,%5]" )
           .arg ( ClientAddr.toString() )
           .arg ( ClientPort )
           .arg ( ServerAddr.toString() )
           .arg ( ServerPort )
           .arg ( getTcpStateName() );
}

void TcpTerminator::flowRemoved()
{
    // This is some cleanup, or a shutdown.

    LOG ( L_DEBUG2, getLogId() << ": TCP terminator removed" );

    delete this;
}

void TcpTerminator::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer == &_tcpTimer );

    if ( _tcpState == TcpBroken || _tcpState == TcpClosed )
    {
        LOG ( L_DEBUG, getLogId() << ": TCP Timer expired; Removing the terminator" );

        delete this;
        return;
    }

    if ( _tcpState == TcpConnected )
    {
        if ( !_sentBuffer.isEmpty() )
        {
            LOG ( L_DEBUG2, getLogId() << ": TCP Timer expired; Re-transmitting first packet from the queue" );

            resendFirst();
            return;
        }
        else if ( shouldSendFin() )
        {
            LOG ( L_DEBUG2, getLogId() << ": TCP Timer expired; Re-transmitting empty FIN packet" );

            sendAck();
            return;
        }
    }

    // TODO - Anything else?
}

void TcpTerminator::setBroken()
{
    if ( _tcpState == TcpBroken )
        return;

    LOG ( L_DEBUG2, getLogId() << ": Entering 'broken' state; Scheduling the timer to remove this object in "
          << LINGER_TIME << " ms" );

    _rcvBufSize = 0;
    _rcvBuffer.clear();

    _unsentBufSize = 0;
    _unsentBuffer.clear();
    _sentBuffer.clear();

    _tcpState = TcpBroken;
    _tcpTimer.start ( LINGER_TIME );
}

void TcpTerminator::close ( ERRCODE reason )
{
    ( void ) reason;

    _rcvBufSize = 0;
    _rcvBuffer.clear();

    if ( _tcpState == TcpConnected )
    {
        LOG_ERR ( L_DEBUG2, reason, getLogId() << ": TCP handler was closed; Sending FIN packet" );

        setTcpFlag ( TcpFlagSentFin );
        sendAck();
        return;
    }

    LOG_ERR ( L_WARN, reason, getLogId() << ": TCP handler was closed; Entering 'TCP broken' state" );

    setBroken();
}

void TcpTerminator::appendData ( MemHandle & data )
{
    if ( _tcpState != TcpConnected )
    {
        LOG ( L_WARN, getLogId() << ": Could not accept data; We are not in 'TCP connected' state" );

        setTcpFlag ( TcpFlagSendBlocked );
        return;
    }

    if ( hasTcpFlag ( TcpFlagSentFin ) )
    {
        // This means that the flow told us it's shutting down.
        // We cannot accept any more data!
        LOG ( L_WARN, getLogId() << ": Could not accept any more data, since TCP FIN packet has already been sent" );
        return;
    }

    if ( data.isEmpty() )
    {
        LOG ( L_ERROR, getLogId() << ": Not accepting empty data segment" );
        return;
    }

    const size_t sendBufSize = getSendBufSize();

    if ( sendBufSize >= _maxSendBufSize )
    {
        LOG ( L_DEBUG2, getLogId() << ": Could not accept more data; Buffer size: "
              << sendBufSize << "; MaxBufSize: " << _maxSendBufSize );

        setTcpFlag ( TcpFlagSendBlocked );
        return;
    }

    assert ( sendBufSize < _maxSendBufSize );

    const size_t toSend = min<size_t> ( data.size(), _maxSendBufSize - sendBufSize );

    assert ( toSend > 0 );

    _unsentBuffer.append ( ( toSend < data.size() ) ? data.getHandle ( 0, toSend ) : data );

    assert ( !_unsentBuffer.isEmpty() );

    const size_t sizeAccepted = _unsentBuffer.last().size();

    LOG ( L_DEBUG4, getLogId()
          << ": Accepted " << sizeAccepted << " byte(s) of " << data.size()
          << " data byte(s) for sending; Buffer size: "
          << sendBufSize << " -> " << ( sendBufSize + sizeAccepted ) << " / " << _maxSendBufSize );

    _unsentBufSize += sizeAccepted;
    data.consume ( sizeAccepted );

    // We are keeping the data for later. Let's make sure that we are not wasting memory!
    // This may be somewhat inefficient if we accept the data partially.
    // If that happens, we will split that data here, and unnecessarily optimize those parts.
    // TODO: Maybe we should accept all the data, or none of it?
    PacketDataStore::optimizePacket ( _unsentBuffer.last() );

    if ( !data.isEmpty() )
    {
        // We didn't consume all of the data we got.
        // This means that we got blocked on the receive window.
        setTcpFlag ( TcpFlagSendBlocked );
    }

    if ( !hasTcpFlag ( TcpFlagEoLSubscribed ) )
    {
        // TODO: Maybe we need to do something differently if we are in 'retransmit' mode?
        //       We may want to postpone sending until the retransmission succeeds...
        //       On the other hand this is the data that is still within receiver's advertised receive window,
        //       so it is a fair game! And we don't do congestion control anyway...

        setTcpFlag ( TcpFlagEoLSubscribed );
        EventManager::loopEndSubscribe ( this );
    }

    return;
}

void TcpTerminator::receiveLoopEndEvent()
{
    clearTcpFlag ( TcpFlagEoLSubscribed );
    sendUnsent();
}

void TcpTerminator::resendFirst()
{
    // Let's send a single packet using the data at the beginning of the buffer.
    // We can send up to _mss bytes, even if they span over multiple chunks.

    MemVector payload;

    assert ( _mss > 0 );

    for ( size_t i = 0; i < _sentBuffer.getNumChunks() && payload.getDataSize() < _mss; ++i )
    {
        payload.append ( _sentBuffer.getChunk ( i ) );
    }

    payload.truncate ( _mss );

    if ( payload.isEmpty() )
    {
        return;
    }

    // This is the first packet in the buffer, so we use _sendDataSeq as the sequence number.

    const TcpPacket dataPacket (
            ServerAddr, ServerPort,
            ClientAddr, ClientPort,
            TcpPacket::FlagAck,
            _sendDataSeq,
            getAckToSend(),
            getWinSizeToAdvertise(),
            payload );

    const ERRCODE eCode = sendPacket ( dataPacket );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, getLogId()
                  << ": Error sending TCP packet [" << dataPacket << "] over the tunnel interface" );
    }
    else
    {
        // We just sent a data packet. If there was a pending ACK needed, it no longer needs to be sent:
        clearTcpFlag ( TcpFlagNeedsAck );

        LOG ( L_DEBUG2, getLogId() << ": Re-transmitted TCP packet: " << dataPacket );
    }

    _tcpTimer.start ( RETRANS_TIME );
}

void TcpTerminator::sendUnsent()
{
    if ( _unsentBuffer.isEmpty() )
        return;

    MemVector payload;

    size_t idx = 0;
    size_t totalSent = 0;

    while ( idx < _unsentBuffer.size() )
    {
        const MemHandle & mh ( _unsentBuffer.at ( idx ) );

        assert ( payload.getDataSize() < _mss );

        const size_t maxSend = _mss - payload.getDataSize();
        bool partialSend = false;

        if ( mh.size() <= maxSend )
        {
            // We can include the entire segment!
            payload.append ( mh );
        }
        else
        {
            // The next segment won't fully fit in the packet...
            payload.append ( mh.getHandle ( 0, maxSend ) );

            partialSend = true;
        }

        if ( payload.getDataSize() < _mss && idx + 1 < _unsentBuffer.size() )
        {
            // The packet is not full yet and there are more elements in the unsent buffer!

            assert ( !partialSend );

            ++idx;
            continue;
        }

        // The packet is full, or this is the last element in the _unsentBuffer.
        // Let's try to send the packet!
        // This is the first packet AFTER current _sentBuffer, so we use _sendDataSeq + sent-buf-size as the SEQ number.

        const TcpPacket dataPacket (
                ServerAddr, ServerPort,
                ClientAddr, ClientPort,
                TcpPacket::FlagAck,
                _sendDataSeq + _sentBuffer.getDataSize(),
                getAckToSend(),
                getWinSizeToAdvertise(),
                payload );

        const ERRCODE eCode = sendPacket ( dataPacket );

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_ERROR, eCode, getLogId()
                      << ": Error sending TCP packet [" << dataPacket << "] over the tunnel interface" );

            payload.clear();
            break;
        }

        LOG ( L_DEBUG2, getLogId() << ": Successfully sent TCP packet: " << dataPacket );

        // Move the data to 'sent' buffer:
        _sentBuffer.append ( payload );

        assert ( payload.getDataSize() <= _unsentBufSize );

        // Those bytes are no longer in unsent buffer:
        _unsentBufSize -= payload.getDataSize();
        totalSent += payload.getDataSize();

        payload.clear();

        // Let's update the unsent buffer.
        // We need to fully remove all elements up to, but not including, idx.
        for ( size_t i = 0; i < idx; ++i )
        {
            _unsentBuffer.removeFirst();
        }

        // Now we either can remove current segment (if it was fully sent), or consume its beginning.
        // Note that we cannot use 'idx' any more, this is now the first element!

        assert ( !_unsentBuffer.isEmpty() );

        if ( partialSend )
        {
            assert ( _unsentBuffer.first().size() > maxSend );

            _unsentBuffer.first().consume ( maxSend );
        }
        else
        {
            // It was fully sent. Let's remove this one as well!
            _unsentBuffer.removeFirst();
        }

        // In both cases we want to start from (the new) beginning of the list.
        // If we had a partial send, the first packet will be the one we partially sent,
        // just with some data removed from the beginning.

        idx = 0;
    }

    if ( totalSent > 0 )
    {
        // We sent something!

        // If there was a pending ACK needed, it no longer needs to be sent:
        clearTcpFlag ( TcpFlagNeedsAck );
    }

    // We want to restart it even if it's running.
    // It will be used to resend the data once it stops flowing.
    // Otherwise (if something gets lost) we should get a duplicate ACK that will trigger re-transmission.
    // Also, if we didn't actually send anything, it would be because the tunnel didn't accept it.
    // From the perspective of this object, it is similar behaviour to having the packet lost.
    // So we still want to try re-transmit it after a delay.
    _tcpTimer.start ( RETRANS_TIME );
}

ERRCODE TcpTerminator::consumeReceivedData()
{
    const bool zeroRcvWindow = ( getWinSizeToAdvertise() < 1 );

    while ( !_rcvBuffer.isEmpty() )
    {
        MemHandle & data = _rcvBuffer.first();
        const size_t dataSize = data.size();

        assert ( dataSize > 0 );
        assert ( dataSize <= _rcvBufSize );

        const ERRCODE eCode = receiveData ( data );

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_WARN, eCode, getLogId() << ": Error receiving the data; Closing the TCP connection" );

            close ( eCode );
            return eCode;
        }

        assert ( data.size() <= dataSize );
        assert ( ( dataSize - data.size() ) <= _rcvBufSize );

        _rcvBufSize -= ( dataSize - data.size() );

        if ( !data.isEmpty() )
        {
            // When the data is not fully consumed, it means that we can't accept more right now.
            // The implementation should figure out when to call consumeReceivedData() again,
            // once more data can be accepted.
            //
            // Since we passed a reference to original MemHandle in the buffer,
            // if anything was written it was already consumed.
            break;
        }

        assert ( data.isEmpty() );

        // The entire segment was consumed.
        _rcvBuffer.removeFirst();
    }

    if ( zeroRcvWindow && getWinSizeToAdvertise() > 0 && _tcpState == TcpConnected )
    {
        // If we previously advertised 'zero window' and we now can accept more data,
        // we should tell the TCP client about that. If we still can't, we don't say anything.
        // If they care they will send us probes, and we do respond to those (in handleAckPacket()).

        LOG ( L_DEBUG2, getLogId()
              << ": Sending an ACK after recovering some space in (previously full) receive buffer" );

        sendAck();
    }

    return Error::Success;
}

void TcpTerminator::sendResetResponse ( const IpPacket & toPacket )
{
    const TcpPacket resp = TcpPacket::generateResetResponse ( toPacket );

    if ( !resp.isValid() )
    {
        // It is possible that the response is invalid (empty).
        // This could happen if the original packet was a reset packet itself
        // (we should not respond to reset packets).

        LOG ( L_DEBUG2, getLogId() << ": NOT sending a RST packet in response to [" << toPacket << "]" );
        return;
    }

    LOG ( L_DEBUG2, getLogId() << ": Sending RST packet [" << resp << "] in response to [" << toPacket << "]" );

    sendPacket ( resp );
}

ERRCODE TcpTerminator::packetReceived ( IpPacket & ipPacket, int32_t userData, void * /*userPtr*/ )
{
    if ( userData != DefaultDescType )
    {
        LOG ( L_WARN, getLogId() << ": Received an IP packet in the wrong direction ("
              << userData << ", expected " << DefaultDescType << "): " << ipPacket << "; Dropping" );

        return Error::InvalidParameter;
    }

    LOG ( L_DEBUG4, getLogId() << ": Received an IP packet: " << ipPacket );

    const TcpPacket::Header * const tcpHdr = ipPacket.getProtoHeader<TcpPacket>();

    if ( !tcpHdr )
        return Error::InvalidParameter;

    if ( _tcpState == TcpBroken )
    {
        // We are "broken" in some way. We only respond with reset packets.
        sendResetResponse ( ipPacket );
        return Error::Success;
    }

    if ( tcpHdr->isRST() )
    {
        LOG ( L_DEBUG2, getLogId() << ": Received RST packet [" << ipPacket << "]" );

        setBroken();
        return Error::Success;
    }

    if ( tcpHdr->isSYN() )
    {
        return handleSynPacket ( ipPacket );
    }

    if ( !tcpHdr->isACK() )
    {
        LOG ( L_WARN, getLogId() << ": Incoming TCP packet has no ACK set; Ignoring: " << ipPacket );
        return Error::Success;
    }

    if ( _tcpState == TcpInit )
    {
        // We are still initializing. This ACK packet may be acknowledging our SYN-ACK!

        if ( tcpHdr->getAckNum() == _sendDataSeq )
        {
            LOG ( L_DEBUG2, getLogId() << ": Our SYN-ACK packet has been acknowledged by [" << ipPacket
                  << "]; Switching to '" << getTcpStateName ( TcpConnected ) << "' state" );

            _tcpState = TcpConnected;
            return Error::Success;
        }

        LOG ( L_WARN, getLogId() << ": Received unexpected ACK packet: [" << ipPacket
              << "]; We are still waiting to have our SYN-ACK acknowledged (with ACK " << _sendDataSeq << ")" );

        return Error::Success;
    }

    LOG ( L_DEBUG4, getLogId() << ": Handling received TCP packet: [" << ipPacket << "]" );

    if ( !handleDataPacket ( ipPacket ) )
    {
        // Something is wrong with the packet... Let's not process it further!
        return Error::Success;
    }

    // It's possible that we now have in-order data. Let's try to deliver it.
    consumeReceivedData();

    if ( hasTcpFlag ( TcpFlagRcvdFin ) && !hasTcpFlag ( TcpFlagSentFin ) && _rcvBuffer.isEmpty() )
    {
        // This means that we consumed all the data received, and we are not getting more data (we got FIN).
        // We finished receiving the data.

        LOG ( L_DEBUG2, getLogId() << ": Consumed all data up to TCP FIN packet; Receiving is done" );

        setTcpFlag ( TcpFlagSentFin | TcpFlagNeedsAck );

        receivingCompleted();
    }

    // Now let's react to the ACK in the packet.
    handleAckPacket ( ipPacket );

    if ( hasTcpFlag ( TcpFlagSendBlocked ) && getSendBufSize() < _maxSendBufSize )
    {
        // Sending was previously blocked by the "max buf size" restriction.
        // Maybe something got removed from the buffer, or maybe the receive window got updated.
        // Either way, we are able to send data again.
        clearTcpFlag ( TcpFlagSendBlocked );
        sendingUnblocked();
    }

    if ( hasTcpFlag ( TcpFlagNeedsAck ) )
    {
        // Something in the meantime required an ACK, but no packets were sent (which would have cleared the flag).
        // Let's send an empty ACK!
        sendAck();
    }

    return Error::Success;
}

bool TcpTerminator::handleDataPacket ( IpPacket & ipPacket )
{
    const TcpPacket::Header * const tcpHdr = ipPacket.getProtoHeader<TcpPacket>();

    assert ( tcpHdr != 0 );
    assert ( tcpHdr->isACK() );

    if ( !tcpHdr || !tcpHdr->isACK() )
    {
        return false;
    }

    const uint32_t seqNum = tcpHdr->getSeqNum();
    const int32_t seqDiff = compareSeq ( _nextRcvSeq, seqNum );

    if ( seqDiff < 0 )
    {
        // This packet is not at the end of our buffer, but further. Something before it got lost?
        // It is also possible that this is the ACK to our FIN, in that case it should be one byte past the buffer.

        if ( hasTcpFlag ( TcpFlagSentFin )
             && seqDiff == -1
             && ipPacket.getProtoPayloadSize<TcpPacket>() == 0 )
        {
            LOG ( L_DEBUG3, getLogId() << ": Potential ACK to FIN received: " << ipPacket );

            // Nothing to do here, handleAckPacket() will handle FIN/ACK.
            return true;
        }

        LOG ( L_DEBUG3, getLogId() << ": Received out-of-order packet: [" << ipPacket
              << "]; Next sequence number expected: " << _nextRcvSeq );

        // We need to send a duplicate ACK.
        // That kind of ACK contains no data, so there is no need to postpone it:
        sendAck();
        return true;
    }

    MemVector tmpPayload;
    MemHandle tcpPayload;

    // NOTE: These will succeed even if the payload is empty; That's what we want (it could be an ACK packet w/o data).
    if ( !ipPacket.getProtoPayload<TcpPacket> ( tmpPayload ) || !tmpPayload.storeContinuous ( tcpPayload ) )
    {
        LOG_LIM ( L_ERROR, getLogId() << ": Could not get the payload from TCP data packet [" << ipPacket << "]" );
        return false;
    }

    // We may need to handle packets in a better way.
    // Right now those packets should only be coming from the tunnel interface, so they should only use
    // a single, big chunk. If they don't, the data will be copied...

    if ( tmpPayload.getNumChunks() > 1 )
    {
        LOG_LIM ( L_WARN, getLogId() << ": Multi-part TCP payload received [" << ipPacket << "]" );
    }

    tmpPayload.clear();

    if ( ( uint32_t ) seqDiff >= tcpPayload.size() )
    {
        // The first byte of this packet is before the end of our receive buffer,
        // by more (or the same) as this packet's size.
        // This means that we already have all of the data in this packet.
        // However, if this is a FIN packet, we need to mark that!

        if ( tcpHdr->isFIN() )
        {
            if ( ( uint32_t ) seqDiff != tcpPayload.size() )
            {
                // This is a FIN packet that ends before previously received data segment. This is very wrong!
                LOG ( L_WARN, getLogId() << ": Received unexpected TCP FIN packet: [" << ipPacket
                      << "]; Next sequence number expected: " << _nextRcvSeq );

                sendResetResponse ( ipPacket );
                setBroken();
                return false;
            }

            // In-order FIN packet. Let's set the flag:
            LOG ( L_DEBUG2, getLogId() << ": Received an in-order TCP FIN packet: [" << ipPacket
                  << "]; Next sequence number expected: " << _nextRcvSeq );

            setTcpFlag ( TcpFlagRcvdFin | TcpFlagNeedsAck );
            return true;
        }

        if ( tcpPayload.size() < 1 )
        {
            return true;
        }

        LOG ( L_DEBUG2, getLogId() << ": Received packet with the data we already have: [" << ipPacket
              << "]; Next sequence number expected: " << _nextRcvSeq );

        // We need to send a duplicate ACK.
        // That kind of ACK contains no data, so there is no need to postpone it:
        sendAck();
        return true;
    }

    // We need at least some of this packet's data (not necessarily all of it).

    assert ( tcpPayload.size() > 0 );
    assert ( ( uint32_t ) seqDiff < tcpPayload.size() );

    if ( hasTcpFlag ( TcpFlagRcvdFin ) )
    {
        // We have already received an in-order TCP FIN packet!
        // This means that the client told us it's shutting down the connection.
        // We cannot accept any more data!
        LOG ( L_WARN, getLogId()
              << ": Could not accept any more TCP data, since TCP FIN packet has already been received" );

        sendResetResponse ( ipPacket );
        setBroken();
        return false;
    }

    if ( seqDiff > 0 )
    {
        // We need only some of this packet's data.
        LOG ( L_DEBUG, getLogId() << ": Received packet with data partially past the end of our buffer: ["
              << ipPacket << "]; Next sequence number expected: " << _nextRcvSeq );

        // seqDiff is the number of bytes we do NOT need.
        tcpPayload.consume ( seqDiff );
    }
    else
    {
        LOG ( L_DEBUG4, getLogId() << ": Received packet with data at the end of our buffer: [" << ipPacket
              << "]; Next sequence number expected: " << _nextRcvSeq );
    }

    if ( tcpHdr->isFIN() )
    {
        // In-order FIN packet. Let's set the flag:
        LOG ( L_DEBUG2, getLogId() << ": Received an in-order TCP FIN packet: [" << ipPacket
              << "]; Next sequence number expected: " << _nextRcvSeq );

        setTcpFlag ( TcpFlagRcvdFin | TcpFlagNeedsAck );
    }
    else
    {
        // Regular data packet, we will need to ACK it.
        // We don't need to do that right away, so let's just set a flag and it will be sent later,
        // unless something sends a packet before that.
        setTcpFlag ( TcpFlagNeedsAck );
    }

    assert ( !tcpPayload.isEmpty() );

    if ( _tcpState != TcpConnected || hasTcpFlag ( TcpFlagSentFin ) )
    {
        // We are not able (or willing) to accept new data.
        // We should respond with 'reset', but not before our FIN packet is acknowledged.
        if ( hasTcpFlag ( TcpFlagSentFin ) && !hasTcpFlag ( TcpFlagRcvdFinAck ) )
        {
            LOG ( L_WARN, getLogId() << ": Could not accept any more TCP data - we have already sent FIN; "
                  "Waiting for our FIN to be acknowledged" );

            return true;
        }

        LOG ( L_WARN, getLogId() << ": Could not accept any more TCP data - we are not in 'connected' state; "
              "Responding with reset to received packet: [" << ipPacket << "]" );

        sendResetResponse ( ipPacket );
        setBroken();
        return false;
    }

    // We are about to buffer the data received. We don't want to waste the memory, so we need to "optimize" it.
    PacketDataStore::optimizePacket ( tcpPayload );

    // TODO: We may want to enforce max receive buffer size here. For now we expect the sender to behave.

    _rcvBuffer.append ( tcpPayload );
    _rcvBufSize += tcpPayload.size();
    _nextRcvSeq += tcpPayload.size();

    return true;
}

void TcpTerminator::handleAckPacket ( IpPacket & ipPacket )
{
    const TcpPacket::Header * const tcpHdr = ipPacket.getProtoHeader<TcpPacket>();

    assert ( tcpHdr != 0 );
    assert ( tcpHdr->isACK() );

    if ( !tcpHdr || !tcpHdr->isACK() )
    {
        return;
    }

    const uint32_t ackNum = tcpHdr->getAckNum();
    const int32_t seqDiff = compareSeq ( ackNum, _sendDataSeq );

    if ( seqDiff < 0 )
    {
        // The ACK is before the first byte in the buffer.
        // Maybe the ACK got reordered? We should ignore it.
        LOG ( L_WARN, getLogId() << ": Received invalid ACK packet [" << ipPacket
              << "]; Our send buffer contains sequence numbers: ["
              << _sendDataSeq << "-" << ( _sendDataSeq + _sentBuffer.getDataSize() ) << "); Ignoring" );

        return;
    }

    // seqDiff is positive, and is the number of bytes at the beginning of _sentBuffer that are being acknowledged.

    assert ( seqDiff >= 0 );

    if ( ( size_t ) seqDiff > _sentBuffer.getDataSize() )
    {
        // This ACK is after the last byte stored in the buffer, which is wrong.
        // One exception is if we sent a FIN packet.
        // In that case seqDiff should be exactly one byte past the buffer.

        if ( !hasTcpFlag ( TcpFlagSentFin ) || ( size_t ) seqDiff > _sentBuffer.getDataSize() + 1 )
        {
            LOG ( L_WARN, getLogId() << ": Received invalid ACK packet [" << ipPacket
                  << "]; Our send buffer contains sequence numbers: ["
                  << _sendDataSeq << "-" << ( _sendDataSeq + _sentBuffer.getDataSize() ) << "); Ignoring" );
            return;
        }

        if ( !hasTcpFlag ( TcpFlagRcvdFinAck ) )
        {
            LOG ( L_DEBUG2, getLogId() << ": Incoming packet acknowledged our FIN: [" << ipPacket
                  << "]; Entering 'closed' state and setting the linger timer ("
                  << ACKED_FIN_LINGER_TIME << " ms)" );

            setTcpFlag ( TcpFlagRcvdFinAck );
            _tcpState = TcpClosed;
            _tcpTimer.start ( ACKED_FIN_LINGER_TIME );
        }

        if ( hasTcpFlag ( TcpFlagRcvdFin ) && !tcpHdr->isFIN() )
        {
            // We previously received a FIN packet from the other side.
            // Now we are getting an acknowledgement of our FIN, and without FIN flag.
            // We don't need to respond to this packet!

            LOG ( L_DEBUG2, getLogId() << ": The final ACK detected: [" << ipPacket << "]; Not responding" );
            clearTcpFlag ( TcpFlagNeedsAck );
        }

        if ( !_sentBuffer.isEmpty() )
        {
            // If that ACK was past the end of send buffer, it means that everything has been delivered!
            LOG ( L_DEBUG, getLogId() << ": Our FIN packet got acknowledged with "
                  << _sentBuffer.getDataSize() << " bytes in our send buffer; Clearing the buffer" );

            _sentBuffer.clear();
        }

        return;
    }

    // This is a valid ACK.
    // First, let's update the max buffer size:
    const uint32_t newBufSize = adjustedMaxSendBufSize ( tcpHdr->getWindow() << _clientWScale );

    if ( newBufSize != _maxSendBufSize )
    {
        // It changed!
        // TODO: In case it's a zero window, we may need to handle this better and start probing...

        LOG ( ( newBufSize < 1 ) ? L_WARN : L_DEBUG2, getLogId()
              << ": Receive window changed: " << _maxSendBufSize << " -> " << newBufSize );

        _maxSendBufSize = newBufSize;
    }

    if ( _sentBuffer.isEmpty() )
    {
        // If the send buffer is empty, this is a keep-alive, or a zero window probe.
        // Let's send an empty ACK:

        LOG ( L_DEBUG2, getLogId() << ": Received an ACK while our send buffer is empty: ["
              << ipPacket << "]; Next send sequence number: " << _sendDataSeq
              << "; We will respond with an empty ACK packet" );

        // At the very least we will send an empty ACK.
        setTcpFlag ( TcpFlagNeedsAck );
        return;
    }

    if ( seqDiff < 1 )
    {
        // This ACK describes the first byte in the buffer (it acknowledges no data).

        assert ( seqDiff == 0 );

        if ( ipPacket.getProtoPayloadSize<TcpPacket>() > 0 )
        {
            // This packet contains data, it means that it was sent by the remote side
            // before they received more data that could be acknowledged.
            // There is nothing else to do here (but the data will be added to receive buffer later):
            return;
        }

        // This is probably a duplicate ACK, meaning that the first packet was lost.
        // Let's send it again!

        // TODO: This may need to be improved.
        //       For instance, we could count duplicate ACKs and only react when that counter reaches
        //       specific value... This could affect the re-transmit timer.

        LOG ( L_DEBUG2, getLogId() << ": Duplicate ACK received: [" << ipPacket
              << "]; Re-transmitting the first packet from the buffer (with sequence number "
              << _sendDataSeq << ")" );

        resendFirst();
        return;
    }

    // The (positive) difference between the sequence number of the first byte in the buffer,
    // and received ACK number is the number of bytes we can remove from the buffer.

    assert ( ( size_t ) seqDiff <= _sentBuffer.getDataSize() );

    LOG ( L_DEBUG4, getLogId()
          << ": Removing " << seqDiff << " acknowledged bytes from send buffer; ACK packet: " << ipPacket );

    _sendDataSeq += seqDiff;
    _sentBuffer.consume ( seqDiff );

    if ( _tcpState != TcpConnected )
    {
        // We only touch the timer in 'connected' state.
        // If we are in 'closed' or 'reset' states, that timer is used for final cleanup.

        return;
    }

    if ( !_sentBuffer.isEmpty() )
    {
        // We still have some data in _sentBuffer, but since something got acknowledged,
        // we want to give the receiver more time to react to other packets.
        _tcpTimer.start ( RETRANS_TIME );
    }
    else if ( !_unsentBuffer.isEmpty() )
    {
        // 'sent' buffer is empty, but there is something in 'unsent' one.
        // This could mean the tunnel didn't accept it earlier and we should try to send it now.
        // This should also start the timer.
        sendUnsent();
    }
    else
    {
        // There is nothing in unsent and nothing in sent buffers, no need to have re-transmit timer running.
        _tcpTimer.stop();
    }
}

uint16_t TcpTerminator::adjustMss ( uint16_t mss ) const
{
    return mss;
}

ERRCODE TcpTerminator::handleSynPacket ( IpPacket & ipPacket )
{
    const TcpPacket::Header * const tcpHdr = ipPacket.getProtoHeader<TcpPacket>();

    assert ( tcpHdr != 0 );
    assert ( tcpHdr->isSYN() );

    if ( !tcpHdr || !tcpHdr->isSYN() )
    {
        return Error::InvalidParameter;
    }

    if ( _tcpState == TcpConnected )
    {
        // We are already 'connected', which means the client already acknowledged our SYN-ACK.
        // If that happens, we simply ignore SYN packets (they may have been sent before and got reordered?).
        LOG ( L_DEBUG3, getLogId() << ": Received SYN packet [" << ipPacket << "] while connected; Ignoring" );
        return Error::Success;
    }

    // Otherwise we should be in 'init' state. If we are not, something is wrong...

    if ( _tcpState != TcpInit )
    {
        // We are in the wrong state!
        LOG ( L_WARN, getLogId()
              << ": Received SYN packet in invalid state; Entering 'broken' state and ignoring received SYN packet: "
              << ipPacket );

        setBroken();
        return Error::WrongState;
    }

    if ( hasTcpFlag ( TcpFlagSentSynAck ) )
    {
        // We already sent an SYN-ACK packet.
        // It means that our SYN-ACK was lost, or that this SYN was sent before the TCP sender received SYN-ACK.
        // Let's just send SYN-ACK again.
        sendSynAck();
        return Error::Success;
    }

    if ( hasTcpFlag ( TcpFlagSynAccepted ) )
    {
        // We already accepted a SYN packet, but we may be still initializing.
        // We just ignore future SYN packets...
        LOG ( L_DEBUG3, getLogId() << ": Received duplicated SYN packet [" << ipPacket << "]; Ignoring" );
        return Error::Success;
    }

    // This is the very first packet.
    // We need to initialize the initial data of this TCP session:

    uint16_t clientMss = 0;

    tcpHdr->getOptMss ( clientMss );
    tcpHdr->getOptWindowScale ( _clientWScale );

    assert ( getSendBufSize() == 0 );
    assert ( _rcvBufSize == 0 );

    if ( clientMss < 1 )
    {
        LOG ( L_WARN, getLogId() << ": MSS not provided; Using " << DEFAULT_MSS );

        clientMss = DEFAULT_MSS;
    }
    else if ( clientMss < MIN_MSS )
    {
        LOG ( L_WARN, getLogId() << ": MSS received (" << clientMss << ") is too small; Using " << MIN_MSS );

        clientMss = MIN_MSS;
    }

    const uint16_t overhead = ClientAddr.isIPv4() ? TCP_IPv4_MTU_OVERHEAD : TCP_IPv6_MTU_OVERHEAD;

    if ( _mtu > 0 && clientMss + overhead > _mtu )
    {
        // We have specific MTU configured, and received (or corrected) client MSS is too large (weird).
        // We need to use a smaller value.

        if ( _mtu <= overhead )
        {
            // This really shouldn't happen!
            LOG ( L_FATAL_ERROR, getLogId() << ": Configured MTU (" << _mtu << ") is lower than the overhead ("
                  << overhead << "); Ignoring configured's MTU" );
        }
        else
        {
            LOG ( L_WARN, getLogId() << ": Client MSS (" << clientMss
                  << ") is too large for configured MTU (" << _mtu
                  << "); Using value: " << ( _mtu - overhead ) );

            clientMss = _mtu - overhead;
        }
    }

    assert ( clientMss > 0 );

    if ( _mss < 1 )
    {
        // We only do this is _mss is 0, meaning it hasn't been configured yet.
        // Otherwise, it means that we already figured it out and this is not the first SYN packet received.
        // We must NOT change the MSS value that was previously advertised.
        // Once MSS is chosen it must stay at the same value.
        _mss = adjustMss ( clientMss );
    }

    if ( clientMss == _mss )
    {
        LOG ( L_DEBUG2, getLogId() << ": Using MSS: " << _mss );
    }
    else
    {
        LOG ( L_DEBUG2, getLogId() << ": Using adjusted MSS: " << clientMss << " -> " << _mss );
    }

    assert ( _mss > 0 );

    _nextRcvSeq = tcpHdr->getSeqNum() + 1;
    _maxSendBufSize = adjustedMaxSendBufSize ( tcpHdr->getWindow() << _clientWScale );

    if ( initializeReceiver ( ipPacket ) )
    {
        LOG ( L_DEBUG2, getLogId() << ": SYN packet accepted [" << ipPacket
              << "]; Initializing receiver for " << ServerAddr << ":" << ServerPort );

        setTcpFlag ( TcpFlagSynAccepted );
    }

    return Error::Success;
}

void TcpTerminator::sendSynAck()
{
    if ( _tcpState != TcpInit )
    {
        // Sending SYN-ACK packets only makes sense in TcpInit state.
        // If there is reordering, we may get a SYN packet after receiving ACK to our own SYN-ACK.
        LOG ( L_WARN, getLogId() << ": We are not in 'TcpInit' state; Not sending SYN-ACK packet" );
        return;
    }

    // We need to send our MSS value.
    // We also want to use receiver's window scaling - if the receiver offered (_clientWScale > 0).
    // At the moment we don't scale our window (for simplicity),
    // but we still need to include that option (with no scaling: = 0).
    // If we didn't include this option at all, scaling would be disabled in both directions.

    TcpPacket::Option opts[ 2 ];
    const uint16_t oMss = htons ( _mss );
    const uint8_t oWScale = 0;

    opts[ 0 ].type = TcpPacket::OptMss;
    opts[ 0 ].data = &oMss;
    opts[ 0 ].dataLength = sizeof ( oMss );

    opts[ 1 ].type = TcpPacket::OptWScale;
    opts[ 1 ].data = &oWScale;
    opts[ 1 ].dataLength = 1;

    // NOTE: We send _sendDataSeq-1 as the sequence number, because it's a SYN-ACK packet.
    //       _sendDataSeq will be used for the first data byte.

    const TcpPacket respPacket (
            ServerAddr, ServerPort, // source of the response
            ClientAddr, ClientPort, // destination of the response
            TcpPacket::FlagSyn | TcpPacket::FlagAck,
            _sendDataSeq - 1,
            _nextRcvSeq,
            getWinSizeToAdvertise(),
            MemVector::EmptyVector,
            opts,
            ( _clientWScale > 0 ) ? 2 : 1 );

    LOG ( L_DEBUG2, getLogId() << ": Sending SYN-ACK packet: " << respPacket );

    const ERRCODE eCode = sendPacket ( respPacket );

    if ( IS_OK ( eCode ) )
    {
        setTcpFlag ( TcpFlagSentSynAck );
    }
}

void TcpTerminator::sendAck()
{
    uint8_t flagsToSend = TcpPacket::FlagAck;

    if ( shouldSendFin() )
    {
        // We should only be in 'closed' state once our FIN is acknowledged
        // (which means shouldSendFin() should be false)
        assert ( _tcpState != TcpClosed );

        flagsToSend |= TcpPacket::FlagFin;
        _tcpTimer.start ( RETRANS_TIME );
    }

    uint32_t dataSeq;

    if ( hasTcpFlag ( TcpFlagRcvdFinAck ) )
    {
        // If our FIN was already acknowledged, it means that we should have no more outgoing packets in the buffer...
        assert ( getSendBufSize() == 0 );

        // If our FIN was already acknowledged, it means that we need to increment the data sequence number by 1.
        dataSeq = _sendDataSeq + 1;
    }
    else
    {
        dataSeq = _sendDataSeq + _sentBuffer.getDataSize();
    }

    const TcpPacket ackPacket (
            ServerAddr, ServerPort,
            ClientAddr, ClientPort,
            flagsToSend,
            dataSeq,
            getAckToSend(),
            getWinSizeToAdvertise() );

    LOG ( L_DEBUG4, getLogId() << ": Sending ACK packet: " << ackPacket );

    clearTcpFlag ( TcpFlagNeedsAck );
    sendPacket ( ackPacket );
}

#define CASE_STATE_NAME( st ) \
    case st: \
        return #st

String TcpTerminator::getTcpStateName ( TcpTerminator::TcpState st )
{
    switch ( st )
    {
        CASE_STATE_NAME ( TcpInit );
        CASE_STATE_NAME ( TcpConnected );
        CASE_STATE_NAME ( TcpClosed );
        CASE_STATE_NAME ( TcpBroken );
    }

    return String ( "Unknown:%1" ).arg ( st );
}
