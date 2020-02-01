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

#include "event/Timer.hpp"
#include "config/ConfigAddrSpec.hpp"
#include "config/ConfigNumber.hpp"
#include "websocket/WebSocketListener.hpp"
#include "WSHandler.hpp"

namespace Pravala
{
class WSTest: public Timer::Receiver, public WebSocketListenerOwner
{
    public:
        /// @brief Delay between sending junk
        static ConfigNumber<uint16_t> optSendDelay;

        /// @brief Address and port to listen on for WebSockets connections
        static ConfigAddrSpec optListenAddrSpec;

        WSTest();

        virtual ~WSTest();

        bool start();

    protected:
        static TextLog _log;

        SimpleTimer _timer;

        WebSocketListener _wsl;
        WSHandler _handler;

        uint32_t _counter;

        virtual void timerExpired ( Timer * timer );

        virtual ERRCODE getHandler (
            WebSocketListener * listener, const String & url, const StringList & protocols,
            WebSocketHandler * & handler, String & protocol );
};
}
