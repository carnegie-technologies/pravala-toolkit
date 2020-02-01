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
#include <linux/if_tun.h>
}

#include <cerrno>
#include <cstdio>

#include "sys/File.hpp"
#include "event/SimpleSocket.hpp"
#include "config/ConfigString.hpp"
#include "config/ConfigNumber.hpp"

#include "../../TunIfaceDev.hpp"

// Android code includes this file, but defines its own TUN_DEV
#ifndef TUN_DEV
#define TUN_DEV    "/dev/net/tun"
#endif

extern "C"
{
// 1st argument - pointer to the memory containing the uncompress module data
// read from the .ko file
// 2nd argument - size of that data
// 3nd argument - options for the module (as a null-terminated string)
extern long init_module ( void *, unsigned long, const char * );
}

using namespace Pravala;

/// @brief The length (in packets) of the transmit queue to set on the tunnel device.
static ConfigLimitedNumber<int> optTxQueueLength
(
        0,
        "os.tun.tx_queue_length",
        "The length (in packets) of the transmit queue to set on the tunnel device",
        1, 0x7FFFFFFF
);

/// @brief The path to the tun module.
static ConfigString optTunModPath (
        0,
        "os.tun_mod.path",
        "The path to the tun module (for example: /lib/modules/tun.ko)"
);

/// @brief Loads the tunnel module.
/// @param [in] modPath Path to the tunnel module.
/// @return True if the module was loaded; False on error.
static bool tryToLoadTunMod ( const String & modPath )
{
    if ( modPath.length() < 1 || access ( modPath.c_str(), F_OK ) != 0 )
    {
        return false;
    }

    MemHandle file;

    if ( modPath.endsWith ( ".gz" ) )
    {
        if ( NOT_OK ( File::readGzip ( modPath.c_str(), file ) ) )
            return false;
    }
    else
    {
        if ( NOT_OK ( File::read ( modPath.c_str(), file ) ) )
            return false;
    }

    if ( file.size() < 1 )
        return false;

    char * const w = file.getWritable();

    if ( !w )
        return false;

    // We don't care, at the moment, about the exact reason.
    // getWritable() instead of get() because init_module expects "void *"
    return ( init_module ( w, file.size(), "" ) == 0 );
}

#ifndef ENABLE_VHOSTNET
TunIfaceDev * TunIfaceDev::generate ( TunIfaceOwner * owner )
{
    return new TunIfaceDev ( owner );
}
#endif

ERRCODE TunIfaceDev::osCreateTunDevice ( int & ifaceFd, String & ifaceName )
{
    int tunFd = open ( TUN_DEV, O_RDWR );

    if ( tunFd < 0 )
    {
        // Opening the TUN_DEV failed, but this is our first attempt.
        // Maybe the module is not loaded?

        // Let's see if there is a config option with the path!
        bool modLoaded = tryToLoadTunMod ( optTunModPath.value() );

        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/lib/modules/tun.ko" ) );
        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/lib/modules/tun.ko.gz" ) );
        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/system/lib/modules/tun.ko" ) );
        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/system/lib/modules/tun.ko.gz" ) );
        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/data/pravala/tun.ko" ) );
        !modLoaded && ( modLoaded = tryToLoadTunMod ( "/data/pravala/tun.ko.gz" ) );

        // It didn't work, let's try reading it from the standard linux location
        if ( !modLoaded )
        {
            MemHandle buf;

            // We don't need to store the return code of appendFile, we want
            // the eCode to contain the error until we load the module properly)
            if ( buf.readFile ( "/proc/version" ) && buf.size() > 0 )
            {
                StringList kernVer = buf.toStringList ( " " );

                if ( kernVer.size() >= 3 )
                {
                    String path ( "/lib/modules/" );
                    path.append ( kernVer[ 2 ] );
                    path.append ( "/kernel/drivers/net/tun.ko" );

                    // Compressed?
                    if ( !tryToLoadTunMod ( path ) )
                    {
                        path.append ( ".gz" );

                        tryToLoadTunMod ( path );
                    }
                }
            }
        }

        tunFd = open ( TUN_DEV, O_RDWR );

        if ( tunFd < 0 )
        {
            return Error::OpenFailed;
        }
    }

    struct ifreq tunIfReq;

    memset ( &tunIfReq, 0, sizeof ( tunIfReq ) );

    // Always TUN device, don't use protocol+flags padding.
    tunIfReq.ifr_flags = IFF_TUN | IFF_NO_PI;

    // Don't force a device name - it will be returned by the ioctl.
    int ret = ioctl ( tunFd, ( unsigned int ) TUNSETIFF, ( void * ) &tunIfReq );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Error setting interface flags with ioctl: " << strerror ( errno ) );

        ::close ( tunFd );
        tunFd = -1;

        return Error::IoctlFailed;
    }

    if ( optTxQueueLength.isSet() && optTxQueueLength.value() > 0 )
    {
        // We need a socket to perform ioctl on the tunnel interface.
        // We cannot use the tunnel FD itself (ioctl doesn't like that).

        SimpleSocket s;
        ERRCODE eCode = s.init ( SimpleSocket::SocketDgram4 );

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_ERROR, eCode, "Error creating a socket for ioctl operations" );

            ::close ( tunFd );
            tunFd = -1;

            return Error::SocketFailed;
        }

        // We use brand-new ifreq, to leave the tunIfReq intact:
        struct ifreq ifr;
        memset ( &ifr, 0, sizeof ( ifr ) );

        ifr.ifr_addr.sa_family = AF_INET;
        ifr.ifr_qlen = optTxQueueLength.value();

        memcpy ( ifr.ifr_name, tunIfReq.ifr_name, sizeof ( ifr.ifr_name ) );

        ret = ioctl ( s.getSock(), ( unsigned int ) SIOCSIFTXQLEN, ( void * ) &ifr );

        if ( ret < 0 )
        {
            LOG ( L_ERROR, "Error setting tunnel device's transmit queue length to "
                  << optTxQueueLength.value() << ": " << strerror ( errno ) );

            ::close ( tunFd );
            tunFd = -1;

            return Error::IoctlFailed;
        }
    }

    ifaceName = tunIfReq.ifr_name;
    ifaceFd = tunFd;

    return Error::Success;
}

bool TunIfaceDev::osGetWriteData ( const IpPacket & ipPacket, MemVector & vec )
{
    return vec.append ( ipPacket.getPacketData() );
}

bool TunIfaceDev::osRead ( MemHandle & data )
{
    char * const w = data.getWritable();

    if ( !w || data.isEmpty() )
    {
        LOG ( L_ERROR, "No memory provided" );
        return false;
    }

    const ssize_t ret = ::read ( _fd, w, data.size() );

    if ( ret == 0 )
    {
        LOG ( L_ERROR, "Tunnel interface has been closed" );

        data.clear();
        return false;
    }

    if ( ret > 0 )
    {
        assert ( ( size_t ) ret <= data.size() );

        data.truncate ( ret );

        return true;
    }

    data.clear();

    if ( errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
         && errno != EWOULDBLOCK
#endif
    )
    {
        LOG ( L_ERROR, "Error reading from the tunnel device: " << strerror ( errno ) << "; Closing the tunnel" );

        return false;
    }

    // read returned an error, but it is EAGAIN, so not really critical...
    return true;
}
