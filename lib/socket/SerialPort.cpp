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

#include "basic/String.hpp"
#include "basic/MemHandle.hpp"
#include "sys/OsUtils.hpp"

#include "SerialPort.hpp"

// Maximum number of bytes to read each time
#define READ_SIZE    4096

using namespace Pravala;

TextLog SerialPort::_log ( "serial_port" );

SerialPort::SerialPort():
    _mode ( Invalid ),
    _fd ( -1 ),
    _evWriteDisabled ( true )
{
}

SerialPort::~SerialPort()
{
    assert ( _fd < 0 );
}

SerialPort * SerialPort::generate ( SerialPortOwner * owner, PortReadMode mode )
{
    assert ( owner != 0 );
    assert ( mode >= Discard );
    assert ( mode < Invalid );

    SerialPort * ptr = getFromPool ( owner );

    assert ( ptr->_portName.isEmpty() );
    assert ( ptr->_fd < 0 );
    assert ( ptr->_mode == Invalid );
    assert ( ptr->_evWriteDisabled );
    assert ( ptr->_readBuf.isEmpty() );
    assert ( ptr->_writeBuf.isEmpty() );

    ptr->_mode = mode;

    return ptr;
}

void SerialPort::returnsToPool()
{
    LOG ( L_DEBUG, "name=" << _portName << "; fd=" << _fd );

    _portName.clear();

    closePort ( false );

    _mode = Invalid;
    _evWriteDisabled = true;

    _readBuf.clear();
    _writeBuf.clear();
}

ERRCODE SerialPort::openPort ( const String & port )
{
    if ( _fd >= 0 )
        return Error::ConfigError;

    // we don't have an FD, so portName should be empty
    assert ( _portName.isEmpty() );

    _portName = port;

    ERRCODE eCode = openPortPriv ( port );

    if ( IS_OK ( eCode ) )
    {
        assert ( _fd >= 0 );

        LOG ( L_DEBUG, "opened port " << port << "; fd=" << _fd );

        EventManager::setFdHandler ( _fd, this, EventManager::EventRead );

        return Error::Success;
    }

    assert ( _fd < 0 );

    LOG ( L_ERROR, "failed to open port " << port );

    return eCode;
}

ERRCODE SerialPort::send ( const String & data, bool appendReturn )
{
    if ( !isActive() )
        return Error::NotConnected;

    // data can be empty if all we want to send is '\r'

    if ( data.isEmpty() && !appendReturn )
        return Error::InvalidParameter;

    _writeBuf.append ( data );

    LOG ( L_DEBUG2, "Write queued. fd=" << _fd << "; contents: '" << _writeBuf.toString() << "'" );

    if ( appendReturn )
        _writeBuf.append ( "\r" );

    if ( _evWriteDisabled )
    {
        _evWriteDisabled = false;
        EventManager::enableWriteEvents ( _fd );
    }

    return Error::Success;
}

ERRCODE SerialPort::send ( const MemHandle & data )
{
    if ( !isActive() )
        return Error::NotConnected;

    if ( data.isEmpty() == 0 )
        return Error::InvalidParameter;

    _writeBuf.append ( data );

    LOG ( L_DEBUG2, "Write queued. fd=" << _fd << "; contents: '" << _writeBuf.toString() << "'" );

    if ( _evWriteDisabled )
    {
        _evWriteDisabled = false;
        EventManager::enableWriteEvents ( _fd );
    }

    return Error::Success;
}

