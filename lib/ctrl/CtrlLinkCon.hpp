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

#include "CtrlLink.hpp"

namespace Pravala
{
/// @brief Control Link Connector
/// It is a CtrlLink that can connect to a CtrlServer
class CtrlLinkConnector: public CtrlLink
{
    public:
        class Owner: public CtrlLink::Owner
        {
            protected:
                /// @brief Callback for notifying the owner about connected ctrl links
                /// @param [in] linkId The ID of the link that generated the callback
                virtual void ctrlLinkConnected ( int linkId ) = 0;

                /// @brief Callback for notifying the owner when the connection attempt failed
                /// @note This callback is only generated if the failure happened before successfully establishing
                /// the connection. If the connection succeeds, ctrlLinkConnected() is generated, followed
                /// by ctrlLinkClosed() when the link is closed.
                /// @note This callback is generated every time a (re)connect attempt fails.
                /// @param [in] link The link that generated the callback.
                virtual void ctrlLinkConnectFailed ( CtrlLinkConnector * link ) = 0;

                friend class CtrlLinkConnector;
        };

        /// @brief Constructor
        /// @param [in] owner The owner of this CtrlManager object (not in an owned-object sense,
        ///                    will just receive the callbacks)
        /// @param [in] linkId The ID of the link that sent the packet
        CtrlLinkConnector ( Owner & owner, int linkId );

        /// @brief Destructor
        virtual ~CtrlLinkConnector();

        /// @brief Starts the connection process.
        /// @param [in] addr IP address to connect to.
        /// @param [in] port Remote port to connect to.
        /// @param [in] restartDelay If > 0, the delay (in milliseconds) after which the failed connection attempts will
        ///                           be restarted. Closed connections are restarted immediately.
        ///                          If <= 0, restarts are disabled (both connection attempts and closed connections)
        /// @param [in] timeout Connection attempt timeout (in milliseconds).
        /// @param [in] pingInterval The interval (in milliseconds) between sending control ping messages.
        ///                           If <= 0, no ping messages will be sent.
        /// @param [in] pongTimeout The time (in milliseconds) after sending the ping message during which we should
        ///                          get the pong response. If there is no corresponding pong message during that time,
        ///                          the connection will be closed and ctrlLinkClosed will be called.
        ///                          This value should not be bigger than pingInterval (if it is, it will be decreased).
        /// @return Standard error code;
        ///          If the connection is in progress, Error::Success is returned. Once the connection
        ///          attempt succeeds, ctrlLinkConnected callback is generated. Note: even if this function
        ///          succeeds immediately (which is possible when connecting to the same host),
        ///          we will still generate a ctrlLinkConnected callback later, at the loop-end event.
        ///          Error::Success is also returned if the link connection attempt failed right away and
        ///          restarts are enabled, because this object will re-attempt the connection after restartDelay
        ///          (if restartDelay is > 0).
        ///          Other ("hard") error codes mean that something was wrong (for example with the arguments)
        ///          and this object will not try to connect.
        inline ERRCODE connectAddr (
            const IpAddress & addr, uint16_t port,
            int restartDelay = 10000, int timeout = 30000,
            int pingInterval = 0, int pongTimeout = 0 )
        {
            return intConnect ( addr, port, String::EmptyString, restartDelay, timeout, pingInterval, pongTimeout );
        }

        /// @brief Starts the connection process.
        /// @param [in] sockName The name of the local socket to connect to. If the first character is '@' then
        ///                          the abstract namespace is used (and the first character is removed).
        ///                           To use abstract namespace with a name starting with '@', use it twice.
        /// @param [in] restartDelay If > 0, the delay (in milliseconds) after which the failed connection attempts will
        ///                           be restarted. Closed connections are restarted immediately.
        ///                          If <= 0, restarts are disabled (both connection attempts and closed connections)
        /// @param [in] timeout Connection attempt timeout (in milliseconds).
        /// @param [in] pingInterval The interval (in milliseconds) between sending control ping messages.
        ///                           If <= 0, no ping messages will be sent.
        /// @param [in] pongTimeout The time (in milliseconds) after sending the ping message during which we should
        ///                          get the pong response. If there is no corresponding pong message during that time,
        ///                          the connection will be closed and ctrlLinkClosed will be called.
        ///                          This value should not be bigger than pingInterval (if it is, it will be decreased).
        /// @return Standard error code;
        ///          If the connection is in progress, Error::Success is returned. Once the connection
        ///          attempt succeeds, ctrlLinkConnected callback is generated. Note: even if this function
        ///          succeeds immediately (which is possible when connecting to the same host),
        ///          we will still generate a ctrlLinkConnected callback later, at the loop-end event.
        ///          Error::Success is also returned if the link connection attempt failed right away and
        ///          restarts are enabled, because this object will re-attempt the connection after restartDelay
        ///          (if restartDelay is > 0).
        ///          Other ("hard") error codes mean that something was wrong (for example with the arguments)
        ///          and this object will not try to connect.
        inline ERRCODE connectLocal (
            const String & sockName,
            int restartDelay = 10000, int timeout = 10000,
            int pingInterval = 0, int pongTimeout = 0 )
        {
            return intConnect ( IpAddress::IpEmptyAddress, 0, sockName,
                                restartDelay, timeout, pingInterval, pongTimeout );
        }

