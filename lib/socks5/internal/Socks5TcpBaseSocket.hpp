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

#include "socket/TcpFdSocket.hpp"

namespace Pravala
{
/// @brief Represents a base TCP socket that performs SOCKS5 handshake before regular data can be exchanged.
/// It is to be used by both client and server SOCKS5 TCP sockets.
class Socks5TcpBaseSocket: public TcpFdSocket
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        Socks5TcpBaseSocket ( SocketOwner * owner );

        virtual TcpFdSocket * generateTcpFdSock ( SocketOwner * owner );

        virtual void consumeReadBuffer ( size_t size );
        virtual const MemHandle & getReadBuffer() const;

        virtual ERRCODE send ( const char * data, size_t & dataSize );
        virtual ERRCODE send ( MemHandle & data );

        /// @brief A helper method that checks if SOCKS5 handshake has been finished.
        /// @return True if SOCKS5 handshake is done and we should behave like a regular TCP socket; False otherwise.
        inline bool isSocks5Connected() const
        {
            return hasFlag ( SockSocks5TcpBaseFlagHandshakeCompleted );
        }

    protected:
        /// @brief Set to mark that SOCKS5 handshake has been completed.
        static const uint16_t SockSocks5TcpBaseFlagHandshakeCompleted = ( 1 << ( SockTcpFdNextFlagShift + 0 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5TcpBaseNextEventShift = SockTcpFdNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSocks5TcpBaseNextFlagShift = SockTcpFdNextFlagShift + 1;

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        /// @param [in] sock Existing TCP connection to use.
        ///                  It should be a TCP socket object that allows for its FD to be stolen.
        ///                  The new socket will take over that FD (if possible).
        Socks5TcpBaseSocket ( SocketOwner * owner, TcpSocket * sock );

        /// @brief Schedules given SOCKS5 data to be sent.
        /// It also enables write events.
        /// @param [in] data Data to be put in the buffer.
        /// @return True if the data was scheduled to be sent;
        ///         False if the send buffer was not empty, or if the SOCKS5 handshake has already been completed.
        bool sendSocks5Data ( const MemHandle & data );

        /// @brief Called whenever there is a write event when there is no SOCKS5 data to be sent.
        /// It is called after write events have been disabled. To send more data, 'sendSocks5Data()' should be used.
        /// After this callback, calling function will return, so it is safe to destroy the socket inside the callback.
        /// If more data is generated inside the callback, it will be sent the next time we receive a write event.
        virtual void readyToSendSocks5Data() = 0;

        /// @brief Processes a message in the read buffer.
        /// The buffer may be empty, but should NOT contain more than a single SOCKS5 message.
        /// @return The number of additional bytes to read to be able to process the message.
        ///         0 if there was a fatal error, or if the message has been successfully processed.
        ///         0 means that the required action (error callback, sending another message, etc.)
        ///         has already been performed, and the caller should return right away.
        virtual size_t receiveSocks5Data() = 0;

        virtual void receiveFdEvent ( int fd, short int events );

    private:
        MemHandle _sendBuffer; ///< Buffer with data to be sent over the socket. Only used for SOCKS5 handshake.
};
}
