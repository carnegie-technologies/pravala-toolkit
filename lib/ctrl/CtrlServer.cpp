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

#include "event/SocketPair.hpp"
#include "sys/SocketApi.hpp"

#include "CtrlServer.hpp"

using namespace Pravala;

TextLog CtrlServer::_log ( "ctrl_server" );

CtrlServer::ExtConReceiver::ExtConReceiver ( CtrlServer & owner ): _owner ( owner )
{
}

void CtrlServer::ExtConReceiver::incomingTcpConnection (
        TcpServer *, uint8_t, int sockFd,
        const IpAddress & localAddr, uint16_t localPort,
        const IpAddress & remoteAddr, uint16_t remotePort )
{
    LOG ( L_DEBUG, "New TCP control connection from " << remoteAddr << ":" << remotePort
          << " (on " << localAddr << ":" << localPort << ")" );

    CtrlLink * ctrlLink = new CtrlLink ( _owner, sockFd );

    ctrlLink->setup ( sockFd, localAddr, localPort, remoteAddr, remotePort );

    _owner._ctrlClients.insert ( sockFd, ctrlLink );

    _owner.ctrlClientLinkConnected ( ctrlLink );
}

void CtrlServer::ExtConReceiver::incomingUnixConnection (
        TcpServer *, uint8_t, int sockFd,
        const String & sockName )
{
    LOG ( L_DEBUG, "New UNIX control connection on '" << sockName << "'; FD (Ctrl ID): " << sockFd );

    CtrlLink * ctrlLink = new CtrlLink ( _owner, sockFd );

    ctrlLink->setup ( sockFd, sockName );

    _owner._ctrlClients.insert ( sockFd, ctrlLink );

    _owner.ctrlClientLinkConnected ( ctrlLink );
}

CtrlServer::CtrlServer(): _extReceiver ( *this ), _ctrlTcpServer ( _extReceiver )
{
}

CtrlServer::~CtrlServer()
{
    for ( HashMap<int, CtrlLink *>::Iterator cIt ( _ctrlClients ); cIt.isValid(); cIt.next() )
    {
        // CtrlLink's destructor should close the file descriptors and unregister it from EventManager
        delete cIt.value();
    }

    _ctrlClients.clear();
}

ERRCODE CtrlServer::ctrlAddListener ( const IpAddress & localAddr, uint16_t localPort )
{
    const ERRCODE eCode = _ctrlTcpServer.addListener ( localAddr, localPort );

    LOG_ERR ( IS_OK ( eCode ) ? L_INFO : L_ERROR, eCode, "Adding listener: " << localAddr << ":" << localPort );

    return eCode;
}

ERRCODE CtrlServer::ctrlAddListener ( const String & sockName )
{
    const ERRCODE eCode = _ctrlTcpServer.addListener ( sockName );

    LOG_ERR ( IS_OK ( eCode ) ? L_INFO : L_ERROR, eCode, "Adding listener: '" << sockName << "'" );

    return eCode;
}

ERRCODE CtrlServer::ctrlAddClient ( int linkFd )
{
    if ( linkFd < 0 )
        return Error::InvalidParameter;

    CtrlLink * ctrlLink = new CtrlLink ( *this, linkFd );

    _ctrlClients.insert ( linkFd, ctrlLink );

    ctrlLink->setup ( linkFd );

    return Error::Success;
}

CtrlLink * CtrlServer::ctrlGenerateConnectedLink ( CtrlLink::Owner & owner, int linkId )
{
    int foo;

    return ctrlGenerateConnectedLink ( owner, linkId, foo );
}

CtrlLink * CtrlServer::ctrlGenerateConnectedLink ( CtrlLink::Owner & owner, int linkId, int & serverLinkId )
{
    SocketPair socks;

    ERRCODE eCode = socks.init();

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error initializing the socket pair" );
        return 0;
    }

    if ( socks.getSockA() < 0 || socks.getSockB() < 0 )
    {
        LOG ( L_ERROR, "Invalid sockets in SocketPair" );
        return 0;
    }

    const int srvFd = socks.takeSockA();

    eCode = ctrlAddClient ( srvFd );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error adding new control client" );

        SocketApi::close ( srvFd );
        return 0;
    }

    serverLinkId = srvFd;

    CtrlLink * ctrlLink = new CtrlLink ( owner, linkId );

    ctrlLink->setup ( socks.takeSockB() );

    return ctrlLink;
}

void CtrlServer::ctrlClientLinkConnected ( CtrlLink * /*ctrlLink*/ )
{
    // Nothing to do.
}

ERRCODE CtrlServer::ctrlSendPacket ( int clientId, Ctrl::Message & packet, bool atLoopEnd )
{
    if ( !_ctrlClients.contains ( clientId ) )
    {
        LOG ( L_ERROR, "Received a packet for non-existing client with ID " << clientId );

        return Error::NotConnected;
    }

    return _ctrlClients[ clientId ]->sendPacket ( packet, atLoopEnd );
}

ERRCODE CtrlServer::ctrlSendResponse (
        int clientId, Ctrl::Update & resp, const Ctrl::Request & request,
        ERRCODE respErrCode )
{
    if ( !_ctrlClients.contains ( clientId ) )
    {
        LOG ( L_ERROR, "Received a packet for non-existing client with ID " << clientId );

        return Error::ResponseSent;
    }

    return _ctrlClients[ clientId ]->sendResponse ( resp, request, respErrCode );
}

void CtrlServer::ctrlBroadcastPacket ( Ctrl::Message & packet, bool atLoopEnd )
{
    if ( _ctrlClients.isEmpty() )
    {
        LOG ( L_DEBUG, "No clients connected. Ignoring" );
        return;
    }

    MemHandle mem;
    ERRCODE eCode = CtrlLink::serializePacket ( packet, mem );

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Error serializing packet; Ignoring broadcast" );

        return;
    }

    for ( HashMap<int, CtrlLink *>::Iterator it ( _ctrlClients ); it.isValid(); it.next() )
    {
        if ( !it.value() )
            continue;

        it.value()->sendData ( packet, mem, atLoopEnd );
    }
}

void CtrlServer::ctrlCloseLink ( int linkId )
{
    LOG ( L_DEBUG, "Closing link with ID " << linkId );

    CtrlLink * link = 0;

    if ( !_ctrlClients.find ( linkId, link ) )
    {
        LOG ( L_ERROR, "Link with ID " << linkId << " could not be found! Ignoring." );
        return;
    }

    _ctrlClients.remove ( linkId );

    delete link;
    link = 0;
}

void CtrlServer::ctrlLinkClosed ( int linkId )
{
    LOG ( L_DEBUG, "Link with ID " << linkId << " closed, trying to remove CtrlLink object" );

    CtrlLink * link = 0;

    if ( !_ctrlClients.find ( linkId, link ) )
    {
        LOG ( L_ERROR, "Link with ID " << linkId << " closed, but could not be found! Ignoring." );
        return;
    }

    delete link;
    link = 0;

    _ctrlClients.remove ( linkId );
}

ERRCODE CtrlServer::ctrlPacketReceived ( int linkId, Ctrl::Message & msg, Pravala::List< int > & )
{
    ( void ) linkId;
    ( void ) msg;

    LOG ( L_ERROR, "Received an unsupported ctrl message (" << msg.getType()
          << ") from link ID " << linkId << "; Ignoring" );

    return Error::Unsupported;
}
