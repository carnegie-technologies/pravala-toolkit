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
#include "basic/IpAddress.hpp"
#include "basic/MemVector.hpp"
#include "net/IpFlow.hpp"
#include "log/TextLog.hpp"
#include "event/Timer.hpp"
#include "event/EventManager.hpp"

namespace Pravala
{
class IpPacket;

/// @brief A TCP terminator that allows for handling TCP connections incoming as IP packets over the tunnel interface.
/// It makes it possible to create TCP servers that operate using IP packets sent/received over the tunnel interface,
/// instead of using regular socket FDs.
/// @note When using IpFlow::packetReceived() call, it expects packets 'DefaultDescType' passed as the 'user data'.
///       Otherwise packets will be dropped. User pointer is always ignored.
class TcpTerminator: protected IpFlow, protected Timer::Receiver, protected EventManager::LoopEndEventHandler
{
    public:
        /// @brief IP address of this flow's client (the client sending the IP packets).
        const IpAddress ClientAddr;

        /// @brief IP address of this flow's destination (the server the client wanted to talk to).
        const IpAddress ServerAddr;

        /// @brief Port number of this flow's client (the client sending the IP packets).
        const uint16_t ClientPort;

        /// @brief Port number of this flow's destination (the server the client wanted to talk to).
        const uint16_t ServerPort;

        /// @brief Describes the state of the terminated TCP the flow.
        enum TcpState
        {
            /// @brief The default state of the flow after it has been created.
            /// It will remain in this state until we receive an ACK to our SYN-ACK packet.
            /// In this state we can only send SYN-ACK packet.
            TcpInit,

            /// @brief The state in which we are after receiving an ACK to our SYN-ACK packet.
            /// Regular data exchange is now possible.
            TcpConnected,

            /// @brief The state in which we are after having our FIN packet acknowledged.
            /// In this state we only ACK received FIN packets.
            TcpClosed,

            /// @brief TCP connection is broken.
            /// This object will stick around for a while, responding with TCP Reset packets.
            TcpBroken
        };

        /// @brief Destructor.
        /// @note The TCP terminator in certain conditions will destroy itself by calling 'delete this'.
        ///       The class inheriting this must be able to handle it.
        virtual ~TcpTerminator();

        /// @brief Returns the name of flow's current TCP state.
        /// @return The name of flow's current TCP state.
        inline String getTcpStateName() const
        {
            return getTcpStateName ( _tcpState );
        }

        virtual String getLogId() const;
        virtual ERRCODE packetReceived ( IpPacket & packet, int32_t userData, void * userPtr );

        /// @brief Returns the name of the given flow TCP state.
        /// @param [in] st The TCP state to return the name of.
        /// @return The name of the given flow TCP state.
        static String getTcpStateName ( TcpState st );

        /// @brief Finds the difference between two sequence numbers.
        /// This function is overflow aware.
        /// If the difference between x and y + 2^32 (or between x + 2^32 and y)
        /// is smaller than the difference between x and y, the smaller difference will be used instead.
        /// @param [in] x A sequence number.
        /// @param [in] y A sequence number.
        /// @return The difference between the two sequence numbers.
        ///         If x < y the difference will be negative, and if x > y the difference will be positive.
        static inline int32_t compareSeq ( uint32_t x, uint32_t y )
        {
            // The unsigned to signed cast converts large positive numbers to small negative ones.
            // UINT32_MAX - x = -( x + 1 )
            // If x and y are almost 4G apart, they are only a few bytes apart in the opposite direction.
            return ( int32_t ) ( x - y );
        }

    protected:
        static TextLogLimited _log; ///< Log stream.

        /// @brief Set to true whenever we fail to send all the data we're given.
        /// This could happen, for example, due to client receive window restrictions.
        /// It will trigger a callback once we can read data again.
        static const uint8_t TcpFlagSendBlocked = ( 1 << 0 );

