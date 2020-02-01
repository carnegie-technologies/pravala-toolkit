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

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
extern "C"
{
#include <io.h>
}
#include "basic/MsvcSupport.hpp"
#endif

#include "sys/SocketApi.hpp"

#include "CorePacketWriter.hpp"

using namespace Pravala;

TextLogLimited CorePacketWriter::_log ( "packet_writer" );

CorePacketWriter::CorePacketWriter ( CorePacketWriter::WriterType wType ):
    Type ( wType ),
    _fd ( -1 )
{
}

CorePacketWriter::~CorePacketWriter()
{
    assert ( _fd < 0 );
}

ERRCODE CorePacketWriter::doWrite ( const SockAddr & addr, const char * data, size_t dataSize ) const
{
    if ( !isValid() )
    {
        return Error::Closed;
    }

    if ( dataSize < 1 )
    {
        // Nothing to write...
        return Error::Success;
    }

    if ( !data )
    {
        return Error::InvalidParameter;
    }

    ssize_t ret = -1;

    if ( Type == BasicWriter )
    {
#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
        ret = _write ( _fd, data, dataSize );
#else
        ret = ::write ( _fd, data, dataSize );
#endif
    }
    else if ( addr.hasIpAddr() )
    {
        ret = ::sendto ( _fd, data, dataSize, 0, &addr.sa, addr.getSocklen() );
    }
    else
    {
        ret = ::send ( _fd, data, dataSize, 0 );
    }

    if ( ret >= 0 )
    {
        // We report errors on incomplete writes!
        return ( ( size_t ) ret < dataSize ) ? ( Error::IncompleteWrite ) : ( Error::Success );
    }

#ifdef EMSGSIZE
    if ( errno == EMSGSIZE )
    {
        return Error::TooMuchData;
    }
#endif

#ifdef EISCONN
    if ( errno == EISCONN )
    {
        return Error::AlreadyConnected;
    }
#endif

#ifdef EINVAL
    if ( errno == EINVAL )
    {
        // We get this error on Android after trying to write to a UDP socket that the OS closed on us.
        // This could happen when disabling Wi-Fi, for instance.
        // Sometimes this code could suggest some other error, like invalid address,
        // but we check most things above ourselves, so it should not happen.
        // If we are not 100% sure which one it is, closing the socket is safer.
        return Error::Closed;
    }
#endif

    if ( SocketApi::isErrnoSoft() )
    {
        // Temporary error;
        return Error::SoftFail;
    }

    if ( SocketApi::isErrnoNonFatal() )
    {
        return Error::WriteFailed;
    }

    return Error::Closed;
}
