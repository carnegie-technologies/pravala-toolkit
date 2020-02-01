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
#include "CtrlLink.hpp"
#include "socket/TcpServer.hpp"

namespace Pravala
{
/// @brief Control Server
/// It opens listening sockets, waits for incoming control connections and creates Ctrl links
class CtrlServer: public NoCopy, public CtrlLink::Owner
{
    public:
        /// @brief Constructor
        CtrlServer();

        /// @brief Destructor
        virtual ~CtrlServer();

        /// @brief Creates a listener
        /// @param [in] localAddr Local address to listen on
        /// @param [in] localPort Local port to listen on
        /// @return Standard error code
        ERRCODE ctrlAddListener ( const IpAddress & localAddr, uint16_t localPort );

        /// @brief Creates a listener
        /// @param [in] sockName The name of the socket to use. If it "looks like" ip_addr:port_number
        ///                      and can be converted to a valid IP address and port number, this function
        ///                      uses the other ctrlAddListener function and creates an IP listener.
        /// @return Standard error code
        ERRCODE ctrlAddListener ( const String & sockName );

        /// @brief Adds a new, already connected, control client
        /// @param [in] linkFd The already connected file descriptor to be used by this link.
        ///                    If this function succeeds but there is an error later, this file descriptor
        ///                    will be closed and ctrlLinkClosed() callback will be generated.
        ///                    This linkFd is also this client's 'Client ID' and will be passed in future callbacks.
        /// @return Standard error code
        ERRCODE ctrlAddClient ( int linkFd );

        /// @brief Generates a new CtrlLink object connected to this control server.
        /// @param [in] owner The owner of the new CtrlLink object returned by this function
        ///                   (not in an owned-object sense, will just receive the callbacks).
        /// @param [in] linkId The ID to assign to the new link object returned by this function.
        ///                    It is not used by the server, but may be required by the caller
        ///                    to distinguish between different links.
        /// @return Pointer to the new CtrlLink object, or 0 on error.
        CtrlLink * ctrlGenerateConnectedLink ( Owner & owner, int linkId );

        /// @brief Adds a packet to internal list of packets to write to the specific client
        ///
        /// @param [in] clientId The ID of the client to send the data to
        /// @param [in] packet The packet to send.
        /// @param [in] atLoopEnd If set to true (by default it's false), the packet will be added to
        ///                        another send queue, which will be appended to the end of the regular queue
        ///                        at the end of the event loop. This can be used to send various status update
        ///                        notifications that should be delivered AFTER the return code from
        ///                        the function that sends that update.
        /// @return Standard error code.
        ERRCODE ctrlSendPacket ( int clientId, Ctrl::Message & packet, bool atLoopEnd = false );

        /// @brief Adds a packet to internal list of packets to write to each of the connected clients
        ///
        /// @param [in] packet The packet to send.
        /// @param [in] atLoopEnd If set to true (by default it's false), the packet will be added to
        ///                        another send queue, which will be appended to the end of the regular queue
        ///                        at the end of the event loop. This can be used to send various status update
        ///                        notifications that should be delivered AFTER the return code from
        ///                        the function that sends that update.
        void ctrlBroadcastPacket ( Ctrl::Message & packet, bool atLoopEnd = false );

        /// @brief Sends a control response
        ///
        /// @param [in] linkId The ID of the control link to send the response to
        /// @param [in] resp The actual response message
        /// @param [in] request The request message that we are responding to.
        /// @param [in] respErrCode The 'response error code' that should be put in the response. Success by default.
        /// @return Always returns ResponseSent, whether there was an error or not. This means it can be used
        ///          for returning from rcvCtrlPacket() callback.
        ERRCODE ctrlSendResponse (
            int linkId, Ctrl::Update & resp, const Ctrl::Request & request,
            ERRCODE respErrCode = Error::Success );

    protected:
        /// @brief Called when a new CtrlLink connects to this CtrlServer.
        /// It is called after it has been configured and added to the internal list of clients.
        /// The default implementation does nothing.
        /// @param [in] ctrlLink Pointer to the CtrlLink that just connected.
        virtual void ctrlClientLinkConnected ( CtrlLink * ctrlLink );

        virtual void ctrlLinkClosed ( int linkId );
        virtual ERRCODE ctrlPacketReceived ( int linkId, Ctrl::Message & msg, List<int> & receivedFds );

        /// @brief Returns a pointer to the control link with given ID
        /// @param [in] linkId The ID of the link
        /// @return Pointer to the CtrlLink requested, or 0 if not found.
        inline CtrlLink * ctrlGetLink ( int linkId )
        {
            return _ctrlClients.value ( linkId );
        }

        /// @brief closes a control link with given ID.
        /// @param [in] linkId The ID of the link to close.
        void ctrlCloseLink ( int linkId );

        /// @brief Generates a new CtrlLink object connected to this control server.
        /// @param [in] owner The owner of the new CtrlLink object returned by this function
        ///                   (not in an owned-object sense, will just receive the callbacks).
        /// @param [in] linkId The ID to assign to the new link object returned by this function.
        ///                    It is not used by the server, but may be required by the caller
        ///                    to distinguish between different links.
        /// @param [out] serverLinkId On success, it will be set to the ID of newly created link on the server side.
        /// @return Pointer to the new CtrlLink object, or 0 on error.
        CtrlLink * ctrlGenerateConnectedLink ( Owner & owner, int linkId, int & serverLinkId );

    private:
        static TextLog _log; ///< Log stream

        /// @brief A class that "forwards" FD events to the CtrlServer
        /// We don't want to inherit TcpServer::Owner directly, because this would complicate
        /// classes that inherit this one, but want to process their own FD events.
        /// Instead we have this object that receives events on behalf of CtrlServer.
        class ExtConReceiver: public TcpServer::Owner
        {
            public:
                /// @brief Constructor
                /// @param [in] owner The CtrlServer to forward FD events to
                ExtConReceiver ( CtrlServer & owner );

                /// @brief Callback for notifying the owner when new TCP connection is successfully "listened to"
                ///
                /// @param [in] listener The listener that generated the callback
                /// @param [in] extraData The extraData used in addListener()
                /// @param [in] sockFd The FD of the new connection. It is owner's responsibility to close it
                /// @param [in] localAddr Address on which the connection was received
                /// @param [in] localPort Port number on which the connection was received
                /// @param [in] remoteAddr Remote host's address
                /// @param [in] remotePort Remote host's port number
                virtual void incomingTcpConnection (
                    TcpServer * listener,
                    uint8_t extraData,
                    int sockFd,
                    const IpAddress & localAddr,
                    uint16_t localPort,
                    const IpAddress & remoteAddr,
                    uint16_t remotePort );

                /// @brief Callback for notifying the owner when new UNIX stream connection
                ///        is successfully "listened to"
                ///
                /// @param [in] listener The listener that generated the callback
                /// @param [in] extraData The extraData used in addListener()
                /// @param [in] sockFd The FD of the new connection. It is owner's responsibility to close it
                /// @param [in] sockName The name of the local socket
                virtual void incomingUnixConnection (
                    TcpServer * listener,
                    uint8_t extraData,
                    int sockFd,
                    const String & sockName );

            private:
                CtrlServer & _owner; ///< CtrlServer to forward FD events to
        };

        ExtConReceiver _extReceiver; ///< External connection receiver

        TcpServer _ctrlTcpServer; ///< TCP server used for listening for TCP/UNIX connections
        HashMap<int, CtrlLink *> _ctrlClients; ///< Connected clients
};
}
