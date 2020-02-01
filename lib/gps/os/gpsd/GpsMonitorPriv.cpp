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

#include "GpsMonitorPriv.hpp"

#include "config/ConfigString.hpp"
#include "basic/IpAddress.hpp"
#include "json/Json.hpp"

#include "GpsdTPV.hpp"
#include "GpsdDevices.hpp"

#include <cerrno>

extern "C"
{
#include <unistd.h>
}

using namespace Pravala;

#define MAX_READ_SIZE    2048

static ConfigString optGpsdAddress (
        ConfigOpt::FlagInitializeOnly,
        "os.gpsd_address.*",
        "Listening socket address of the gpsd daemon to use for GPS location; set as an "
        "ip_address:port_number pair",
        "127.0.0.1:2947"
);

TextLog GpsMonitorPriv::_log ( "gpsd_client" );

GpsMonitorPriv::GpsMonitorPriv ( GpsMonitor & owner ): _owner ( owner )
{
}

bool GpsMonitorPriv::start()
{
    if ( !optGpsdAddress.isSet() || optGpsdAddress.isEmpty() )
    {
        LOG ( L_ERROR, "No value is set for os.gpsd_address; can't connect to the gpsd instance" );
        return false;
    }

    // If we are already connected, we don't need to re-connect.
    if ( _sock.getSock() >= 0 )
        return true;

    IpAddress ipAddr;
    uint16_t ipPort;

    if ( !IpAddress::convertAddrSpec ( optGpsdAddress.value(), ipAddr, ipPort ) )
    {
        LOG ( L_ERROR, "Unable to convert provided value for os.gpsd_address to an IP address & port pair; "
              "ensure that the provided value is in the format 'ip_address:port'" );
        return false;
    }

    ERRCODE eCode = _sock.init ( ipAddr.isIPv4() ? SocketApi::SocketStream4 : SocketApi::SocketStream6 );

    UNTIL_ERROR ( eCode, _sock.setNonBlocking() );
    UNTIL_ERROR ( eCode, _sock.connect ( ipAddr, ipPort ) );

    if ( eCode != Error::Success && eCode != Error::ConnectInProgress )
    {
        LOG ( L_ERROR, "Unable to connect to " << ipAddr << ":" << ipPort << "; " << eCode.toString() );
        return false;
    }

    EventManager::setFdHandler ( _sock.getSock(), this, EventManager::EventRead );

    sendWatchCommand ( true );
    return true;
}

void GpsMonitorPriv::stop()
{
    _sock.close();
}

void GpsMonitorPriv::receiveFdEvent ( int fd, short int events )
{
    ( void ) fd;
    assert ( fd == _sock.getSock() );

    if ( events & EventManager::EventRead )
    {
        char * const toRead = _toProcess.getAppendable ( MAX_READ_SIZE );

        assert ( toRead != 0 );

        int ret = -1;

        if ( !toRead )
        {
            errno = ENOMEM;
        }
        else
        {
            ret = ::read ( _sock.getSock(), toRead, MAX_READ_SIZE );
        }

        if ( ret <= 0 )
        {
            if ( ret != 0 )
            {
                LOG ( L_ERROR, "Unable to read from gpsd socket: " << strerror ( errno ) );
            }
            else
            {
                LOG ( L_ERROR, "gpsd socket closed" );
            }

            _sock.close();
            return;
        }

        _toProcess.markAppended ( ret );

        processData();
    }

    if ( events & EventManager::EventWrite )
    {
        int ret = ::write ( _sock.getSock(), _toWrite.get(), _toWrite.size() );
        LOG ( L_INFO, "Wrote " << ret << " bytes of " << _toWrite.size() << " bytes to gpsd socket" );

        if ( ret < 0 )
        {
            LOG ( L_ERROR, "Unable to write to gpsd socket: " << strerror ( errno ) );
            _sock.close();
            return;
        }
        else if ( ( size_t ) ret < _toWrite.size() )
        {
            _toWrite.consumeData ( ( size_t ) ret );
        }
        else
        {
            assert ( ( size_t ) ret == _toWrite.size() );
            EventManager::disableWriteEvents ( _sock.getSock() );
        }
    }
}

void GpsMonitorPriv::sendWatchCommand ( bool isActive )
{
    Json watch;
    watch.put ( "enable", isActive );
    watch.put ( "json", isActive );

    String msg ( "?WATCH=" );
    watch.encode ( msg, false );
    msg.append ( "\n" );

    _toWrite.clear();
    _toWrite.append ( msg );

    EventManager::enableWriteEvents ( _sock.getSock() );
}

void GpsMonitorPriv::processData()
{
    // We keep the empty lines so we can determine if the last line is empty or not;
    // A non-empty last line tells us we have a partially received line which we need to buffer for the next read.
    const StringList lines = _toProcess.toStringList ( "\r\n", true );
    _toProcess.clear();

    for ( size_t i = 0; i < lines.size(); ++i )
    {
        const String & line = lines.at ( i );

        if ( line.isEmpty() )
            continue;

        // This is the last line; if we are here it isn't empty, which means we have a non-empty last line;
        // so we need to buffer it
        if ( ( i + 1 ) >= lines.size() )
        {
            _toProcess.append ( line );
            return;
        }

        Json parsedLine;

        if ( !parsedLine.decode ( line ) )
        {
            // We aren't on an empty line and we aren't on the last line (those are both checked above)
            // so the received JSON is invalid; log the error
            LOG ( L_ERROR, "Unable to decode the JSON provided from gpsd: " << parsedLine.getLastErrorMessage() );
            continue;
        }

        String classField;
        if ( !parsedLine.get ( "class", classField ) )
        {
            LOG ( L_ERROR, "Processing a JSON object without a 'class' field; this is invalid" );
            continue;
        }

        if ( classField == "DEVICES" )
        {
            GpsdDevices devices;
            if ( !devices.parse ( parsedLine ) )
            {
                LOG ( L_ERROR, "Error parsing DEVICES object; skipping" );
                continue;
            }

            if ( devices.getDevices().size() > 0 )
            {
                LOG ( L_DEBUG2, "Received a set of devices from gpsd; first one is " << devices.getDevices().first() );
            }
            else
            {
                LOG ( L_DEBUG2, "Received an empty set of devices from gpsd" );
            }

            _owner._receiver.gpsDevicesChanged ( devices.getDevices() );
            return;
        }
        else if ( classField == "TPV" )
        {
            GpsdTPV tpv;
            if ( !tpv.parse ( parsedLine ) )
            {
                LOG ( L_ERROR, "Error parsing TPV object; skipping" );
                continue;
            }

            if ( !tpv.hasGpsLock() )
            {
                LOG ( L_DEBUG, "Received a TPV message with no lock from gpsd" );
            }
            else
            {
                LOG ( L_DEBUG2, "Received a TPV message from gpsd; lat=" << tpv.getLocation().latitude.value
                      << "; lon=" << tpv.getLocation().longitude.value
                      << "; alt=" << tpv.getLocation().altitude.value );

                _owner._receiver.gpsUpdate ( tpv.getLocation(), tpv.getVector() );
                return;
            }
        }
        else
        {
            LOG ( L_DEBUG2, "Received a message of class " << classField << " from gpsd; ignoring" );
        }
    }
}
