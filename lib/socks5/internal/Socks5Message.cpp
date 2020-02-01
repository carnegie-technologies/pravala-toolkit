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

#include "Socks5Message.hpp"

using namespace Pravala;

TextLog Socks5Message::_log ( "socks5_message" );

ERRCODE Socks5Message::setAndConsume ( MemHandle & data, size_t msgSize, size_t & bytesNeeded )
{
    if ( data.size() < msgSize )
    {
        LOG ( L_DEBUG4, getLogId()
              << ": Not enough message data bytes; Size: " << data.size()
              << "; Needed: " << msgSize );

        bytesNeeded = ( msgSize - data.size() );
        return Error::IncompleteData;
    }

    bytesNeeded = 0;
    _data = data.getHandle ( 0, msgSize );
    data.consume ( msgSize );

    return Error::Success;
}