void SerialPort::receiveFdEvent ( int fd, short int events )
{
    ( void ) fd;
    assert ( fd == _fd );
    assert ( getOwner() != 0 );

    if ( events & EventManager::EventWrite )
    {
        LOG ( L_DEBUG2, "Got write event; fd=" << _fd );

        if ( _writeBuf.isEmpty() )
        {
            LOG ( L_DEBUG2, "Nothing to write, disabling write events; fd=" << _fd );

            _evWriteDisabled = true;
            EventManager::disableWriteEvents ( _fd );

            return;
        }

        LOG ( L_DEBUG2, "About to write " << _writeBuf.size() << " bytes; fd=" << _fd );

        ssize_t wRet = write ( _fd, _writeBuf.get(), _writeBuf.size() );

        LOG ( L_DEBUG2, "Wrote bytes/write returned: " << wRet << "; fd=" << _fd );

        if ( wRet <= 0 )
        {
            if ( errno != EAGAIN
#ifdef EWOULDBLOCK
                 && errno != EWOULDBLOCK
#endif
            )
            {
                // otherwise, if the error wasn't EAGAIN or EWOULDBLOCK, then socket write failed

                LOG ( L_ERROR, "Write failed. fd=" << _fd << "; Error: " << strerror ( errno ) << "; closing port" );

                closePort();

                return;
            }
        }
        else if ( ( size_t ) wRet == _writeBuf.size() )
        {
            _writeBuf.clear();

            // we wrote all of it
            _evWriteDisabled = true;
            EventManager::disableWriteEvents ( _fd );
        }
        else
        {
            // we wrote some of it
            _writeBuf.consumeData ( wRet );
        }
    }

    if ( events & EventManager::EventRead )
    {
        LOG ( L_DEBUG2, "Got read event" );

        char * rbuf = _readBuf.getAppendable ( READ_SIZE );

        assert ( rbuf != 0 );

        int rRet = -1;

        if ( !rbuf )
        {
            errno = ENOMEM;
        }
        else
        {
            rRet = read ( fd, rbuf, READ_SIZE );
        }

        if ( rRet <= 0 )
        {
            if ( errno != EAGAIN
#ifdef EWOULDBLOCK
                 && errno != EWOULDBLOCK
#endif
            )
            {
                LOG ( L_ERROR, "Read failed. fd=" << _fd << "; Error: " << strerror ( errno ) << "; closing port" );

                closePort();

                return;
            }
        }
        else
        {
            _readBuf.markAppended ( rRet );

            LOG ( L_DEBUG2, "Read bytes: " << rRet << "; fd=" << _fd << "; contents: '"
                  << String::hexDump ( _readBuf.get(), _readBuf.size() ) << "'" );

            // SELF reference
            // in case these callbacks result in us getting unrefed
            // any return statements MUST be preceded by a unref first!
            simpleRef();

            assert ( getOwner() != 0 );

            if ( _mode == ByteMode )
            {
                consumeReceivedDataByByte();
            }
            else if ( _mode == LineMode )
            {
                consumeReceivedDataByLine();
            }
            else
            {
                // mode is discard, so we don't care about what we're reading... just discard it all
                _readBuf.clear();
            }

            // LAST CALL -- unref self reference
            simpleUnref();
            return;
        } /* rRet > 0 */
    } /* events & EventManager::EventRead */
}

void SerialPort::consumeReceivedDataByByte()
{
    const char * buf = _readBuf.get();

    assert ( buf != 0 );

    int len = _readBuf.size();

    for ( int i = 0; i < len; ++i )
    {
        assert ( getOwner() != 0 );

        getOwner()->receiveByte ( this, buf[ i ] );

        if ( !getOwner() )
        {
            // we lost our owner
            return;
        }
    }

    _readBuf.clear();
}

void SerialPort::consumeReceivedDataByLine()
{
    const char * buf = _readBuf.get();

    assert ( buf != 0 );

    const size_t totalSize = _readBuf.size();

    // first character of line
    size_t firstChar = 0;

    // length since last EOL
    size_t sizeSinceLastEOL = 0;

    // find '\r' or '\n' and return the lines to our owner
    for ( size_t i = 0; i < totalSize; ++i )
    {
        if ( buf[ i ] != '\r' && buf[ i ] != '\n' )
        {
            // Not EOL

            ++sizeSinceLastEOL;
            continue;
        }

        if ( sizeSinceLastEOL < 1 )
        {
            // the last character was an EOL
            assert ( i == 0 || buf[ i - 1 ] == '\r' || buf[ i - 1 ] == '\n' );

            // we want to call the callback if either:
            // 1. this EOL is the first character
            // 2. this EOL is not part of a \r\n sequence that we've already called back for
            // (we call back on \r, so we don't need to call back again on the \n part of \r\n)

            if ( i == 0 || !( buf[ i - 1 ] == '\r' && buf[ i ] == '\n' ) )
            {
                assert ( getOwner() != 0 );

                // empty line, but call back anyways (to tell our owner that we got an empty line)
                getOwner()->receiveLine ( this, String() );
            }
        }
        else
        {
            assert ( getOwner() != 0 );

            LOG ( L_DEBUG2, "Completed a line, firstChar at " << firstChar << "; size is " << sizeSinceLastEOL );

            getOwner()->receiveLine ( this, String ( _readBuf.get ( firstChar ), sizeSinceLastEOL ) );
        }

        if ( !getOwner() )
        {
            // we lost our owner
            return;
        }

        // We need to go 1 past the last EOL to start the new string
        firstChar = i + 1;
        sizeSinceLastEOL = 0;
    }

    if ( sizeSinceLastEOL > 0 )
    {
        // some leftovers for the next read
        _readBuf.consumeData ( firstChar );
    }
    else
    {
        // entire buffer processed
        _readBuf.clear();
    }
}

bool SerialPort::closePort ( bool notifyOwner )
{
    if ( _fd < 0 )
        return false;

    EventManager::closeFd ( _fd );
    _fd = -1;

    // LAST CALL before return since our owner might unref us
    if ( notifyOwner && getOwner() != 0 )
        getOwner()->portClosed ( this );

    return true;
}

void SerialPortOwner::receiveLine ( SerialPort * /* port */, const String & /* line */ )
{
    // if port's mode is LineMode, then owner should implement this function
    assert ( false );
}

void SerialPortOwner::receiveByte ( SerialPort * /* port */, uint8_t /* byte */ )
{
    // if port's mode is ByteMode, then owner should implement this function
    assert ( false );
}
