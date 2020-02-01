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

extern "C"
{
#include <net/if.h>
}

#include "../shared/SocketApi.cpp"

using namespace Pravala;

ERRCODE SocketApi::bindToIface ( int sockFd, SocketType, const String & ifaceName )
{
    if ( ifaceName.isEmpty() )
    {
        return Error::TooShortName;
    }

    if ( ifaceName.length() + 1 > IFNAMSIZ )
    {
        // +1, because NULL needs to fit as well.

        return Error::TooLongName;
    }

    if ( !SocketApi::setOption ( sockFd, SOL_SOCKET, SO_BINDTODEVICE, ifaceName.c_str(), ifaceName.length() + 1 ) )
    {
        return Error::BindFailed;
    }

    return Error::Success;
}
