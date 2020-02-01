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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include "app/StdApp.hpp"
#include "socks5/Socks5TcpSocket.hpp"

#include "config/ConfigAddrSpec.hpp"
#include "config/ConfigString.hpp"

using namespace Pravala;

static ConfigAddrSpec optProxyServer (
        ConfigOpt::FlagIsCommon | ConfigOpt::FlagReqNonEmpty,
        "proxy-server",
        'p',
        "",
        "The address:port of the SOCKS5 proxy server"
);

static ConfigAddrSpec optRemoteServer (
        ConfigOpt::FlagIsCommon | ConfigOpt::FlagReqNonEmpty,
        "server",
        's',
        "",
        "The address:port of the remote web server"
);

static ConfigString optOutputFile (
        ConfigOpt::FlagIsCommon | ConfigOpt::FlagReqNonEmpty,
        "output",
        'o',
        "",
        "The name of the output file; '-' for standard output (default)",
        "-"
);

class TestClass: protected SocketOwner
{
    public:
        TestClass ( const String & path ):
            _path ( path ),
            _sock ( new Socks5TcpSocket ( this, optProxyServer.value() ) ),
            _bytesReceived ( 0 ),
            _outputFd ( -1 ),
            _retCode ( EXIT_SUCCESS )
        {
            if ( optOutputFile.value() != "-"
                 && ( _outputFd = ::open ( optOutputFile.value().c_str(), O_WRONLY | O_CREAT, 0644 ) ) < 0 )
            {
                fprintf ( stderr, "Could not open output file '%s' for writing: %s\n",
                          optOutputFile.value().c_str(), strerror ( errno ) );

                _retCode = EXIT_FAILURE;
            }
        }

        virtual ~TestClass()
        {
            close();
        }

        void close()
        {
            if ( _sock != 0 )
            {
                _sock->unrefOwner ( this );
                _sock = 0;
            }

            if ( _outputFd >= 0 )
            {
                ::close ( _outputFd );
                _outputFd = -1;
            }
        }

        int run()
        {
            if ( _retCode != EXIT_SUCCESS )
                return _retCode;

            assert ( _sock != 0 );

            ERRCODE eCode = _sock->connect ( optRemoteServer.value() );

            if ( NOT_OK ( eCode ) )
            {
                fprintf ( stderr, "Could not connect to %s: %s\n",
                          optRemoteServer.value().toString().c_str(), eCode.toString() );

                return ( _retCode = EXIT_FAILURE );
            }

            EventManager::run();
            close();

            return _retCode;
        }

        virtual void socketConnected ( Socket * )
        {
            fprintf ( stderr, "Socket connected\n" );

            Buffer buf;

            buf.append ( "GET " );
            buf.append ( _path );
            buf.append ( "\n\n" );

            MemHandle mh ( buf );

            ERRCODE eCode = _sock->send ( mh );

            fprintf ( stderr, "Requesting '%s'\n", _path.c_str() );
            fprintf ( stderr, "Result: %s; Remaining bytes: %lu\n", eCode.toString(), ( long unsigned ) mh.size() );
        }

        virtual void socketDataReceived ( Socket *, MemHandle & data )
        {
            _bytesReceived += data.size();

            const ssize_t ret = ::write ( ( _outputFd >= 0 ) ? _outputFd : STDOUT_FILENO, data.get(), data.size() );

            if ( ret < 0 )
            {
                fprintf ( stderr, "Error writing to output file '%s': %s\n",
                          optOutputFile.value().c_str(), strerror ( errno ) );

                EventManager::stop();
                _retCode = EXIT_FAILURE;
                return;
            }
            else if ( ( size_t ) ret != data.size() )
            {
                fprintf ( stderr, "Incomplete write to output file '%s'; Data written: %lu; Expected: %lu\n",
                          optOutputFile.value().c_str(), ( long unsigned ) ret, ( long unsigned ) data.size() );

                EventManager::stop();
                _retCode = EXIT_FAILURE;
                return;
            }

            // Consume the data:
            data.clear();
        }

        virtual void socketReadyToSend ( Socket * )
        {
            fprintf ( stderr, "Socket is ready to send\n" );
        }

        virtual void socketClosed ( Socket *, ERRCODE reason )
        {
            fprintf ( stderr, "Socket closed: %s; Total number of bytes received: %lu\n",
                      reason.toString(), ( long unsigned ) _bytesReceived );

            EventManager::stop();

            _retCode = ( reason == Error::Closed ) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        virtual void socketConnectFailed ( Socket *, ERRCODE reason )
        {
            fprintf ( stderr, "Socket failed to connect: %s\n", reason.toString() );

            EventManager::stop();

            _retCode = EXIT_FAILURE;
        }

    private:
        const String _path;
        Socks5TcpSocket * _sock;
        size_t _bytesReceived;
        int _outputFd;
        int _retCode;
};

int main ( int argc, char * argv[] )
{
    StdApp app ( argc, argv, StdApp::FeatStdFeaturesWithArgs );

    app.init();

    if ( app.getExtraCmdLineArgs().size() != 1 )
    {
        fprintf ( stderr, "A single argument with the path on the remote server is required\n" );
        return EXIT_FAILURE;
    }

    const String path = app.getExtraCmdLineArgs().at ( 0 );

    fprintf ( stderr,
              "We will try to get '%s' address from '%s' using '%s' as a proxy\n",
              path.c_str(), optRemoteServer.value().toString().c_str(), optProxyServer.value().toString().c_str() );

    TestClass test ( path );

    return test.run();
}
