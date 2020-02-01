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

#include <cassert>

#include "DBusConnection.hpp"
#include "DBusMessageReader.hpp"
#include "DBusMessageWriter.hpp"
#include "internal/DBusErrorWrapper.hpp"
#include "internal/DBusConnectionPriv.hpp"
#include "DBusError.hpp"

using namespace Pravala;

TextLog Pravala::DBusConnection::_log ( "dbus" );

Pravala::DBusConnection * Pravala::DBusConnection::_systemBus ( 0 );
Pravala::DBusConnection * Pravala::DBusConnection::_sessionBus ( 0 );

Pravala::DBusConnection::RequestReceiver::~RequestReceiver()
{
    HashSet<DBusConnection *>::Iterator it ( _con );

    _con.clear();

    for ( ; it.isValid(); it.next() )
    {
        it.value()->removeReceiver ( this );
    }
}

Pravala::DBusConnection::ReplyReceiver::~ReplyReceiver()
{
    HashSet<DBusConnection *>::Iterator it ( _con );

    _con.clear();

    for ( ; it.isValid(); it.next() )
    {
        it.value()->removeReceiver ( this );
    }
}

Pravala::DBusConnection::SignalReceiver::~SignalReceiver()
{
    HashSet<DBusConnection *>::Iterator it ( _con );

    _con.clear();

    for ( ; it.isValid(); it.next() )
    {
        it.value()->removeReceiver ( this );
    }
}

void Pravala::DBusConnection::RequestReceiver::addCon ( Pravala::DBusConnection * con )
{
    if ( con != 0 )
    {
        _con.insert ( con );
    }
}

void Pravala::DBusConnection::ReplyReceiver::addCon ( Pravala::DBusConnection * con )
{
    if ( con != 0 )
    {
        _con.insert ( con );
    }
}

void Pravala::DBusConnection::SignalReceiver::addCon ( Pravala::DBusConnection * con )
{
    if ( con != 0 )
    {
        _con.insert ( con );
    }
}

void Pravala::DBusConnection::RequestReceiver::removeCon ( Pravala::DBusConnection * con )
{
    _con.remove ( con );
}

void Pravala::DBusConnection::ReplyReceiver::removeCon ( Pravala::DBusConnection * con )
{
    _con.remove ( con );
}

void Pravala::DBusConnection::SignalReceiver::removeCon ( Pravala::DBusConnection * con )
{
    _con.remove ( con );
}

Pravala::DBusConnection::DBusConnection ( int busType ): _dbusConnection ( 0 )
{
    DBusErrorWrapper dbusError;

    _dbusConnection = dbus_bus_get_private ( ( DBusC::DBusBusType ) busType, &dbusError );

    DBusError err;

    err.set ( dbusError );

    if ( !_dbusConnection )
    {
        LOG ( L_ERROR, "Error opening private connection to DBus' "
              << ( ( busType == DBusC::DBUS_BUS_SYSTEM ) ? "system" : "session" )
              << " bus: " << err );
        return;
    }

    _uniqueName = dbus_bus_get_unique_name ( _dbusConnection );

    LOG ( L_DEBUG, "Successfully opened a private connection to DBus' "
          << ( ( busType == DBusC::DBUS_BUS_SYSTEM ) ? "system" : "session" )
          << " bus; Unique name is '" << _uniqueName << "'" );

    dbus_connection_set_exit_on_disconnect ( _dbusConnection, false );

    if ( !dbus_connection_set_watch_functions (
             _dbusConnection,
             DBusConnectionPriv::addWatch,
             DBusConnectionPriv::removeWatch,
             DBusConnectionPriv::toggleWatch,
             this, 0 ) )
    {
        LOG ( L_ERROR, "dbus_connection_set_watch_functions() failed" );
        close();
        return;
    }

    if ( !dbus_connection_set_timeout_functions (
             _dbusConnection,
             DBusConnectionPriv::addTimeout,
             DBusConnectionPriv::removeTimeout,
             DBusConnectionPriv::toggleTimeout,
             this, 0 ) )
    {
        LOG ( L_ERROR, "dbus_connection_set_timeout_functions() failed" );
        close();
        return;
    }

    if ( !dbus_connection_add_filter (
             _dbusConnection,
             DBusConnectionPriv::filterMessage,
             this, 0 ) )
    {
        LOG ( L_ERROR, "dbus_connection_add_filter() failed" );
        close();
        return;
    }

    dbus_connection_set_dispatch_status_function (
        _dbusConnection,
        DBusConnectionPriv::handleDispatchStatus,
        this, 0 );

    EventManager::loopEndSubscribe ( this );
}

