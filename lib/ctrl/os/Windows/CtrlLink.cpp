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
#include <sys/types.h>
}

#include "sys/OsUtils.hpp"
#include "../../CtrlLink.hpp"

#define MAX_RCV_DATA    2048

using namespace Pravala;

ssize_t CtrlLink::osInternalRead ( int & rcvdFds, bool & nonFatal )
{
    rcvdFds = 0;

    char * const w = _readBuffer.getAppendable ( MAX_RCV_DATA );

    if ( !w )
    {
        nonFatal = false;
        return -1;
    }

    const ssize_t readRet = recv ( _linkFd, w, MAX_RCV_DATA, 0 );

    nonFatal = ( readRet < 0 && WSAGetLastError() == WSAEWOULDBLOCK );

    return readRet;
}
