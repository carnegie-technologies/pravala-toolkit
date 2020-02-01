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

#include "../../WifiMgrControl.hpp"

#include "WifiMgrControlPriv.hpp"

using namespace Pravala;

TextLog WifiMgrControl::_log ( "wifimgr_control" );

WifiMgrControl::WifiMgrControl ( const String & ctrlInfo ): _p ( new WifiMgrControlPriv ( ctrlInfo ) )
{
}

WifiMgrControl::~WifiMgrControl()
{
    delete _p;
    _p = 0;
}

ERRCODE WifiMgrControl::getConfiguredNetworks ( List<WifiMgrTypes::NetworkConfiguration> & networks )
{
    String resp;
    ERRCODE eCode = _p->executeCommand ( WifiMgrControlPriv::ListNetworksCmd, resp );

    if ( IS_OK ( eCode ) )
    {
        networks.clear();
        return _p->parseListNetworks ( resp, networks );
    }

    LOG_ERR ( L_ERROR, eCode, "Unable to get configured networks" );
    return eCode;
}

ERRCODE WifiMgrControl::getAvailableNetworks ( List<WifiMgrTypes::NetworkInstance> & networks )
{
    String resp;
    ERRCODE eCode = _p->executeCommand ( WifiMgrControlPriv::ScanResultsCmd, resp );

    if ( IS_OK ( eCode ) )
    {
        networks.clear();
        return _p->parseScanResults ( resp, networks );
    }

    LOG_ERR ( L_ERROR, eCode, "Unable to get available networks" );
    return eCode;
}

ERRCODE WifiMgrControl::getStatus ( WifiMgrTypes::Status & status )
{
    String resp;
    ERRCODE eCode = _p->executeCommand ( WifiMgrControlPriv::StatusCmd, resp );

    if ( IS_OK ( eCode ) )
    {
        status.clear();
        return _p->parseStatus ( resp, status );
    }

    LOG_ERR ( L_ERROR, eCode, "Unable to get status" );
    return eCode;
}

ERRCODE WifiMgrControl::getState ( WifiMgrTypes::State & state )
{
    WifiMgrTypes::Status status;
    ERRCODE eCode = getStatus ( status );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error retrieving status for state" );
        return eCode;
    }

    state = status.state;
    return Error::Success;
}

ERRCODE WifiMgrControl::addNetwork ( const WifiMgrTypes::NetworkProfile & network, bool enable )
{
    // NOTE: Order of quotes here is critical! must put additional quotes around text
    // Adding a network with just an SSID requires 2 commands be sent:
    // ADD_NETWORK - returns netId
    // SET_NETWORK <netId> ssid "<arg>" - returns OK/FAIL
    // SET_NETWORK <netId> key_mgmt NONE - returns OK/FAIL
    // ENABLE_NETWORK <netId> - returns OK/FAIL

    // Adding a network with a PSK requires also sending
    // SET_NETWORK <netId> key_mgmt WPA-PSK - returns OK/FAIL
    // SET_NETWORK <netId> psk "<credential>" - returns OK/FAIL
    // ENABLE_NETWORK <netId> - returns OK/FAIL

    // Adding a network with EAP requires also sending
    // SET_NETWORK <netId> key_mgmt WPA-EAP IEEE8021X - returns OK/FAIL
    // SET_NETWORK <netId> eap <eap> - returns OK/FAIL
    // SET_NETWORK <netId> identity "<identity>" - returns OK/FAIL
    // SET_NETWORK <netId> password "<password>" - returns OK/FAIL
    // ENABLE_NETWORK <netId> - returns OK/FAIL

    String resp;

    ERRCODE eCode = _p->executeCommand ( WifiMgrControlPriv::AddNetworkCmd, resp );

    bool isOk = false;
    int id = resp.trimmed().toInt32 ( &isOk );

    if ( NOT_OK ( eCode ) || !isOk || resp == WifiMgrControlPriv::FailResult )
    {
        LOG ( L_ERROR, "Unable to add network. Failed in: ADD_NETWORK. Return:" << resp );
        return Error::Unknown;
    }

    // Leaving this on its own so that the full command creation below is clear
    String cmd;

    cmd.append ( " ssid \"" ).append ( network.ssid ).append ( "\"" );

    if ( !_p->executeSetNetworkCommand ( id, cmd ) )
    {
        return Error::InvalidParameter;
    }

    if ( network.isHidden )
    {
        if ( !_p->executeSetNetworkCommand ( id, " scan_ssid 1" ) )
        {
            return Error::InvalidParameter;
        }
    }

    if ( network.secType == WifiMgrTypes::WPA_PSK || network.secType == WifiMgrTypes::WPA2_PSK )
    {
        cmd = " key_mgmt WPA-PSK";
    }
    else if ( network.secType == WifiMgrTypes::WPA_EAP || network.secType == WifiMgrTypes::WPA2_EAP )
    {
        cmd = " key_mgmt IEEE8021X";
    }
    else
    {
        cmd = " key_mgmt NONE";
    }

    if ( !_p->executeSetNetworkCommand ( id, cmd ) )
    {
        return Error::InvalidParameter;
    }

    if ( network.secType == WifiMgrTypes::WPA_PSK || network.secType == WifiMgrTypes::WPA_EAP
         || network.secType == WifiMgrTypes::WPA2_PSK || network.secType == WifiMgrTypes::WPA2_EAP )
    {
        if ( network.secType == WifiMgrTypes::WPA2_PSK || network.secType == WifiMgrTypes::WPA2_EAP )
        {
            // RSN = WPA2, so force WPA2 mode
            cmd = " proto RSN";
        }
        else
        {
            // Otherwise, prefer WPA2, but allow both WPA and WPA2
            cmd = " proto RSN WPA";
        }

        if ( !_p->executeSetNetworkCommand ( id, cmd ) )
        {
            return Error::InvalidParameter;
        }
    }

    if ( network.secType == WifiMgrTypes::WEP )
    {
        cmd.clear();
        cmd.append ( " wep_key0 " ).append ( network.credential );

        if ( !_p->executeSetNetworkCommand ( id, cmd ) )
        {
            return Error::InvalidParameter;
        }
    }
    else if ( network.secType == WifiMgrTypes::WPA_PSK || network.secType == WifiMgrTypes::WPA2_PSK )
    {
        cmd.clear();
        cmd.append ( " psk \"" ).append ( network.credential ).append ( "\"" );

        if ( !_p->executeSetNetworkCommand ( id, cmd ) )
        {
            return Error::InvalidParameter;
        }
    }
    else if ( network.secType == WifiMgrTypes::WPA_EAP || network.secType == WifiMgrTypes::WPA2_EAP )
    {
        cmd.clear();
        cmd.append ( " eap " );

        switch ( network.l2AuthType )
        {
            case WifiMgrTypes::PEAP_GTC:
            case WifiMgrTypes::PEAP_MSCHAP:
            case WifiMgrTypes::PEAP_PAP:
                cmd.append ( "PEAP" );
                break;

            case WifiMgrTypes::AKA:
                cmd.append ( "EAP-AKA" );
                break;

            case WifiMgrTypes::SIM:
                cmd.append ( "EAP-SIM" );
                break;

            default:
                LOG ( L_ERROR, "Unsupported EAP version specified, ignoring" );
                cmd.clear();
                cmd.append ( WifiMgrControlPriv::RemoveNetworkPrefix ).append ( String::number ( id ) );

                resp.clear();
                _p->executeCommand ( cmd, resp );
                return Error::InvalidParameter;
        }

        if ( !_p->executeSetNetworkCommand ( id, cmd ) )
        {
            return Error::InvalidParameter;
        }

        if ( network.l2AuthType == WifiMgrTypes::PEAP_GTC
             || network.l2AuthType == WifiMgrTypes::PEAP_MSCHAP
             || network.l2AuthType == WifiMgrTypes::PEAP_PAP )
        {
            cmd.clear();
            cmd.append ( " identity \"" ).append ( network.identifier ).append ( "\"" );

            if ( !_p->executeSetNetworkCommand ( id, cmd ) )
            {
                return Error::InvalidParameter;
            }

            cmd.clear();
            cmd.append ( " password \"" ).append ( network.credential ).append ( "\"" );

            if ( !_p->executeSetNetworkCommand ( id, cmd ) )
            {
                return Error::InvalidParameter;
            }
        }
    }

    if ( enable )
    {
        return enableNetwork ( id );
    }

    return Error::Success;
}

