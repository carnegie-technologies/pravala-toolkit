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
#include "basic/HashSet.hpp"
#include "log/TextLog.hpp"
#include "event/EventManager.hpp"
#include "event/Timer.hpp"

namespace DBusC
{
struct DBusConnection;
struct DBusPendingCall;
struct DBusTimeout;
struct DBusWatch;
struct DBusMessage;
}

namespace Pravala
{
class DBusMessageReader;
class DBusMessageWriter;

/// @brief A wrapper class around DBus connection.
/// @note There is currently no way to register a well-known name on the bus.
class DBusConnection:
    public NoCopy,
    protected Timer::Receiver,
    protected EventManager::FdEventHandler,
    protected EventManager::LoopEndEventHandler
{
    public:
        /// @brief Interface that allows objects to receive D-Bus method calls for an object
        class RequestReceiver
        {
            protected:
                /// @brief called when a message is sent to a path this is registered on.
                /// @param [in] connection DBusConnection object that generated the callback.
                /// @param [in] msg The request message.
                /// @return True if the message was successfully handled; False otherwise.
                virtual bool dbusReceiveRequest ( DBusConnection * connection, DBusMessageReader & message ) = 0;

                /// @brief Destructor.
                virtual ~RequestReceiver();

            private:
                /// @brief All DBusConnection objects that this receiver has been used with.
                /// Used for cleanup.
                HashSet<DBusConnection *> _con;

                /// @brief Marks DBusConnection object as being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be stored in _con.
                void addCon ( DBusConnection * con );

                /// @brief Marks DBusConnection object as NOT being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be removed from _con.
                void removeCon ( DBusConnection * con );

                friend class DBusConnection;
        };

        /// @brief Interface that allows objects to receive responses to D-Bus messages sent.
        class ReplyReceiver
        {
            protected:
                /// @brief Called when there is a reply to a method call.
                /// @param [in] connection DBusConnection object that generated the callback.
                /// @param [in] msgSerial The serial number of the request message that this is a response to.
                /// @param [in] msg The reply message. This receiver owns this message after this call,
                ///                  so it's receive's responsibility to delete this message object.
                virtual void dbusReceiveReply (
                    DBusConnection * connection, uint32_t msgSerial, DBusMessageReader * msg ) = 0;

                /// @brief Destructor.
                virtual ~ReplyReceiver();

            private:
                /// @brief All DBusConnection objects that this receiver has been used with.
                /// Used for cleanup.
                HashSet<DBusConnection *> _con;

                /// @brief Marks DBusConnection object as being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be stored in _con.
                void addCon ( DBusConnection * con );

                /// @brief Marks DBusConnection object as NOT being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be removed from _con.
                void removeCon ( DBusConnection * con );

                friend class DBusConnection;
        };

        /// @brief Interface that allows objects to receive D-Bus signals.
        class SignalReceiver
        {
            protected:
                /// @brief Called when a signal message is received.
                /// @param [in] connection DBusConnection object that generated the callback.
                /// @param [in] msg The signal message. The caller will reset the read iterator before this is called.
                virtual void dbusReceiveSignal ( DBusConnection * connection, DBusMessageReader & msg ) = 0;

                /// @brief Destructor.
                virtual ~SignalReceiver();

            private:
                /// @brief All DBusConnection objects that this receiver has been used with.
                /// Used for cleanup.
                HashSet<DBusConnection *> _con;

                /// @brief Marks DBusConnection object as being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be stored in _con.
                void addCon ( DBusConnection * con );

                /// @brief Marks DBusConnection object as NOT being used by this receiver.
                /// @param [in] con The pointer to the DBusConnection that should be removed from _con.
                void removeCon ( DBusConnection * con );

                friend class DBusConnection;
        };

        /// @brief Exposes system D-Bus connection.
        /// @return Global system D-Bus connection.
        static DBusConnection & getSys();

        /// @brief Exposes session D-Bus connection.
        /// @return Global session D-Bus connection.
        static DBusConnection & getSession();

        /// @brief Closes the system D-Bus connection.
        static void closeSys();

        /// @brief Closes the session D-Bus connection.
        static void closeSession();

        /// @brief Determines whether this DBusConnection object has an active connection to D-Bus bus.
        /// @return True if there is an active connection to D-Bus bus; False otherwise.
        inline bool isOpen() const
        {
            return ( _dbusConnection != 0 );
        }

        /// @brief Get the unique name of this connection.
        /// The unique name is the name assigned to this connection by DBus.
        /// @return The unique name of this connection
        inline const String & getUniqueName()
        {
            return _uniqueName;
        }

        /// @brief Register a receiver for an object path.
        /// @param [in] receiver The object that will receive callbacks when a method call is received for the
        /// given path. Must be set!
        /// @param [in] path The path that this receiver will receive callbacks for.
        /// @return True on success; False on failure.
        bool registerRequestReceiver ( RequestReceiver * receiver, const String & path );

        /// @brief Unregister the receiver associated with an object path.
        /// @param path The path to remove the receiver for.
        void unregisterRequestReceiver ( const String & path );

        /// @brief Sends a request message that we want to receive a response to.
        /// @param [in] receiver The object that will receive the response callback. Must be set!
        /// @param [in] msg The D-Bus message to send.
        /// @param [in] timeout The timeout (in milliseconds), or -1 for default timeout.
        /// @return The serial number of the request message. It will be passed in dbusReceiveReply() callback.
        ///         If there was a failure, 0 is returned.
        uint32_t sendWithReply ( ReplyReceiver * receiver, DBusMessageWriter & msg, int timeout = -1 );

        /// @brief Sends a request message that we do not want to receive a response to.
        /// @param [in] msg The D-Bus message to send.
        /// @return True on success; False on failure.
        bool send ( DBusMessageWriter & msg );

        /// @brief Subscribes to D-Bus signal messages.
        /// @param [in] receiver The object that will receive the signal callbacks. Must be set!
        /// @param [in] interface The name of the interface we want to subscribe to. Must not be empty.
        void registerSignalReceiver ( SignalReceiver * receiver, const String & interface );

        /// @brief Unsubscribes from D-Bus signal messages.
        /// @param [in] receiver The object that is unsubscribing. Must be set!
        /// @param [in] interface The name of the interface we want to unsubscribe from. Must not be empty.
        void unregisterSignalReceiver ( SignalReceiver * receiver, const String & interface );

        /// @brief Removes the request receiver.
        /// It removes the receiver completely, from all the paths it is registered on.
        /// @param [in] receiver The receiver to remove.
        void removeReceiver ( RequestReceiver * receiver );

        /// @brief Removes the reply receiver.
        /// It removes the receiver completely, from all the responses it is waiting for.
        /// @param [in] receiver The receiver to remove.
        void removeReceiver ( ReplyReceiver * receiver );

        /// @brief Removes the signal receiver.
        /// It removes the receiver completely, from all the signals it is subscribed to.
        /// @param [in] receiver The receiver to remove.
        void removeReceiver ( SignalReceiver * receiver );

    protected:
        static TextLog _log; ///< Log stream.

        /// @brief Constructor.
        /// @param [in] busType The D-Bus bus type (session or system).
        DBusConnection ( int busType );

        /// @brief Destructor.
        ~DBusConnection();

        /// @brief Closes the D-Bus connection.
        /// It also removes all receivers.
        void close();

        /// @brief Adds a D-Bus watch object.
        /// It will configure EventManager to receive FD events for that object.
        /// @param [in] watch The D-Bus watch object to add.
        /// @return True on success; False otherwise.
        uint32_t addWatch ( DBusC::DBusWatch * watch );

        /// @brief Removes a D-Bus watch object.
        /// It will remove that object's FD from EventManager.
        /// @param [in] watch The D-Bus watch object to remove.
        void removeWatch ( DBusC::DBusWatch * watch );

        /// @brief Adds a D-Bus timeout object.
        /// It will create and setup a timer to handle this timeout.
        /// @param [in] timeout The D-Bus timeout object to remove.
        /// @return True on success; False otherwise.
        uint32_t addTimeout ( DBusC::DBusTimeout * timeout );

        /// @brief Removes a D-Bus timeout object.
        /// It will remove the timer that handles this timeout.
        /// @param [in] timeout The D-Bus timeout object to remove.
        void removeTimeout ( DBusC::DBusTimeout * timeout );

        /// @brief Called when there is a message received (not handled by a pending call object).
        /// @param [in] msg The message received.
        /// @return DBUS_HANDLER_RESULT_HANDLED if the message has been handled and processing should stop;
        ///         DBUS_HANDLER_RESULT_NOT_YET_HANDLED otherwise.
        int filterMessage ( DBusC::DBusMessage * msg );

        /// @brief Called when the dispatch status changes.
        /// @param [in] status The new D-Bus dispatch status.
        void handleDispatchStatus ( int status );

        /// @brief Called when a pending call object receives a reply message.
        /// @param [in] pending The pending call object that received the message.
        void pendingCallNotify ( DBusC::DBusPendingCall * pending );

        /// @brief Called when a message is received for a path with a registered receiver.
        /// @param [in] msg The message that was received
        /// @return DBUS_HANDLER_RESULT_HANDLED if the message has been handled and processing should stop;
        ///         DBUS_HANDLER_RESULT_NOT_YET_HANDLED otherwise.
        int receiveRequest ( DBusC::DBusMessage * msg );

        virtual void receiveFdEvent ( int fd, short int events );
        virtual void timerExpired ( Timer * timer );
        virtual void receiveLoopEndEvent();

    private:
        /// @brief Describes a single watched file descriptor.
        struct WatchPair
        {
            DBusC::DBusWatch * readWatch; ///< D-Bus object to notify about read events.
            DBusC::DBusWatch * writeWatch; ///< D-Bus object to notify about write events.

            /// @brief Default constructor.
            WatchPair(): readWatch ( 0 ), writeWatch ( 0 )
            {
            }
        };

        /// @brief A pointer to DBus connection to system bus.
        static DBusConnection * _systemBus;

        /// @brief A pointer to DBus connection to session bus.
        static DBusConnection * _sessionBus;

        /// @brief All watched file descriptors.
        /// Key is the file descriptor. The value is a read/write watch object pair.
        HashMap<int, WatchPair> _watches;

        /// @brief Maps D-Bus timeout objects to SimpleTimer objects responsible for handling them.
        HashMap<DBusC::DBusTimeout *, SimpleTimer *> _timers;

        /// @brief Maps timer objects to D-Bus timeout objects handled by them.
        HashMap<SimpleTimer *, DBusC::DBusTimeout *> _timeouts;

        /// @brief Maps object paths to their handlers
        HashMap<String, RequestReceiver *> _requestReceivers;

        /// @brief Maps response serial numbers to reply receivers that are waiting for them.
        HashMap<uint32_t, ReplyReceiver *> _replyReceivers;

        /// @brief Maps interface names to the set of signal receivers that want to be notified about those signals.
        HashMap<String, HashSet<SignalReceiver *> > _signalSubs;

        /// @brief Collection of all reply receivers that ever were waiting for responses from this object
        ///        and still haven't removed themselves. Used for cleanup.
        HashSet<ReplyReceiver *> _allReplyReceivers;

        /// @brief Collection of all reply receivers that ever were waiting for signals from this object
        ///        and still haven't removed themselves. Used for cleanup.
        HashSet<SignalReceiver *> _allSignalReceivers;

        /// @brief The internal D-Bus connection pointer.
        DBusC::DBusConnection * _dbusConnection;

        /// @brief The unique name for this connection, as assigned by DBus.
        String _uniqueName;

        /// @brief Adds or removes a signal match rule.
        /// @param [in] interface The name of the signal interface we want to monitor/stop monitoring.
        ///                       This function does NOT verify whether this name is valid!
        /// @param [in] add If true, the matching rule will be added. Otherwise it will be removed.
        void setSignalMatchRule ( const String & interface, bool add );

        friend class DBusConnectionPriv;
};
}
