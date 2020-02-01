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

#include "object/SimpleObject.hpp"
#include "UdpSocket.hpp"

namespace Pravala
{
class UdpListenerSocket;

/// @brief Represents an object "bound" to an IP/port using UDP
/// How this object is bound to an IP/port is implementation specific.
///
/// Passing UdpListener objects between threads is NOT SUPPORTED.
class UdpListener: public SimpleObject, public LogId
{
    public:
        /// @brief Generates a UdpSocket object that will receive all packets not sent to a connected socket,
        /// and registers it with _listening.
        ///
        /// This will fail if a listening socket already exists.
        ///
        /// @note If the backing socket is closed, this listening socket will be notified first,
        ///       before any connected sockets.
        /// @param [in] owner A pointer to the object that will be receiving callbacks.
        /// @param [out] errCode If used, the status of the operation will be stored there.
        /// @return A pointer to a newly created UDP socket, or 0 if it could not be created.
        ///         It is caller's responsibility to delete this socket.
        UdpSocket * generateListeningSock ( SocketOwner * owner, ERRCODE * errCode = 0 );

        /// @brief Generates a UdpSocket object that will only receive packets sent to
        /// its "connected" remote peer, and registers it with _connected.
        ///
        /// This will fail if a socket already exists for this remote peer.
        ///
        /// @note If the backing socket is closed, the listening socket will be notified first,
        ///       before any connected sockets.
        /// @param [in] owner A pointer to the object that will be receiving callbacks.
        /// @param [in] remoteAddr The remote address and port to connect to.
        /// @param [out] errCode If used, the status of the operation will be stored there.
        /// @return A pointer to a newly created UDP socket, or 0 if it could not be created.
        ///         It is caller's responsibility to delete this socket.
        UdpSocket * generateConnectedSock (
            SocketOwner * owner, const SockAddr & remoteAddr, ERRCODE * errCode = 0 );

        /// @brief Returns the local address of this listener
        /// @return The local address of this listener
        inline const SockAddr & getLocalAddr() const
        {
            return _localAddr;
        }

        /// @brief (Re)Initializes a bound UdpListener.
        /// It will only succeed if there are no UdpSocket objects currently using this listener.
        /// @param [in] localAddr The local address and port to bind to.
        ///                        A "zero" address means "any" address.
        ///                        Zero port number means to dynamically allocate a port number.
        /// @return Standard error code.
        ERRCODE reinitialize ( const SockAddr & localAddr );

        /// @brief Checks whether there are any UDP sockets associated with this listener.
        /// @return True if there is at least one listening or connected UdpSocket associated; False otherwise.
        inline bool hasSockets() const
        {
            return ( _listeningSock != 0 || !_connectedSocks.isEmpty() );
        }

        /// @brief Tries to increase receive buffer size of underlying UDP socket.
        /// This function changes SO_RCVBUF option of the socket.
        /// If the currently used socket's buffer size is greater (or equal) to the size requested,
        /// it will NOT be modified (this function newer shrinks the buffer).
        /// Otherwise the buffer will be increased up to the size requested, if possible.
        /// If it cannot be increased to the requested size, it will be increased as much as possible.
        /// Default implementation always fails.
        /// @param [in] size The requested size (in bytes); Should be > 0.
        /// @return New receive buffer size in bytes (even if it was not modified, can be larger than size requested);
        ///         -1 on error.
        virtual int increaseRcvBufSize ( int size );

        /// @brief Tries to increase send buffer size of underlying UDP socket.
        /// This function changes SO_SNDBUF option of the socket.
        /// If the currently used socket's buffer size is greater (or equal) to the size requested,
        /// it will NOT be modified (this function newer shrinks the buffer).
        /// Otherwise the buffer will be increased up to the size requested, if possible.
        /// If it cannot be increased to the requested size, it will be increased as much as possible.
        /// Default implementation always fails.
        /// @param [in] size The requested size (in bytes); Should be > 0.
        /// @return New send buffer size in bytes (even if it was not modified, can be larger than size requested);
        ///         -1 on error.
        virtual int increaseSndBufSize ( int size );

