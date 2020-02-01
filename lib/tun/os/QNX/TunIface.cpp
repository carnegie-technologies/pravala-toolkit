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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_tun.h>
}

#include <cerrno>

#include "sys/File.hpp"
#include "../../TunIfaceDev.hpp"

#include "../shared/PrefixTunReadWrite.cpp"

using namespace Pravala;

TunIfaceDev * TunIfaceDev::generate ( TunIfaceOwner * owner )
{
    return new TunIfaceDev ( owner );
}

/// @brief Setup a TUN device on QNX
/// @param [in] log Reference to the TextLog instance (so that the LOG macros work)
/// @param [in] fd FD of the tun device
/// @return Standard error code
static ERRCODE setupTun ( TextLog & log, const int fd )
{
    int i = IFF_POINTOPOINT;

    if ( ioctl ( fd, TUNSIFMODE, &i ) < 0 )
    {
        SLOG ( log, L_ERROR, "Failed to set PtP mode" );
        return Error::IoctlFailed;
    }

    i = 0;

    if ( ioctl ( fd, TUNSLMODE, &i ) < 0 )
    {
        SLOG ( log, L_ERROR, "Failed to disable link-level mode" );
        return Error::IoctlFailed;
    }

    i = 1;

    if ( ioctl ( fd, TUNSIFHEAD, &i ) < 0 )
    {
        SLOG ( log, L_ERROR, "Failed to enable multi-af mode" );
        return Error::IoctlFailed;
    }

    return Error::Success;
}

ERRCODE TunIfaceDev::osCreateTunDevice ( int & ifaceFd, String & ifaceName )
{
    String devName; // Device name, e.g. "tun0"
    int tunFd = -1;

    struct ifreq ifr;

    int ios = ::socket ( AF_INET, SOCK_DGRAM, 0 );

    if ( ios < 0 )
    {
        return Error::SocketFailed;
    }

    // On QNX (and NetBSD) /dev/tun[0-9] are tun devices that may already exist or can be created.
    // If a device already exists, we will try to use it. If it's already in use, the open will
    // fail. Otherwise, if the open succeeds, we'll configure it and use it.
    for ( int i = 0; i < 9; ++i )
    {
        devName = "tun";
        devName.append ( String::number ( i ) );

        String devPath ( "/dev/" ); // This will be the full path to the device, e.g. "/dev/net/tun0"
        devPath.append ( devName );

        // Device already exists, try to open it
        if ( File::exists ( devPath ) )
        {
            tunFd = ::open ( devPath.c_str(), O_RDWR );

            if ( tunFd >= 0 )
            {
                // Open succeeded
                break;
            }

            // Open failed, try the next one
            LOG ( L_DEBUG, "Failed trying to open tunnel device: " << devPath );
            continue;
        }

        // Device doesn't exist, try to create it

        ::memset ( &ifr, 0, sizeof ( ifr ) );
        ::strncpy ( ifr.ifr_name, devName.c_str(), devName.length() );

        // This can fail if we try to create a device that's already been created,
        // which is possible if two processes try to create the same device at the
        // same time.
        if ( ::ioctl ( ios, SIOCIFCREATE, &ifr ) < 0 )
        {
            LOG ( L_DEBUG, "Failed trying to create tunnel device: " << devName );

            // Try to create the next one
            continue;
        }

        LOG ( L_DEBUG, "Created tunnel device: " << devName );

        // Try to open it now
        tunFd = ::open ( devPath.c_str(), O_RDWR );

        if ( tunFd >= 0 )
        {
            // Open succeeded
            break;
        }

        // Open failed, try the next one
        LOG ( L_DEBUG, "Failed trying to open tunnel device: " << devPath );
    }

    ::close ( ios );
    ios = -1;

    if ( tunFd < 0 )
    {
        LOG ( L_ERROR, "Failed to open any tunnel devices" );
        return Error::OpenFailed;
    }

    LOG ( L_DEBUG, "Opened tunnel device: " << devName );

    ERRCODE eCode = setupTun ( _log, tunFd );

    if ( NOT_OK ( eCode ) )
    {
        ::close ( tunFd );
        return eCode;
    }

    ifaceName = devName;
    ifaceFd = tunFd;

    return Error::Success;
}
