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

#include <cerrno>

#include "basic/Math.hpp"
#include "sys/OsUtils.hpp"
#include "sys/SocketApi.hpp"

#include "CtrlLinkCon.hpp"

using namespace Pravala;

CtrlLinkConnector::CtrlLinkConnector ( CtrlLinkConnector::Owner & owner, int linkId ):
    CtrlLink ( owner, linkId ),
    _conOwner ( owner ),
    _timer ( *this ),
    _pongTimer ( *this ),
    _port ( 0 ),
    _pendingFd ( -1 ),
    _restartDelay ( -1 ),
    _conTimeout ( -1 ),
    _pingInterval ( -1 ),
    _pongTimeout ( -1 ),
    _loopEndConnectedCallback ( false )
{
}

CtrlLinkConnector::~CtrlLinkConnector()
{
    close();
}

void CtrlLinkConnector::close()
{
    _timer.stop();
    _pongTimer.stop();

    if ( _pendingFd >= 0 )
    {
        EventManager::closeFd ( _pendingFd );
        _pendingFd = -1;
    }

    CtrlLink::close();
}

void CtrlLinkConnector::linkClosed()
{
    // We DON'T want to call CtrlLink::linkClosed(), since all it does is close() and the callback.
    // We need to also setup the reconnect (if needed).

    close();

    if ( _restartDelay > 0 )
    {
        LOG ( L_DEBUG, "Restarts are enabled (delay is " << _restartDelay
              << " ms) and we were connected, so we will try re-connecting to "
              << getAddrDesc() << " (almost) right away" );

        // Let's schedule a very short timer.
        _timer.start ( 1 );
    }

    // Otherwise we don't need to reconnect.
    // In both cases, however, we need to tell the owner:

    // No operations after this!
    _conOwner.ctrlLinkClosed ( LinkId );
    return;
}

void CtrlLinkConnector::conAttemptFailed()
{
    close();

    if ( _restartDelay > 0 )
    {
        LOG ( L_DEBUG, "Scheduling restart of connection attempt to " << getAddrDesc()
              << " in " << _restartDelay << " ms" );

        _timer.start ( _restartDelay );
    }

    // No operations after this!
    _conOwner.ctrlLinkConnectFailed ( this );
    return;
}

String CtrlLinkConnector::getAddrDesc() const
{
    if ( !_sockName.isEmpty() )
    {
        return String ( "'%1'" ).arg ( _sockName );
    }

    return String ( "%1:%2" ).arg ( _addr.toString ( true ), String::number ( _port ) );
}

ERRCODE CtrlLinkConnector::connect (
        const String & sockName, int restartDelay, int timeout, int pingInterval, int pongTimeout )
{
    IpAddress addr;
    uint16_t port = 0;

    if ( IpAddress::convertAddrSpec ( sockName, addr, port ) )
    {
        return intConnect ( addr, port, String::EmptyString,
                            restartDelay, timeout, pingInterval, pongTimeout );
    }
    else
    {
        return intConnect ( IpAddress::IpEmptyAddress, 0, sockName,
                            restartDelay, timeout, pingInterval, pongTimeout );
    }
}

