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
#include <cstring>
#include <cerrno>

extern "C"
{
#include <unistd.h>
}

#include "basic/HashSet.hpp"
#include "config/ConfigString.hpp"
#include "app/StdApp.hpp"
#include "event/EventManager.hpp"
#include "socket/UdpFdListener.hpp"
#include "ssl/DtlsListener.hpp"

#define PORT    4455

using namespace Pravala;

class TestSocketOwner: public DtlsSocket::Owner
{
    public:
        virtual void socketDataReceived ( Socket * sock, MemHandle & data )
        {
            String str = data.toString();

            printf ( "Received data '%s': '%s'\n", sock->getRemoteDesc().c_str(), str.c_str() );

            if ( str.startsWith ( "PING" ) )
            {
                Buffer resp ( 32 + str.length() );

                resp.append ( "PONG; PID: " );
                resp.append ( String::number ( getpid() ) );
                resp.append ( "; Data: '" );
                resp.append ( str );
                resp.append ( "'" );

                MemHandle mh ( resp );

                ERRCODE ret = sock->send ( mh );

                if ( IS_OK ( ret ) )
                {
                    printf ( "send(PONG): %lu bytes\n", ( unsigned long ) resp.size() );
                }
                else
                {
                    printf ( "send(PONG): Error: %s\n", ret.toString() );
                }
            }
        }

        virtual void socketReadyToSend ( Socket * sock )
        {
            printf ( "Can-Write from '%s'\n", sock->getRemoteDesc().c_str() );
        }

        virtual void socketConnected ( Socket * sock )
        {
            printf ( "Connected: '%s'\n", sock->getRemoteDesc().c_str() );
        }

        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason )
        {
            printf ( "TestSocketOwner: Failed to connect: '%s' (%s)\n",
                     sock->getRemoteDesc().c_str(), reason.toString() );
        }

        virtual void socketClosed ( Socket * sock, ERRCODE reason )
        {
            printf ( "TestSocketOwner: Closed: '%s' (%s)\n",
                     sock->getRemoteDesc().c_str(), reason.toString() );
        }
};

class ListenerOwner: public DtlsListener::Owner, public TestSocketOwner, public Timer::Receiver
{
    public:
        List<Socket *> clients;
        SimpleTimer myTimer;
        size_t idx;

        ListenerOwner(): myTimer ( *this ), idx ( 0 )
        {
            myTimer.start ( 5 * 1000 );
        }

        ~ListenerOwner()
        {
            for ( size_t i = 0; i < clients.size(); ++i )
            {
                clients[ i ]->unrefOwner ( this );
            }

            clients.clear();
        }

        virtual void incomingDtlsConnection ( DtlsListener *, DtlsSocket * socket )
        {
            printf ( "New client: %s (on: %s)\n",
                     socket->getRemoteSockAddr().toString().c_str(),
                     socket->getLocalSockAddr().toString().c_str() );

            socket->refOwner ( this );
            clients.append ( socket );
        }

        virtual void socketClosed ( Socket * sock, ERRCODE reason )
        {
            printf ( "ListenerOwner: Sock to '%s' closed: '%s'\n",
                     sock->getRemoteDesc().c_str(), reason.toString() );

            TestSocketOwner::socketClosed ( sock, reason );

            clients.removeValue ( sock );

            assert ( !clients.findValue ( sock ) );

            sock->unrefOwner ( this );
        }

        virtual void socketConnectFailed ( Socket * sock, ERRCODE reason )
        {
            printf ( "ListenerOwner: Sock to '%s' failed to connect: '%s'\n",
                     sock->getRemoteDesc().c_str(), reason.toString() );

            TestSocketOwner::socketConnectFailed ( sock, reason );

            clients.removeValue ( sock );

            assert ( !clients.findValue ( sock ) );

            sock->unrefOwner ( this );
        }

        virtual void receivedUnexpectedData ( DtlsListener *, DtlsSocket * socket, const MemHandle & data )
        {
            printf ( "ListenerOwner: Received %d unexpected data bytes from '%s'\n",
                     ( int ) data.size(), socket->getLogId().c_str() );
        }

        void timerExpired ( Timer * timer )
        {
            ( void ) timer;
            assert ( timer == &myTimer );

            myTimer.start ( 5 * 1000 );

            ++idx;

            if ( idx >= clients.size() )
                idx = 0;

            if ( idx < clients.size() )
            {
                Buffer resp ( 32 );

                resp.append ( "PING; PID: " );
                resp.append ( String::number ( getpid() ) );

                MemHandle mh ( resp );

                ERRCODE ret = clients[ idx ]->send ( mh );

                if ( IS_OK ( ret ) )
                {
                    printf ( "send(PING): %lu bytes\n", ( unsigned long ) resp.size() );
                }
                else
                {
                    printf ( "send(PING): Error: %s\n", ret.toString() );
                }
            }
        }
};

static ConfigString optSrvKeyPath (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.key_path",
        "The path to the key file",
        "certs/server.key"
);

static ConfigString optSrvCertPath (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.cert_path",
        "The path to the cert file",
        "certs/server.crt"
);

static ConfigString optSrvCipherList (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.cipher_list",
        "Cipher list to use (see openssl ciphers)",
        "HIGH"
);

static ConfigString optSrvTrustedCasFile (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.trusted_cas_file",
        "Path to the file with trusted CAs",
        "certs/ca.crt"
);

static ConfigString optSrvTrustedCasDir (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.trusted_cas_dir",
        "Path to the directory with trusted CAs"
);

