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

#include "../DBusConnection.hpp"

namespace DBusC
{
extern "C"
{
#include <dbus/dbus.h>
}
}

namespace Pravala
{
/// @brief A private D-Bus connection wrapper.
/// It implements all static callbacks used by D-Bus.
/// We put them here so that we wouldn't have to inherit D-Bus headers in DBusConnection's main header file.
class DBusConnectionPriv: protected DBusConnection
{
    protected:
        /// @brief Called when D-Bus adds a new watch.
        /// @param [in] watch The D-Bus watch object being added.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        /// @return True on success; False otherwise.
        static uint32_t addWatch ( DBusC::DBusWatch * watch, void * data );

        /// @brief Called when D-Bus removes an existing watch.
        /// @param [in] watch The D-Bus watch object being removed.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void removeWatch ( DBusC::DBusWatch * watch, void * data );

        /// @brief Called when D-Bus toggles the state of an existing watch (when it becomes or stops being active).
        /// This will actually call either addWatch or removeWatch (based on the new state of the watch).
        /// @param [in] watch The D-Bus watch object that is toggled.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void toggleWatch ( DBusC::DBusWatch * watch, void * data );

        /// @brief Called when D-Bus adds a new timeout.
        /// @param [in] timeout The D-Bus timeout object being added.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        /// @return True on success; False otherwise.
        static uint32_t addTimeout ( DBusC::DBusTimeout * timeout, void * data );

        /// @brief Called when D-Bus removes an existing timeout.
        /// @param [in] timeout The D-Bus timeout object being removed.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void removeTimeout ( DBusC::DBusTimeout * timeout, void * data );

        /// @brief Called when D-Bus toggles the state of an existing timeout (when it becomes or stops being active).
        /// This will actually call either addTimeout or removeTimeout (based on the new state of the timeout).
        /// @param [in] timeout The D-Bus timeout object that is toggled.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void toggleTimeout ( DBusC::DBusTimeout * timeout, void * data );

        /// @brief Called when there is a message received (not handled by a pending call object).
        /// @param [in] connection The D-Bus connection that this message was received over.
        /// @param [in] message The message received.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        /// @return DBUS_HANDLER_RESULT_HANDLED if the message has been handled and processing should stop;
        ///         DBUS_HANDLER_RESULT_NOT_YET_HANDLED otherwise.
        static DBusC::DBusHandlerResult filterMessage (
            DBusC::DBusConnection * connection, DBusC::DBusMessage * message, void * data );

        /// @brief Called when the dispatch status changes.
        /// @param [in] connection The D-Bus connection whose status changed.
        /// @param [in] status The new D-Bus dispatch status.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void handleDispatchStatus (
            DBusC::DBusConnection * connection, DBusC::DBusDispatchStatus status, void * data );

        /// @brief Called when a pending call object receives a reply message.
        /// @param [in] pending The pending call object that received the message.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static void pendingCallNotify ( DBusC::DBusPendingCall * pending, void * data );

        /// @brief Called when a message is received for a registered object path.
        /// @param [in] connection The D-Bus connection that this message was received over.
        /// @param [in] message The message received.
        /// @param [in] data Pointer to the DBusConnection object that configured this callback.
        static DBusC::DBusHandlerResult receiveRequest (
            DBusC::DBusConnection * connection, DBusC::DBusMessage * message, void * data );

        /// @brief Returns the name of a D-Bus dispatch status value.
        /// @param [in] status The status value to convert.
        /// @return The name of the status value.
        static String dispatchStatusStr ( int status );

    private:
        /// @brief Private constructor, doesn't exist.
        DBusConnectionPriv();

        friend class DBusConnection;
};
}
