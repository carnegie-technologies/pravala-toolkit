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

#include "basic/NoCopy.hpp"

#include "DtlsSocket.hpp"

// BoringSSL does not support DTLS server mode
#ifndef OPENSSL_IS_BORINGSSL

namespace Pravala
{
class UdpSocket;

/// @brief DTLS listener
/// It opens a listening socket and waits for incoming DTLS "connections".
class DtlsListener: public NoCopy, protected DtlsSocket::Owner
{
    public:
        /// @brief The owner of the DtlsListener
        class Owner
        {
            protected:
                /// @brief Callback for notifying the owner when new DTLS "connection" is successfully "listened to".
                /// @note The SSL handshake for this connection is not completed yet, the socket will eventually
                /// become connected, or it will fail.
                /// @param [in] listener The listener that generated the callback.
                /// @param [in] socket The DtlsSocket for the new "connection".
                virtual void incomingDtlsConnection ( DtlsListener * listener, DtlsSocket * socket ) = 0;

                //// @brief Called when the listener receives data that is not part of DTLS handshake.
                /// @param [in] listener The listener that generated the callback.
                /// @param [in] socket The DtlsSocket over which the data has been received.
                /// @param [in] data The data received.
                /// @warning The data received by this function may NOT be aligned properly!
                virtual void receivedUnexpectedData (
                    DtlsListener * listener, DtlsSocket * socket, const MemHandle & data ) = 0;

                /// @brief Destructor.
                virtual ~Owner()
                {
                }

                friend class DtlsListener;
        };

        /// @brief Constructor.
        /// @param [in] owner The owner of this DtlsListener object (not in an owned-object sense,
        ///                    it will just receive the callbacks).
        /// @param [in] dtlsContext The DTLS context (server version) to use for DtlsSocket objects.
        ///                         It should remain valid as long as this object exists.
        DtlsListener ( Owner & owner, DtlsServer & dtlsContext );

        /// @brief Initializes the listener
        /// @param [in] localAddr Local address and port to listen on
        /// @return Standard error code.
        ERRCODE init ( const SockAddr & localAddr );

        /// @brief Initializes the listener
        /// @param [in] localAddr Local address to listen on
        /// @param [in] localPort Local port to listen on
        /// @return Standard error code.
        inline ERRCODE init ( const IpAddress & localAddr, uint16_t localPort )
        {
            return DtlsListener::init ( SockAddr ( localAddr, localPort ) );
        }

        /// @brief Destructor
        ~DtlsListener();

    protected:
        virtual void socketClosed ( Socket * sock, ERRCODE );
        virtual void socketConnected ( Socket * sock );

        virtual void socketConnectFailed ( Socket * sock, ERRCODE );
        virtual void socketDataReceived ( Socket * sock, MemHandle & data );
        virtual void socketReadyToSend ( Socket * sock );

        virtual void dtlsSocketListenSucceeded ( DtlsSocket * sock );
        virtual void dtlsSocketUnexpectedDataReceived ( DtlsSocket * sock, const MemHandle & data );

    private:
        static TextLog _log; ///< Log stream

        Owner & _owner; ///< Owner of the listener
        DtlsServer & _dtlsContext; ///< The DTLS context to use

        UdpSocket * _listeningUdpSock; ///< The UDP socket that DTLS sockets are using for listening.

        /// @brief DtlsSocket object used for listening.
        /// Once DTLS listen succeeds, this object will be used as the new connection,
        /// and a new 'listening' DtlsSocket will be created to replace this.
        DtlsSocket * _listeningDtlsSock;

        /// @brief Helper function that creates a new listening DTLS socket.
        /// It doesn't do anything if _listeningUdpSock is not set, or _listeningDtlsSock is already set.
        void createDtlsListeningSock();
};
}
#endif
