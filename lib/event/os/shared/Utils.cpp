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

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

extern "C"
{
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
}

#include "basic/String.hpp"
#include "../../Utils.hpp"
#include "../../EventManager.hpp"
#include "../../SocketPair.hpp"

using namespace Pravala;

static ConfigNumber<uint32_t> optVMemMax (
        0,
        "os.vmem_max",
        "Maximum virtual memory size (in kilo-bytes)"
);

static ConfigNumber<uint32_t> optNumFdMax (
        0,
        "os.numfd_max",
        "Maximum number of open file descriptors"
);

ERRCODE Utils::daemonize ( bool autoParentExit )
{
    if ( EventManager::getNumManagers() > 0 )
    {
        fprintf ( stderr, "Could not daemonize, at least one EventManager has already been created!\n" );
        return Error::WrongState;
    }

    // If we are already a daemon we don't need to do anything.
    if ( getppid() == 1 )
        return Error::NothingToDo;

    pid_t pid = fork();

    if ( pid < 0 )
    {
        fprintf ( stderr, "Failed to run as a daemon: %s\n",
                  strerror ( errno ) );
        return Error::ForkFailed;
    }

    // If pid>0 it means we are in the parent and we should return (or exit)
    if ( pid > 0 )
    {
        if ( autoParentExit )
        {
            fprintf ( stdout, "Program will continue running as a daemon in the background\n" );
            exit ( EXIT_SUCCESS );
        }

        return Error::ForkParent;
    }

    // Here we are running in the child process, the one that is the new "daemon" process

    // In case we wanted to change the file creation mode mask:
    // umask ( 0 );

    // Let's create a new session for the process
    if ( setsid() < 0 )
    {
        fprintf ( stderr, "Failed to create a new SID for the daemon: %s\n",
                  strerror ( errno ) );
        return Error::SetSidFailed;
    }

    // Change the directory to /
    // This will unlock the directory we started in, in case it
    // needs to be umounted.
    if ( ( chdir ( "/" ) ) < 0 )
    {
        fprintf ( stderr, "Failed to change directory to /: %s\n",
                  strerror ( errno ) );
        return Error::ChdirFailed;
    }

    // Redirecting standard in/out/err files to/from /dev/null.
    // We can't simply close them, because there might be things
    // trying to use them and expect them to be there.
    if ( !freopen ( "/dev/null", "r", stdin )
         || !freopen ( "/dev/null", "w", stdout )
         || !freopen ( "/dev/null", "w", stderr ) )
    {
        return Error::OpenFailed;
    }

    return Error::ForkChild;
}

ERRCODE Utils::forkChild ( pid_t * childPid, int * commFd )
{
    if ( EventManager::getNumManagers() > 0 )
    {
        fprintf ( stderr, "Could not fork, at least one EventManager has already been created!\n" );
        return Error::WrongState;
    }

    // SockA will be used by the parent, SockB - by the child.

    SocketPair sPair;

    if ( commFd != 0 )
    {
        ERRCODE eCode = sPair.init();

        if ( NOT_OK ( eCode ) )
        {
            fprintf ( stderr, "Failed to initialize a SocketPair: %s\n", eCode.toString() );
            return eCode;
        }
    }

    pid_t pid = fork();

    if ( pid < 0 )
    {
        fprintf ( stderr, "Failed to fork child: %s\n", strerror ( errno ) );
        return Error::ForkFailed;
    }

    if ( childPid != 0 )
    {
        *childPid = pid;
    }

    // If pid>0 it means we are in the parent
    if ( pid > 0 )
    {
        if ( commFd != 0 )
        {
            // Parent uses SockA. We are calling takeSockA (instead of get),
            // which means that this FD will NOT be closed by SocketPair's destructor.
            *commFd = sPair.takeSockA();
        }

        return Error::ForkParent;
    }

    // Here we are in the child::

    if ( commFd != 0 )
    {
        // The child uses SockB. We are calling takeSockB (instead of get),
        // which means that this FD will NOT be closed by SocketPair's destructor.
        *commFd = sPair.takeSockB();
    }

    return Error::ForkChild;
}

bool Utils::setupDebugCore()
{
#ifndef NDEBUG
    struct rlimit rlp;

    if ( getrlimit ( RLIMIT_CORE, &rlp ) != 0 )
    {
        fprintf ( stderr, "Error setting allowed core file size; getrlimit(): %s\n", strerror ( errno ) );
        return false;
    }

    rlp.rlim_cur = rlp.rlim_max;

    if ( setrlimit ( RLIMIT_CORE, &rlp ) != 0 )
    {
        fprintf ( stderr, "Error setting allowed core file size; setrlimit(): %s\n", strerror ( errno ) );
        return false;
    }
#endif

    return true;
}

void Utils::setup()
{
    // Set the maximum virtual memory size, if option set in the config file

    if ( optVMemMax.isSet() )
    {
        struct rlimit rlim;

        rlim.rlim_max = optVMemMax.value() * 1024; // maxVMemSize is in kilo-bytes
        rlim.rlim_cur = rlim.rlim_max;

        if ( setrlimit ( RLIMIT_AS, &rlim ) != 0 )
        {
            fprintf ( stderr, "Could not set the maximum virtual memory size; setrlimit(): %s\n",
                      strerror ( errno ) );
            // not fatal
        }
    }

    // Set the maximum number of open file descriptors, if option set in the config file

    if ( optNumFdMax.isSet() )
    {
        struct rlimit rlim;

        rlim.rlim_max = optNumFdMax.value();
        rlim.rlim_cur = optNumFdMax.value();

        if ( setrlimit ( RLIMIT_NOFILE, &rlim ) != 0 )
        {
            fprintf ( stderr, "Could not set the maximum files; setrlimit(): %s\n",
                      strerror ( errno ) );
            // not fatal
        }
    }
}
