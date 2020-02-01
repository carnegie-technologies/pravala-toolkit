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

#include "basic/MemVector.hpp"
#include "object/OwnedObject.hpp"
#include "error/Error.hpp"
#include "sys/SocketApi.hpp"
#include "log/TextLog.hpp"
#include "log/LogId.hpp"
#include "event/EventManager.hpp"

namespace Pravala
{
class Socket;
class IpSocket;
class LocalSocket;

/// @brief The owner of the Socket
class SocketOwner
{
    protected:
        /// @brief Called when data is received over a socket.
        /// @param [in] sock Pointer to the socket that received the data.
        /// @param [in] data The data received. If the receiver consumes that data,
        ///                  it should modify this object using, for example, data.consume() or data.clear().
        ///                  If the data is only partially consumed, this callback will be called again,
        ///                  with the same data object (with remaining data). It will be repeated until
        ///                  either the data is fully consumed, or when not even a single byte of it is consumed.
        ///                  If the data is not consumed, the behaviour depends on the socket type.
        ///
        ///                  Sockets that can lose data (like UDP), will simply drop the data.
        ///
        ///                  Lossless sockets (like TCP) will keep it around in internal buffer,
        ///                  which can be accessed using getReadBuffer(). They also may stop reading more data until
        ///                  pending data is fully consumed (using consumeReadBuffer() call).
        virtual void socketDataReceived ( Socket * sock, MemHandle & data ) = 0;

        /// @brief Called when the socket is closed.
        /// @param [in] sock Pointer to the socket that was closed.
        /// @param [in] reason An additional reason for the socket being closed.
        virtual void socketClosed ( Socket * sock, ERRCODE reason ) = 0;

        /// @brief Callback generated when the socket successfully connects.
        /// @param [in] sock Pointer to the socket that was connected.
        virtual void socketConnected ( Socket * sock ) = 0;

        /// @brief Callback for notifying the owner when the connection attempt failed.
        /// @note This callback is only generated if the failure happened before successfully establishing
        /// the connection. If the connection succeeds, socketConnected() is generated, followed
        /// by socketClosed() when it is disconnected.
        /// @param [in] sock Pointer to the socket that failed to connect.
        /// @param [in] reason An additional reason for the connection failing.
        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason ) = 0;

        /// @brief Callback for notifying the owner when the socket is ready to send data.
        /// @note This callback is only generated if previous send*() call failed due to insufficient buffer space.
        /// @param [in] sock The socket that is now ready to send the data again.
        virtual void socketReadyToSend ( Socket * sock ) = 0;

        /// @brief Callback for notifying the owner when the write size hint of a socket changes.
        /// Default implementation does nothing.
        /// @see getWriteSizeHint() for details.
        /// @param [in] sock The socket that generated the callback.
        /// @param [in] sizeHint The new size hint (in bytes); Could be 0.
        virtual void socketWriteSizeHintUpdated ( Socket * sock, size_t sizeHint );

        /// @brief Destructor.
        virtual ~SocketOwner();

        friend class Socket;
};

/// @brief An abstract socket that can be used to connect, send and receive data.
/// @note Passing Socket objects between threads is NOT SUPPORTED.
class Socket: public OwnedObject<SocketOwner>, public LogId, protected EventManager::LoopEndEventHandler
{
    public:
        /// @brief Closes the socket.
        /// No further callbacks will be generated.
        /// Safe to call on a socket that is already closed.
        /// Default implementation clears all the flags and scheduled events.
        /// @note Closing the socket typically clears the read buffer, so all unread data will be lost!
        virtual void close();

        /// @brief Sends the data over the socket.
        /// This API simply uses underlying socket's behaviour.
        /// If it is a stream socket (like TCP), it will write the data into the stream.
        /// If it is a datagram socket (like UDP), it will treat the data as a single datagram.
        /// @param [in,out] data Data to send.
        ///                      The data is consumed to reflect how much of it has actually been written,
        ///                      and it's possible that it won't be fully accepted. If not all of the data is sent,
        ///                      socketReadyToSend() callback will be generated once the socket is ready to accept
        ///                      more data. Note that packet-based sockets (like UDP) will either send all or none
        ///                      of the data, and either not modify or clear this object.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE send ( MemHandle & data ) = 0;