        /// @brief A helper flag set to 'true' whenever we need to send a packet with an ACK in it.
        /// It is used to avoid sending empty ACK packets when we are sending data packets anyway.
        static const uint8_t TcpFlagNeedsAck = ( 1 << 1 );

        /// @brief Set to true when the SYN packet from the client is accepted.
        /// It causes future SYN packets to be ignored.
        static const uint8_t TcpFlagSynAccepted = ( 1 << 2 );

        /// @brief Set to true when we send SYN-ACK packet to TCP client.
        static const uint8_t TcpFlagSentSynAck = ( 1 << 3 );

        /// @brief Set to true when we send FIN packet to TCP client.
        /// It means that the data stream in client's direction has ended.
        static const uint8_t TcpFlagSentFin = ( 1 << 4 );

        /// @brief Set to true when the FIN packet sent to TCP client is acknowledged.
        /// It means that the client acknowledged our FIN request.
        static const uint8_t TcpFlagRcvdFinAck = ( 1 << 5 );

        /// @brief Set to true when we receive in-order FIN packet from TCP client.
        /// It means that the data stream from client's direction has ended.
        static const uint8_t TcpFlagRcvdFin = ( 1 << 6 );

        /// @brief Set to true when we subscribe to end-of-loop events.
        static const uint8_t TcpFlagEoLSubscribed = ( 1 << 7 );

        SimpleTimer _tcpTimer; ///< Timer for TCP operations.

        TcpState _tcpState; ///< The state of the terminated TCP flow.

        MemVector _sentBuffer; ///< Data sent over the TCP connection, that needs to be acknowledged.
        List<MemHandle> _unsentBuffer; ///< The data that has not been sent yet.

        List<MemHandle> _rcvBuffer;  ///< Data received over the TCP connection. It only includes in-order data.

        // TODO: Could these use uint16_t?

        uint32_t _unsentBufSize; ///< The size of the data in _unsentBuffer (in bytes).
        uint32_t _rcvBufSize;  ///< The size of the data in _rcvBuffer (in bytes).

        /// @brief The max size of the send buffer to allow.
        /// This is based on client's receive window.
        uint32_t _maxSendBufSize;

        /// @brief The next sequence number expected from the client.
        /// This is the sequence number of the last data byte in _rcvBuffer + 1.
        /// This value will be sent as an ACK in outgoing TCP packets.
        /// It is initialized to the sequence number in SYN packet + 1.
        uint32_t _nextRcvSeq;

        /// @brief The sequence number of the first byte of data in _sentBuffer.
        /// This is the sequence number associated with the first byte of data that has
        /// not been acknowledged by the client yet.
        uint32_t _sendDataSeq;

        /// @brief The MTU to be used by the terminator to adjust the MSS.
        /// If 0, the MSS will not be adjusted.
        const uint16_t _mtu;

        /// @brief Maximum Segment Size to use.
        /// It is based on the MSS received from the TCP client, potentially lowered to fit in tunnel iface's MTU.
        uint16_t _mss;

        uint8_t _clientWScale; ///< Window-scale value received from the TCP client.
        uint8_t _tcpFlags; ///< Helper flags.

        /// @brief Constructor.
        /// @param [in] flowDesc FlowDesc object describing this flow. It MUST describe a TCPv4 or TCPv6 packet!
        /// @param [in] mtu The MTU to be used by the terminator to adjust the MSS.
        ///                 If 0, the MSS will not be adjusted.
        TcpTerminator ( const FlowDesc & flowDesc, uint16_t mtu = 0 );

        /// @brief Sets specified TCP flag.
        /// @param [in] flag The flag to set.
        inline void setTcpFlag ( uint8_t flag )
        {
            _tcpFlags |= flag;
        }

        /// @brief Clears specified TCP flag.
        /// @param [in] flag The flag to clear.
        inline void clearTcpFlag ( uint8_t flag )
        {
            _tcpFlags &= ~flag;
        }

