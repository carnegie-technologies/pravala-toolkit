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

#include "event/Timer.hpp"

#include "SslSocket.hpp"
#include "DtlsBio.hpp"

namespace Pravala
{
class DtlsContext;
class UdpSocket;

/// @brief A class representing a DTLS socket.
/// All Socket::send functions implemented by the class perform writes over the encrypted channel.
class DtlsSocket: protected DtlsBio, public SslSocket, protected Timer::Receiver
{
    public:
        /// @brief DTLS-specific owner.
        class Owner: public SocketOwner
        {
            protected:
                /// @brief Called when the socket receives unexpected (invalid) data.
                /// @param [in] sock The socket that generated the callback
                /// @param [in] data Payload of the message. It is not buffered after this call.
                /// @warning The data received by this function may be NOT aligned properly!
                virtual void dtlsSocketUnexpectedDataReceived ( DtlsSocket * sock, const MemHandle & data );

                /// @brief Called when the DTLS 'listen' succeeds, meaning that this socket is now ready
                ///        to "accept" DTLS connection. This socket will also start using a new, connected UDP socket.
                /// @param [in] sock The socket that generated the callback
                virtual void dtlsSocketListenSucceeded ( DtlsSocket * sock );

                friend class DtlsSocket;
        };

        /// @brief DTLS stats.
        struct Stats
        {
            uint32_t packetsReceived; ///< The number of packets received.
            uint32_t packetsLost; ///< The number of lost packets.

            /// @brief clears data and packet counters.
            inline void clearCounters()
            {
                packetsReceived = packetsLost = 0;
            }
        };

        /// @brief The size of buffer used for calculating packet losses.
        static ConfigLimitedNumber<uint16_t> optDtlsPacketLossBufferSize;

        /// @brief Constructor.
        /// Creates a DtlsSocket that uses a specified UDP data socket - which should be connected or connecting.
        /// @param [in] owner The initial owner to set.
        /// @param [in] dtlsContext The DtlsContext to use for the internal SSL object.
        /// @param [in] udpSocket The UdpSocket object to connect over.
        ///                       It should be in either 'connected' or 'connecting' state.
        /// @param [in] sslFlags If not 0 the SSL_set_options is called with the given value.
        DtlsSocket ( Owner * owner, DtlsContext & dtlsContext, UdpSocket * udpSocket, long sslFlags = 0 );

        /// @brief Sets object's owner.
        /// Should be used instead of base setOwner().
        /// If the base version is used instead, DTLS-specific owner will be unset.
        /// @note It does not modify the reference counter.
        /// @param [in] owner The new owner
        void setOwner ( Owner * owner );

        /// @brief Increments the reference counter and sets the new owner.
        /// Should be used instead of base refOwner().
        /// If the base version is used instead, DTLS-specific owner will be unset.
        /// @param [in] owner New owner of this object, 0 to leave it unchanged
        void refOwner ( Owner * owner );

        /// @brief Exposes this socket's stats.
        /// @return A reference to this socket's stats.
        inline const Stats & getStats() const
        {
            return _stats;
        }

        /// @brief Exposes this socket's stats.
        /// @return A reference to this socket's stats.
        inline Stats & getStats()
        {
            return _stats;
        }

        /// @brief Performs a test encryption (without actually sending anything) to verify the overhead.
        /// It will perform a write using SSL, and return the resulting data size that would be sent.
        /// @note This will cause the data in the read buffer (if any) to be discarded!
        /// @param [in] size The number of bytes to write.
        /// @return The result amount of bytes that would be written over the network.
        ///         This does NOT include IP and UDP header that would be added.
        ///         0 if there was an error.
        size_t testEncryptionOverhead ( size_t size );

        virtual DtlsSocket * getDtlsSocket();

        virtual String getLogId ( bool extended = false ) const;
        virtual void close();

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE bindToIface ( const String & ifaceName, IpAddress::AddressType addrType );
        virtual ERRCODE connect ( const SockAddr & addr );

