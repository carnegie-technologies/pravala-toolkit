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

#include <cerrno>

#include "basic/MemHandle.hpp"

#include "LwipSocket.hpp"
#include "LwipBufferIterator.hpp"

struct tcp_pcb;
struct pbuf;

namespace Pravala
{
/// @brief Wrapper class for an lwIP TCP socket
class LwipTcpSocket: public LwipSocket
{
    public:
        /// @brief Class that should be inherited by classes receiving data and events from an LwipTcpSocket
        class Receiver
        {
            protected:
                /// @brief Called when a TCP socket receives data
                /// @note It is not safe to destroy the TCP socket in this callback.
                /// @param [in] sock The LwipTcpSocket that received the data
                /// @param [in] iter The iterator to the data received
                virtual void lwipTcpSocketReceivedData ( LwipTcpSocket * sock, const LwipBufferIterator & iter ) = 0;

                /// @brief Called when a TCP socket is connected
                /// @note It is not safe to destroy the TCP socket in this callback.
                /// @param [in] sock The LwipTcpSocket that is connected
                virtual void lwipTcpSocketConnected ( LwipTcpSocket * sock ) = 0;

                /// @brief Called when a TCP socket is disconnected for any reason
                /// @param [in] sock The LwipTcpSocket that is disconnected
                /// @param [in] error The errno code: reason for the disconnect, 0 if closed normally
                virtual void lwipTcpSocketDisconnected ( LwipTcpSocket * sock, int32_t error ) = 0;

                /// @brief Called when a TCP socket can send more data
                /// @param [in] sock The LwipTcpSocket that can send more data
                /// @param [in] maxSendSize The maximum number of bytes that can be sent
                virtual void lwipTcpSocketMaxSendSizeIncreased ( LwipTcpSocket * sock, size_t maxSendSize ) = 0;

                /// @brief Destructor.
                virtual ~Receiver()
                {
                }

                friend class LwipTcpSocket;
        };

        /// @brief Constructor
        /// @param [in] receiver The receiver object that will be called to receive TCP data and events
        LwipTcpSocket ( Receiver & receiver );

        /// @brief Destructor
        ~LwipTcpSocket();

        /// @brief Close the TCP socket.
        /// Once the socket is closed:
        ///   - all socket operations will return an error
        ///   - getTag will always return 0, and setTag won't do anything
        ///   - the IpV4Only and IpV6Only flags are still valid
        /// @note This does NOT call lwipTcpSocketDisconnected.
        inline void close()
        {
            closeOrAbort();
        }

        /// @brief Get the errno code for why the socket disconnected or failed to connect.
        /// This will be zero if close() is called to close the socket.
        /// @return The errno code for why the socket disconnected or failed to connect.
        inline int32_t getLastError() const
        {
            return _lastError;
        }

        /// @brief Get the maximum number of bytes that can be sent.
        /// Attempts to call send with more bytes than this number will cause send to return an error.
        /// @return The maximum number of bytes that can be sent. 0 if the socket is closed.
        size_t getMaxSendSize() const;

        /// @brief Send data to the remote connected address. Returns an error if the socket is not connected.
        /// @param [in] data The data to send
        /// @param [in] sockAddr This parameter is ignored
        /// @return 0 on success, otherwise returns the errno code indicating the failure reason
        virtual int32_t sendTo ( const MemHandle & data, const SockAddr & sockAddr );

        virtual int32_t getOption ( int32_t level, int32_t optName, MemHandle & optVal );
        virtual int32_t setOption ( int32_t level, int32_t optName, const MemHandle & optVal );
        virtual int32_t bind ( const SockAddr & address );
        virtual int32_t connect ( const SockAddr & address );
        virtual int32_t send ( const MemHandle & data );
        virtual LwipTcpSocket * getTCP();
        virtual const char * getTypeName() const;

    private:
        /// @brief Receives callbacks from this LwipTcpSocket
        Receiver & _receiver;

        /// @brief Queue of data payloads that have been sent but not yet acknowledged.
        /// When we receive a callback from lwIP indicating the length of data acknowledged,
        /// we will consume that length of data from the beginning of the queue.
        ///
        /// @note We must maintain a const ref to the data payload; lwIP expects that the data
        /// will not change until it has been sent and acknowledged by the remote side.
        ///
        /// To achieve this, we create a copy of the MemHandle that holds a reference to the data
        /// payload and append it to _sentQueue. MemHandle guarantees that as long as we don't try
        /// to modify our copy, the memory referenced by it won't be modified and won't be moved.
        ///
        /// To guarantee our copy of the data remains unmodified, we must ABSOLUTELY NOT call
        /// any MemHandle methods that could modify the memory.
        ///
        /// However it is still safe to call some non-const methods, such as consume(), that
        /// will modify the MemHandle object, but won't change or move the memory referenced by
        /// the MemHandle.
        List<MemHandle> _sentQueue;

        /// @brief Pointer to the lwIP TCP socket, 0 if the socket is closed
        /// It is stored in a union, to be able to treat it as ip_pcb pointer.
        /// We can do that, because beginning of tcp_pcb is exactly the same as the entire ip_pcb.
        /// In C++ tcp_pcb would inherit ip_pcb, but this is C code..
        union
        {
            ip_pcb * ip; ///< Pointer to lwIP socket, as ip_pcb.
            tcp_pcb * tcp; ///< Pointer to lwIP socket, as tcp_pcb.
        } _lwipSock;

        /// @brief The errno code set when the socket is unexpectedly disconnected, or connect fails
        int32_t _lastError;

        /// @brief Tries to close the TCP socket. If close fails, then aborts the socket instead. Then sets _tcp = 0.
        /// @return ERR_OK if the connection was closed properly, ERR_ABRT if the connection was aborted.
        err_t closeOrAbort();

