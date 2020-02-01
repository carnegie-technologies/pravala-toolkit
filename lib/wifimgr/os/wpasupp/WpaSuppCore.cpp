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

#include "WpaSuppCore.hpp"

extern "C"
{
#include "wpa/common/wpa_ctrl.h"
#include <netinet/ip.h>
}

/// @brief Don't read anything larger than this
#define MAX_READ_BUFFER    2048

using namespace Pravala;

TextLog WpaSuppCore::_log ( "wpasupp_core" );

WpaSuppCore::WpaSuppCore ( const String & ctrlName ): _conn ( 0 ), _sockName ( ctrlName )
{
}

WpaSuppCore::~WpaSuppCore()
{
    reset();
}

void WpaSuppCore::reset()
{
    if ( _conn != 0 )
    {
        wpa_ctrl_close ( _conn );
        _conn = 0;
    }
}

ERRCODE WpaSuppCore::connect()
{
    if ( _conn != 0 )
        return Error::Success;

    assert ( _conn == 0 );

    if ( _sockName.length() < 1 )
    {
        LOG ( L_ERROR, "Empty sock name; can't connect\n" );
        return Error::NoNameProvided;
    }

    // open the command socket, using the provided name
    _conn = wpa_ctrl_open ( _sockName.c_str() );

    if ( !_conn )
    {
        LOG ( L_ERROR, "Unable to open sock: " << _sockName );
        reset();
        return Error::OpenFailed;
    }

    String resp;
    ERRCODE eCode = executeCommand ( "PING", resp );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Unable to execute command PING" );
        return eCode;
    }

    if ( resp != "PONG\n" )
    {
        LOG ( L_ERROR, "Error sending command 'PING'. Expected response: 'PONG'. "
              "Received response: '" << resp << "'." );

        reset();
        return Error::SocketFailed;
    }

    return Error::Success;
}

ERRCODE WpaSuppCore::executeCommand ( const String & cmd, String & result )
{
    // for the response from the wpa_ctrl_request
    static char buf[ MAX_READ_BUFFER ];
    size_t len = sizeof ( buf );

    if ( !_conn )
    {
        ERRCODE eCode = connect();

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_ERROR, eCode, "Unable to reconnect to wpa socket, ignoring command: " << cmd );
            return eCode;
        }

        LOG ( L_DEBUG2, "Reconnected wpa socket" );
    }

    LOG ( L_DEBUG2, "Sending command to wpa socket: '" << cmd << "'." );

    // Returns 0 on success; -1 on error; -2 on socket timeout
    int ret = wpa_ctrl_request ( _conn, cmd.c_str(), strlen ( cmd.c_str() ), buf, &len, 0 );

    if ( ret == -1 )
    {
        LOG ( L_ERROR, "Error sending command '" << cmd << "'." );
        return Error::Unknown;
    }
    else if ( ret == -2 )
    {
        LOG ( L_ERROR, "Timeout when sending command '" << cmd << "." );
        return Error::Timeout;
    }

    assert ( len < sizeof ( buf ) );
    result = String ( buf, len );

    return Error::Success;
}

int WpaSuppCore::getFd() const
{
    if ( _conn == 0 )
        return -1;

    return wpa_ctrl_get_fd ( _conn );
}

ERRCODE WpaSuppCore::parseScanResults ( const String & buf, List< WifiMgrTypes::NetworkInstance > & results )
{
    StringList scanResults = buf.split ( "\n" );

    if ( scanResults.size() < 1 )
    {
        LOG ( L_ERROR, "Scan results must have at least 1 row - the header row." );
        return Error::InvalidData;
    }

    for ( size_t i = 1; i < scanResults.size(); ++i )
    {
        // Sample output
        // bssid / frequency / signal level / flags / ssid
        // 00:09:5b:95:e0:4e       2412    208     [WPA-PSK-CCMP]  jkm private
        // 02:55:24:33:77:a3       2462    187     [WPA-PSK-TKIP]  testing
        // 00:09:5b:95:e0:4f       2412    209             jkm guest

        // Keep empty since we need to assume all output has the same number of fields
        StringList fields = scanResults.at ( i ).split ( "\t", true );

        if ( fields.size() != 5 )
        {
            LOG ( L_ERROR, "Scan result '" << scanResults.at ( i ) << "' has unknown format" );
            return Error::InvalidData;
        }

        bool isOk = false;

        WifiMgrTypes::NetworkInstance network;
        network.bssid = fields.at ( 0 );
        network.frequency = fields.at ( 1 ).toInt32 ( &isOk );

        if ( !isOk )
        {
            LOG ( L_INFO, "Unable to parse frequency: " << fields.at ( 1 ) );
            network.frequency = 0;
            // Not fatal since we don't use frequency at this point
        }

        network.signalLevel = fields.at ( 2 ).toInt32 ( &isOk );

        if ( !isOk )
        {
            LOG ( L_ERROR, "Unable to parse signal level: " << fields.at ( 2 ) );
            network.signalLevel = 0;

            return Error::InvalidData;
        }

        // There may be no flags; this isn't an error
        parseNetworkFlags ( fields.at ( 3 ), network );
        network.ssid = fields.at ( 4 );

        results.append ( network );
    }

    return Error::Success;
}