Pravala::DBusConnection::~DBusConnection()
{
    close();
}

void Pravala::DBusConnection::close()
{
    HashSet<ReplyReceiver *>::Iterator rIt ( _allReplyReceivers );
    HashSet<SignalReceiver *>::Iterator sIt ( _allSignalReceivers );

    _allReplyReceivers.clear();
    _allSignalReceivers.clear();

    _requestReceivers.clear();
    _replyReceivers.clear();
    _signalSubs.clear();

    for ( ; rIt.isValid(); rIt.next() )
    {
        rIt.value()->removeCon ( this );
    }

    for ( ; sIt.isValid(); sIt.next() )
    {
        sIt.value()->removeCon ( this );
    }

    HashMap<int, WatchPair>::Iterator wIt ( _watches );

    _watches.clear();

    for ( ; wIt.isValid(); wIt.next() )
    {
        EventManager::removeFdHandler ( wIt.key() );
    }

    HashMap<DBusC::DBusTimeout *, SimpleTimer *>::Iterator tIt ( _timers );

    _timers.clear();
    _timeouts.clear();

    for ( ; tIt.isValid(); tIt.next() )
    {
        delete tIt.value();
    }

    if ( !_dbusConnection )
        return;

    _uniqueName.clear();

    dbus_connection_flush ( _dbusConnection );
    dbus_connection_close ( _dbusConnection );
    dbus_connection_unref ( _dbusConnection );

    _dbusConnection = 0;
}

DBusConnection & Pravala::DBusConnection::getSys()
{
    if ( !_systemBus )
    {
        _systemBus = new DBusConnection ( DBusC::DBUS_BUS_SYSTEM );
    }

    return *_systemBus;
}

DBusConnection & Pravala::DBusConnection::getSession()
{
    if ( !_sessionBus )
    {
        _sessionBus = new DBusConnection ( DBusC::DBUS_BUS_SESSION );
    }

    return *_sessionBus;
}

void Pravala::DBusConnection::closeSys()
{
    if ( _systemBus != 0 )
    {
        _systemBus->close();

        delete _systemBus;
        _systemBus = 0;
    }
}

void Pravala::DBusConnection::closeSession()
{
    if ( _sessionBus != 0 )
    {
        _sessionBus->close();

        delete _sessionBus;
        _sessionBus = 0;
    }
}

uint32_t Pravala::DBusConnection::addWatch ( DBusC::DBusWatch * w )
{
    // We always return TRUE in this function, even if something went wrong.
    // FALSE is for memory allocation errors.

    if ( !w || !dbus_watch_get_enabled ( w ) )
        return TRUE;

    const int fd = dbus_watch_get_unix_fd ( w );
    const unsigned int flags = dbus_watch_get_flags ( w );

    DBusC::DBusWatch * const readWatch = ( ( flags & DBusC::DBUS_WATCH_READABLE ) ? w : 0 );
    DBusC::DBusWatch * const writeWatch = ( ( flags & DBusC::DBUS_WATCH_WRITABLE ) ? w : 0 );

    LOG ( L_DEBUG2, "Adding watch " << w << " for FD " << fd << " ["
          << ( ( readWatch != 0 ) ? "R" : "" ) << ( ( writeWatch != 0 ) ? "W" : "" )
          << "]" );

    if ( fd < 0 || ( !readWatch && !writeWatch ) )
    {
        LOG ( L_FATAL_ERROR, "Invalid watch FD (" << fd << ") or watch flags (" << flags << ") received" );
        return TRUE;
    }

    WatchPair & watchPair = _watches[ fd ];

    if ( readWatch != 0 )
        watchPair.readWatch = readWatch;

    if ( writeWatch != 0 )
        watchPair.writeWatch = writeWatch;

    // We are setting events based on which watches are now configured, and NOT based on what we just added
    // (it is possible that something was already set before).

    EventManager::setFdHandler ( fd, this,
                                 0
                                 | ( ( watchPair.readWatch != 0 ) ? ( EventManager::EventRead ) : 0 )
                                 | ( ( watchPair.writeWatch != 0 ) ? ( EventManager::EventWrite ) : 0 ) );

    return TRUE;
}

