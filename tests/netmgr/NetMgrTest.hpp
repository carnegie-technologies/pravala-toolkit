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

#include "config/ConfigSwitch.hpp"
#include "netmgr/NetManager.hpp"

namespace Pravala
{
/// @brief Used for testing NetMgr by printing out current links (interfaces), IP addresses, and routes
/// on the system, then monitoring it all for changes.
class NetMgrTest:
    public NetManager::RouteMonitor,
    public NetManager::AddressMonitor,
    public NetManager::IfaceMonitor
{
    public:
        static ConfigSwitch swOnlyIpv4;

        /// @brief Start NetMgrTest
        void start();

    protected:
        virtual void netIfacesChanged (
            const HashSet< int > & activated,
            const HashSet<int> & deactivated,
            const HashSet< int > & removed );
        virtual void netIfaceAddressesChanged (
            const HashSet< NetManagerTypes::Address > & added,
            const HashSet< NetManagerTypes::Address > & removed );
        virtual void netRoutesChanged (
            const HashSet< NetManagerTypes::Route > & added,
            const HashSet< NetManagerTypes::Route > & removed );
};
}