        /// @brief Checks specified TCP flag.
        /// @param [in] flag The flag to check.
        /// @return True if the flag is set (if multiple flags are passed, it checks if any of them is set);
        ///         False if the flag (or none of multiple flags) is set.
        inline bool hasTcpFlag ( uint8_t flag ) const
        {
            return ( _tcpFlags & flag ) != 0;
        }

        /// @brief A helper function that calculates an ACK number to include in TCP packets sent.
        /// It returns _nextRcvSeq increased by one if we have already received a FIN packet from the client.
        /// @return ACK number to include in TCP packets sent.
        inline uint32_t getAckToSend() const
        {
            return hasTcpFlag ( TcpFlagRcvdFin ) ? ( _nextRcvSeq + 1 ) : _nextRcvSeq;
        }

        /// @brief A helper function that checks if we should be sending TCP FIN packets.
        /// It returns true if the 'FIN Sent' flag is set, and 'FIN Acknowledged' is not.
        /// This means that we have started shutting down the connection, but the other side
        /// has not acknowledged that yet.
        /// @return Whether FIN flag should be included in outgoing TCP packets.
        inline bool shouldSendFin() const
        {
            return ( ( _tcpFlags & ( TcpFlagSentFin | TcpFlagRcvdFinAck ) ) == TcpFlagSentFin );
        }

        /// @brief Helper function that calculates the total size of data accepted for sending.
        /// It includes both the data in _sentBuffer and in _unsentBuffer.
        /// @return The total data bytes that were sent and are still unsent (in bytes).
        inline uint32_t getSendBufSize() const
        {
            return ( _sentBuffer.getDataSize() + _unsentBufSize );
        }

        /// @brief Calculates adjusted max send buffer size.
        /// We clamp the max buffer size for sending data to at most 2*MSS.
        /// This is to limit the amount of memory used for buffering data.
        /// We want to allow at least 1*MSS to take advantage of tunnel's MTU.
        /// However, 1*MSS results in poor performance (most likely due to delayed ACKs).
        /// 2*MSS is sufficient to achieve good speeds, and that's the limit we use.
        /// @param [in] maxSendBufSize The max send buffer size to adjust.
        /// @return The adjusted value to be used as the max send buffer size.
        inline uint32_t adjustedMaxSendBufSize ( uint32_t maxSendBufSize ) const
        {
            assert ( _mss > 0 );

            return min<uint32_t> ( maxSendBufSize, _mss * 2 );
        }

        /// @brief Calculates the size of the receive windows to advertise.
        /// It takes into account the amount of data remaining in the receive buffer.
        /// It will also clamp the window to 2*MSS, to limit the amount of buffered data.
        /// Using 1*MSS results in poor throughput, 2*MSS makes the throughput much better.
        /// @return The size of the receive window to advertise.
        inline uint16_t getWinSizeToAdvertise() const
        {
            assert ( _mss > 0 );

            const uint32_t doubleMss = _mss * 2;

            // If _rcvBufSize is too large (for whatever reason) we advertise 0 window.
            // We also can't go beyond 0xFFFF.

            return min<uint32_t> ( 0xFFFF, ( _rcvBufSize < doubleMss ) ? ( doubleMss - _rcvBufSize ) : 0 );
        }

        /// @brief Appends data to be sent by the TCP terminator over the TCP connection.
        /// @param [in,out] data The data to be sent. It is consumed to reflect the data accepted.
        void appendData ( MemHandle & data );

        /// @brief Called to consume data received over the TCP connection.
        /// It will repeatedly call receivedData ( MemHandle& ) until data stops being accepted
        /// or an error is returned (in which case close() will be called).
        /// It will also send an ACK if the terminator leaves the 'zero window' state.
        /// @return The code received from receiveData().
        ERRCODE consumeReceivedData();

        /// @brief Sends TCP SYN-ACK packet to the client.
        /// It will also switch the terminator into 'connected' state.
        /// It only works in 'init' state.
        void sendSynAck();