ERRCODE WpaSuppCore::parseListNetworks ( const String & buf, List< WifiMgrTypes::NetworkConfiguration > & results )
{
    StringList netResults = buf.split ( "\n" );

    if ( netResults.size() < 1 )
    {
        LOG ( L_ERROR, "List network results must have at least 1 row - the header row." );
        return Error::InvalidData;
    }

    for ( size_t i = 1; i < netResults.size(); ++i )
    {
        // Sample output
        // network id / ssid / bssid / flags
        // 0       example network any     [CURRENT]

        // Keep empty since we need to assume all output has the same number of fields
        StringList fields = netResults.at ( i ).split ( "\t", true );

        if ( fields.size() != 4 )
        {
            LOG ( L_ERROR, "List network result '" << netResults.at ( i )
                  << "' has unknown format. # fields: " << fields.size() );
            return Error::InvalidData;
        }

        bool isOk = false;

        WifiMgrTypes::NetworkConfiguration network;
        network.id = fields.at ( 0 ).toInt32 ( &isOk );

        if ( !isOk )
        {
            LOG ( L_ERROR, "Unable to parse network id: " << fields.at ( 0 ) );
            network.id = -1;

            return Error::InvalidData;
        }

        network.ssid = fields.at ( 1 );

        // There may be no flags; this isn't an error
        parseNetworkFlags ( fields.at ( 3 ), network );

        results.append ( network );
    }

    return Error::Success;
}

ERRCODE WpaSuppCore::parseStatus ( const String & buf, WifiMgrTypes::Status & status )
{
    StringList statusResults = buf.split ( "\n" );

    if ( statusResults.size() < 1 )
    {
        LOG ( L_ERROR, "Status must have at least 1 row - the wpa_state row." );
        return Error::InvalidData;
    }

    for ( size_t i = 0; i < statusResults.size(); ++i )
    {
        // Sample output
        // bssid=02:00:01:02:03:04
        // ssid=test network
        // pairwise_cipher=CCMP
        // group_cipher=CCMP
        // key_mgmt=WPA-PSK
        // wpa_state=COMPLETED
        // ip_address=192.168.1.21
        // Supplicant PAE state=AUTHENTICATED
        // suppPortStatus=Authorized
        // EAP state=SUCCESS

        StringList fields = statusResults.at ( i ).split ( "=" );

        if ( fields.size() != 2 )
        {
            LOG ( L_ERROR, "Status result '" << statusResults.at ( i ) << "' has unknown format" );
            return Error::InvalidData;
        }

        bool isOk = false;

        if ( fields.at ( 0 ) == "bssid" )
        {
            status.bssid = fields.at ( 1 );
        }
        else if ( fields.at ( 0 ) == "ssid" )
        {
            status.ssid = fields.at ( 1 );
        }
        else if ( fields.at ( 0 ) == "ip_address" )
        {
            status.ipAddress = IpAddress ( fields.at ( 1 ) );
        }
        else if ( fields.at ( 0 ) == "wpa_state" )
        {
            if ( fields.at ( 1 ) == "COMPLETED" )
            {
                status.state = WifiMgrTypes::Connected;
            }
            else if ( fields.at ( 1 ) == "INACTIVE" )
            {
                status.state = WifiMgrTypes::Disconnected;
            }
            else
            {
                status.state = WifiMgrTypes::Associating;
            }
        }
        else if ( fields.at ( 0 ) == "key_mgmt" )
        {
            if ( fields.at ( 1 ) == "NONE" )
            {
                status.secType = WifiMgrTypes::Open;
            }
            else if ( fields.at ( 1 ) == "WPA2-PSK" )
            {
                status.secType = WifiMgrTypes::WPA2_PSK;
            }
            else if ( fields.at ( 1 ) == "WPA-PSK" )
            {
                status.secType = WifiMgrTypes::WPA_PSK;
            }
            else if ( fields.at ( 1 ) == "WPA2-EAP" )
            {
                status.secType = WifiMgrTypes::WPA2_EAP;
            }
            else if ( fields.at ( 1 ) == "WPA-EAP" )
            {
                status.secType = WifiMgrTypes::WPA_EAP;
            }
            else
            {
                LOG ( L_ERROR, "Unknown security type: " << fields.at ( 1 ) << ". Defaulting to open." );
                status.secType = WifiMgrTypes::Open;
            }
        }
        else if ( fields.at ( 0 ) == "pairwise_cipher" )
        {
            isOk = strToCipher ( fields.at ( 1 ), status.pairwiseCipher );

            if ( !isOk )
            {
                LOG ( L_ERROR, "Unknown pairwise cipher: " << fields.at ( 1 ) << ". Defaulting to none." );
                status.pairwiseCipher = WifiMgrTypes::NoCipher;
            }
        }
        else if ( fields.at ( 0 ) == "group_cipher" )
        {
            isOk = strToCipher ( fields.at ( 1 ), status.groupCipher );

            if ( !isOk )
            {
                LOG ( L_ERROR, "Unknown group cipher: " << fields.at ( 1 ) << ". Defaulting to none." );
                status.groupCipher = WifiMgrTypes::NoCipher;
            }
        }
        else if ( fields.at ( 0 ) == "id" )
        {
            status.id = fields.at ( 1 ).toInt32 ( &isOk );

            if ( !isOk )
            {
                LOG ( L_ERROR, "Unable to parse id: " << fields.at ( 1 ) );
                status.id = -1;
            }
        }
    }

    return Error::Success;
}

