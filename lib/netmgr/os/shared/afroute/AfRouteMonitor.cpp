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

#include "AfRouteMonitor.hpp"
#include "RouteParser.hpp"

#include <cerrno>

extern "C"
{
#include <net/route.h>
}

#define READ_SIZE    4096

using namespace Pravala;

TextLog AfRouteMonitor::_log ( "afroute_monitor" );

AfRouteMonitor::Owner::~Owner()
{
}

AfRouteMonitor::AfRouteMonitor ( Owner & owner ): _owner ( owner ), _sock ( -1 )
{
    reset();
}

AfRouteMonitor::~AfRouteMonitor()
{
    if ( _sock >= 0 )
    {
        // In case there were some event handlers registered...
        EventManager::closeFd ( _sock );
        _sock = -1;
    }
}

void Pravala::AfRouteMonitor::reset()
{
    if ( _sock >= 0 )
    {
        EventManager::closeFd ( _sock );
        _sock = -1;
    }

    _sock = ::socket ( AF_ROUTE, SOCK_RAW, 0 );

    if ( _sock < 0 )
    {
        LOG ( L_ERROR, "socket() failed: " << strerror ( errno ) );
        return;
    }

    EventManager::setFdHandler ( _sock, this, EventManager::EventRead );
}

void AfRouteMonitor::receiveFdEvent ( int fd, short events )
{
    ( void ) fd;
    assert ( fd == _sock );

    if ( ( events & EventManager::EventRead ) == 0 )
    {
        return;
    }

    char * const data = _sockData.getAppendable ( READ_SIZE );

    ssize_t readSize = -1;

    if ( !data )
    {
        errno = ENOMEM;
    }
    else
    {
        readSize = ::read ( _sock, data, READ_SIZE );
    }

    if ( readSize <= 0 )
    {
        LOG ( L_ERROR, "read() failed: " << strerror ( errno ) );
        reset();
        return;
    }

    _sockData.markAppended ( readSize );

    List<AfRouteTypes::Link> links;
    List<AfRouteTypes::Address> addrs;
    List<AfRouteTypes::Route> routes;

    ERRCODE ret = RouteParser::processBuffer ( _sockData, links, addrs, routes );

    if ( NOT_OK ( ret ) && ret != Error::IncompleteData )
    {
        LOG_ERR ( L_ERROR, ret, "Unexpected error when processing route buffer" );
        reset();
        return;
    }

    _owner.afRouteMonUpdate ( this, links, addrs, routes );
}