void Pravala::DBusConnection::removeWatch ( DBusC::DBusWatch * w )
{
    if ( !w )
        return;

    const int fd = dbus_watch_get_unix_fd ( w );

    if ( !_watches.contains ( fd ) )
    {
        LOG ( L_DEBUG3, "Could not find a watch " << w << " with FD " << fd
              << " (could be due to removal of disabled watch); Ignoring" );
        return;
    }

    WatchPair & watchPair = _watches[ fd ];

    int removed = 0;

    if ( watchPair.readWatch == w )
    {
        ++removed;
        watchPair.readWatch = 0;

        LOG ( L_DEBUG2, "Removing read watch " << w << " with FD " << fd );
    }

    if ( watchPair.writeWatch == w )
    {
        ++removed;
        watchPair.writeWatch = 0;

        LOG ( L_DEBUG2, "Removing write watch " << w << " with FD " << fd );
    }

    if ( removed < 1 )
    {
        LOG ( L_DEBUG2, "Watch " << w << " with FD " << fd << " does not match configured read ("
              << watchPair.readWatch << ") or write (" << watchPair.writeWatch
              << ") watches (could be due to removal of disabled watch); Ignoring" );
        return;
    }

    if ( !watchPair.readWatch && !watchPair.writeWatch )
    {
        LOG ( L_DEBUG2, "Removing handler for FD " << fd );

        EventManager::removeFdHandler ( fd );

        // We CANNOT use watchPair after this:
        _watches.remove ( fd );
        return;
    }

    LOG ( L_DEBUG2, "Setting active events for FD " << fd << " to "
          << ( ( watchPair.readWatch != 0 ) ? "R" : "" ) << ( ( watchPair.writeWatch != 0 ) ? "W" : "" ) );

    EventManager::setFdEvents ( fd,
                                0
                                | ( ( watchPair.readWatch != 0 ) ? ( EventManager::EventRead ) : 0 )
                                | ( ( watchPair.writeWatch != 0 ) ? ( EventManager::EventWrite ) : 0 ) );
}

void Pravala::DBusConnection::receiveFdEvent ( int fd, short int events )
{
    if ( !_watches.contains ( fd ) )
    {
        LOG ( L_ERROR, "Could not find a watch with FD " << fd << "; Ignoring" );
        return;
    }

    if ( ( events & ( EventManager::EventRead | EventManager::EventWrite ) ) == 0 )
    {
        return;
    }

    const WatchPair & watchPair = _watches[ fd ];

    if ( !watchPair.readWatch && !watchPair.writeWatch )
    {
        LOG ( L_FATAL_ERROR, "No watches configured for FD " << fd );
        return;
    }

    if ( watchPair.readWatch == watchPair.writeWatch )
    {
        // They are both the same, we can do a single call:
        const int flags
            = ( 0
                | ( ( ( events & EventManager::EventRead ) == EventManager::EventRead )
                    ? ( DBusC::DBUS_WATCH_READABLE ) : 0 )
                | ( ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
                    ? ( DBusC::DBUS_WATCH_WRITABLE ) : 0 )
              );

        LOG ( L_DEBUG2, "Handling watch " << watchPair.readWatch << " for FD " << fd << " ["
              << ( ( flags & DBusC::DBUS_WATCH_READABLE ) ? "R" : "" )
              << ( ( flags & DBusC::DBUS_WATCH_WRITABLE ) ? "W" : "" )
              << "]" );

        dbus_watch_handle ( watchPair.readWatch, flags );
        return;
    }

    if ( ( events & EventManager::EventWrite ) == EventManager::EventWrite )
    {
        if ( !watchPair.writeWatch )
        {
            LOG ( L_FATAL_ERROR, "Write event received, but there is no write watch configured for FD " << fd );
            return;
        }
        else
        {
            LOG ( L_DEBUG2, "Handling watch " << watchPair.writeWatch << " for FD " << fd << " [W]" );

            dbus_watch_handle ( watchPair.readWatch, DBusC::DBUS_WATCH_WRITABLE );
        }
    }

    if ( ( events & EventManager::EventRead ) == EventManager::EventRead )
    {
        if ( !watchPair.readWatch )
        {
            LOG ( L_FATAL_ERROR, "Read event received, but there is no read watch configured for FD " << fd );
        }
        else
        {
            LOG ( L_DEBUG2, "Handling watch " << watchPair.readWatch << " for FD " << fd << " [R]" );

            dbus_watch_handle ( watchPair.readWatch, DBusC::DBUS_WATCH_READABLE );
        }
    }
}

