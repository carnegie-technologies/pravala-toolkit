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

#include "WifiMgrTypes.hpp"

using namespace Pravala;

const char * WifiMgrTypes::authTypeToStr ( WifiMgrTypes::AuthType authType )
{
    switch ( authType )
    {
        case None:
            return "none";

        case WISPr1:
            return "wispr1";

        case WISPr2:
            return "wispr2";

        case PEAP:
            return "peap";

        case PEAP_MSCHAP:
            return "peap-mschap";

        case PEAP_PAP:
            return "peap-pap";

        case PEAP_GTC:
            return "peap-gtc";

        case TLS:
            return "tls";

        case TTLS:
            return "ttls";

        case SIM:
            return "sim";

        case AKA:
            return "aka";

        case FAST:
            return "fast";

        default:
            return "unknown";
    }
}

const char * WifiMgrTypes::secTypeToStr ( WifiMgrTypes::SecType secType )
{
    switch ( secType )
    {
        case Open:
            return "open";

        case WEP:
            return "wep";

        case WPA_PSK:
            return "wpa-psk";

        case WPA2_PSK:
            return "wpa2-psk";

        case WPA_EAP:
            return "wpa-eap";

        case WPA2_EAP:
            return "wpa2-eap";

        default:
            return "unknown";
    }
}

const char * WifiMgrTypes::stateToStr ( State state )
{
    switch ( state )
    {
        case Off:
            return "off";

        case Disconnected:
            return "disconnected";

        case Associated:
            return "associated";

        case Associating:
            return "associating";

        case Connected:
            return "connected";

        default:
            return "unknown";
    }
}

WifiMgrTypes::AuthType WifiMgrTypes::strToAuthType ( const String & authTypeStr )
{
    if ( authTypeStr == "none" )
    {
        return None;
    }
    else if ( authTypeStr == "wispr1" )
    {
        return WISPr1;
    }
    else if ( authTypeStr == "wispr2" )
    {
        return WISPr2;
    }
    else if ( authTypeStr == "peap" )
    {
        return PEAP;
    }
    else if ( authTypeStr == "peap-mschap" )
    {
        return PEAP_MSCHAP;
    }
    else if ( authTypeStr == "peap-pap" )
    {
        return PEAP_PAP;
    }
    else if ( authTypeStr == "peap-gtc" )
    {
        return PEAP_GTC;
    }
    else if ( authTypeStr == "tls" )
    {
        return TLS;
    }
    else if ( authTypeStr == "ttls" )
    {
        return TTLS;
    }
    else if ( authTypeStr == "sim" )
    {
        return SIM;
    }
    else if ( authTypeStr == "aka" )
    {
        return AKA;
    }
    else if ( authTypeStr == "fast" )
    {
        return FAST;
    }
    else
    {
        return UnknownAuthType;
    }
}

WifiMgrTypes::SecType WifiMgrTypes::strToSecType ( const String & secTypeStr )
{
    if ( secTypeStr == "open" )
    {
        return Open;
    }
    else if ( secTypeStr == "wep" )
    {
        return WEP;
    }
    else if ( secTypeStr == "wpa-psk" )
    {
        return WPA_PSK;
    }
    else if ( secTypeStr == "wpa2-psk" )
    {
        return WPA2_PSK;
    }
    else if ( secTypeStr == "wpa-eap" )
    {
        return WPA_EAP;
    }
    else if ( secTypeStr == "wap2-eap" )
    {
        return WPA2_EAP;
    }
    else
    {
        return UnknownSecType;
    }
}

const char * WifiMgrTypes::cipherToStr ( WifiMgrTypes::Cipher cipher )
{
    switch ( cipher )
    {
        case NoCipher:
            return "none";

        case TKIP:
            return "TKIP";

        case CCMP:
            return "CCMP";

        case TKIP_CCMP:
            return "TKIP-CCMP";

        default:
            return "unknown";
    }
}

WifiMgrTypes::NetworkCommon::NetworkCommon():
    isHidden ( false ), secType ( Open ), l2AuthType ( None )
{
}

bool WifiMgrTypes::NetworkCommon::operator== ( const WifiMgrTypes::NetworkCommon & other ) const
{
    return ( isHidden == other.isHidden
             && secType == other.secType
             && l2AuthType == other.l2AuthType );
}

WifiMgrTypes::NetworkInstance::NetworkInstance(): signalLevel ( 0 ), frequency ( 0 )
{
}

WifiMgrTypes::NetworkProfile::NetworkProfile(): l3AuthType ( None )
{
}

WifiMgrTypes::NetworkConfiguration::NetworkConfiguration(): id ( -1 ), isConnected ( false ), isDisabled ( false )
{
}

WifiMgrTypes::Status::Status():
    state ( Disconnected ), id ( -1 ), pairwiseCipher ( NoCipher ), groupCipher ( NoCipher ), secType ( Open )
{
}

void WifiMgrTypes::Status::clear()
{
    state = Disconnected;
    id = -1;
    pairwiseCipher = NoCipher;
    groupCipher = NoCipher;
    secType = Open;
    ssid.clear();
    bssid.clear();
    ipAddress.clear();
}
