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

#include "event/SimpleSocket.hpp"
#include "event/EventManager.hpp"
#include "net/TunIface.hpp"

namespace Pravala
{
/// @brief Abstract external tunnel interface that sends/receives IP packets using an external API.
class TunIfaceExt: public TunIface
{
    public:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        TunIfaceExt ( TunIfaceOwner * owner );

        /// @brief Initializes anything needed before starting the tunnel.
        /// This should be called after EventManager is initialized.
        /// @return Standard error code
        virtual ERRCODE init() = 0;

        /// @brief Shuts down the tunnel.
        /// This should be called after EventManager::run() returns.
        virtual void shutdown() = 0;

        virtual ERRCODE startUnmanaged ( int fd, const HashSet<IpAddress> & ipAddresses, int ifaceMtu = -1 );
        virtual ERRCODE startManaged ( int ifaceMtu = -1 );
        virtual bool isManaged();

        virtual ERRCODE sendPacket ( const IpPacket & packet );

        virtual bool isInitialized() const;

        virtual int getIfaceId() const;
        virtual const String & getIfaceName() const;

    protected:
        /// @brief Destructor.
        virtual ~TunIfaceExt();

        /// @brief Called by sendPacket to send an IP packet to the external tunnel interface.
        /// @param [in] packet The packet to send.
        /// @return Standard error code.
        virtual ERRCODE doSendPacket ( const IpPacket & packet ) = 0;
};
}
