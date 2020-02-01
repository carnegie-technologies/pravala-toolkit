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

#include "socket/IpSocket.hpp"

typedef struct ssl_st SSL;

namespace Pravala
{
class SslContext;
class TlsSocket;
class DtlsSocket;

/// @brief A base class representing an abstract SSL socket.
class SslSocket: public IpSocket, protected SocketOwner
{
    public:
        /// @brief Returns SNI hostname of the SSL session
        /// @return SNI hostname of the SSL session
        String getSessionSniHostname() const;

        /// @brief Sets SNI hostname in underlying SSL object
        /// @param [in] sniHostname The hostname to set
        /// @return True if the operation succeeded; false otherwise
        bool setSniHostname ( const String & sniHostname );

        /// @brief Returns the X509 subject_name of remote peer's certificate
        /// @return The subject name of remote peer's certificate
        String getCertSubject() const;

        /// @brief Returns the X509 issuer of remote peer's certificate
        /// @return The issuer of remote peer's certificate
        String getCertIssuer() const;

        /// @brief Returns the X509 serial number of remote peer's certificate
        /// @return The serial number of remote peer's certificate
        MemHandle getCertSerialNumber() const;

        /// @brief Retrieves the IP addresses stored in peer's certificate
        /// It looks for Alt-Name that includes the IP addresses
        /// @return The IP addresses stored in peer's certificate (if any)
        List<IpAddress> getCertIpAddresses() const;

        /// @brief Returns the description of remote peer's certificate.
        /// @return The description of remote peer's certificate.
        String getCertDesc() const;

        /// @brief Returns the MD5 hash of the SSL session's master key
        /// @param [in] printableHex If true (default) a printable string with hex values is returned;
        ///                           Otherwise binary data (which most likely is not printable) is returned.
        /// @return the MD5 hash of the SSL session's master key, or an empty string if it cannot be obtained
        String getSessionMasterKeyHash ( bool printableHex = true ) const;

        /// @brief Returns this object as a TlsSocket.
        /// @return This object as a TlsSocket pointer, or 0 if it is not a TlsSocket.
        virtual TlsSocket * getTlsSocket();

        /// @brief Returns this object as a TlsSocket.
        /// @return This object as a TlsSocket pointer, or 0 if it is not a TlsSocket.
        virtual DtlsSocket * getDtlsSocket();

        virtual String getLogId ( bool extended = false ) const;

    protected:
        /// @brief When set, the respective socket should perform 'SSL_write' even when receiving a 'read' event
        static const uint16_t SockSslFlagDoWriteOnRead = ( 1 << ( SockIpNextFlagShift + 0 ) );

        /// @brief When set, the respective socket should perform 'SSL_read' even when receiving a 'write' event
        static const uint16_t SockSslFlagDoReadOnWrite = ( 1 << ( SockIpNextFlagShift + 1 ) );

        /// @brief When set, the respective socket should only perform 'SSL_accept' operation until it succeeds.
        static const uint16_t SockSslFlagAcceptNeeded = ( 1 << ( SockIpNextFlagShift + 2 ) );

        /// @brief When set, the respective socket should only perform 'SSL_connect' operation until it succeeds.
        static const uint16_t SockSslFlagConnectNeeded = ( 1 << ( SockIpNextFlagShift + 3 ) );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSslNextEventShift = SockIpNextEventShift;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockSslNextFlagShift = SockIpNextFlagShift + 4;

        static TextLog _log; ///< Log stream

        /// @brief Helper enum used for determining the SSL call types
        enum CallType
        {
            CallUnknown, ///< Unknown call type
            CallDtlsListen, ///< DTLSv1_listen
            CallAccept, ///< SSL_accept
            CallConnect, ///< SSL_connect
            CallRead, ///< SSL_read
            CallWrite ///< SSL_write
        };

        SSL * _ssl; ///< The pointer to internal SSL object generated by SSL.

        ERRCODE _closedReason; ///< The reason to pass in 'closed' event.

        /// @brief Constructor.
        /// It initializes internal SSL state using provided SSL context.
        /// It will also set either 'accept needed' or 'connect needed' flag, depending on the context type.
        /// @param [in] owner The initial owner to set.
        /// @param [in] sslContext The SSL context to use for the internal SSL object.
        SslSocket ( SocketOwner * owner, SslContext & sslContext );

        /// @brief Destructor.
        virtual ~SslSocket();

        /// @brief Returns the internal socket descriptor
        /// @return The internal socket descriptor, or -1 if not set.
        int getSslSockFd() const;

        /// @brief Handles SSL error.
        /// Should be called every time one of SSL_*() API calls returns an error.
        /// @param [in] callType Which call returned an error (for logging).
        /// @param [in] callRet The return code from the SSL API call.
        /// @param [in] fd Socket's file descriptor. Can be invalid.
        ///                If it is valid, FD events for that socket can be adjusted (if SSL wants something).
        /// @param [in] delayCallbacks If true, this function will not call any callbacks, but it will schedule them.
        ///                            If false, this function may generate some callbacks immediately.
        /// @return Standard error code that can be returned by read/write methods.
        ERRCODE handleSslError ( CallType callType, int callRet, int fd, bool delayCallbacks );

        /// @brief Schedules a specific 'closed' event.
        /// @param [in] reason The reason to pass in the callback.
        void scheduleClosedEvent ( ERRCODE reason );

        /// @brief Helper function for printing the description of SSL call type (for debugging)
        /// @param [in] callType The type of the call
        /// @return The name of the SSL call type
        static const char * callTypeName ( CallType callType );

        virtual bool runEvents ( uint16_t events );

        virtual void socketClosed ( Socket * sock, ERRCODE reason );
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason );
        virtual void socketConnected ( Socket * sock ) = 0;
        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketReadyToSend ( Socket * sock );
};
}