uint32_t Pravala::DBusConnection::addTimeout ( DBusC::DBusTimeout * tout )
{
    // We always return TRUE in this function, even if something went wrong.
    // FALSE is for memory allocation errors.

    if ( !tout || !dbus_timeout_get_enabled ( tout ) )
        return TRUE;

    const int ms = dbus_timeout_get_interval ( tout );

    SimpleTimer * timer = 0;

    if ( _timers.find ( tout, timer ) && timer != 0 )
    {
        LOG ( L_DEBUG2, "Modifying existing timeout " << tout << ": " << ms << "ms [timer: " << timer << "]" );
    }
    else
    {
        timer = new SimpleTimer ( *this );

        _timers.insert ( tout, timer );
        _timeouts.insert ( timer, tout );

        LOG ( L_DEBUG2, "Adding timeout " << tout << ": " << ms << "ms [timer: " << timer << "]" );
    }

    assert ( timer != 0 );

    timer->start ( ms );

    return TRUE;
}

void Pravala::DBusConnection::removeTimeout ( DBusC::DBusTimeout * tout )
{
    SimpleTimer * timer = 0;

    if ( !_timers.find ( tout, timer ) || !timer )
    {
        LOG ( L_DEBUG3, "Could not find a timeout " << tout
              << " (could be due to removal of disabled timer); Ignoring" );
        return;
    }

    LOG ( L_DEBUG2, "Removing timeout " << tout << " [timer: " << timer << "]" );

    assert ( timer != 0 );

    timer->stop();

    _timers.remove ( tout );
    _timeouts.remove ( timer );

    delete timer;
}

void Pravala::DBusConnection::timerExpired ( Timer * t )
{
    DBusC::DBusTimeout * tout = 0;
    SimpleTimer * timer = ( SimpleTimer * ) t;

    if ( !timer || !_timeouts.find ( timer, tout ) || !tout )
    {
        LOG ( L_DEBUG3, "Could not find a timer " << timer << "; Ignoring" );
        return;
    }

    assert ( tout != 0 );
    assert ( timer != 0 );

    const int ms = dbus_timeout_get_interval ( tout );

    LOG ( L_DEBUG2, "Rescheduling timeout " << tout << ": " << ms << "ms [timer: " << timer << "]" );

    timer->start ( ms );

    LOG ( L_DEBUG2, "Handling timeout " << tout << " [timer: " << timer << "]" );

    dbus_timeout_handle ( tout );
}

void Pravala::DBusConnection::handleDispatchStatus ( int status )
{
    LOG ( L_DEBUG3, "New dispatch status: " << DBusConnectionPriv::dispatchStatusStr ( status ) );

    if ( status == DBusC::DBUS_DISPATCH_DATA_REMAINS )
    {
        LOG ( L_DEBUG2, "There is more data to dispatch; Subscribing to end-of-loop callback" );

        EventManager::loopEndSubscribe ( this );
    }
}