        /// @brief Sends the data over the socket.
        /// This API simply uses underlying socket's behaviour.
        /// If it is a stream socket (like TCP), it will write the data into the stream.
        /// If it is a datagram socket (like UDP), it will treat the data as a single datagram.
        /// @param [in,out] data Data to send.
        ///                      The data is consumed to reflect how much of it has actually been written,
        ///                      and it's possible that it won't be fully accepted. If not all of the data is sent,
        ///                      socketReadyToSend() callback will be generated once the socket is ready to accept
        ///                      more data. Note that packet-based sockets (like UDP) will either send all or none
        ///                      of the data, and either not modify or clear this object.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE send ( MemVector & data ) = 0;

        /// @brief Sends the data over the socket.
        /// This API simply uses underlying socket's behaviour.
        /// If it is a stream socket (like TCP), it will write the data into the stream.
        /// If it is a datagram socket (like UDP), it will treat the data as a single datagram.
        /// @param [in] data Pointer to the data to send.
        /// @param [in,out] dataSize The size of the data to send.
        ///                          On success it is set to the number of bytes sent (or queued),
        ///                          which may be less than the original size. If not all of the data is sent,
        ///                          socketReadyToSend() callback will be generated once the socket is ready to accept
        ///                          more data. Note that packet-based sockets (like UDP) will either send all or none
        ///                          of the data, so they never modify this value.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        virtual ERRCODE send ( const char * data, size_t & dataSize ) = 0;

        /// @brief Gets the data received over the socket without removing it from the receive buffer.
        /// Default implementation always returns an empty buffer.
        /// @return The read buffer (could be empty).
        virtual const MemHandle & getReadBuffer() const;

        /// @brief Consumes the data in the read buffer.
        /// This should be called to notify the socket that data returned by getReadBuffer() has been fully
        /// or partially consumed. If the socket's read is blocked, this may resume it.
        /// @param [in] size The number of consumed bytes from the buffer returned by getReadBuffer().
        virtual void consumeReadBuffer ( size_t size );

        /// @brief Gets the size of the data in the read buffer.
        /// @return The number of bytes of data in the read buffer.
        inline size_t getReadBufferSize() const
        {
            return getReadBuffer().size();
        }

        /// @brief Returns the size of a single write/send operation supported by the socket internally.
        /// It may not be supported (and return 0), and should be treated by the caller as a hint
        /// for an optimal size of a single packet. Even when socket returns a non-zero value,
        /// there is no guarantee that when sending data, this size of data will actually be accepted.
        /// Default implementation returns 0.
        /// @return The optimal size of a single data write for this socket (in bytes).
        ///         0 if unknown or not relevant.
        virtual size_t getWriteSizeHint() const;

        /// @brief Checks if this socket is valid.
        /// @return True if this socket is valid; False otherwise.
        inline bool isValid() const
        {
            return hasFlag ( SockFlagValid );
        }

        /// @brief Checks if this socket is connecting.
        /// @return True if this socket is connecting; False otherwise.
        inline bool isConnecting() const
        {
            return hasFlag ( SockFlagConnecting );
        }

        /// @brief Checks if this socket is connected.
        /// @note This should typically only used from the outside. Classes inheriting Socket should
        ///       check specific flags. Some sockets may require several steps to become fully connected.
        /// @return True if this socket is connected; False otherwise.
        inline bool isConnected() const
        {
            return hasFlag ( SockFlagConnected );
        }

        /// @brief This function returns the underlying file descriptor and removes it from the socket.
        /// This file descriptor will be unsubscribed from EventManager, and the socket will be no longer
        /// responsible for closing it. Not all socket types support this operation, in which case nothing
        /// will happen (and -1 will be returned). If this function succeeds, it will be equivalent
        /// to calling close() on the socket, except the actual file descriptor will not be closed.
        /// @note Successful call typically clears the read buffer, so all unread data will be lost!
        /// @return Underlying file descriptor, or -1 on error (or if the socket does not support this operation).
        virtual int stealSockFd();

        /// @brief Returns this object as an IpSocket.
        /// @return This object as an IpSocket pointer, or 0 if it is not an IpSocket.
        virtual IpSocket * getIpSocket();

        /// @brief Returns this object as a LocalSocket.
        /// @return This object as a LocalSocket pointer, or 0 if it is not a LocalSocket.
        virtual LocalSocket * getLocalSocket();

        /// @brief Returns the description of the remote endpoint.
        /// The default implementation returns an empty string.
        /// @return The description of the remote endpoint.
        virtual String getLocalDesc() const;

        /// @brief Returns the description of the local endpoint.
        /// The default implementation returns an empty string.
        /// @return The description of the local endpoint.
        virtual String getRemoteDesc() const;

