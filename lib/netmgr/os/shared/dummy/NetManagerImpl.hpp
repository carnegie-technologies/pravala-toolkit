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

#include "../../../NetManagerBase.hpp"

namespace Pravala
{
/// @brief A dummy implementation of the Network Manager (it doesn't do anything).
class NetManagerImpl: public NetManagerBase
{
    public:
        /// @brief Returns global instance of the NetManagerImpl
        static NetManagerImpl & get();

        virtual ERRCODE addIfaceAddress ( int ifaceId, const IpAddress & address );
        virtual ERRCODE removeIfaceAddress ( int ifaceId, const IpAddress & address );

        virtual ERRCODE setIfaceMtu ( int ifaceId, int mtu );
        virtual ERRCODE setIfaceState ( int ifaceId, bool isUp );

        virtual ERRCODE addRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );
        virtual ERRCODE removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        virtual ERRCODE getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface );
        virtual ERRCODE getUncachedIface ( const String & ifaceName, NetManagerTypes::Interface & iface );

        virtual ERRCODE readIfaceUsage ( const String & ifaceName, uint64_t & rxBytes, uint64_t & txBytes );

    private:
        static NetManagerImpl * _instance; ///< The instance of NetManagerImpl.

        /// @brief Constructor
        NetManagerImpl();

        /// @brief Destructor
        ~NetManagerImpl();
};
}