        virtual const SockAddr & getLocalSockAddr() const;
        virtual const SockAddr & getRemoteSockAddr() const;

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );
        virtual ERRCODE send ( MemVector & data );

        virtual void timerExpired ( Timer * timer );

    protected:
        /// @brief Set when the initial connect/accept should be performed.
        static const uint16_t SockDtlsEventInitialOp = ( 1 << ( SockSslNextEventShift + 0 ) );

        /// @brief When set, the respective socket should only perform 'DTLSv1_listen' operation until it succeeds.
        static const uint16_t SockDtlsFlagListenNeeded = ( 1 << ( SockSslNextFlagShift + 0 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockDtlsNextEventShift = SockSslNextEventShift + 1;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockDtlsNextFlagShift = SockSslNextFlagShift + 1;

        /// @brief Destructor.
        ~DtlsSocket();

        virtual void ownerChanged();

        virtual bool runEvents ( uint16_t events );

        virtual void socketConnected ( Socket * sock );
        virtual void socketDataReceived ( Socket * sock, MemHandle & data );

    private:
        static MemHandle _testData; ///< Data used to perform test writes.

        Stats _stats; ///< Internal stats.

        SimpleTimer _dtlsTimer; ///< The timer used for DTLS' own retransmissions (during handshake)

        Owner * _dtlsOwner; ///< DTLS-specific owner.

        /// @brief DTLS epoch value we expect to see in the next DTLS packet.
        uint8_t _dtlsExpectedEpoch[ 2 ];

        /// @brief DTLS sequence number we expect to see in the next DTLS packet.
        uint16_t _dtlsExpectedSeqNum;

        /// @brief A helper function that delivers received data.
        /// It may partially consume the 'data' object, which should always be cleared after calling this.
        /// @param [in] data The data to deliver.
        void doDeliverData ( MemHandle & data );

        /// @brief Internal function for sending data over the socket in encrypted mode.
        ///
        /// In DTLS mode partial writes are possible. If during write operation only some of the bytes of the packet
        /// are written, the rest of the packet will not be written (and will be lost).
        /// In that case Error::IncompleteWrite error code is returned.
        ///
        /// @param [in] data The data to send.
        /// @param [in] dataSize The size of the data.
        /// @return Standard error code.
        ERRCODE doSendEncrypted ( const char * data, size_t dataSize );

        /// @brief Internal function for sending data over the socket in encrypted mode.
        /// @param [in] data The data to send.
        /// @return Standard error code.
        ERRCODE doSendEncrypted ( const MemVector & data );

        /// @brief Helper function that performs SSL_accept or SSL_connect.
        /// Caller should return after calling this.
        /// @note This HAS to be called asynchronously (so as a direct result of a timer expiring,
        ///       EventManager's event, etc., and not from a public function call like 'send').
        void doInitialOp();

        /// @brief Helper function that performs DTLSv1_listen.
        /// Caller should return after calling this.
        /// @note This HAS to be called asynchronously (so as a direct result of a timer expiring,
        ///       EventManager's event, etc., and not from a public function call like 'send').
        void doDtlsListen();

        /// @brief Checks whether the DTLS timer should be started (and (re)starts if it does)
        ///
        /// Note that even when the timer should fire right away, the timer is used regardless (with 1ms delay).
        /// This way we avoid problems with nested callbacks. It will trigger at the next timer slot.
        ///
        /// @param [in] callType The type of the SSL call that was used before calling this (for logging).
        void checkDtlsTimer ( CallType callType );

        /// @brief Helper function that increments packet counters.
        /// It adds 1 to the number of packets received, and given number to the lost counter.
        /// @param [in] numPacketsLost The number of packets lost.
        ///                            If 0, it means that packet was received in order.
        inline void addLostPackets ( uint32_t numPacketsLost )
        {
            ++_stats.packetsReceived;
            _stats.packetsLost += numPacketsLost;
        }

        virtual SocketApi::SocketType ipSockGetType ( const SockAddr & forAddr );
};
}
