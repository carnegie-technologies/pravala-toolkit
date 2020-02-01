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

#include "DBusConnectionPriv.hpp"

using namespace Pravala;

uint32_t DBusConnectionPriv::addWatch ( DBusC::DBusWatch * watch, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received watch function call with invalid data pointer" );
        return TRUE;
    }

    return dCon->addWatch ( watch );
}

void DBusConnectionPriv::removeWatch ( DBusC::DBusWatch * watch, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received watch function call with invalid data pointer" );
        return;
    }

    dCon->removeWatch ( watch );
}

void DBusConnectionPriv::toggleWatch ( DBusC::DBusWatch * watch, void * data )
{
    LOG ( L_DEBUG2, "Toggling watch " << watch );

    if ( dbus_watch_get_enabled ( watch ) )
    {
        addWatch ( watch, data );
    }
    else
    {
        removeWatch ( watch, data );
    }
}

uint32_t DBusConnectionPriv::addTimeout ( DBusC::DBusTimeout * timeout, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received timeout function call with invalid data pointer" );
        return TRUE;
    }

    return dCon->addTimeout ( timeout );
}

void DBusConnectionPriv::removeTimeout ( DBusC::DBusTimeout * timeout, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received timeout function call with invalid data pointer" );
        return;
    }

    dCon->removeTimeout ( timeout );
}

void DBusConnectionPriv::toggleTimeout ( DBusC::DBusTimeout * timeout, void * data )
{
    LOG ( L_DEBUG2, "Toggling timeout " << timeout );

    if ( dbus_timeout_get_enabled ( timeout ) )
    {
        addTimeout ( timeout, data );
    }
    else
    {
        removeTimeout ( timeout, data );
    }
}

DBusC::DBusHandlerResult DBusConnectionPriv::filterMessage (
        DBusC::DBusConnection * /* connection */, DBusC::DBusMessage * message, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received message filter function call with invalid data pointer" );
        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return ( DBusC::DBusHandlerResult ) dCon->filterMessage ( message );
}

void DBusConnectionPriv::handleDispatchStatus (
        DBusC::DBusConnection * /* connection */, DBusC::DBusDispatchStatus status, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received dispatch status function call with invalid data pointer" );
        return;
    }

    dCon->handleDispatchStatus ( status );
}

void DBusConnectionPriv::pendingCallNotify ( DBusC::DBusPendingCall * pending, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received pending call notify function call with invalid data pointer" );
        return;
    }

    dCon->pendingCallNotify ( pending );
}

DBusC::DBusHandlerResult DBusConnectionPriv::receiveRequest (
        DBusC::DBusConnection * /* connection */, DBusC::DBusMessage * message, void * data )
{
    DBusConnection * const dCon = static_cast<DBusConnection *> ( data );

    if ( !dCon )
    {
        LOG ( L_FATAL_ERROR, "Received receive request function call with invalid data pointer" );
        return DBusC::DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return ( DBusC::DBusHandlerResult ) dCon->receiveRequest ( message );
}

String DBusConnectionPriv::dispatchStatusStr ( int status )
{
    switch ( ( DBusC::DBusDispatchStatus ) status )
    {
        case DBusC::DBUS_DISPATCH_DATA_REMAINS:
            return "Data-Remains";
            break;

        case DBusC::DBUS_DISPATCH_COMPLETE:
            return "Complete";
            break;

        case DBusC::DBUS_DISPATCH_NEED_MEMORY:
            return "Need-Memory";
            break;
    }

    return String ( "Unknown [%d]" ).arg ( status );
}