        /// @brief Gets the value of a socket option.
        /// It is analogous to POSIX 'getsockopt' call and may not be supported.
        /// Default implementation always fails.
        /// @param [in] level The protocol level at which the option resides.
        /// @param [in] optName Specifies the option to get.
        /// @param [out] value Memory to store the value in.
        ///                    If not empty, the size of the memory will be used as expected option value size.
        ///                    Otherwise an automatic mode will be used.
        /// @return True on success, false on error.
        virtual bool getOption ( int level, int optName, MemHandle & value ) const;

        /// @brief Tries to increase receive buffer size of the socket.
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

        /// @brief Tries to increase send buffer size of the socket.
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

    protected:
        /// @brief Socket will be closed (using close()) and socket "closed" callback will be generated.
        /// This will NOT clear any flags! Instead, flags should be handled in close().
        static const uint16_t SockEventClosed = ( 1 << 0 );

        /// @brief Socket will be marked 'connected', and "connected" callback will be generated.
        /// It will set 'connected' and unset 'connecting' flag.
        static const uint16_t SockEventConnected = ( 1 << 1 );

        /// @brief Socket will be marked as not connected and not connecting.
        /// Also "failed to connect" callback will be generated.
        static const uint16_t SockEventConnectFailed = ( 1 << 2 );

        static const uint16_t SockFlagValid = ( 1 << 0 ); ///< Set when this socket is valid.
        static const uint16_t SockFlagConnecting = ( 1 << 1 ); ///< Set when this socket is connecting.
        static const uint16_t SockFlagConnected = ( 1 << 2 ); ///< Set when this socket is connected.

        /// @brief This flag is set if the send buffer was filled up and send() blocked.
        static const uint16_t SockFlagSendBlocked = ( 1 << 3 );

        /// @brief The lowest event bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockNextEventShift = 3;

        /// @brief The lowest flag bit that can be used by the class inheriting this one.
        /// Classes that inherit it should use ( 1 << next_shift + 0), ( 1 << next_shift + 1), etc. values.
        static const uint8_t SockNextFlagShift = 4;

        static TextLog _log; ///< Log stream.

        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        Socket ( SocketOwner * owner );

        /// @brief Destructor.
        virtual ~Socket();

        /// @brief Sets given flags.
        /// @param [in] flags The flags to set.
        inline void setFlags ( uint16_t flags )
        {
            _sockFlags |= flags;
        }

        /// @brief Clears given flags.
        /// @param [in] flags The flags to clear.
        inline void clearFlags ( uint16_t flags )
        {
            _sockFlags &= ~flags;
        }

        /// @brief Clears all flags.
        inline void clearAllFlags()
        {
            _sockFlags = 0;
        }

        /// @brief Checks if at least one of given socket flags is set.
        /// @param [in] flags Flags to check.
        /// @return True if at least one of the flags passed is set; False otherwise.
        inline bool hasFlag ( uint16_t flags ) const
        {
            return ( ( _sockFlags & flags ) != 0 );
        }

        /// @brief Returns a multi-bit flag value stored in socket flags.
        /// "Flag value" is a value stored in multiple flag bits, but interpreted as a number, not using
        /// individual bits. valueMask describes which bits of socket flags are be used.
        /// @param [in] valueMask The mask to use for the value.
        /// @return Value stored in flags using given mask.
        inline uint16_t getFlagValue ( uint16_t valueMask ) const
        {
            return ( _sockFlags & valueMask );
        }

        /// @brief Sets a multi-bit flag value to be stored in socket flags.
        /// "Flag value" is a value stored in multiple flag bits, but interpreted as a number, not using
        /// individual bits. 'valueMask' describes which bits of socket flags are be used.
        /// All the bits that are part of 'value' will be set, and all the bits that are part of 'valueMask',
        /// but not the 'value' will be cleared. getFlagValue() will return the exact value set.
        /// @param [in] value The value to set. No bits that are outside of 'valueMask' will be set.
        /// @param [in] valueMask The mask in flags to use.
        inline void setFlagValue ( uint16_t value, uint16_t valueMask )
        {
            _sockFlags = ( _sockFlags & ( ~valueMask ) ) | ( value & valueMask );
        }

        /// @brief Runs socket events.
        /// It may change socket flags, and generate some callback(s).
        /// The default implementation runs a single 'closed', 'connect failed', or 'connected' callback
        /// (it selects the first one, prioritized in this order).
        /// For error callbacks it will use either 'closed' or 'connect failed' error codes.
        /// If this returns 'true', the event was not 'closed' or 'connect failed' (so it was 'connected'),
        /// and there are some other events remaining, they will be re-scheduled.
        /// @param [in] events The events to run.
        /// @return True if the event has been handled; False otherwise.
        ///         If this returns 'true', other versions cannot access local state and should return right away,
        ///         since a callback might have been called by then.
        virtual bool runEvents ( uint16_t events );

