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

#include "Socks5RequestMessage.hpp"
#include "Socks5ReplyMessage.hpp"

#include "Socks5TcpBaseSocket.hpp"

namespace Pravala
{
class Socks5Server;

/// @brief Represents a server-side socket that is used by SOCKS5 proxy server to accept incoming SOCKS5 requests.
/// This is a TCP socket, which could be used either directly, or as a coordinating socket for UDP sockets.
class Socks5ServerSocket: public Socks5TcpBaseSocket, protected Timer::Receiver
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] clientSock Incoming TCP connection (from proxy's client).
        ///                        It should be a TCP socket object that allows for its FD to be stolen.
        ///                        The new socket will take over that FD (if possible).
        Socks5ServerSocket ( Socks5Server * owner, TcpSocket * clientSock );

        /// @brief Helper function to gracefully cancel a SOCKS5 connection in progress.
        /// Called after a reply indicating failure is sent to the client.
        /// Schedules the socket to be closed in 10 seconds (as per RFC 1928),
        /// unless the socket is closed by the client sooner.
        /// It also changes the state to 'cancelled'.
        /// @param [in] reason Error code with cancellation reason. It will be passed in 'connection failed' callback.
        void cancelSocks5 ( ERRCODE reason );

        /// @brief Sets object's owner.
        /// Should be used instead of base setOwner().
        /// If the base version is used instead, SOCKS5-specific owner will be unset.
        /// @note It does not modify the reference counter.
        /// @param [in] owner The new owner
        void setOwner ( Socks5Server * owner );

        /// @brief Increments the reference counter and sets the new owner.
        /// Should be used instead of base refOwner().
        /// If the base version is used instead, SOCKS5-specific owner will be unset.
        /// @param [in] owner New owner of this object, 0 to leave it unchanged
        void refOwner ( Socks5Server * owner );

        /// @brief Should be called by the server when a "TCP connect" request has been handled.
        /// It also enables write events and will cancel the socket on error.
        /// @param [in] result The result of the connect request.
        /// @param [in] addr The address to include with the message.
        ///                  If the result is 'success', it should be the local address of the socket used
        ///                  to connect to remote host.
        /// @return Standard error code.
        ///         If 'success' is returned, it means that this socket is finalizing the handshake.
        ///         Once it is finished, a 'connected' callback will be generated.
        ERRCODE handledTcpConnect ( Socks5ReplyMessage::Reply result, const SockAddr & addr = EmptySockAddress );

        virtual void close();
        virtual String getLogId ( bool extended = false ) const;

        virtual ERRCODE bind ( const SockAddr & addr );
        virtual ERRCODE connect ( const SockAddr & addr );

    protected:
        /// @brief Waiting for the client to send a version ID / method select message
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueReceivingVersion = 0;

        /// @brief Waiting for the client to send a request message
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueReceivingRequest
            = ( 1 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Waiting for the proxy server to connect to the remote host via TCP
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueAwaitingTcpConnection
            = ( 2 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Waiting for the proxy server to return a UDP association
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueAwaitingUdpAssociation
            = ( 3 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Waiting for the proxy server to bind a listening TCP socket
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueAwaitingBoundTcpListener
            = ( 4 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Waiting for a remote host to connect to the listening TCP socket
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueAwaitingInboundTcpConnection
            = ( 5 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief SOCKS5 TCP connect completed successfully
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueTcpConnected
            = ( 6 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief SOCKS5 UDP associate completed successfully
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueUdpAssociated
            = ( 7 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief SOCKS5 TCP bind completed successfully; A remote host has connected to the listening TCP socket
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueTcpBound
            = ( 8 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief The connection attempt was cancelled
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5ServerStateValueCancelled
            = ( 9 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Socket flags mask where the state values are stored.
        /// @note State values use 4 flag bits (to hold values: 0, 1, ..., 9),
        ///       the next flag should be SockSocks5TcpBaseNextFlagShift + 4 (instead of +1).
        static const uint16_t SockSocks5ServerStateFlagMask
            = ( SockSocks5ServerStateValueReceivingVersion
                | SockSocks5ServerStateValueReceivingRequest
                | SockSocks5ServerStateValueAwaitingTcpConnection
                | SockSocks5ServerStateValueAwaitingUdpAssociation
                | SockSocks5ServerStateValueAwaitingBoundTcpListener
                | SockSocks5ServerStateValueAwaitingInboundTcpConnection
                | SockSocks5ServerStateValueTcpConnected
                | SockSocks5ServerStateValueUdpAssociated
                | SockSocks5ServerStateValueTcpBound
                | SockSocks5ServerStateValueCancelled );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5ServerNextEventShift = SockSocks5TcpBaseNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5ServerNextFlagShift = SockSocks5TcpBaseNextFlagShift + 4;

        /// @brief Destructor
        virtual ~Socks5ServerSocket();

        virtual void readyToSendSocks5Data();
        virtual size_t receiveSocks5Data();
        virtual void ownerChanged();
        virtual void timerExpired ( Timer * timer );

        virtual void doSockClosed ( ERRCODE reason );
        virtual void doSockConnectFailed ( ERRCODE reason );

    private:
        SimpleTimer _cancelTimer; ///< Timer to close the socket after delay when SOCKS5 handshake is cancelled.
        ERRCODE _cancelReason; ///< The reason for socket cancellation.

        Socks5Server * _socks5Server; ///< Pointer to SOCKS5 proxy server this socket is a part of.

        /// @brief Processes version identifier / method select message.
        /// If the client offered an authentication method that is supported, it continues the SOCKS5
        /// negotiation by sending a method selection message confirming the authentication method.
        /// If the client offered no acceptable methods, a reply indicating failure is sent.
        /// This function may modify the internal socket state.
        /// @note Currently, only NoAuthRequired is supported.
        /// @param [out] reqBytes Will be set to the number of additional data bytes required
        ///                       to be able to receive the message (if IncompleteData is returned).
        /// @return SOCKS5 reply code to send back.
        ///         If not 'success', an error reply will be generated by the caller and socket will be canceled.
        ///         'Success' is returned when there is no need for the caller to send an error response.
        ///         This code is returned in the following cases:
        ///          - operation succeeded
        ///          - operation failed but a response has already been sent
        ///          - more data is needed (in which case 'reqBytes' should be set).
        Socks5ReplyMessage::Reply receiveVersion ( size_t & reqBytes );

        /// @brief Processes a SOCKS5 request message.
        /// If the client sent a valid request (connect, bind, or UDP associate), it is passed up to the owner.
        /// If the client sent an invalid request, a reply indicating failure is sent.
        /// This function may modify the internal socket state.
        /// @param [out] reqBytes Will be set to the number of additional data bytes required
        ///                       to be able to receive the message (if IncompleteData is returned).
        /// @return SOCKS5 reply code to send back.
        ///         If not 'success', an error reply will be generated by the caller and socket will be canceled.
        ///         'Success' is returned when there is no need for the caller to send an error response.
        ///         This code is returned in the following cases:
        ///          - operation succeeded
        ///          - operation failed but a response has already been sent
        ///          - more data is needed (in which case 'reqBytes' should be set).
        Socks5ReplyMessage::Reply receiveRequest ( size_t & reqBytes );

        /// @brief Puts a SOCKS5 method select message in send buffer.
        /// It also enables write events. It should only be called in SockSocks5ServerStateValueReceivingVersion state,
        /// and only when the buffer is empty.
        /// @param [in] method The authentication method chosen.
        /// @return True if the buffer was empty, state was correct, and message was successfully generated;
        ///         False otherwise.
        bool sendSocks5MethodSelect ( Socks5Message::AuthenticationMethod method );

        /// @brief Puts a SOCKS5 reply message in send buffer.
        /// It also enables write events. It should only be called when the buffer is empty.
        /// @param [in] replyCode The reply code to put in the message.
        /// @param [in] addr The address to include with the message.
        ///                  If reply code is different than 'success', the address is ignored.
        /// @return True if the buffer was empty, state was correct, and message was successfully generated;
        ///         False otherwise.
        bool sendSocks5Reply ( Socks5ReplyMessage::Reply replyCode, const SockAddr & addr = EmptySockAddress );

        /// @brief Helper function that modifies socket's state.
        /// @param [in] state The state to set. One of SockSocks5ServerStateValue* values.
        inline void setState ( uint16_t state )
        {
            setFlagValue ( state, SockSocks5ServerStateFlagMask );
        }

        /// @brief Helper function that returns socket's state.
        /// @return Current state. One of SockSocks5ServerStateValue* values.
        inline uint16_t getState() const
        {
            return getFlagValue ( SockSocks5ServerStateFlagMask );
        }

        /// @brief A helper function that returns the name of the given state.
        /// @param [in] state The state whose name to return.
        /// @return The name of the given state, of 'Invalid' if unknown.
        static String getStateName ( uint16_t state );
};
}