void Pravala::DBusConnection::receiveLoopEndEvent()
{
    LOG ( L_DEBUG3, "Dispatching" );

    while ( dbus_connection_get_dispatch_status ( _dbusConnection ) == DBusC::DBUS_DISPATCH_DATA_REMAINS )
    {
        dbus_connection_dispatch ( _dbusConnection );
    }
}

void Pravala::DBusConnection::pendingCallNotify ( DBusC::DBusPendingCall * pending )
{
    if ( !pending )
    {
        LOG ( L_ERROR, "Received pending call notify callback with invalid data" );
        return;
    }

    // Note that we do NOT unreference 'pending' pointer.
    // We didn't store it and it was unreferenced right after sending the request message!

    DBusC::DBusMessage * dMsg = dbus_pending_call_steal_reply ( pending );

    if ( !dMsg )
    {
        LOG ( L_ERROR, "Received pending call notify callback with pending pointer "
              << pending << " and no message object; Ignoring" );

        return;
    }

    const uint32_t msgSerial = dbus_message_get_reply_serial ( dMsg );

    if ( msgSerial < 1 )
    {
        LOG ( L_ERROR, "Received a reply message with invalid reply serial set: '"
              << DBusMessageReader ( dMsg ) << "'; Ignoring" );

        dbus_message_unref ( dMsg );
        dMsg = 0;

        return;
    }

    ReplyReceiver * rcvr = 0;

    if ( !_replyReceivers.findAndRemove ( msgSerial, rcvr ) || !rcvr )
    {
        LOG ( L_ERROR, "Received a reply message that we don't have a receiver for: '"
              << DBusMessageReader ( dMsg ) << "'; Ignoring" );

        dbus_message_unref ( dMsg );
        dMsg = 0;

        return;
    }

    DBusMessageReader * msg = new DBusMessageReader ( dMsg );

    dbus_message_unref ( dMsg );
    dMsg = 0;

    LOG ( L_DEBUG3, "Delivering a reply message '" << *msg << "' to receiver: " << rcvr );

    rcvr->dbusReceiveReply ( this, msgSerial, msg );
}

