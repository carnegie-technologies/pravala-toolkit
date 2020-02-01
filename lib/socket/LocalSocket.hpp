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

#include "Socket.hpp"

namespace Pravala
{
/// @brief An abstract UNIX socket that extends base socket with calls that use local names.
class LocalSocket: public Socket
{
    public:
        /// @brief Binds socket to a name.
        /// @param [in] name The name to bind the socket to. If the first character is '@' then
        ///                  the abstract namespace is used (and the first character is removed).
        ///                  To use abstract namespace with a name starting with '@', use it twice.
        /// @return Standard error code.
        virtual ERRCODE bind ( const String & name ) = 0;

        /// @brief Starts connecting socket to a name.
        /// This call always only starts connection procedure.
        /// When it returns an error, it means that there is something wrong with the socket,
        /// or with the name used. In that case there will be no 'connected' or 'connect failed' callback.
        /// If this method succeeds, it means that there will be a 'connected' or 'connect failed' callback.
        /// Specifically, this method does NOT return ConnectInProgress code (and 'Success' code has this
        /// exact meaning). If the socket gets connected right away, the callback will
        /// be generated during the end of the event loop.
        /// @param [in] addr The address to connect the socket to.
        /// @return Standard error code.
        virtual ERRCODE connect ( const String & name ) = 0;

        /// @brief Exposes the name this socket is bound to.
        /// @return the name this socket is bound to.
        virtual const String & getLocalSockName() const = 0;

        /// @brief Exposes the name this socket is connected (or connecting) to.
        /// @return the name this socket is connected to.
        virtual const String & getRemoteSockName() const = 0;

        virtual LocalSocket * getLocalSocket()
        {
            return this;
        }

        virtual String getLocalDesc() const
        {
            return getLocalSockName();
        }

        virtual String getRemoteDesc() const
        {
            return getRemoteSockName();
        }

        virtual String getLogId ( bool extended = false ) const
        {
            ( void ) extended;

            return getLocalSockName();
        }

    protected:
        /// @brief Constructor.
        /// @param [in] owner The initial owner to set.
        inline LocalSocket ( SocketOwner * owner ): Socket ( owner )
        {
        }
};
}
