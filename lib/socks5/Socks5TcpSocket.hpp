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

#include "internal/Socks5RequestMessage.hpp"
#include "internal/Socks5TcpBaseSocket.hpp"

namespace Pravala
{
/// @brief Represents a TCP socket that can connect to a SOCKS5 proxy server and send/receive data over TCP.
/// It can also be used by SOCKS5 UDP socket as a coordinating TCP socket (after overriding sendSocks5Request method).
class Socks5TcpSocket: public Socks5TcpBaseSocket
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] proxyAddr The address of SOCKS5 proxy server to use.
        Socks5TcpSocket ( SocketOwner * owner, const SockAddr & proxyAddr );

        virtual ERRCODE connect ( const SockAddr & addr );

        virtual String getLogId ( bool extended = false ) const;

        /// @brief Exposes the address of SOCKS5 proxy server used by this connection.
        /// @return The address of SOCKS5 proxy server used by this connection.
        inline const SockAddr & getProxySockAddr() const
        {
            return _proxyAddr;
        }

    protected:
        /// @brief Version identifier sent, awaiting a method selection message from the server.
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5TcpStateValueNotConnected = 0;

        /// @brief Version identifier sent, awaiting a method selection message from the server.
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5TcpStateValueAwaitingMethod = ( 1 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Request message sent, awaiting a reply message from the server.
        /// @note This is not a regular "flag", this is a multi-bit value stored in a number of flag bits.
        static const uint16_t SockSocks5TcpStateValueAwaitingReply = ( 2 << ( SockSocks5TcpBaseNextFlagShift + 0 ) );

        /// @brief Socket flags mask where the state values are stored.
        /// @note State values use 2 flag bits (to hold values: 0, 1, 2),
        ///       the next flag should be SockSocks5TcpBaseNextFlagShift + 2 (instead of +1).
        static const uint16_t SockSocks5TcpStateFlagMask
            = ( SockSocks5TcpStateValueAwaitingMethod | SockSocks5TcpStateValueAwaitingReply );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5TcpNextEventShift = SockSocks5TcpBaseNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5TcpNextFlagShift = SockSocks5TcpBaseNextFlagShift + 2;

        const SockAddr _proxyAddr; ///< The SOCKS5 proxy server to use

        /// @brief Puts a SOCKS5 request message in send buffer.
        /// It also enables write events.
        /// It should only be called in SockSocks5TcpStateValueAwaitingMethod state, and only when the buffer is empty.
        /// @param [in] reqCommand The command to put in the request.
        /// @param [in] addr The address to put in the request.
        /// @return True if the buffer was empty, state was correct, and message was successfully generated;
        ///         False otherwise.
        bool sendSocks5Request ( Socks5RequestMessage::Command reqCommand, const SockAddr & addr );

        /// @brief Puts a SOCKS5 request message in send buffer.
        /// It also enables write events.
        /// It should only be called in SockSocks5TcpStateValueAwaitingMethod state, and only when the buffer is empty.
        /// The default implementation calls sendSocks5Request ( Socks5RequestMessage::CmdTcpConnect, _remoteAddr ).
        /// It can be overridden to run other requests, like CmdUDPAssociate.
        /// @return True if the buffer was empty, state was correct, message was successfully generated
        ///         and added to the buffer. False otherwise.
        virtual bool sendSocks5Request();

        /// @brief Called when a reply is received with a 'success' code.
        /// @param [in] addr The address received in the reply message.
        virtual void socks5RequestSucceeded ( const SockAddr & addr );

        virtual void readyToSendSocks5Data();
        virtual size_t receiveSocks5Data();

    private:
        /// @brief Processes a 'method select' message.
        /// @param [out] reqBytes Will be set to the number of additional data bytes required
        ///                       to be able to receive the message (if IncompleteData is returned).
        /// @return Standard error code.
        ERRCODE receiveMethodSelect ( size_t & reqBytes );

        /// @brief Processes a 'reply' message.
        /// @param [out] reqBytes Will be set to the number of additional data bytes required
        ///                       to be able to receive the message (if IncompleteData is returned).
        /// @return Standard error code.
        ERRCODE receiveReply ( size_t & reqBytes );
};
}