ERRCODE WpaSuppCore::parseNetworkFlags ( const String & flags, WifiMgrTypes::NetworkInstance & network )
{
    StringList flagList = flags.split ( "[]" );

    if ( flagList.size() < 1 )
    {
        LOG ( L_DEBUG, "No flags provided" );
        return Error::NothingToDo;
    }

    for ( size_t i = 0; i < flagList.size(); ++i )
    {
        // Sample flags
        // [WPA-PSK-CCMP] etc.

        const String & flag = flagList.at ( i );
        LOG ( L_DEBUG2, "Found flag: " << flag );

        if ( flag.find ( "WPA-PSK" ) >= 0 )
        {
            network.secType = WifiMgrTypes::WPA_PSK;
            network.l2AuthType = WifiMgrTypes::None;
        }
        else if ( flag.find ( "WPA2-PSK" ) >= 0 )
        {
            network.secType = WifiMgrTypes::WPA2_PSK;
            network.l2AuthType = WifiMgrTypes::None;
        }
    }

    return Error::Success;
}

ERRCODE WpaSuppCore::parseNetworkFlags ( const String & flags, WifiMgrTypes::NetworkConfiguration & network )
{
    StringList flagList = flags.split ( "[]" );

    if ( flagList.size() < 1 )
    {
        LOG ( L_DEBUG, "No flags provided" );
        return Error::NothingToDo;
    }

    for ( size_t i = 0; i < flagList.size(); ++i )
    {
        // Sample flags
        // [CURRENT] etc.

        const String & flag = flagList.at ( i );
        LOG ( L_DEBUG2, "Found flag: " << flag );

        if ( flag.find ( "DISABLED" ) >= 0 )
        {
            network.isDisabled = true;
        }
        else if ( flag.find ( "CURRENT" ) >= 0 )
        {
            network.isConnected = true;
        }
    }

    return Error::Success;
}

bool WpaSuppCore::strToCipher ( const String & in, WifiMgrTypes::Cipher & out )
{
    if ( in.find ( "TKIP" ) >= 0 && in.find ( "CCMP" ) >= 0 )
    {
        out = WifiMgrTypes::TKIP_CCMP;
    }
    else if ( in.find ( "TKIP" ) >= 0 )
    {
        out = WifiMgrTypes::TKIP;
    }
    else if ( in.find ( "CCMP" ) >= 0 )
    {
        out = WifiMgrTypes::CCMP;
    }
    else if ( in.find ( "NONE" ) >= 0 )
    {
        out = WifiMgrTypes::NoCipher;
    }
    else
    {
        return false;
    }

    return true;
}
