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

#include "socket/UdpFdListener.hpp"

#include "DtlsListener.hpp"
#include "DtlsSocket.hpp"

// BoringSSL does not support DTLS server mode
#ifndef OPENSSL_IS_BORINGSSL

using namespace Pravala;

TextLog DtlsListener::_log ( "dtls_listener" );

DtlsListener::DtlsListener ( Owner & owner, DtlsServer & dtlsContext ):
    _owner ( owner ),
    _dtlsContext ( dtlsContext ),
    _listeningUdpSock ( 0 ),
    _listeningDtlsSock ( 0 )
{
    assert ( _dtlsContext.getContext() != 0 );
}

ERRCODE DtlsListener::init ( const SockAddr & localAddr )
{
    if ( _listeningUdpSock != 0 || _listeningDtlsSock != 0 )
    {
        return Error::AlreadyInitialized;
    }

    if ( localAddr.getPort() < 1 || !localAddr.hasIpAddr() )
    {
        return Error::InvalidParameter;
    }

    ERRCODE eCode;

    UdpListener * listener = UdpFdListener::generate ( localAddr, &eCode );

    if ( !listener )
    {
        LOG ( L_ERROR, localAddr << ": Error creating a UDP listener" );

        return eCode;
    }

    assert ( !_listeningUdpSock );

    _listeningUdpSock = listener->generateListeningSock ( 0, &eCode );

    // _listeningUdpSock now holds the reference to the listener, and we don't need it anymore, so we can unref it.
    listener->unref();
    listener = 0;

    if ( !_listeningUdpSock )
    {
        LOG ( L_ERROR, localAddr << ": Error creating a listening UDP socket" );

        return eCode;
    }

    LOG ( L_DEBUG, localAddr << ": Created a listening UDP socket" );

    createDtlsListeningSock();

    return Error::Success;
}

DtlsListener::~DtlsListener()
{
    if ( _listeningDtlsSock != 0 )
    {
        _listeningDtlsSock->unrefOwner ( this );
        _listeningDtlsSock = 0;
    }

    if ( _listeningUdpSock != 0 )
    {
        _listeningUdpSock->unrefOwner ( this );
        _listeningUdpSock = 0;
    }
}

void DtlsListener::createDtlsListeningSock()
{
    if ( !_listeningUdpSock || _listeningDtlsSock != 0 )
    {
        assert ( false );

        return;
    }

    _listeningDtlsSock = new DtlsSocket ( this, _dtlsContext, _listeningUdpSock, SSL_OP_COOKIE_EXCHANGE );
}

void DtlsListener::socketConnected ( Socket * )
{
    // We shouldn't really be getting this...
    LOG ( L_ERROR, "Received unexpected socket callback; Ignoring" );
}

void DtlsListener::socketConnectFailed ( Socket *, ERRCODE )
{
    // We shouldn't really be getting this...
    LOG ( L_ERROR, "Received unexpected socket callback; Ignoring" );
}

void DtlsListener::socketReadyToSend ( Socket * )
{
    // We shouldn't really be getting this...
    LOG ( L_ERROR, "Received unexpected socket callback; Ignoring" );
}

void DtlsListener::socketDataReceived ( Socket *, MemHandle & )
{
    // We shouldn't really be getting this...
    LOG ( L_ERROR, "Received unexpected socket callback; Ignoring" );
}

void DtlsListener::socketClosed ( Socket * sock, ERRCODE reason )
{
    ( void ) sock;
    assert ( sock != 0 );
    assert ( sock->getIpSocket() != 0 );
    assert ( sock == _listeningUdpSock || sock == _listeningDtlsSock );

    if ( sock == _listeningUdpSock )
    {
        LOG_ERR ( L_FATAL_ERROR, reason,
                  "UDP socket listening on " << sock->getIpSocket()->getRemoteSockAddr() << " has been closed" );

        // There is not much we can do about it...
        return;
    }

    if ( sock == _listeningDtlsSock )
    {
        // This could happen if something goes wrong while generating connected UDP socket or configuring
        // the DTLS socket.

        LOG_ERR ( L_ERROR, reason,
                  "Listening DTLS socket closed; Remote address: " << sock->getIpSocket()->getRemoteSockAddr() );

        _listeningDtlsSock = 0;
        sock->unrefOwner ( this );

        createDtlsListeningSock();
        return;
    }
}

void DtlsListener::dtlsSocketListenSucceeded ( DtlsSocket * sock )
{
    assert ( sock != 0 );
    assert ( sock == _listeningDtlsSock );

    // DTLS listen succeeded. The listening DTLS socket is now connecting to a specific remote host.
    // It also uses a new, connected UDP socket.
    // We need to create a new listening DTLS socket.

    _listeningDtlsSock = 0;

    createDtlsListeningSock();

    _owner.incomingDtlsConnection ( this, sock );

    // This is our local state, so it's OK to call this even after the callback:
    sock->unrefOwner ( this );

    // Return after the callback!
    return;
}

void DtlsListener::dtlsSocketUnexpectedDataReceived ( DtlsSocket * sock, const MemHandle & data )
{
    ( void ) sock;
    assert ( sock != 0 );
    assert ( sock == _listeningDtlsSock );

    LOG ( L_DEBUG, "Received unexpected data on DTLS socket while listening; Data (size: "
          << data.size() << "): " << String::hexDump ( data.get(), data.size() ) );

    _owner.receivedUnexpectedData ( this, sock, data );
}
#endif
