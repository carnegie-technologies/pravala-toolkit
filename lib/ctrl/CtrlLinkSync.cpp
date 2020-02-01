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

#include "CtrlLinkSync.hpp"

#include "proto/ProtoError.hpp"
#include "sys/SocketApi.hpp"
#include "sys/OsUtils.hpp"

#include "auto/ctrl/Ctrl/Message.hpp"
#include "auto/ctrl/Ctrl/Request.hpp"
#include "auto/ctrl/Ctrl/Update.hpp"

using namespace Pravala;

CtrlLinkSync::CtrlLinkSync():
    _socketFd ( -1 )
{
}

CtrlLinkSync::~CtrlLinkSync()
{
    close();
}

void CtrlLinkSync::close()
{
    SocketApi::close ( _socketFd );
    _socketFd = -1;
    _localSockName.clear();
    _readBuffer.clear();
}

ERRCODE CtrlLinkSync::connect ( const String & sockName )
{
    if ( _socketFd >= 0 )
    {
        return Error::AlreadyInitialized;
    }

    _socketFd = SocketApi::create ( SocketApi::SocketLocal );

    if ( _socketFd < 0 )
    {
        return Error::SocketFailed;
    }

    if ( !SocketApi::setNonBlocking ( _socketFd, false ) )
    {
        close();

        return Error::SocketFailed;
    }

    const ERRCODE conResult = SocketApi::connect ( _socketFd, sockName );

    if ( conResult == Error::Success )
    {
        _localSockName = sockName;

        return Error::Success;
    }

    close();

    return conResult;
}

ERRCODE CtrlLinkSync::request ( Ctrl::Request & req, Ctrl::Update & resp )
{
    ERRCODE eCode;

    // Normally a response is only sent on error. In this case, we always want a response.
    // Otherwise, since this is synchronous, it wouldn't return - it would just wait indefinitely
    req.setRequestResponse ( true );

    eCode = send ( req );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    do
    {
        eCode = recv ( resp );
    }
    while ( eCode == Error::DefinedValueMismatch );

    return eCode;
}

ERRCODE CtrlLinkSync::send ( Ctrl::Message & msg )
{
    ProtoError eCode;
    MemHandle data = msg.serializeWithLength ( &eCode );

    if ( eCode != ProtoError::Success )
    {
        return eCode;
    }

    ssize_t sent = 0;

    while ( data.size() > 0 )
    {
        // Call the global send
        sent = ::send ( _socketFd, data.get(), data.size(), 0 );

        if ( sent <= 0 )
        {
            return Error::WriteFailed;
        }

        data.consume ( sent );
    }

    return Error::Success;
}

ERRCODE CtrlLinkSync::recv ( Ctrl::Message & msg )
{
    size_t readSize = 0;

    assert ( _socketFd > 0 );

    if ( _socketFd < 0 )
    {
        return Error::NotInitialized;
    }

    // Deserialize the empty buffer to fill in readSize with a sensible default value
    // This may consume data that was previously read, possibly a full message
    ERRCODE eCode = msg.deserializeWithLength ( _readBuffer, &readSize );

    while ( eCode == Error::IncompleteData )
    {
        // Call the global recv
        // @todo Use MemHandle instead of appending to a buffer
        char * appendable = _readBuffer.getAppendable ( readSize );

        if ( !appendable )
        {
            return Error::MemoryError;
        }

        ssize_t readRet = ::recv ( _socketFd, appendable, readSize, 0 );

        if ( readRet > 0 )
        {
            _readBuffer.markAppended ( readRet );
            eCode = msg.deserializeWithLength ( _readBuffer, &readSize );
        }
        else if ( readRet == 0 )
        {
            // The socket has been closed by the peer
            close();

            return Error::Closed;
        }
        else
        {
            switch ( errno )
            {
                case EAGAIN:
                    // EAGAIN and EWOULDBLOCK may or may not be the same
                    // depending on the POSIX version and implementation
#if EWOULDBLOCK != EAGAIN
                case EWOULDBLOCK:
#endif
                    return Error::SoftFail;
                    break;

                case ENOMEM:
                    return Error::MemoryError;
                    break;

                case ENOTCONN:
                    return Error::NotInitialized;
                    break;

                case ENOTSOCK:
                case EBADF:
                    return Error::WrongState;
                    break;

                case EINTR:
                    // If we were interrupted, we should return control to the application
                    return Error::IncompleteData;
                    break;

                case EFAULT:
                case ECONNREFUSED:
                default:
                    return Error::Unknown;
                    break;
            }
        }
    }

    return eCode;
}