        /// @brief Called to close this TCP terminator.
        /// It will clear the buffers and, if we are currently connected, send FIN packet and configure the flags.
        /// If we are not connected, it will call setBroken().
        /// @param [in] reason The reason it was closed (only used for logging).
        virtual void close ( ERRCODE reason );

        /// @brief Called when the terminator enters 'broken' state.
        /// It sets 'broken' state, clears the buffers, and schedules timer to remove this object.
        virtual void setBroken();

        /// @brief Adjusts given MSS.
        /// If specific MSS value needs to be used, this is where it can be done.
        /// The default implementation just returns the MSS received.
        /// @param [in] mss The MSS received from the client.
        /// @return The MSS to use.
        virtual uint16_t adjustMss ( uint16_t mss ) const;

        /// @brief Receives (and consumes) data received over the TCP connection.
        /// @param [in,out] data The data to receive.
        ///                      It should be consumed to reflect the amount of data consumed.
        ///                      When not all data is consumed, this method will stop being called.
        /// @return Standard error code. If this returns an error, close() is called with the received error code.
        ///         A success should be returned if there was no error to cause closing the terminator,
        ///         even if no data is accepted.
        virtual ERRCODE receiveData ( MemHandle & data ) = 0;

        /// @brief Sends IP packet to TCP client handled by this terminator.
        /// @param [in] packet The packet to write.
        /// @return Standard error code.
        virtual ERRCODE sendPacket ( const IpPacket & packet ) = 0;

        /// @brief This method is called when the TCP terminator is unblocked and can accept more data for sending.
        /// The implementation can call appendData() again.
        virtual void sendingUnblocked() = 0;

        /// @brief Called the first time we receive a SYN packet from the client.
        /// @param [in] ipPacket The IP packet with the TCP SYN flag that triggered this call.
        /// @return True if the SYN request has been "accepted". Future SYN packets will be ignored.
        ///         Following this call, an implementation should either transition into 'connected' state,
        ///         or close the connection.
        virtual bool initializeReceiver ( const IpPacket & ipPacket ) = 0;

        /// @brief This method is called when the TCP terminator completes receiving TCP data.
        /// It means that the client closed the connection, and all the data received has been consumed.
        virtual void receivingCompleted() = 0;

        virtual void flowRemoved();

        virtual void timerExpired ( Timer * timer );
        virtual void receiveLoopEndEvent();

    private:
        /// @brief Handles received TCP SYN packet.
        /// @param [in] packet The packet received. It must be valid and have 'SYN' flag set.
        /// @return Standard error code.
        ERRCODE handleSynPacket ( IpPacket & packet );

        /// @brief Handles received TCP packet with data.
        /// @param [in] packet The packet received. It must be valid and have 'ACK' flag set.
        /// @return True if the packet can be further processed; False if there is something wrong with it.
        bool handleDataPacket ( IpPacket & packet );

        /// @brief Handles received TCP ACK packet.
        /// It will remove any acknowledged packets from the send buffer, and may perform re-transmissions.
        /// @param [in] packet The packet received. It must be valid and have 'ACK' flag set.
        void handleAckPacket ( IpPacket & packet );

        /// @brief Sends TCP ACK packet to the client.
        /// It will use the next data sequence number, but it will not carry any data.
        void sendAck();

        /// @brief A helper function that sends a TCP RESET packet in response to given IP packet.
        /// @param [in] toPacket The packet to respond to. Nothing is sent if that packet has RST flag set.
        void sendResetResponse ( const IpPacket & toPacket );

        /// @brief Re-transmits a single packet from the beginning of 'sent' buffer.
        /// It also restarts the timer.
        void resendFirst();

        /// @brief Sends all data in _unsentBuffer.
        /// It also moves the data to _sentBuffer and restarts the timer.
        void sendUnsent();
};
}
