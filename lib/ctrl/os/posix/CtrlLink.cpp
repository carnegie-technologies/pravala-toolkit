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

#include <cerrno>

extern "C"
{
#include <sys/types.h>
#include <unistd.h>
}

#include "../../CtrlLink.hpp"

#define MAX_RCV_DATA       2048
#define MAX_RCV_CONTROL    1024

using namespace Pravala;

ssize_t CtrlLink::osInternalRead ( int & rcvdFds, bool & nonFatal )
{
    rcvdFds = 0;
    nonFatal = false;

    struct iovec iov;
    memset ( &iov, 0, sizeof ( iov ) );

    iov.iov_len = MAX_RCV_DATA;
    iov.iov_base = _readBuffer.getAppendable ( MAX_RCV_DATA );

    if ( !iov.iov_base )
    {
        errno = ENOMEM;
        nonFatal = false;
        return -1;
    }

    char control[ MAX_RCV_CONTROL ];
    memset ( control, 0, sizeof ( control ) );

    struct msghdr msg;
    memset ( &msg, 0, sizeof ( msg ) );

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof ( control );

    const ssize_t readRet = recvmsg ( _linkFd, &msg, 0 );

    if ( readRet >= 0 )
    {
        struct cmsghdr * cmsg = CMSG_FIRSTHDR ( &msg );

        while ( cmsg != NULL )
        {
            // File descriptors:
            if ( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS )
            {
                int * fds = ( int * ) CMSG_DATA ( cmsg );
                rcvdFds = ( cmsg->cmsg_len - CMSG_LEN ( 0 ) ) / sizeof ( int );

                for ( int i = 0; i < rcvdFds; ++i )
                {
                    _readFds.append ( fds[ i ] );
                }
            }

#if 0
            // We don't use credentials, at least for now.

            // Credentials:
            else if ( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS )
            {
                struct ucred cred;

                memcpy ( &cred, CMSG_DATA ( cmsg ), sizeof ( cred ) );

                fprintf ( log, "recv SCM_CREDENTIALS; PID: % d; UID: % u; GID: % u\n",
                          cred.pid, cred.uid, cred.gid );
            }
#endif

            cmsg = CMSG_NXTHDR ( &msg, cmsg );
        }
    }
    else if ( errno == EAGAIN || errno == EWOULDBLOCK )
    {
        // Not really an error, we just can't read at this time...
        nonFatal = true;
    }

    return readRet;
}