int Pravala::DBusConnection::filterMessage ( DBusC::DBusMessage * dMsg )
{
    if ( !dMsg )
    {
        LOG ( L_ERROR, "Received invalid DBus message pointer" );
        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    DBusMessageReader msg ( dMsg );

    if ( !msg.isSignal() )
    {
        LOG ( L_DEBUG, "Received an unhandled, non-signal message: " << msg );

        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    bool handled = false;

    for ( HashSet<SignalReceiver *>::Iterator it ( _signalSubs.value ( msg.getInterface() ) ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        msg.restartArgReadIterator();

        it.value()->dbusReceiveSignal ( this, msg );

        handled = true;
    }

    if ( handled )
    {
        LOG ( L_DEBUG4, "Handled signal message: " << msg );
        return DBusC::DBUS_HANDLER_RESULT_HANDLED;
    }

    LOG ( L_DEBUG, "No handler found for signal message: " << msg );
    return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int Pravala::DBusConnection::receiveRequest ( DBusC::DBusMessage * dMsg )
{
    if ( !dMsg )
    {
        LOG ( L_ERROR, "Received invalid DBus message pointer" );
        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    DBusMessageReader msg ( dMsg );

    const String & path = msg.getPath();

    RequestReceiver * receiver;

    if ( !_requestReceivers.find ( path, receiver )
         || !receiver->dbusReceiveRequest ( this, msg ) )
    {
        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return DBusC::DBUS_HANDLER_RESULT_HANDLED;
}

bool DBusConnection::registerRequestReceiver ( RequestReceiver * receiver, const String & path )
{
    if ( !_dbusConnection )
    {
        LOG ( L_ERROR, "Could not register receiver for path '" << path << "' - D-Bus connection is not initialized" );
        return false;
    }

    if ( !receiver )
    {
        LOG ( L_ERROR, "Not registering invalid receiver for path '" << path << "'" );
        return false;
    }

    if ( _requestReceivers.contains ( path ) )
    {
        LOG ( L_ERROR, "Cannot register request receiver; Path '" << path << "' is already in use" );
        return false;
    }

    LOG ( L_DEBUG4, "Adding receiver for path '" << path << "'" );

    DBusC::DBusObjectPathVTable vTable;

    memset ( &vTable, 0, sizeof ( vTable ) );

    vTable.message_function = DBusConnectionPriv::receiveRequest;

    if ( !DBusC::dbus_connection_register_object_path ( _dbusConnection, path.c_str(), &vTable, this ) )
    {
        return false;
    }

    _requestReceivers[ path ] = receiver;

    return true;
}

void DBusConnection::unregisterRequestReceiver ( const String & path )
{
    RequestReceiver * receiver = 0;

    if ( !_requestReceivers.findAndRemove ( path, receiver ) )
    {
        LOG ( L_ERROR, "Attempting to remove request receiver for unregistered path '" << path << "'" );
        return;
    }

    dbus_connection_unregister_object_path ( _dbusConnection, path.c_str() );

    _requestReceivers.remove ( path );
}

bool Pravala::DBusConnection::send ( Pravala::DBusMessageWriter & msg )
{
    if ( !_dbusConnection )
    {
        LOG ( L_ERROR, "Could not send message '" << msg << "' - D-Bus connection is not initialized" );
        return false;
    }

    LOG ( L_DEBUG4, "Sending message '" << msg << "'" );

    // send message
    if ( !dbus_connection_send ( _dbusConnection, msg.getInternalMessage(), 0 ) )
    {
        LOG ( L_ERROR, "Error sending message '" << msg << "' - dbus_connection_send failed" );
        return false;
    }

    return true;
}

uint32_t Pravala::DBusConnection::sendWithReply ( ReplyReceiver * receiver, DBusMessageWriter & msg, int timeout )
{
    if ( !_dbusConnection )
    {
        LOG ( L_ERROR, "Could not send message '" << msg << "' - D-Bus connection is not initialized" );
        return 0;
    }

    if ( !receiver )
    {
        LOG ( L_ERROR, "Could not send message '" << msg << "' - invalid receiver!" );
        return 0;
    }

    LOG ( L_DEBUG4, "Sending message '" << msg << "'" );

    DBusC::DBusPendingCall * pending = 0;

    // send message and get a handle for a reply
    if ( !dbus_connection_send_with_reply ( _dbusConnection, msg.getInternalMessage(), &pending, timeout ) )
    {
        LOG ( L_ERROR, "Error sending message '" << msg << "' - dbus_connection_send_with_reply failed" );
        return 0;
    }

    if ( !pending )
    {
        LOG ( L_ERROR, "Error sending message '" << msg << "' - 'pending' was not set" );
        return 0;
    }

    const uint32_t msgSerial = msg.getSerial();

    if ( msgSerial < 1 )
    {
        LOG ( L_ERROR, "Message '" << msg << "' sent successfully, but the assigned serial (" << msgSerial
              << ") is invalid; Removing pending call" );

        dbus_pending_call_unref ( pending );
        pending = 0;

        return 0;
    }

    if ( !dbus_pending_call_set_notify ( pending, DBusConnectionPriv::pendingCallNotify, this, 0 ) )
    {
        LOG ( L_ERROR, "Error setting pending call notify callback in " << pending );

        dbus_pending_call_unref ( pending );
        pending = 0;

        return 0;
    }

    // We don't use this pending pointer anymore. It will still exist internally.

    dbus_pending_call_unref ( pending );
    pending = 0;

    _replyReceivers[ msgSerial ] = receiver;

    receiver->addCon ( this );
    _allReplyReceivers.insert ( receiver );

    LOG ( L_DEBUG2, "Message '" << msg << "' sent successfully; Serial: " << msgSerial << "; Receiver: " << receiver );

    return msgSerial;
}

void Pravala::DBusConnection::removeReceiver ( Pravala::DBusConnection::RequestReceiver * receiver )
{
    if ( !receiver )
        return;

    LOG ( L_DEBUG, "Removing request receiver " << receiver );

    receiver->removeCon ( this );

    for ( HashMap<String, RequestReceiver *>::MutableIterator it ( _requestReceivers ); it.isValid(); )
    {
        if ( it.value() == receiver )
        {
            LOG ( L_DEBUG2, "Removing receiver " << receiver << "; Path: " << it.key() );

            dbus_connection_unregister_object_path ( _dbusConnection, it.key().c_str() );

            it.remove();
        }
        else
        {
            it.next();
        }
    }
}

void Pravala::DBusConnection::removeReceiver ( Pravala::DBusConnection::ReplyReceiver * receiver )
{
    if ( !receiver )
        return;

    LOG ( L_DEBUG, "Removing reply receiver " << receiver );

    receiver->removeCon ( this );
    _allReplyReceivers.remove ( receiver );

    for ( HashMap<uint32_t, ReplyReceiver *>::MutableIterator it ( _replyReceivers ); it.isValid(); )
    {
        if ( it.value() == receiver )
        {
            LOG ( L_DEBUG2, "Removing receiver " << receiver << "; MsgSerial: " << it.key() );

            it.remove();
        }
        else
        {
            it.next();
        }
    }
}

void Pravala::DBusConnection::removeReceiver ( Pravala::DBusConnection::SignalReceiver * receiver )
{
    if ( !receiver )
        return;

    LOG ( L_DEBUG, "Removing signal receiver " << receiver );

    receiver->removeCon ( this );
    _allSignalReceivers.remove ( receiver );

    for ( HashMap<String, HashSet<SignalReceiver *> >::MutableIterator it ( _signalSubs ); it.isValid(); )
    {
        HashSet<SignalReceiver *> & rcvrs = it.value();

        const size_t numRemoved = rcvrs.remove ( receiver );

        if ( rcvrs.isEmpty() )
        {
            if ( numRemoved > 0 )
            {
                setSignalMatchRule ( it.key(), false );
            }

            it.remove();
        }
        else
        {
            it.next();
        }
    }
}

void Pravala::DBusConnection::registerSignalReceiver (
        Pravala::DBusConnection::SignalReceiver * receiver, const String & interface )
{
    if ( !receiver || !DBusMessage::isLegalInterfaceName ( interface ) )
    {
        LOG ( L_ERROR, "Invalid subscription request; Receiver: "
              << receiver << "; Interface: '" << interface << "'" );
        return;
    }

    receiver->addCon ( this );
    _allSignalReceivers.insert ( receiver );

    LOG ( L_DEBUG, "Adding signal subscription; Receiver: " << receiver << "; Interface: '" << interface << "'" );

    HashSet<SignalReceiver *> & rcvrs = _signalSubs[ interface ];

    if ( rcvrs.isEmpty() )
    {
        setSignalMatchRule ( interface, true );
    }

    rcvrs.insert ( receiver );
}

void Pravala::DBusConnection::unregisterSignalReceiver (
        Pravala::DBusConnection::SignalReceiver * receiver, const String & interface )
{
    if ( !receiver || interface.isEmpty() )
        return;

    LOG ( L_DEBUG, "Removing signal subscription; Receiver: " << receiver << "; Interface: '" << interface << "'" );

    if ( !_signalSubs.contains ( interface ) )
        return;

    HashSet<SignalReceiver *> & rcvrs = _signalSubs[ interface ];

    const size_t numRemoved = rcvrs.remove ( receiver );

    if ( rcvrs.isEmpty() )
    {
        if ( numRemoved > 0 )
        {
            setSignalMatchRule ( interface, false );
        }

        _signalSubs.remove ( interface );
    }

    return;
}

void Pravala::DBusConnection::setSignalMatchRule ( const String & interface, bool add )
{
    const String rule ( String ( "type='signal',interface='%1'" ).arg ( interface ) );

    LOG ( L_DEBUG, ( add ? "Adding" : "Removing" ) << " match rule: \"" << rule << "\"" );

    if ( add )
    {
        dbus_bus_add_match ( _dbusConnection, rule.c_str(), 0 );
    }
    else
    {
        dbus_bus_remove_match ( _dbusConnection, rule.c_str(), 0 );
    }
}
