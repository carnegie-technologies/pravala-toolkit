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

#include "basic/IpAddress.hpp"
#include "event/EventManager.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
class TcpSocket;

/// @brief TCP server
/// It opens listening sockets and waits for incoming TCP connections.
/// It can also be used for listening on the local (UNIX) sockets
class TcpServer: public EventManager::FdEventHandler
{
    public:
        /// @brief The owner of the TcpServer
        class Owner
        {
            protected:
                /// @brief Callback for notifying the owner when new TCP connection is successfully "listened to".
                /// @param [in] tcpServer The server that generated the callback
                /// @param [in] extraData The extraData used in addListener()
                /// @param [in] socket The socket representing the new connection.
                virtual void incomingTcpConnection ( TcpServer * tcpServer, uint8_t extraData, TcpSocket * socket );

                /// @brief Callback for notifying the owner when new TCP connection is successfully "listened to".
                /// @note This is a legacy version of the callback, called by the default implementation of the
                ///       other one.
                /// @param [in] tcpServer The server that generated the callback
                /// @param [in] extraData The extraData used in addListener()
                /// @param [in] sockFd The FD of the new connection. It is owner's responsibility to close it
                /// @param [in] localAddr Address on which the connection was received
                /// @param [in] localPort Port number on which the connection was received
                /// @param [in] remoteAddr Remote host's address
                /// @param [in] remotePort Remote host's port number
                virtual void incomingTcpConnection (
                    TcpServer * tcpServer,
                    uint8_t extraData,
                    int sockFd,
                    const IpAddress & localAddr,
                    uint16_t localPort,
                    const IpAddress & remoteAddr,
                    uint16_t remotePort );

                /// @brief Callback for notifying the owner when a new UNIX connection is successfully "listened to"
                ///
                /// @param [in] tcpServer The server that generated the callback
                /// @param [in] extraData The extraData used in addListener()
                /// @param [in] sockFd The FD of the new connection. It is owner's responsibility to close it
                /// @param [in] sockName The name of the local socket
                virtual void incomingUnixConnection (
                    TcpServer * tcpServer, uint8_t extraData, int sockFd, const String & sockName );

                /// @brief Destructor
                virtual ~Owner()
                {
                }

                friend class TcpServer;
        };

        /// @brief Constructor
        /// @param [in] owner The owner of this TcpServer object (not in an owned-object sense,
        ///                    it will just receive the callbacks)
        TcpServer ( Owner & owner );

        /// @brief Creates a new listening socket (IP version)
        /// @param [in] localAddr Local address to listen on
        /// @param [in] localPort Local port to listen on
        /// @param [in] extraData Additional data delivered to the owner in callbacks (iface index, for example)
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        inline ERRCODE addListener (
            const IpAddress & localAddr, uint16_t localPort,
            uint8_t extraData = 0, int backlog = 4 )
        {
            return addListener ( SockAddr ( localAddr, localPort ), extraData, backlog );
        }

        /// @brief Creates a new listening socket (IP version)
        /// @param [in] localAddr Local address to listen on
        /// @param [in] extraData Additional data delivered to the owner in callbacks (iface index, for example)
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        ERRCODE addListener ( const SockAddr & localAddr, uint8_t extraData = 0, int backlog = 4 );

        /// @brief Creates a new listening socket
        /// @param [in] sockName The name of the socket to use. If it "looks like" ip_addr:port_number
        ///                      and can be converted to a valid IP address and port number, this function
        ///                      uses the other addListener function and creates an IP listener.
        /// @param [in] extraData Additional data delivered to the owner in callbacks (iface index, for example)
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        ERRCODE addListener (
            const String & sockName,
            uint8_t extraData = 0, int backlog = 4 );

        /// @brief Closes all listeners
        void closeListeners();

        /// @brief Destructor
        ~TcpServer();

    protected:
        /// @brief Function called when an event occurs on specific descriptor.
        ///
        /// @param [in] fd File descriptor that generated this event.
        /// @param [in] events Is a bit sum of Event* values and describes what kind of events were detected.
        void receiveFdEvent ( int fd, short int events );

    private:
        static TextLog _log; ///< Log stream

        /// @brief Listener's data
        struct ListenerData
        {
            SockAddr addr; ///< Local address
            String name; ///< The name of the local (UNIX) socket
            uint8_t extraData; ///< Extra data to pass in the callback

            /// @brief Default constructor
            ListenerData(): extraData ( 0 )
            {
            }
        };

        Owner & _owner; ///< Owner of the listener
        HashMap<int, ListenerData> _listeners; ///< Listeners; listening_sock:data pairs
};
}