        /// @brief Return whether this listener is valid (i.e. is capable of sending/receiving data)
        /// @return True if this listener is valid
        virtual bool isValid() const = 0;

    protected:
        static TextLog _log; // < Log stream.

        SockAddr _localAddr; ///< Local address.

        UdpListenerSocket * _listeningSock; ///< Listening UDP socket

        /// @brief Map of connected sockets
        /// <remote IP/port, Socket object for this combination>
        /// References are not kept to objects in this map. UdpSockets are expected to unregister themself
        /// which will remove them from this map, before returning to pool.
        HashMap<SockAddr, UdpListenerSocket *> _connectedSocks;

        /// @brief Constructor
        UdpListener();

        /// @brief Destructor
        virtual ~UdpListener();

        /// @brief Called by UdpSocket to unregister itself from this Listener
        /// @param [in] sock Socket to unregister
        void unregisterUdpSocket ( UdpListenerSocket * sock );

        /// @brief Called by an implementation when data is received
        /// This calls the appropriate UdpSocket callback if it exists, otherwise the data is dropped
        /// @param [in,out] remote Remote origin of this packet; It will be modified (converted to IPv4 address)
        ///                        if it contains an IPv6-mapped-IPv4 address.
        /// @param [in] data Data that was received
        void dataReceived ( SockAddr & remote, MemHandle & data );

        /// @brief Called to notify all sockets that they are not valid anymore.
        /// This should only be called once after the listener is invalid (i.e. isValid() returns false)!
        void notifyClosed();

        /// @brief Sends the data to the given address and port - Implementation specific.
        /// @param [in] sa Remote address/port to send the data to.
        /// @param [in] data Pointer to the data to send.
        /// @param [in] dataSize The size of the data to send.
        /// @return Standard error code.
        ///         If Closed is returned, that means that the socket has been closed.
        ///         All other error codes mean the socket is still valid.
        ///         If this call resulted in the socket being closed, a udpSocketClosed callback
        ///         will be dispatched next end of loop to all sockets sharing the same listener.
        ///         Subsequent calls will still fail, but will not call the udpSocketClosed callback.
        virtual ERRCODE send ( const SockAddr & sa, const char * data, size_t dataSize ) = 0;

        /// @brief Sends the data to the given address and port - Implementation specific.
        /// @param [in] sa Remote address/port to send the data to.
        /// @param [in] data MemHandle with the data to send.
        /// @return Standard error code.
        ///         If Closed is returned, that means that the socket has been closed.
        ///         All other error codes mean the socket is still valid.
        ///         If this call resulted in the socket being closed, a udpSocketClosed callback
        ///         will be dispatched next end of loop to all sockets sharing the same listener.
        ///         Subsequent calls will still fail, but will not call the udpSocketClosed callback.
        virtual ERRCODE send ( const SockAddr & sa, MemHandle & data ) = 0;

        /// @brief Sends the data to the given address and port - Implementation specific.
        /// @param [in] sa Remote address/port to send the data to.
        /// @param [in] data MemVector with the data to send.
        /// @return Standard error code.
        ///         If Closed is returned, that means that the socket has been closed.
        ///         All other error codes mean the socket is still valid.
        ///         If this call resulted in the socket being closed, a udpSocketClosed callback
        ///         will be dispatched next end of loop to all sockets sharing the same listener.
        ///         Subsequent calls will still fail, but will not call the udpSocketClosed callback.
        virtual ERRCODE send ( const SockAddr & sa, MemVector & data ) = 0;

        /// @brief (Re)Initializes a specific implementation of UdpListener.
        /// This is only called when there are no listening or connected UDP sockets.
        /// @param [in] localAddr The local address and port to bind to.
        ///                        A "zero" address means "any" address.
        ///                        Zero port number means to dynamically allocate a port number.
        /// @return Standard error code.
        virtual ERRCODE reinitializeImpl ( const SockAddr & localAddr ) = 0;

    private:
        /// @brief Copy constructor - does not exist!
        UdpListener ( const UdpListener & other );

        /// @brief Assignment operator - does not exist!
        UdpListener & operator= ( const UdpListener & other );

        friend class UdpListenerSocket;
};

typedef UdpListener * UdpListenerPtr;
}
