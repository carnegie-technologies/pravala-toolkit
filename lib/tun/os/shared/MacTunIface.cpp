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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/kern_control.h> // for sockaddr_ctl
#include <sys/sys_domain.h> // for SYSPROTO_CONTROL

#include <net/if.h> // for struct ifreq
#include <arpa/inet.h> // for inet_pton
}

#include <cerrno>
#include <cstring>

#include "sys/File.hpp"
#include "../../TunIfaceDev.hpp"

#define TUN_DEV            "utun"
#define UTUN_CTL_NAME      "com.apple.net.utun_control"
#define UTUN_OPT_IFNAME    2

#include "PrefixTunReadWrite.cpp"

using namespace Pravala;

TunIfaceDev * TunIfaceDev::generate ( TunIfaceOwner * owner )
{
    return new TunIfaceDev ( owner );
}

ERRCODE TunIfaceDev::osCreateTunDevice ( int & ifaceFd, String & ifaceName )
{
    int tunFd = socket ( PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL );

    if ( tunFd < 0 )
    {
        return Error::SocketFailed;
    }

    struct ctl_info ctlInfo;

    memset ( &ctlInfo, 0, sizeof ( ctlInfo ) );

    strncpy ( ctlInfo.ctl_name, UTUN_CTL_NAME, sizeof ( ctlInfo.ctl_name ) );

    int ret = ioctl ( tunFd, CTLIOCGINFO, &ctlInfo );

    if ( ret < 0 )
    {
        ::close ( tunFd );
        tunFd = -1;

        return Error::IoctlFailed;
    }

    struct sockaddr_ctl sc;

    memset ( &sc, 0, sizeof ( sc ) );

    sc.sc_family = PF_SYSTEM;

    sc.ss_sysaddr = AF_SYS_CONTROL;

    sc.sc_id = ctlInfo.ctl_id;

    sc.sc_len = sizeof ( sc );

    sc.sc_unit = 0; // allocate tun ID (i.e. X in utunX) dynamically

    ret = ::connect ( tunFd, ( struct sockaddr * ) &sc, sizeof ( sc ) );

    if ( ret < 0 )
    {
        ::close ( tunFd );
        tunFd = -1;

        return Error::ConnectFailed;
    }

    char ifName[ IFNAMSIZ ];
    uint32_t ifNameLen = sizeof ( ifName );

    ret = getsockopt ( tunFd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifName, &ifNameLen );

    if ( ret < 0 )
    {
        ::close ( tunFd );
        tunFd = -1;

        return Error::GetSockOptFailed;
    }

    ifaceFd = tunFd;
    ifaceName = ifName;

    return Error::Success;
}
