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

#include "basic/String.hpp"
#include "basic/List.hpp"
#include "basic/IpAddress.hpp"

namespace Pravala
{
/// @brief A class of helpful structures used by the Wi-Fi manager.
class WifiMgrTypes
{
    public:
        /// @brief The different supported security types of a network.
        enum SecType
        {
            Open,   ///< Open Wi-Fi network
            WEP,    ///< WEP "secured" network
            WPA_PSK,    ///< WPA PSK secured network
            WPA2_PSK,   ///< WPA2 PSK secured network
            WPA_EAP,    ///< WPA EAP secured network
            WPA2_EAP,   ///< WPA2 EAP secured network
            UnknownSecType  ///< Unknown security type
        };

        /// @brief The supported authentication methods for a network.
        enum AuthType
        {
            None,   ///< No authentication
            WISPr1, ///< WISPr v1 authentication
            WISPr2, ///< WISPr v2 authentication
            PEAP,   ///< PEAP authentication (only for EAP secured networks)
            PEAP_MSCHAP,    ///< PEAP+MSCHAP authentication (only for EAP secured networks)
            PEAP_PAP,   ///< PEAP+PAP authentication (only for EAP secured networks)
            PEAP_GTC,   ///< PEAP+GTC authentication (only for EAP secured networks)
            TLS,    ///< TLS authentication (only for EAP secured networks)
            TTLS,   ///< TTLS authentication (only for EAP secured networks)
            SIM,    ///< SIM authentication (only for EAP secured networks)
            AKA,    ///< AKA authentication (only for EAP secured networks)
            FAST,   ///< FAST authentication (only for EAP secured networks)
            UnknownAuthType ///< Unknown authentication type
        };

        /// @brief The types of ciphers
        enum Cipher
        {
            NoCipher,   ///< No encryption cipher (WEP or Open networks)
            TKIP,   ///< TKIP encrypted network
            CCMP,   ///< CCMP (AES) encrypted network
            TKIP_CCMP   ///< TKIP or CCMP (AES) encrypted network
        };

        /// @brief States of the Wi-Fi network
        enum State
        {
            Off,    ///< Interface/radio is off and not available
            Disconnected,   ///< Interface is enabled and available, but not connected
            Associating,    ///< Interface is associating to a network
            Associated, ///< Interface is associated but not yet connected (i.e. authentication is not yet complete)
            Connected   ///< Interface is connected to a network
        };

        /// @brief Convert an auth type to its JSON string
        /// @param [in] authType The authentication type
        /// @return The string-ified authentication type
        static const char * authTypeToStr ( AuthType authType );

        /// @brief Convert a JSON string to its auth type
        /// @param [in] authTypeStr The auth type as a JSON type string
        /// @return The WifiAuthType value
        static AuthType strToAuthType ( const String & authTypeStr );

        /// @brief Convert an security type to its JSON string
        /// @param [in] secType The security type
        /// @return The string-ified security type
        static const char * secTypeToStr ( SecType secType );

        /// @brief Convert a JSON string to its auth type
        /// @param [in] secTypeStr The security security as a JSON type string
        /// @return The WifiSecType value
        static SecType strToSecType ( const String & secTypeStr );

        /// @brief Convert a cipher type to its string representation
        /// @param [in] cipher The cipher
        /// @return The string-ified cipher
        static const char * cipherToStr ( Cipher cipher );

        /// @brief Convert a state to a string
        /// @param [in] state The state
        /// @return The string-ified state
        static const char * stateToStr ( State state );

        /// @brief Common fields for all network types.
        struct NetworkCommon
        {
            String ssid; ///< The SSID of this network

            bool isHidden; ///< Whether the network is hidden or not

            SecType secType; ///< The security type of the network

            AuthType l2AuthType; ///< The Layer 2 auth type (i.e. AKA, SIM, etc.)

            /// @brief Constructor
            NetworkCommon();

            /// @brief Compare two instances
            /// @param [in] other To compare against
            /// @return True on success; false otherwise
            bool operator== ( const NetworkCommon & other ) const;
        };

        /// @brief A physical instantiation of this network.
        struct NetworkInstance: public NetworkCommon
        {
            String bssid; ///< The BSSID of the AP

            int signalLevel; ///< The signal level

            int frequency; ///< The frequency

            /// @brief Constructor
            NetworkInstance();
        };

        /// @brief A provided profile for a network (i.e. to be set in the OS).
        struct NetworkProfile: public NetworkCommon
        {
            AuthType l3AuthType; ///< The Layer 3 auth type (i.e. WISPr1, etc.)

            String credential; ///< The authentication credential (may be empty).

            String identifier; ///< The domain associated with this network (may be empty).

            StringList partners; ///< The list of partners associated with this network (may be empty).

            /// @brief Constructor
            NetworkProfile();
        };

        /// @brief A configuration of a network on the operating system.
        struct NetworkConfiguration: public NetworkCommon
        {
            int id; ///< The system identifier of this network

            bool isConnected; ///< Whether this network is currently connected

            bool isDisabled; ///< Whether this network is available for use (i.e. if true, won't use).

            /// @brief Constructor
            NetworkConfiguration();
        };

        /// @brief Connection-specific information about the currently connected network.
        struct Status
        {
            State state; ///< The state. If Disconnected; all other fields are invalid.

            int id; ///< The OS identifier of the network

            String ssid; ///< The SSID of the network we are using

            String bssid; ///< The BSSID of the AP we are using

            Cipher pairwiseCipher; ///< The pairwise cipher

            Cipher groupCipher; ///< The group cipher

            SecType secType; ///< The WPA* type

            IpAddress ipAddress; ///< The associated IP address

            /// @brief Constructor
            Status();

            /// @brief Reset the structure
            void clear();
        };
};
}
