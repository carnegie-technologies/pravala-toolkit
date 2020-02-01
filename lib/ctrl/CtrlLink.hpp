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

#include "basic/List.hpp"
#include "basic/HashMap.hpp"
#include "basic/IpAddress.hpp"
#include "basic/MsvcSupport.hpp"

#include "proto/ExtProtoError.hpp"

#include "event/Timer.hpp"
#include "log/TextLog.hpp"
#include "event/EventManager.hpp"
#include "log/LogOutput.hpp"

#include "auto/ctrl/Ctrl/Update.hpp"
#include "auto/ctrl/Ctrl/Request.hpp"

/// @brief A tricky macro to simplify ctrl message processing
///
/// It converts, for example, "CASE_CTRL_MSG_TYPE ( SetupSsl );" into:
///
/// case Ctrl::SetupSsl::DEF_TYPE:
///   {
///     ExtProtoError eErr;
///     Ctrl::SetupSsl tmpMsg;
///
///     ERRCODE tmpErrCode = tmpMsg.deserialize ( msg, &eErr );
///
///     LOG_ERR (L_DEBUG2, tmpErrCode, "Received Ctrl::SetupSsl packet. Trying to deserialize");
///
///     if ( NOT_OK ( tmpErrCode) )
///     {
///         LOG_ERR (L_ERROR, tmpErrCode, "Received Ctrl::SetupSsl packet. Deserializing FAILED: '"
///                  << EXT_ERROR_DESC ( eErr ) << "'" );
///         return tmpErrCode;
///     }
///
///     tmpErrCode = handleCtrlMessage ( linkId, tmpMsg );
///
///     LOG_ERR (IS_OK(tmpErrCode)||tmpErrCode==Error::ResponseSent)?L_DEBUG2:L_ERROR, tmpErrCode,
///              "Called handleCtrlMessage ( Ctrl::SetupSsl ) method" );
///
///     return tmpErrCode;
///   }
/// break;
///
/// This way we have WAY less to write!
///
/// Note that it uses the SAME argument names as the ctrlPacketReceived() callback: linkId and msg
///
#define CASE_CTRL_MSG_TYPE( msg_type )  \
    case Ctrl::msg_type::DEF_TYPE: \
        { \
            ExtProtoError eErr; Ctrl::msg_type tmpMsg; ERRCODE tmpErrCode = tmpMsg.deserialize ( msg, &eErr ); \
            LOG_ERR ( L_DEBUG2, tmpErrCode, "Received Ctrl::" #msg_type " packet. Trying to deserialize" ); \
            if ( NOT_OK ( tmpErrCode ) ) { \
                LOG_ERR ( L_ERROR, tmpErrCode, "Received Ctrl::" #msg_type " packet. Deserializing FAILED: '" \
                          << eErr.toString() << "'" ); \
                return tmpErrCode; } \
            tmpErrCode = handleCtrlMessage ( linkId, tmpMsg ); \
            LOG_ERR ( ( IS_OK ( tmpErrCode ) \
                        || tmpErrCode == Error::ResponseSent \
                        || tmpErrCode == Error::ResponsePending \
                      ) ? L_DEBUG2 : L_ERROR, \
                      tmpErrCode, \
                      "Called handleCtrlMessage ( Ctrl::" #msg_type " ) method" ); \
            return tmpErrCode; } \
                               break

/// @brief A version of the macro that also passes receivedFds
#define CASE_CTRL_MSG_TYPE_FDS( msg_type )  \
    case Ctrl::msg_type::DEF_TYPE: \
        { \
            ExtProtoError eErr; Ctrl::msg_type tmpMsg; ERRCODE tmpErrCode = tmpMsg.deserialize ( msg, &eErr ); \
            LOG_ERR ( L_DEBUG2, tmpErrCode, "Received Ctrl::" #msg_type " packet (and " \
                      << receivedFds.size() << " FDs). Trying to deserialize" ); \
            if ( NOT_OK ( tmpErrCode ) ) { \
                LOG_ERR ( L_ERROR, tmpErrCode, "Received Ctrl::" #msg_type " packet. Deserializing FAILED: '" \
                          << eErr.toString() << "'" ); \
                return tmpErrCode; } \
            tmpErrCode = handleCtrlMessage ( linkId, tmpMsg, receivedFds ); \
            LOG_ERR ( ( IS_OK ( tmpErrCode ) \
                        || tmpErrCode == Error::ResponseSent \
                        || tmpErrCode == Error::ResponsePending \
                      ) ? L_DEBUG2 : L_ERROR, tmpErrCode, \
                      "Called handleCtrlMessage ( Ctrl::" #msg_type " ) method" ); \
            return tmpErrCode; } \
                               break

namespace Pravala
{
namespace Ctrl
{
class Ping;
class Pong;
class GetLogList;
class LogSubscribe;
class LogUnsubscribe;
class GetDiagnostics;
class SimpleSubscriptionResponse;
class LoadConfig;
}

class CtrlSubHandler;

/// @brief A control link
class CtrlLink:
    public EventManager::FdEventHandler,
    public EventManager::LoopEndEventHandler,
    public Timer::Receiver,
    public BinLogOutput
{
    public:
        /// @brief The owner of the CtrlLink
        /// Not in the "owned object" sense. It will be used for delivering callbacks.
        class Owner
        {
            protected:
                /// @brief Callback for notifying the owner on incoming control packets
                /// @param [in] linkId The ID of the link that generated the callback
                /// @param [in] msg The base control message received
                /// @param [in] receivedFds The list of file descriptors received. If the owner "consumes" the
                ///                          descriptor in some way, it should remove it from the list, or set to -1.
                ///                          Otherwise it will be automatically closed after this callback.
                /// @return The error code to be returned to the sender of the original message using
                ///          'SimpleResponse' message. If 'success' is returned, the response message is sent
                ///          only if the original control message has the 'request_response' bit set,
                ///          in which case the 'success' message is generated. To prevent the link
                ///          from generating the response the 'ResponseSent' error code should be returned.
                ///          Alternatively, a 'ResponsePending' error code can be returned, if a response will be
                ///          sent later. At that point it is this object's responsibility to eventually send
                ///          a response, taking into account things like whether 'request response' bit was set.
                virtual ERRCODE ctrlPacketReceived ( int linkId, Ctrl::Message & msg, List<int> & receivedFds ) = 0;

                /// @brief Callback for notifying the owner on incoming 'SimpleSubscriptionResponse' packets.
                /// The default implementation will call ctrlPacketReceived() for every update stored inside
                /// the SimpleSubscriptionResponse message. If a particular owner wants to handle
                /// that response differently, it can override this function.
                /// @param [in] linkId The ID of the link that generated the callback
                /// @param [in] msg The SimpleSubscriptionResponse message received.
                /// @param [in] receivedFds The list of file descriptors received. If the owner "consumes" the
                ///                          descriptor in some way, it should remove it from the list, or set to -1.
                ///                          Otherwise it will be automatically closed after this callback.
                ///                          The default implementation will keep passing the same list to
                ///                          ctrlPacketReceived(). If it gets modified, the following
                ///                          ctrlPacketReceived() will get modified version.
                /// @return The standard error code. Note that since this is an 'update' message,
                ///          the code doesn't really matter - the response will not be generated anyway.
                ///          This code is only used for logging. The standard implementation returns, in order:
                ///           - the first error code returned by any of ctrlPacketReceived() calls.
                ///           - "ResponseSent" if returned by any of ctrlPacketReceived() calls.
                ///           - "ResponsePending" if returned by any of ctrlPacketReceived() calls.
                ///           - Success.
                virtual ERRCODE ctrlSubscriptionResponseReceived (
                    int linkId, Ctrl::SimpleSubscriptionResponse & msg, List<int> & receivedFds );

                /// @brief Callback for notifying the owner about closed ctrl links
                /// @param [in] linkId The ID of the link that generated the callback
                virtual void ctrlLinkClosed ( int linkId ) = 0;

                /// @brief Destructor
                virtual ~Owner();

            private:
                /// @brief The CtrlSubHandler objects responsible for specific subscription IDs.
                HashMap<uint32_t, CtrlSubHandler * > _subHandlers;

                /// @brief Called by the CtrlSubHandler when it is created.
                /// This will register this handler.
                /// Handler will be deleted once it is not needed if it's AutoDelete property is set to true.
                /// @param [in] subId The ID of the subscription request this handler is responsible for.
                ///                   If there already is another handler that uses the same ID, it will be removed.
                /// @param [in] handler The pointer to the handler. Could be 0.
                void ctrlAddSubHandler ( uint32_t subId, CtrlSubHandler * handler );

                /// @brief Called by the CtrlSubHandler when it is deleted.
                /// It unregister the handler IF it is currently registered for the given ID.
                /// This does NOT delete the handler.
                /// @param [in] subId The ID of the subscription request this handler is responsible for.
                /// @param [in] handler The pointer to the handler.
                void ctrlRemoveSubHandler ( uint32_t subId, CtrlSubHandler * handler );

                friend class CtrlLink;
                friend class CtrlLinkConnector;
                friend class CtrlSubHandler;
        };

        const int LinkId; ///< The ID of the link

        /// @brief Constructor
        /// @param [in] owner The owner of this CtrlLink object (not in an owned-object sense,
        ///                    will just receive the callbacks)
        /// @param [in] linkId The ID of the link that sent the packet
        CtrlLink ( Owner & owner, int linkId );

        /// @brief Destructor
        virtual ~CtrlLink();

        /// @brief Checks if this control link is connected.
        /// @return True if this link is already connected; False otherwise.
        inline bool isConnected() const
        {
            return ( _linkFd >= 0 );
        }

        /// @brief Returns local address of underlying TCP connection.
        /// @return Local address of underlying TCP connection, or invalid address if it's not a TCP connection,
        ///          or if the address was not configured.
        inline const IpAddress & getLocalAddress() const
        {
            return _localAddr;
        }

        /// @brief Returns local port number of underlying TCP connection.
        /// @return Local port number of underlying TCP connection, or 0 if it's not a TCP connection,
        ///          or if the port number was not configured.
        inline uint16_t getLocalPort() const
        {
            return _localPort;
        }

        /// @brief Returns remote address of underlying TCP connection.
        /// @return Remote address of underlying TCP connection, or invalid address if it's not a TCP connection,
        ///          or if the address was not configured.
        inline const IpAddress & getRemoteAddress() const
        {
            return _remoteAddr;
        }

        /// @brief Returns remote port number of underlying TCP connection.
        /// @return Remote port number of underlying TCP connection, or 0 if it's not a TCP connection,
        ///          or if the port number was not configured.
        inline uint16_t getRemotePort() const
        {
            return _remotePort;
        }

        /// @brief Returns the name of underlying local socket.
        /// @return The name of underlying local socket, or empty string if it's not a local socket,
        ///          or the name was not configured.
        inline const String & getSockName() const
        {
            return _localSockName;
        }

        /// @brief Returns the internal file descriptor and unsets it in the link object.
        /// After this it is caller's responsibility to close that descriptor.
        /// It will also remove this link from all control subscription handlers
        /// and close all file descriptors received over the control channel.
        /// It behaves like calling setup(-1) followed by cleanupFds(), except the underlying FD will not
        /// be closed but returned instead.
        /// @return Internal file descriptor; Could be -1 if link was not connected.
        int takeFd();

        /// @brief Configures this link object.
        /// It also clears local and remote addresses, ports and the socket name.
        /// @param [in] linkFd The file descriptor to use. If this link already has a file descriptor set
        ///                     (and it's different) it will be closed using the event manager.
        ///                    If it's invalid (<0), the link will be closed.
        void setup ( int linkFd );

        /// @brief Configures this link object.
        /// It also clears the socket name.
        /// @param [in] linkFd The file descriptor to use. If this link already has a file descriptor set
        ///                     (and it's different) it will be closed using the event manager.
        ///                    If it's invalid (<0), the link will be closed.
        /// @param [in] localAddr Address on which the connection was received
        /// @param [in] localPort Port number on which the connection was received
        /// @param [in] remoteAddr Remote host's address
        /// @param [in] remotePort Remote host's port number
        void setup (
            int linkFd,
            const IpAddress & localAddr,
            uint16_t localPort,
            const IpAddress & remoteAddr,
            uint16_t remotePort );

        /// @brief Configures this link object.
        /// It also clears local and remote addresses.
        /// @param [in] linkFd The file descriptor to use. If this link already has a file descriptor set
        ///                     (and it's different) it will be closed using the event manager.
        ///                    If it's invalid (<0), the link will be closed.
        /// @param [in] sockName The name of the local socket.
        void setup ( int linkFd, const String & sockName );

        /// @brief Adds a packet to internal list of packets to write to the specific client
        ///
        /// @param [in] packet The packet to send.
        /// @param [in] atLoopEnd If set to true (by default it's false), the packet will be added to
        ///                        another send queue, which will be appended to the end of the regular queue
        ///                        at the end of the event loop. This can be used to send various status update
        ///                        notifications that should be delivered AFTER the return code from
        ///                        the function that sends that update.
        /// @return Standard error code.
        ERRCODE sendPacket ( Ctrl::Message & packet, bool atLoopEnd = false );

        /// @brief Sends a control response
        ///
        /// @param [in] resp The actual response message
        /// @param [in] request The request message that we are responding to.
        /// @param [in] respErrCode The 'response error code' that should be put in the response. Success by default.
        /// @return Always returns ResponseSent, whether there was an error or not. This means it can be used
        ///          for returning from rcvCtrlPacket() callback.
        ERRCODE sendResponse (
            Ctrl::Update & resp, const Ctrl::Request & request,
            ERRCODE respErrCode = Error::Success );

        /// @brief Sends a control ping message.
        /// @param [in] payload Payload to include in the message (if not empty).
        /// @return Standard error code.
        ERRCODE sendPing ( const String & payload = String::EmptyString );

        /// @brief Tries to read from this link
        ///
        /// It is normally only used by receiveFdEvent when 'read' event is received,
        /// but can be used from outside to force the link to perform a read.
        ///
        /// @note This function can generate both ctrlPacketReceived() and ctrlLinkClosed() callbacks!
        ///
        /// @return True if there were no errors, false if there was an error or the link was closed.
        ///         When it is false, the ctrlLinkClosed() callback should have been generated.
        bool doCtrlRead();

        /// @brief Closes the link.
        /// It will close and clear the descriptors and stop the timers.
        virtual void close();

        /// @brief Serializes packet and appends it to MemHandle provided
        /// @param [in] packet Packet to serialize
        /// @param [out] memHandle MemHandle to serialize the packet to
        /// @return Standard error code
        static ERRCODE serializePacket ( Ctrl::Message & packet, MemHandle & memHandle );

    protected:
        static TextLog _log; ///< Log stream

        int _linkFd; ///< The FD to be used by the link

        Owner & _owner; ///< This link's owner
        SimpleTimer _fdCleanupTimer; ///< Timer used for cleaning up file descriptors

        List<MemHandle> _writeQueue; ///< The data to write
        List<MemHandle> _loopEndWriteQueue; ///< The data to write at the end of the event loop
        Buffer _readBuffer; ///< The data that has been read, but not yet consumed
        List<int> _readFds; ///< The FDs read so far

        IpAddress _localAddr; ///< Local address of the underlying TCP connection, or invalid if it's a local socket.
        IpAddress _remoteAddr; ///< Remote address of the underlying TCP connection, or invalid if it's a local socket.

        String _localSockName; ///< The name of the local socket, or empty if it's a TCP socket.

        uint16_t _localPort; ///< Local port number of the underlying TCP connection, or 0 if it's a local socket.
        uint16_t _remotePort; ///< Remote port number of the underlying TCP connection, or 0 if it's a local socket.

        virtual void timerExpired ( Timer * timer );
        virtual void receiveFdEvent ( int fd, short int events );
        virtual void receiveLoopEndEvent();
        virtual void sendBinLog ( Log::LogMessage & logMessage );

        /// @brief Closes the descriptors still stored in readFds and clears the list
        void cleanupFds();

        /// @brief Performs the actual read
        /// @return True if the read was successful (even if it didn't read a complete message yet);
        ///         False if there was an error and the link should be closed.
        bool internalRead();

        /// @brief Performs the OS-specific read operations
        /// @param [out] rcvdFds The number of FDs read
        /// @param [out] nonFatal Set to true if despite returning < 0 it is not an error (read would block)
        /// @return Returns the number of bytes read.
        ssize_t osInternalRead ( int & rcvdFds, bool & nonFatal );

        /// @brief Adds data to internal list of packets to write to the specific client
        ///
        /// @param [in] packet The original packet that was serialized to 'data'. Not actually
        ///                    serialized (only the 'data' is sent), but used for debugging.
        /// @param [in] data Data to send.
        /// @param [in] atLoopEnd If set to true (by default it's false), the packet will be added to
        ///                        another send queue, which will be appended to the end of the regular queue
        ///                        at the end of the event loop. This can be used to send various status update
        ///                        notifications that should be delivered AFTER the return code from
        ///                        the function that sends that update.
        void sendData ( Ctrl::Message & packet, MemHandle & data, bool atLoopEnd = false );

        /// @brief Called when the link is closed.
        /// Performs cleanup using close() and calls ctrlLinkClosed() callback.
        virtual void linkClosed();

        /// @brief A helper function for handling built-in requests
        /// @param [in] msg Ctrl message to process
        /// @param [in] receivedFds The list of file descriptors received with the message
        /// @return Standard error code
        ERRCODE processBuiltInMessage ( const Ctrl::Message & msg, List<int> & receivedFds );

        /// @brief Processes a 'Ping' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::Ping & msg );

        /// @brief Processes a 'Pong' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::Pong & msg );

        /// @brief Processes a 'GetLogList' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::GetLogList & msg );

        /// @brief Processes a 'LogSubscribe' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::LogSubscribe & msg );

        /// @brief Processes a 'GetDiagnostics' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::GetDiagnostics & msg );

        /// @brief Processes a 'LogUnsubscribe' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::LogUnsubscribe & msg );

        /// @brief Processes a 'LoadConfig' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        ERRCODE handleCtrlMessage ( int linkId, const Ctrl::LoadConfig & msg );

        /// @brief Processes a 'SimpleSubscriptionResponse' message
        ///
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @param [in] receivedFds The list of file descriptors received with the message
        /// @return Standard error code
        ERRCODE handleCtrlMessage (
            int linkId, Ctrl::SimpleSubscriptionResponse & msg,
            List<int> & receivedFds );

        /// @brief Processes a 'Pong' message
        ///
        /// @note This version doesn't do anything...
        /// @param [in] linkId The ID of the link over which this message was received.
        /// @param [in] msg The actual message
        /// @return Standard error code
        virtual void processPong ( int linkId, const Ctrl::Pong & msg );

        friend class CtrlServer;
};
}
