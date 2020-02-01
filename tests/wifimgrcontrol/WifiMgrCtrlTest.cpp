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

#include "WifiMgrCtrlTest.hpp"

#include "wifimgr/WifiMgrControl.hpp"

#include <cstdio>

using namespace Pravala;

TextLog WifiMgrCtrlTest::_log ( "wifimgr_ctrl_test" );

WifiMgrCtrlTest::WifiMgrCtrlTest()
{
}

WifiMgrCtrlTest::~WifiMgrCtrlTest()
{
}

void WifiMgrCtrlTest::start ( const String & ctrlName )
{
    WifiMgrControl wmCtrl ( ctrlName );

    List<WifiMgrTypes::NetworkInstance> availNetworks;
    List<WifiMgrTypes::NetworkConfiguration> configNetworks;
    WifiMgrTypes::Status status;
    WifiMgrTypes::State state;

    ERRCODE eCode = wmCtrl.getState ( state );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error getting state\n" );
    }
    else
    {
        LOG ( L_INFO, "State " << state );
    }

    eCode = wmCtrl.getStatus ( status );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error getting status\n" );
    }
    else
    {
        if ( status.state != WifiMgrTypes::Connected )
        {
            LOG ( L_INFO, "Not connected; only displaying status state: " << status.state );
        }
        else
        {
            LOG ( L_INFO, "Connected. ID: " << status.id << "; SSID: " << status.ssid << "; BSSID: "
                  << status.bssid << "; pairwise cipher: " << status.pairwiseCipher << "; group cipher: "
                  << status.groupCipher << "; sectype: " << status.secType << "; ip address: " << status.ipAddress );
        }
    }

    eCode = wmCtrl.getAvailableNetworks ( availNetworks );

#ifndef NO_LOGGING
    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error getting available networks" );
    }
    else
    {
        for ( size_t i = 0; i < availNetworks.size(); ++i )
        {
            const WifiMgrTypes::NetworkInstance & net = availNetworks.at ( i );
            LOG ( L_INFO, "Scan result. SSID: " << net.ssid << "; BSSID: " << net.bssid << "; isHidden: "
                  << net.isHidden << "; secType: " << net.secType << "; authType: " << net.l2AuthType
                  << "; sigLvl: " << net.signalLevel << "; freq: " << net.frequency );
        }
    }
#endif

    eCode = wmCtrl.getConfiguredNetworks ( configNetworks );

#ifndef NO_LOGGING
    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error getting configured networks" );
    }
    else
    {
        for ( size_t i = 0; i < configNetworks.size(); ++i )
        {
            const WifiMgrTypes::NetworkConfiguration & net = configNetworks.at ( i );
            LOG ( L_INFO, "Config network. SSID: " << net.ssid << "; secType: " << net.secType
                  << "; isConnected: " << net.isConnected << "; isDisabled: " << net.isDisabled
                  << "; isHidden: " << net.isHidden << "; id: " << net.id << "; l2authType: "
                  << net.l2AuthType );
        }
    }
#endif

    WifiMgrTypes::NetworkProfile open;
    open.ssid = "PravalaHotspot";
    open.secType = WifiMgrTypes::Open;

    eCode = wmCtrl.addNetwork ( open );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error adding open network" );
    }

    WifiMgrTypes::NetworkProfile psk;
    psk.ssid = "Pravala";
    psk.secType = WifiMgrTypes::WPA2_PSK;
    psk.credential = "01123581321345589";

    eCode = wmCtrl.addNetwork ( psk );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error adding psk network" );
    }

    eCode = wmCtrl.removeNetwork ( psk.ssid );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error removing psk network" );
    }
}