ERRCODE CtrlLinkConnector::intConnect (
        const IpAddress & addr, uint16_t port, const String & sockName,
        int restartDelay, int timeout, int pingInterval, int pongTimeout )
{
    close();

    assert ( _pendingFd < 0 );

    // Errors while creating a socket or setting it in non-blocking mode are fatal,
    // we will not retry anything later.

    if ( !sockName.isEmpty() )
    {
        if ( ( _pendingFd = SocketApi::create ( SocketApi::SocketLocal ) ) < 0 )
        {
            LOG ( L_ERROR, "Could not create a local socket: " << SocketApi::getLastErrorDesc() );
            return Error::SocketFailed;
        }

        if ( !SocketApi::setNonBlocking ( _pendingFd ) )
        {
            LOG ( L_ERROR, "Error setting local socket in non-blocking mode: " << SocketApi::getLastErrorDesc() );

            close();

            return Error::SocketFailed;
        }
    }
    else if ( addr.isValid() && port > 0 )
    {
        if ( ( _pendingFd = SocketApi::create (
                   addr.isIPv4() ? ( SocketApi::SocketStream4 ) : ( SocketApi::SocketStream6 ) )
             ) < 0 )
        {
            LOG ( L_ERROR, "Could not create an IPv" << ( addr.isIPv4() ? "4" : "6" )
                  << " socket: " << SocketApi::getLastErrorDesc() );

            return Error::SocketFailed;
        }

        if ( !SocketApi::setNonBlocking ( _pendingFd ) )
        {
            LOG ( L_ERROR, "Error setting " << ( addr.isIPv4() ? "IPv4" : "IPv6" )
                  << " socket in non-blocking mode: " << SocketApi::getLastErrorDesc() );

            close();

            return Error::SocketFailed;
        }
    }
    else
    {
        LOG ( L_ERROR, "Invalid parameters to connect (" << addr << ":" << port << ", '" << sockName << "')" );

        return Error::InvalidParameter;
    }

    assert ( _pendingFd >= 0 );

    // Let's store the settings!

    _sockName = sockName;
    _addr = addr;
    _port = port;
    _restartDelay = restartDelay;
    _conTimeout = timeout;
    _pingInterval = pingInterval;

    _pongTimeout = max ( pongTimeout, pingInterval );

    const ERRCODE conResult = ( _sockName.isEmpty() )
                              ? ( SocketApi::connect ( _pendingFd, _addr, _port ) )
                              : ( SocketApi::connect ( _pendingFd, _sockName ) );

    // Should have been stopped by close()
    assert ( !_timer.isActive() );

    // Just in case...
    _timer.stop();

    if ( conResult == Error::Success )
    {
        LOG ( L_DEBUG, "Connection to " << getAddrDesc() << " succeeded" );

        // We connected right away. Let's setup the socket.
        setupPendingSock();

        assert ( _pendingFd < 0 );

        // Here we schedule a ctrlLinkConnected callback to happen later (at loop-end event).
        // This way the code paths for immediately connected and connected after a delay
        // are the same; both return Error::Success immediately, indicating connect-in-progress,
        // then ctrlLinkConnected will be generated later.
        if ( !_loopEndConnectedCallback )
        {
            _loopEndConnectedCallback = true;

            EventManager::loopEndSubscribe ( this );
        }

        return Error::Success;
    }

    if ( conResult == Error::ConnectInProgress )
    {
        LOG ( L_DEBUG, "Connection to " << getAddrDesc() << " is in progress" );

        // Connection is in progress. We need to setup the timeout timer (if it's enabled):

        if ( _conTimeout > 0 )
        {
            LOG ( L_DEBUG, "Setting timeout to " << _conTimeout << " ms" );

            _timer.start ( _conTimeout );
        }

        EventManager::setFdHandler ( _pendingFd, this, EventManager::EventRead | EventManager::EventWrite );

        return Error::Success;
    }

    // Connection attempt failed. It either failed with ConnectFailed error, or some other, more critical, error.
    // If it just failed to connect, we may want to retry it later. Otherwise we simply fail.

    // First, we need to close the socket (in both cases).
    close();

    if ( conResult == Error::ConnectFailed && _restartDelay > 0 )
    {
        // If this was ConnectFailed error, and restarts are enabled, we want to start the timer
        // and tell the caller that the connection is in progress!

        LOG_ERR ( L_DEBUG, conResult, "Connection to " << getAddrDesc()
                  << " failed; We will retry connecting in " << _restartDelay << " ms" );

        _timer.start ( _restartDelay );

        return Error::Success;
    }

    LOG_ERR ( L_ERROR, conResult, "Connection to " << getAddrDesc()
              << " failed; We will NOT retry connecting" );

    // This is some other error, or restarts were not enabled.

    return conResult;
}

void CtrlLinkConnector::setupPendingSock()
{
    assert ( _pendingFd >= 0 );

    if ( _pendingFd < 0 )
        return;

    if ( _sockName.isEmpty() )
    {
        SockAddr localAddr;

        if ( !SocketApi::getName ( _pendingFd, localAddr ) )
        {
            LOG ( L_ERROR, "Could not read socket's local address and port: " << SocketApi::getLastErrorDesc() );
        }

        // If getsockname fails, local address/port will be invalid... nothing we can do about it...

        setup ( _pendingFd, localAddr.getAddr(), localAddr.getPort(), _addr, _port );
    }
    else
    {
        setup ( _pendingFd, _sockName );
    }

    _pendingFd = -1;

    if ( _pingInterval > 0 )
    {
        // We also need to start pinging

        _timer.start ( _pingInterval );
    }
}

