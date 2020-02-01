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

#include "NetManagerImpl.hpp"

using namespace Pravala;

THREAD_LOCAL NetManagerImpl * NetManagerImpl::_instance ( 0 );

NetManager & NetManager::get()
{
    return NetManagerImpl::get();
}

NetManagerImpl & NetManagerImpl::get()
{
    if ( !_instance )
    {
        _instance = new NetManagerImpl();

        assert ( _instance != 0 );
    }

    return *_instance;
}

NetManagerImpl::NetManagerImpl()
{
}

NetManagerImpl::~NetManagerImpl()
{
    if ( _instance == this )
    {
        _instance = 0;
    }
}

ERRCODE NetManagerImpl::addIfaceAddress ( int, const IpAddress & )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::removeIfaceAddress ( int, const IpAddress & )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::setIfaceMtu ( int, int )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::setIfaceState ( int, bool )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::addRoute ( const IpAddress &, uint8_t, const IpAddress &, int, int, int )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::removeRoute ( const IpAddress &, uint8_t, const IpAddress &, int, int, int )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::getUncachedIface ( int, NetManagerTypes::Interface & )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::getUncachedIface ( const String &, NetManagerTypes::Interface & )
{
    return Error::Unsupported;
}

ERRCODE NetManagerImpl::readIfaceUsage ( const String &, uint64_t &, uint64_t & )
{
    return Error::Unsupported;
}
