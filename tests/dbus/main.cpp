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

#include "app/StdApp.hpp"
#include "log/TextLog.hpp"
#include "dbus/DBusConnection.hpp"
#include "dbus/DBusMessageReader.hpp"
#include "dbus/DBusMessageWriter.hpp"

using namespace Pravala;

/// @brief Log stream.
static TextLog _log ( "dbus_test" );

/// @brief Receives requests for /number and receives replies and signals from D-Bus connection.
/// Requests for the path '/number' with the interface and method 'com.pravala.Number.Add' must have two numbers.
/// This class will add those two numbers and send the sum back in the response message.
class MyReceiver:
    public DBusConnection::RequestReceiver,
    public DBusConnection::ReplyReceiver,
    public DBusConnection::SignalReceiver
{
    public:
        static const String interface;
        static const String method;
        static const String path;

        MyReceiver()
        {
            DBusConnection::getSys().registerRequestReceiver ( this, path );
        }

    protected:
        virtual bool dbusReceiveRequest ( DBusConnection * connection, DBusMessageReader & msg )
        {
            if ( msg.getInterface() != interface )
            {
                LOG ( L_ERROR, "Incoming request is for unexpected interface: " << msg.getInterface() );
                return false;
            }

            if ( msg.getMethod() != method )
            {
                LOG ( L_ERROR, "Incoming request is for unexpected method: " << msg.getMethod() );
                return false;
            }

            int64_t a = 0;
            int64_t b = 0;

            if ( !msg.getNextArg ( a ) || !msg.getNextArg ( b ) )
            {
                return false;
            }

            DBusMessageWriter response ( msg );

            response.appendArg ( a + b );

            return connection->send ( response );
        }

        virtual void dbusReceiveReply ( DBusConnection * connection, uint32_t msgSerial, DBusMessageReader * msg )
        {
            ( void ) connection;
            ( void ) msgSerial;

            if ( !msg )
            {
                LOG ( L_ERROR, "Received invalid message pointer" );
                return;
            }

            LOG ( msg->isError() ? L_ERROR : L_INFO, "Received D-Bus response; Connection: " << connection
                  << "; Serial: " << msgSerial << "; Message: '" << ( *msg ) << "'" );

            LOG ( L_DEBUG2, "Message: '" << ( *msg ) << "'; " << msg->dumpArgs() );

            delete msg;
            msg = 0;
        }

        virtual void dbusReceiveSignal ( DBusConnection * connection, DBusMessageReader & msg )
        {
            ( void ) connection;
            ( void ) msg;

            LOG ( L_INFO, "Received D-Bus signal; Connection: " << connection << "; Message: '" << msg << "'" );

            LOG ( L_DEBUG2, "Message: '" << msg << "'; " << msg.dumpArgs() );
        }
};

/// @brief Receives responses to an org.freedesktop.DBus.Properties.GetAll request and logs the version property
class GetAllPropertiesReceiver: public DBusConnection::ReplyReceiver
{
    protected:
        virtual void dbusReceiveReply ( DBusConnection * connection, uint32_t msgSerial, DBusMessageReader * msg )
        {
            ( void ) connection;
            ( void ) msgSerial;

            if ( !msg )
            {
                LOG ( L_ERROR, "Received invalid message pointer" );
                return;
            }

            LOG ( msg->isError() ? L_ERROR : L_INFO, "Received D-Bus response; Connection: " << connection
                  << "; Serial: " << msgSerial << "; Message: '" << ( *msg ) << "'" );

            LOG ( L_DEBUG2, "Message: '" << ( *msg ) << "'; " << msg->dumpArgs() );

            // Response for GetAllProperties should have signature "a{sv}"
            if ( !msg->openContainer() )
            {
                LOG ( L_ERROR, "Unable to open container: Expected array of properties" );
            }

            // Loop until there are no more dictionary entries
            for ( ; msg->hasNextArg() && msg->openContainer(); msg->closeContainer() )
            {
                String property;

                if ( !msg->getNextArg ( property ) )
                {
                    LOG ( L_ERROR, "First argument in dictionary entry is not a string" );
                    continue;
                }

                if ( property == "Version" )
                {
                    String version;

                    if ( !msg->getNextArg ( version ) )
                    {
                        LOG ( L_ERROR, "'Version' property has incorrect type. Expected 'vs'" );
                    }
                    else
                    {
                        LOG ( L_INFO, "Network Manager version is '" << version << "'" );
                    }

                    // Close the dictionary entry
                    msg->closeContainer();

                    break;
                }
            }

            // Close the array of dictionary entries
            msg->closeContainer();

            delete msg;
            msg = 0;
        }
};

const String MyReceiver::interface ( "com.pravala.Number" );
const String MyReceiver::method ( "Add" );
const String MyReceiver::path ( "/number" );

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv );

    app.init();

    MyReceiver myReceiver;
    GetAllPropertiesReceiver getAllPropertiesReceiver;

    DBusConnection & sysCon = DBusConnection::getSys();

    if ( !sysCon.isOpen() )
    {
        LOG ( L_ERROR, "Could not initialize D-Bus system bus connection" );
        return EXIT_FAILURE;
    }

    LOG ( L_INFO, "Opened system bus connection with name '" << sysCon.getUniqueName() << "'" );

    // To get session bus:
    // DBusConnection & sesCon = DBusConnection::getSession();

    DBusMessageWriter msgA (
            "org.freedesktop.NetworkManager",
            "/org/freedesktop/NetworkManager",
            "org.freedesktop.DBus.Properties",
            "GetAll" );

    msgA.appendArg ( "org.freedesktop.NetworkManager" );

    uint32_t msgSerial = sysCon.sendWithReply ( &getAllPropertiesReceiver, msgA );

    if ( msgSerial < 1 )
    {
        LOG ( L_ERROR, "Error sending D-Bus message: " << msgA );
        return EXIT_FAILURE;
    }

    LOG ( L_INFO, "Message sent; Serial: " << msgSerial );

    DBusMessageWriter msgB (
            "org.freedesktop.NetworkManager",
            "/org/freedesktop/NetworkManager",
            "org.freedesktop.NetworkManager",
            "GetLogging" );

    // To get invalid method signature:
    // msgB.appendArg ( "superfluous argument" );

    msgSerial = sysCon.sendWithReply ( &myReceiver, msgB );

    if ( msgSerial < 1 )
    {
        LOG ( L_ERROR, "Error sending D-Bus message: " << msgB );
        return EXIT_FAILURE;
    }

    // If we don't want to see responses:
    // sysCon.removeReceiver ( &myReceiver );

    sysCon.registerSignalReceiver ( &myReceiver, "org.freedesktop.NetworkManager" );

    EventManager::run();

    DBusConnection::closeSys();

    return 0;
}