void CtrlLinkConnector::receiveFdEvent ( int fd, short int events )
{
    if ( _pendingFd >= 0 && _pendingFd == fd )
    {
#ifdef SYSTEM_WINDOWS
        // The socket error test doesn't work on windows.
        // Let's try to perform a small read to see if there was an error!
        // (write event means no errors)

        if ( ( events & EventManager::EventRead ) != 0 )
        {
            char c = 0;

            if ( recv ( fd, &c, 1, 0 ) < 1 )
            {
                LOG ( L_DEBUG, "Connection attempt to " << getAddrDesc() << " failed" );

                // No operations after this!
                conAttemptFailed();
                return;
            }

            // We just read a byte, let's add it to the buffer!
            // CtrlLink::setup() should NOT clear the buffer.

            _readBuffer.appendData ( &c, 1 );
        }
#else
        int sockError = 0;

        const bool ret = SocketApi::getOption ( _pendingFd, SOL_SOCKET, SO_ERROR, sockError );

        if ( !ret || sockError != 0 )
        {
            LOG ( L_DEBUG, "Connection attempt to " << getAddrDesc() << " failed" );

            // No operations after this!
            conAttemptFailed();
            return;
        }
#endif

        _timer.stop();

        setupPendingSock();

        assert ( _pendingFd < 0 );

        _conOwner.ctrlLinkConnected ( LinkId );
        return;
    }

    CtrlLink::receiveFdEvent ( fd, events );
}

void CtrlLinkConnector::processPong ( int linkId, const Ctrl::Pong & /*msg*/ )
{
    ( void ) linkId;

    LOG ( L_DEBUG, "Received a Pong message on link " << linkId << "; Stopping pong timeout timer" );

    _pongTimer.stop();
}

void CtrlLinkConnector::timerExpired ( Timer * timer )
{
    if ( timer == &_pongTimer )
    {
        assert ( _pingInterval > 0 );
        assert ( _pongTimeout > 0 );

        LOG ( L_DEBUG, "We haven't seen the pong response to the ping message we sent "
              << _pongTimeout << " ms ago; Closing the link to " << getAddrDesc() );

        linkClosed();
        return;
    }

    if ( timer != &_timer )
    {
        CtrlLink::timerExpired ( timer );
        return;
    }

    if ( _pendingFd >= 0 )
    {
        // Pending FD is set, which means we were trying to connect.
        // Expired timer in this state means that the connection attempt hasn't finished in time.
        // We need to kill it and (possibly) schedule a restart.

        LOG ( L_DEBUG, "Connection attempt to " << getAddrDesc() << " timed-out" );

        // No operations after this!
        conAttemptFailed();
        return;
    }

    // Pending FD is NOT set, which means we are either already connected, or not connecting at all.

    if ( !CtrlLink::isConnected() )
    {
        // We are not connected (and not connecting), so the timer was for connection restart.

        assert ( _restartDelay > 0 );

        LOG ( L_DEBUG, "Restarting connection attempt to " << getAddrDesc() );

        intConnect ( _addr, _port, _sockName, _restartDelay, _conTimeout, _pingInterval, _pongTimeout );
        return;
    }

    assert ( _pendingFd < 0 );
    assert ( CtrlLink::isConnected() );

    // We are already connected - the timer is to send the next ping message.
    // If, however, the pong timer is still active, we have a pong timeout.

    assert ( _pingInterval > 0 );

    if ( _pongTimer.isActive() )
    {
        // This should not happen - but it doesn't hurt if it does.
        // pong-timeout <= ping-interval is enforced in intConnect() but,
        // in case they are close and those timers get reordered, the timer for sending the next ping message
        // would expire before the 'pong timeout' timer (if there was no pong message).
        // If that happens, it means that we are supposed to send the next ping message before we received
        // a pong response to the previous ping message.
        // This should be treated as a pong-timeout which we do here (in this if block).

        assert ( _pingInterval > 0 );
        assert ( _pongTimeout > 0 );

        LOG ( L_DEBUG, "It is the time to send the next Ping message, but the Pong response to the previous one "
              "still hasn't been received; Closing the link to " << getAddrDesc() );

        linkClosed();
        return;
    }

    // Finally, we should send a Ping message and, if needed, start the pong timeout timer.

    if ( _pongTimeout > 0 )
    {
        _pongTimer.start ( _pongTimeout );
    }

    LOG ( L_DEBUG, "Sending a Ping message to " << getAddrDesc() );

    sendPing();
}

void CtrlLinkConnector::receiveLoopEndEvent()
{
    CtrlLink::receiveLoopEndEvent();

    if ( _loopEndConnectedCallback )
    {
        _loopEndConnectedCallback = false;

        _conOwner.ctrlLinkConnected ( LinkId );

        // We have to return after the callback because this may be destroyed
        return;
    }
}