        /// @brief Called when the TCP socket is connected, and its receiver should be notified.
        ///
        /// @return An lwIP error code indicating how the connect event was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        err_t connectedEvent();

        /// @brief Called when the TCP socket is disconnected, and its receiver should be notified.
        ///
        /// @param error The error code indicating why the socket was disconnected, ERR_OK if closed normally.
        ///
        /// @return An lwIP error code indicating how the disconnect event was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: if we successfully called tcp_close to close the socket.
        err_t disconnectEvent ( err_t error );

        /// @brief Called when the TCP socket has received some data, and its receiver should be notified.
        ///
        /// @param buffer The received data buffer.
        ///               This can be null, which means the connection was closed normally,
        ///                 in this case we will call disconnectEvent(ERR_OK) instead.
        ///
        /// @param error The lwIP error code indicating if there was an error receiving data.
        ///              All errors are treated as fatal, and we will call disconnectEvent instead.
        ///
        /// @return An lwIP error code indicating how the read event was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        err_t readEvent ( pbuf * buffer, err_t error );

        /// @brief Called when the TCP socket has consumed some sent data, and it should be freed from the _sentQueue.
        /// @param len The length of data that has been consumed.
        /// @return This always returns ERR_OK.
        err_t consumedEvent ( uint16_t len );

        /// @brief Get the TCP_INFO for this socket.
        /// @note Only implemented on Linux.
        /// @return A MemHandle containing a struct tcp_info on success, or an empty handle on failure.
        MemHandle getTcpInfo();

        /// @brief Called by lwIP when the TCP connection is connected to the remote side after a tcp_connect().
        /// This is an lwIP callback.
        ///
        /// @param arg Pointer to the LwipTcpSocket that should receive this event
        /// @param tcp The lwIP TCP socket which is connected
        /// @param error The lwIP error code, this is currently always ERR_OK
        ///
        /// @return An lwIP error code indicating how this callback was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        static err_t connectedFunc ( void * arg, tcp_pcb * tcp, err_t error );

        /// @brief Called by lwIP when the TCP connection was unexpectedly closed for any reason.
        /// This is an lwIP callback.
        ///
        /// @note The corresponding lwIP TCP socket is already freed when this callback is called!
        ///
        /// @note This will never be called by us, because we always disable callbacks before calling tcp_abort.
        ///
        /// @param arg Pointer to the LwipTcpSocket that should receive this event
        ///
        /// @param error The lwIP error code indicating why the connection was closed:
        ///              ERR_ABRT: aborted by lwIP's TCP stack or by tcp_abort if we didn't
        ///                        disable callbacks before calling it.
        ///              ERR_RST: the connection was reset by the remote host
        static void errorFunc ( void * arg, err_t error );

        /// @brief Called by lwIP when TCP data has been received.
        /// This is an lwIP callback.
        ///
        /// @param arg Pointer to the LwipTcpSocket that should receive this event
        /// @param tcp The lwIP TCP socket which received data
        ///
        /// @param buffer The received data buffer.
        ///               This can be null, which means the connection was closed normally,
        ///                 in this case we will call disconnectEvent(ERR_OK) instead.
        ///
        /// @param error The lwIP error code indicating if there was an error receiving data.
        ///              All errors are treated as fatal, and we will call disconnectEvent instead.
        ///
        /// @return An lwIP error code indicating how this callback was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        static err_t recvFunc ( void * arg, tcp_pcb * tcp, pbuf * buffer, err_t error );

        /// @brief Called by lwIP when sent TCP data has been acknowledged.
        /// This is an lwIP callback.
        ///
        /// @param arg Pointer to the LwipTcpSocket that should receive this event
        /// @param tcp The lwIP TCP socket which sent the data
        /// @param len The number of bytes of data that have been acknowledged
        ///
        /// @return An lwIP error code indicating how this callback was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        static err_t ackedFunc ( void * arg, tcp_pcb * tcp, uint16_t len );

        /// @brief Called by lwIP when sent TCP data has been acknowledged.
        /// This is an lwIP callback.
        /// @note This callbacks is used by TCP sockets that are being closed.
        ///       There is no LwipTcpSocket object anymore, and all that remains is tcp_pcb object inside lwip
        ///       (that object will be eventually destroyed by lwip itself), and memory associated with it
        ///       in a List of MemHandle objects.
        ///       As the data is being sent out, this callback will be called to release that memory.
        /// @param arg Pointer to a list of MemHandle objects with the memory associated with the tcp_pcb object.
        /// @param tcp The lwIP TCP socket which sent the data
        /// @param len The number of bytes of data that have been acknowledged
        ///
        /// @return An lwIP error code indicating how this callback was handled:
        ///         ERR_ABRT: if we called tcp_abort to close the socket.
        ///                   This is a special case for lwIP because the socket is freed in the callback.
        ///         ERR_OK: in all other cases.
        static err_t closedAckedFunc ( void * arg, tcp_pcb * tcp, uint16_t len );

        /// @brief Called by lwIP when the TCP connection was unexpectedly closed for any reason.
        /// This is an lwIP callback.
        /// @note This callback is used by TCP sockets that are being closed.
        ///       There is no LwipTcpSocket object anymore, and all that remains is tcp_pcb object inside lwip
        ///       and memory associated with it in a List of MemHandle objects. This callback
        ///       is called when tcp_pcb is already destroyed, so all it does is destroy the memory.
        /// @param arg Pointer to a list of MemHandle objects with the memory associated with the tcp_pcb object.
        /// @param error The lwIP error code indicating why the connection was closed.
        static void closedErrorFunc ( void * arg, err_t error );
};
}