ERRCODE WifiMgrControl::removeNetwork ( const String & ssid )
{
    List<WifiMgrTypes::NetworkConfiguration> configuredNetworks;
    ERRCODE eCode = getConfiguredNetworks ( configuredNetworks );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Unable to remove network since we couldn't get networks" );
        return eCode;
    }

    bool hasRemoved = false;

    for ( size_t i = 0; i < configuredNetworks.size(); ++i )
    {
        const WifiMgrTypes::NetworkConfiguration & configuredNetwork = configuredNetworks.at ( i );

        // We need to use the SSID for now since list_networks doesn't seem to return the security type!
        if ( ssid == configuredNetwork.ssid )
        {
            String cmd;
            cmd.append ( WifiMgrControlPriv::RemoveNetworkPrefix ).append ( String::number ( configuredNetwork.id ) );

            if ( !_p->executeBoolCommand ( cmd ) )
            {
                LOG ( L_ERROR, "Unknown error executing remove command " << cmd );
                return Error::Unknown;
            }

            hasRemoved = true;
        }
    }

    if ( !hasRemoved )
    {
        LOG ( L_DEBUG, "Trying to remove a non-configured network; not possible" );
        return Error::NotFound;
    }

    return Error::Success;
}

ERRCODE WifiMgrControl::enableNetwork ( int id )
{
    String cmd;

    cmd.append ( WifiMgrControlPriv::EnableNetworkPrefix ).append ( String::number ( id ) );

    if ( !_p->executeBoolCommand ( cmd ) )
    {
        LOG ( L_ERROR, "Unknown error executing enable command " << cmd );
        return Error::Unknown;
    }

    return Error::Success;
}

ERRCODE WifiMgrControl::disableNetwork ( int id )
{
    String cmd;

    cmd.append ( WifiMgrControlPriv::DisableNetworkPrefix ).append ( String::number ( id ) );

    if ( !_p->executeBoolCommand ( cmd ) )
    {
        LOG ( L_ERROR, "Unknown error executing disable command " << cmd );
        return Error::Unknown;
    }

    return Error::Success;
}

ERRCODE WifiMgrControl::disableAllNetworks()
{
    String cmd;

    cmd.append ( WifiMgrControlPriv::DisableNetworkPrefix ).append ( "all" );

    if ( !_p->executeBoolCommand ( cmd ) )
    {
        LOG ( L_ERROR, "Unknown error executing disable command " << cmd );
        return Error::Unknown;
    }

    return Error::Success;
}

ERRCODE WifiMgrControl::scan()
{
    String cmd;

    cmd.append ( WifiMgrControlPriv::ScanCmd );

    if ( !_p->executeBoolCommand ( cmd ) )
    {
        LOG ( L_ERROR, "Unknown error executing scan command: " << cmd );
        return Error::Unknown;
    }

    return Error::Success;
}
