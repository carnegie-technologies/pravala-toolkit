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

#include "websocket/WebSocketListener.hpp"
#include "WSTest.hpp"

#define LIBWEBSOCKETS_PROTO1    "dumb-increment-protocol"
#define LIBWEBSOCKETS_PROTO2    "lws-mirror-protocol"

using namespace Pravala;

ConfigAddrSpec WSTest::optListenAddrSpec (
        ConfigOpt::FlagInitializeOnly | ConfigOpt::FlagIsCommon | ConfigOpt::FlagReqNonEmpty,
        "listen", 'l', "wstest.listen",
        "Address and port to listen on for WebSockets connections"
);

ConfigNumber<uint16_t> WSTest::optSendDelay ( "wait", 'w', "Time to wait between sending packets (ms)", 1000 );

TextLog WSTest::_log ( "web_socket_test" );

WSTest::WSTest():
    _timer ( *this ),
    _wsl ( *this ),
    _counter ( 0 )
{
}

WSTest::~WSTest()
{
}

bool WSTest::start()
{
    ERRCODE eCode = _wsl.addListener ( optListenAddrSpec.address(), optListenAddrSpec.port() );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error" );
        return false;
    }

    if ( optSendDelay.value() > 0 )
    {
        _timer.start ( optSendDelay.value() );
    }

    LOG ( L_INFO, "Started" );

    return true;
}

ERRCODE WSTest::getHandler (
        WebSocketListener * listener, const String & url, const StringList & protocols,
        WebSocketHandler * & handler, String & protocol )
{
    ( void ) listener;
    assert ( listener == &_wsl );

    if ( url != "/" )
    {
        return Error::NotFound;
    }

    if ( protocols.isEmpty() )
    {
        assert ( protocol.isEmpty() );

        handler = &_handler;
        return Error::Success;
    }

    for ( size_t i = 0; i < protocols.size(); ++i )
    {
        const String & proto = protocols.at ( i );

        // Also claim to support these protocols used by libwebsockets' test client. We do this
        // just to test the WebSocket protocol negotiation.
        //
        // We want to be case sensitive for these protocols, so just do exact comparison.
        if ( proto == LIBWEBSOCKETS_PROTO1
             || proto == LIBWEBSOCKETS_PROTO2 )
        {
            handler = &_handler;
            protocol = proto;
            return Error::Success;
        }
    }

    // URL valid, but handler won't support the requested protocol
    return Error::Unsupported;
}

void WSTest::timerExpired ( Timer * )
{
    _handler.broadcast ( String::number ( ++_counter ) );

    if ( optSendDelay.value() > 0 )
    {
        _timer.start ( optSendDelay.value() );
    }
}
