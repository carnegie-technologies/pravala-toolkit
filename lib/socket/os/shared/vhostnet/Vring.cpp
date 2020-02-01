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
#include <sys/ioctl.h>
#include <linux/vhost.h>
#include <linux/if_tun.h>
}

#include <cstdlib>
#include <cerrno>

#include "basic/Platform.hpp"
#include "Vring.hpp"

using namespace Pravala;

TextLog Vring::_log ( "vhost_net_vring" );

Vring::Vring ( uint16_t maxDescs, uint8_t memTag ):
    MaxDescs ( maxDescs ),
    MemTag ( memTag ),
    _ring ( 0 ),
    _descMH ( 0 ),
    _nextDescIdx ( 0 ),
    _freeDescs ( MaxDescs ),
    _vheaderLen ( 0 )
{
    assert ( MaxDescs >= 8 );

    // checking that MaxDescs is a power of 2
    assert ( ( MaxDescs & ( MaxDescs - 1 ) ) == 0 );
}

Vring::~Vring()
{
    clear();
}

void Vring::clear()
{
    if ( _ring != 0 )
    {
        if ( _ring->desc != 0 )
        {
            free ( _ring->desc );
        }

        free ( _ring );
        _ring = 0;
    }

    delete[] _descMH;
    _descMH = 0;

    _freeDescs = MaxDescs;
    _nextDescIdx = 0;
    _vheaderLen = 0;
}

ERRCODE Vring::internalSetup ( NetVringIdx vringIdx, int vhostFd, int backendFd )
{
    assert ( vringIdx == RxVringIdx || vringIdx == TxVringIdx );
    assert ( vhostFd >= 0 );
    assert ( backendFd >= 0 );
    assert ( !_ring );
    assert ( !_descMH );

    if ( _ring != 0 || _descMH != 0 )
    {
        LOG ( L_ERROR, "Vring already initialized" );
        return Error::AlreadyInitialized;
    }

    // Error if MaxDescs <8 or MaxDescs isn't a power of 2.
    if ( MaxDescs < 8 || ( ( MaxDescs & ( MaxDescs - 1 ) ) != 0 ) )
    {
        LOG ( L_ERROR, "MaxDescs invalid: " << MaxDescs );
        return Error::InternalError;
    }

    if ( vringIdx != RxVringIdx && vringIdx != TxVringIdx )
    {
        LOG ( L_ERROR, "Invalid vringIdx specified: " << vringIdx );
        return Error::InvalidParameter;
    }

    if ( vhostFd < 0 )
    {
        LOG ( L_ERROR, "Invalid vhostFd: " << vhostFd );
        return Error::InvalidParameter;
    }

    if ( backendFd < 0 )
    {
        LOG ( L_ERROR, "Invalid backendFd: " << backendFd );
        return Error::InvalidParameter;
    }

    // Get the size of the tunnel's vnet header.
    // According to virtio specs, this must exist, however its size may change in the future.
    int ret = ioctl ( backendFd, TUNGETVNETHDRSZ, &_vheaderLen );

    if ( ret < 0 || _vheaderLen < 1 )
    {
        LOG ( L_WARN, "Failed to get VNET header size from tunnel with FD: " << backendFd
              << "; Not using vhost-net. virtio header length: " << _vheaderLen << "; Error: " << strerror ( errno ) );

        return Error::IoctlFailed;
    }

    _ring = ( struct vring * ) malloc ( sizeof ( struct vring ) );

    const size_t ringDataSize = vring_size ( MaxDescs, Platform::PageSize );

    void * ringMem = 0;

    ret = posix_memalign ( &ringMem, Platform::PageSize, ringDataSize );

    _descMH = new MemHandle[ MaxDescs ];

    if ( !_ring || !ringMem || !_descMH )
    {
        LOG ( L_ERROR, "Failed to allocate memory for vring structures" );

        if ( ringMem != 0 )
        {
            free ( ringMem );
            ringMem = 0;
        }

        return Error::MemoryError;
    }

    memset ( _ring, 0, sizeof ( struct vring ) );
    memset ( ringMem, 0, ringDataSize );

    // This just sets up the structures - i.e. it can't fail
    vring_init ( _ring, MaxDescs, ringMem, Platform::PageSize );

    // Ring should be accessed through _ring
    ringMem = 0;

    struct vhost_vring_state vrs;

    vrs.index = vringIdx;
    vrs.num = MaxDescs;
    ret = ioctl ( vhostFd, VHOST_SET_VRING_NUM, &vrs );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set ring size on vhost-net FD: " << vhostFd << "; Vring idx: " << vringIdx
              << "; Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    vrs.num = 0; // start using them from idx 0
    ret = ioctl ( vhostFd, VHOST_SET_VRING_BASE, &vrs );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set ring base on vhost-net FD: " << vhostFd << "; Vring idx: " << vringIdx
              << "; Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    struct vhost_vring_addr addr;
    memset ( &addr, 0, sizeof ( addr ) );

    addr.index = vringIdx;
    addr.desc_user_addr = ( uint64_t ) _ring->desc;
    addr.avail_user_addr = ( uint64_t ) _ring->avail;
    addr.used_user_addr = ( uint64_t ) _ring->used;

    ret = ioctl ( vhostFd, VHOST_SET_VRING_ADDR, &addr );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set ring address on vhost-net FD: " << vhostFd << "; Vring idx: " << vringIdx
              << "; Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    struct vhost_vring_file backendf;
    backendf.index = vringIdx;
    backendf.fd = backendFd;

    ret = ioctl ( vhostFd, VHOST_NET_SET_BACKEND, &backendf );

    if ( ret < 0 )
    {
        LOG ( L_ERROR, "Failed to set ring backend on vhost-net FD: " << vhostFd << "; Vring idx: " << vringIdx
              << "; backend FD: " << backendFd << "; Error: " << strerror ( errno ) );
        return Error::IoctlFailed;
    }

    return Error::Success;
}