        /// @brief Starts the connection process.
        /// @param [in] sockName The name of the local socket to connect to.
        ///                       If this name "looks like" ip_addr:port_number, connectAddr() is used.
        ///                       Otherwise a local socket is created. In that case, if the first character is '@',
        ///                       then the abstract namespace is used (and the first character is removed).
        ///                       To use abstract namespace with a name starting with '@', use it twice.
        /// @param [in] restartDelay If > 0, the delay (in milliseconds) after which the failed connection attempts will
        ///                           be restarted. Closed connections are restarted immediately.
        ///                          If <= 0, restarts are disabled (both connection attempts and closed connections)
        /// @param [in] timeout Connection attempt timeout (in milliseconds).
        /// @param [in] pingInterval The interval (in milliseconds) between sending control ping messages.
        ///                           If <= 0, no ping messages will be sent.
        /// @param [in] pongTimeout The time (in milliseconds) after sending the ping message during which we should
        ///                          get the pong response. If there is no corresponding pong message during that time,
        ///                          the connection will be closed and ctrlLinkClosed will be called.
        ///                          This value should not be bigger than pingInterval (if it is, it will be decreased).
        /// @return Standard error code;
        ///          If the connection is in progress, Error::Success is returned. Once the connection
        ///          attempt succeeds, ctrlLinkConnected callback is generated. Note: even if this function
        ///          succeeds immediately (which is possible when connecting to the same host),
        ///          we will still generate a ctrlLinkConnected callback later, at the loop-end event.
        ///          Error::Success is also returned if the link connection attempt failed right away and
        ///          restarts are enabled, because this object will re-attempt the connection after restartDelay
        ///          (if restartDelay is > 0).
        ///          Other ("hard") error codes mean that something was wrong (for example with the arguments)
        ///          and this object will not try to connect.
        ERRCODE connect (
            const String & sockName,
            int restartDelay = 10000, int timeout = 30000,
            int pingInterval = 0, int pongTimeout = 0 );

        /// @brief Returns the description of this link's address.
        /// @return The socket name if local socket is used; address:port otherwise.
        String getAddrDesc() const;

        /// @brief Closes the link.
        /// It will close and clear the descriptors and stop the timers.
        /// It will NOT clear settings (socket name/address-port, timeouts, delays, intervals...)
        virtual void close();

    protected:
        CtrlLinkConnector::Owner & _conOwner; ///< CtrlLinkConnector version of the owner
        SimpleTimer _timer; ///< Timer for doing timeouts, restarts and pinging.
        SimpleTimer _pongTimer; ///< Timer for detecting pong timeouts.

        String _sockName; ///< The name of the local socket we are using.
        IpAddress _addr; ///< The IP address we are using.
        uint16_t _port; ///< The port number we are using.

        int _pendingFd; ///< FD of the socket while connecting

        /// @brief The delay before restarting connections (in milliseconds).
        /// If > 0, the delay after which the failed connection attempts will be restarted.
        /// Closed connections are restarted immediately.
        /// If <= 0, restarts are disabled (for both connection attempts and closed connections)
        int _restartDelay;

        /// @brief The connection timeout (in milliseconds).
        /// If the connection cannot be established within that time it is considered "failed".
        int _conTimeout;

        /// @brief The interval (in milliseconds) between sending control ping messages.
        /// If <= 0, no ping messages will be sent.
        int _pingInterval;

        /// @brief The time (in milliseconds) after sending the ping message during which we should get the pong
        /// response. If there is no corresponding pong message during that time, the connection will be closed.
        int _pongTimeout;

        /// @brief Indicates whether we should generate a ctrlLinkConnected() callback on loop-end event.
        /// This is unset once the callback is generated.
        bool _loopEndConnectedCallback;

        virtual void receiveFdEvent ( int fd, short int events );
        virtual void timerExpired ( Timer * timer );
        virtual void receiveLoopEndEvent();

        virtual void linkClosed();

        virtual void processPong ( int linkId, const Ctrl::Pong & msg );

        /// @brief Called when the connection attempt fails.
        /// It will schedule a restart (if needed) and call a callback.
        /// @note Since this generates a callback, caller of this function has to return immediately after calling it.
        void conAttemptFailed();

        /// @brief Internal connection procedure.
        /// @param [in] addr IP address to connect to.
        /// @param [in] port Remote port to connect to.
        /// @param [in] sockName The name of the local socket to connect to. The "abstract" namespace is used.
        /// @param [in] restartDelay If > 0, the delay (in milliseconds) after which the failed connection attempts will
        ///                           be restarted. Closed connections are restarted immediately.
        ///                          If <= 0, restarts are disabled (both connection attempts and closed connections)
        /// @param [in] timeout Connection attempt timeout (in milliseconds).
        /// @param [in] pingInterval The interval (in milliseconds) between sending control ping messages.
        ///                           If <= 0, no ping messages will be sent.
        /// @param [in] pongTimeout The time (in milliseconds) after sending the ping message during which we should
        ///                          get the pong response. If there is no corresponding pong message during that time,
        ///                          the connection will be closed and ctrlLinkClosed will be called.
        ///                          This value should not be bigger than pingInterval (if it is, it will be decreased).
        /// @return Standard error code;
        ///          If the connection is in progress, Error::Success is returned. Once the connection
        ///          attempt succeeds, ctrlLinkConnected callback is generated. Note: even if this function
        ///          succeeds immediately (which is possible when connecting to the same host),
        ///          we will still generate a ctrlLinkConnected callback later, at the loop-end event.
        ///          Error::Success is also returned if the link connection attempt failed right away and
        ///          restarts are enabled, because this object will re-attempt the connection after restartDelay
        ///          (if restartDelay is > 0).
        ///          Other ("hard") error codes mean that something was wrong (for example with the arguments)
        ///          and this object will not try to connect.
        ERRCODE intConnect (
            const IpAddress & addr, uint16_t port, const String & sockName,
            int restartDelay, int timeout, int pingInterval, int pongTimeout );

    private:
        /// @brief A helper function that calls the appropriate version of Link::setup() with _pendingFd.
        /// It determines local/remote socket address/name and clears _pendingFd.
        /// It also starts ping timer (if needed).
        void setupPendingSock();
};
}