        /// @brief Schedules given event (one or more of SockEvent*).
        /// If multiple events are scheduled at the same time, it's up to runEvents() to select
        /// the event (or events) to run, and ignore or re-schedule additional events.
        /// @param [in] events The event(s) to schedule.
        void scheduleEvents ( uint16_t events );

        /// @brief Checks if the given socket event is scheduled.
        /// @param [in] events The event(s) to check.
        /// @return True if at least one of the events passed is scheduled.
        inline bool isEventScheduled ( uint16_t events ) const
        {
            return ( ( _sockEvents & events ) != 0 );
        }

        /// @brief Clears passed events.
        /// @param [in] events Event (or events) to clear.
        inline void clearEvents ( uint16_t events )
        {
            _sockEvents &= ~events;
        }

        /// @brief Clears all scheduled events.
        inline void clearAllEvents()
        {
            _sockEvents = 0;
        }

        /// @brief Helper function that sends memory vector one chunk at a time.
        /// It can be used by stream sockets to send the entire memory vector, one chunk at a time.
        /// It will keep sending until there is an error, or a partial write.
        /// @note It does not make sense for datagram sockets, as each chunk would result in a separate message.
        /// @param [in,out] data Data to send.
        ///                      The data is consumed to reflect how much of it has actually been written,
        ///                      and it's possible that it won't be fully accepted. If not all of the data is sent,
        ///                      socketReadyToSend() callback will be generated once the socket is ready to accept
        ///                      more data.
        /// @return Standard error code. If there is an error resulting in socket being closed,
        ///         a socketClosed() callback will be generated at the end of the event loop.
        ///         That callback is generated only once, when the socket becomes closed.
        ///         If this method is called on a socket that is already closed,
        ///         no additional socketClosed() callbacks will be generated.
        ERRCODE streamSend ( MemVector & data );

        /// @brief A helper function that initializes socket's FD.
        /// It does nothing if the socket is already initialized.
        /// If a new socket is created, this function will also enable non-blocking mode on it.
        /// It also sets 'valid' flag if the socket is initialized successfully.
        /// @param [in] sockType The type of the socket to initialize.
        /// @param [in,out] sockFd The FD of the socket to initialize. It is not modified if it's already >= 0.
        /// @return True if the socket has been initialized properly (or was already initialized);
        ///         False if we failed to generate a new socket file descriptor or enable non-blocking mode.
        virtual bool sockInitFd ( SocketApi::SocketType sockType, int & sockFd );

        /// @brief Immediately sends a 'data received' callback.
        /// It will hold a temporary reference and keep calling the callback until all the data is accepted,
        /// or the owner stops accepting the data (or it goes away).
        /// See SocketOwner for detailed description.
        /// @param [in] data Data received.
        virtual void doSockDataReceived ( MemHandle & data );

        /// @brief Helper function that calls 'data received' callback in the owner (if it's valid).
        /// @param [in] data Data received.
        inline void callSockDataReceived ( MemHandle & data )
        {
            if ( getOwner() != 0 )
            {
                getOwner()->socketDataReceived ( this, data );
            }
        }

        /// @brief Closes the socket (using close()) and immediately sends a 'closed' callback.
        /// See SocketOwner for detailed description.
        /// @param [in] reason Reason code.
        virtual void doSockClosed ( ERRCODE reason );

        /// @brief Sets 'connected' flag, clears 'connecting' flag, and immediately sends a 'connected' callback.
        /// See SocketOwner for detailed description.
        virtual void doSockConnected();

        /// @brief Clears 'connected' and 'connecting' flags, and immediately sends a 'connect failed' callback.
        /// See SocketOwner for detailed description.
        /// @param [in] reason Reason code.
        virtual void doSockConnectFailed ( ERRCODE reason );

        /// @brief Clears 'send blocked' flag and immediately sends a 'ready to send' callback.
        /// See SocketOwner for detailed description.
        virtual void doSockReadyToSend();

        /// @brief Sends 'write size hint updated' callback.
        /// @param [in] sizeHint The new size hint (in bytes); Could be 0.
        virtual void doSockWriteSizeHintUpdated ( size_t sizeHint );

        virtual void receiveLoopEndEvent();

    private:
        uint16_t _sockFlags;  ///< Socket flags.
        uint16_t _sockEvents; ///< Pending socket events.
};
}