static ConfigString optSrvAllowedCas (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.server.allowed_cas_file",
        "Path to the file with allowed CAs",
        "certs/ca.crt"
);

static ConfigString optClientKeyPath (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.key_path",
        "The path to the key file",
        "certs/client.key"
);

static ConfigString optClientCertPath (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.cert_path",
        "The path to the cert file",
        "certs/client.crt"
);

static ConfigString optClientCipherList (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.cipher_list",
        "Cipher list to use (see openssl ciphers)",
        "HIGH"
);

static ConfigString optClientTrustedCasFile (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.trusted_cas_file",
        "Path to the file with trusted CAs",
        "certs/ca.crt"
);

static ConfigString optClientTrustedCasDir (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.trusted_cas_dir",
        "Path to the directory with trusted CAs"
);

static ConfigString optClientAllowedCas (
        ConfigOpt::FlagInitializeOnly,
        "dtls_test.client.allowed_cas_file",
        "Path to the file with allowed CAs",
        "certs/ca.crt"
);

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs );

    app.init();

    // We need to be careful, to only use ClientOwner with DtlsClientSockets!
    // It uses casting to reconnect sockets, if it tries to do that with a basick SslSocket we will have a problem...
    TestSocketOwner clientOwner;
    ListenerOwner listenerOwner;

    SslContext * sslContext = 0;
    DtlsListener * listener = 0;
    HashSet<Socket *> clients;

    ERRCODE eCode = Error::Success;

    const StringList & extraArgs = app.getExtraCmdLineArgs();

    if ( extraArgs.isEmpty() )
    {
        printf ( "Running in server mode on port %d\n", PORT );

        DtlsServer * srv = new DtlsServer;
        sslContext = srv;

        UNTIL_ERROR ( eCode, srv->setKeyPair ( optSrvKeyPath.value(), optSrvCertPath.value() ) );

        if ( optSrvCipherList.isSet() )
            UNTIL_ERROR ( eCode, srv->setCipherList ( optSrvCipherList.value() ) );

        if ( optSrvTrustedCasFile.isSet() || optSrvTrustedCasDir.isSet() )
            UNTIL_ERROR ( eCode, srv->setTrustedCAs ( optSrvTrustedCasFile.value(),
                                                      optSrvTrustedCasDir.value() ) );

        if ( optSrvAllowedCas.isSet() )
            UNTIL_ERROR ( eCode, srv->setAllowedCAs ( optSrvAllowedCas.value() ) );

        // Compression disabled: srv->setCompressionEnabled ( enable-or-not );

        if ( IS_OK ( eCode ) )
        {
            listener = new DtlsListener ( listenerOwner, *srv );
            eCode = listener->init ( IpAddress::Ipv4ZeroAddress, PORT );
        }
    }
    else
    {
        printf ( "Running in client mode; port %d\n", PORT );

        DtlsClient * client = new DtlsClient;
        sslContext = client;

        UNTIL_ERROR ( eCode, client->setKeyPair ( optClientKeyPath.value(), optClientCertPath.value() ) );

        if ( optClientCipherList.isSet() )
            UNTIL_ERROR ( eCode, client->setCipherList ( optClientCipherList.value() ) );

        if ( optClientTrustedCasFile.isSet() || optClientTrustedCasDir.isSet() )
            UNTIL_ERROR ( eCode, client->setTrustedCAs ( optClientTrustedCasFile.value(),
                                                         optClientTrustedCasDir.value() ) );

        if ( optClientAllowedCas.isSet() )
            UNTIL_ERROR ( eCode, client->setAllowedCAs ( optClientAllowedCas.value() ) );

        // Compression disabled: client->setCompressionEnabled ( enable-or-not );

        if ( IS_OK ( eCode ) )
        {
            for ( size_t i = 0; i < extraArgs.size(); ++i )
            {
                IpAddress addr ( extraArgs.at ( i ) );

                if ( !addr.isValid() )
                {
                    fprintf ( stderr, "Incorrect IP address: '%s'\n", extraArgs.at ( i ).c_str() );
                    eCode = Error::InvalidParameter;
                    break;
                }

                UdpListener * udpListener = UdpFdListener::generate ( Ipv4ZeroSockAddress );
                UdpSocket * udpSock = udpListener->generateConnectedSock ( 0, SockAddr ( addr, PORT ) );

                udpListener->unref();
                udpListener = 0;

                if ( !udpSock )
                {
                    fprintf ( stderr, "Error setting up client %s -> '%s':%d\n",
                              udpListener->getLocalAddr().toString().c_str(),
                              extraArgs.at ( i ).c_str(), PORT );

                    eCode = Error::InvalidParameter;
                    break;
                }

                clients.insert ( new DtlsSocket ( &clientOwner, *client, udpSock ) );
            }
        }
    }

    int ret = EXIT_SUCCESS;

    if ( IS_OK ( eCode ) )
    {
        EventManager::run();

        printf ( "Exiting the service\n" );
    }
    else
    {
        fprintf ( stderr, "Error starting the program: '%s'\n", eCode.toString() );
        ret = EXIT_FAILURE;
    }

    for ( HashSet<Socket *>::Iterator it ( clients ); it.isValid(); it.next() )
    {
        it.value()->unrefOwner ( &clientOwner );
    }

    clients.clear();

    if ( listener != 0 )
    {
        delete listener;
        listener = 0;
    }

    if ( sslContext != 0 )
    {
        delete sslContext;
        sslContext = 0;
    }

    return ret;
}
