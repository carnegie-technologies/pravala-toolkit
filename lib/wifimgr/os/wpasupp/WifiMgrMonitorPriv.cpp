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

#include "WifiMgrMonitorPriv.hpp"

extern "C"
{
#include "wpa/common/wpa_ctrl.h"
}

// in milliseconds
#define RECONNECT_TIME     60000

/// @brief Don't read anything larger than this
#define MAX_READ_BUFFER    2048

using namespace Pravala;

WifiMgrMonitorPriv::WifiMgrMonitorPriv ( WifiMgrMonitor & owner, const String & ctrlInfo ):
    WpaSuppCore ( ctrlInfo ),
    _owner ( owner ),
    _reconnTimer ( *this ),
    _disconnectCbTimer ( *this ),
    _scanResultsCbTimer ( *this )
{
    restart();
}

WifiMgrMonitorPriv::~WifiMgrMonitorPriv()
{
    reset();
}

void WifiMgrMonitorPriv::receiveFdEvent ( int fd, short int events )
{
    ( void ) fd;
    assert ( fd == getFd() );

    if ( events & EventManager::EventWrite )
    {
        assert ( false ); // we don't subscribe to write events at the moment; this should never happen.
        return;
    }

    // This structure is based on the source code for the wpa_cli client (wpa_cli.c in wpa_supplicant)
    //
    // It appears that wpa_ctrl_pending returns the number of messages or bytes available.
    // The documentation indicates that 0, -1 and 1 are the only valid return values,
    // however this has not been observed to be true (pending returning values >1).
    //
    // <0 is assumed to be an error condition (we reset ourselves)
    // 0 is assumed to mean there is no data left to read
    // >0 is assumed to mean there is more data to be read
    while ( true )
    {
        const int pending = wpa_ctrl_pending ( _conn );

        if ( pending == 0 )
        {
            return;
        }

        if ( pending < 0 )
        {
            LOG ( L_ERROR, "Error in wpa_ctrl_pending, resetting" );
            reset();
            return;
        }

        // for the response from the wpa_ctrl_recv

        size_t memSize = MAX_READ_BUFFER;
        MemHandle buffer ( memSize );
        char * const w = buffer.getWritable();

        if ( buffer.size() < memSize || !w )
        {
            LOG ( L_ERROR, "Unable to allocate memory to read into; ignoring" );
            return;
        }

        int ret = wpa_ctrl_recv ( _conn, w, &memSize );

        buffer.truncate ( memSize );

        if ( ret < 0 )
        {
            LOG ( L_ERROR, "Unable to read from wpa socket" );
            reset();
            return;
        }

        assert ( ret == 0 );

        const StringList results = buffer.toStringList ( "\r\n" );

        // TODO: We need to split out the callback component from this parsing, we should not be calling
        // them in a loop like this (We could be deleted during one of the callback calls).

        // The format seems to be <2>XXXXX
        for ( size_t i = 0; i < results.size(); ++i )
        {
            const String & result = results.at ( i );

            LOG ( L_DEBUG2, "Received an event from wpa supplicant: " << result );

            if ( result.find ( "CTRL-EVENT-CONNECTED" ) > 0 )
            {
                _disconnectCbTimer.stop();
                _owner._owner.wifiStateChanged ( &_owner, WifiMgrTypes::Connected );
            }
            else if ( result.find ( "CTRL-EVENT-DISCONNECTED" ) > 0 )
            {
                // Notify that we've disconnected in 5 seconds
                //
                // This is to work around WPA supplicant sometimes telling us disconnected,
                // but then succeeding to connect to it immediately afterwards. Since to us,
                // this means that it hasn't reached a terminal "disconnected" state and can
                // still transition to "connected" on its own.
                _disconnectCbTimer.start ( 5000 );
            }
            else if ( result.find ( "CTRL-EVENT-TERMINATING" ) > 0 )
            {
                _disconnectCbTimer.stop();
                _owner._owner.wifiStateChanged ( &_owner, WifiMgrTypes::Off );
            }
            else if ( result.find ( "CTRL-EVENT-SCAN-RESULTS" ) > 0 )
            {
                // Notify that we've got scan results in 5 seconds
                //
                // This is to work around WPA supplicant sometimes telling us repeatedly that there are scan results
                // in a short period of time.
                if ( !_scanResultsCbTimer.isActive() )
                {
                    _scanResultsCbTimer.start ( 5000 );
                }
            }
            else if ( result.find ( "Trying to associate with" ) > 0
                      || result.find ( "Trying to authenticate with" ) > 0 )
            {
                _owner._owner.wifiStateChanged ( &_owner, WifiMgrTypes::Associating );
            }
            else if ( result.find ( "Associated with" ) > 0 )
            {
                _owner._owner.wifiStateChanged ( &_owner, WifiMgrTypes::Associated );
            }
            else
            {
                LOG ( L_DEBUG, "Received an unhandled event from wpa supplicant: " << result );
            }
        }
    }
}

void WifiMgrMonitorPriv::reset()
{
    if ( getFd() >= 0 )
    {
        // In case there were some event handlers registered...
        EventManager::removeFdHandler ( getFd() );

        wpa_ctrl_detach ( _conn );
    }

    WpaSuppCore::reset();
}

void WifiMgrMonitorPriv::timerExpired ( Timer * timer )
{
    if ( timer == &_reconnTimer )
    {
        restart();
    }
    else if ( timer == &_disconnectCbTimer )
    {
        _owner._owner.wifiStateChanged ( &_owner, WifiMgrTypes::Disconnected );
    }
    else if ( timer == &_scanResultsCbTimer )
    {
        _owner._owner.wifiScanResultReady ( &_owner );
    }
}

void WifiMgrMonitorPriv::restart()
{
    if ( NOT_OK ( connect() ) )
    {
        _reconnTimer.start ( RECONNECT_TIME );
        return;
    }

    int ret = wpa_ctrl_attach ( _conn );

    if ( ret == -1 )
    {
        LOG ( L_ERROR, "Error attaching to wpa socket to monitor" );
        reset();
        _reconnTimer.start ( RECONNECT_TIME );

        return;
    }
    else if ( ret == -2 )
    {
        LOG ( L_ERROR, "Timeout when attaching to wpa socket to monitor" );
        reset();
        _reconnTimer.start ( RECONNECT_TIME );

        return;
    }
    else if ( ret < 0 )
    {
        LOG ( L_ERROR, "Unknown error attaching to monitor" );
        reset();
        _reconnTimer.start ( RECONNECT_TIME );

        return;
    }

    assert ( getFd() >= 0 );

    // we only read from this connection
    EventManager::setFdHandler ( getFd(), this, EventManager::EventRead );
}
