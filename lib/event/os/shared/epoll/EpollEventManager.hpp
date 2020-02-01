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

#include "../PosixEventManager.hpp"

namespace Pravala
{
/// @brief epoll-based Event Manager.
class EpollEventManager: public PosixEventManager
{
    protected:
        /// @brief File descriptor for epoll operations.
        int _epollFd;

        /// @brief Constructor.
        /// @param [in] epollFd File descriptor for epoll operations.
        EpollEventManager ( int epollFd );

        /// @brief Destructor
        ~EpollEventManager();

        virtual void implRun();
        virtual void implSetFdHandler ( int fd, FdEventHandler * handler, int events );
        virtual void implSetFdEvents ( int fd, int events );
        virtual void implRemoveFdHandler ( int fd );

        friend class EventManager;
};
}
