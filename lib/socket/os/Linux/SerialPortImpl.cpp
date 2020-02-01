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
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
}

#include "../../SerialPort.hpp"

using namespace Pravala;

ERRCODE SerialPort::openPortPriv ( const String & port )
{
    assert ( _fd < 0 );

    // using fcntl to set FD_CLOEXEC does not work!
    _fd = open ( port.c_str(), O_CLOEXEC | O_RDWR | O_NOCTTY | O_NONBLOCK );

    if ( _fd < 0 )
    {
        LOG ( L_ERROR, "Error opening port: " << port << "; " << strerror ( errno ) );

        return Error::OpenFailed;
    }

    LOG ( L_DEBUG, "openPort(): opened port " << port << "; fd=" << _fd );

    assert ( _fd >= 0 );

    struct termios tio;

    memset ( &tio, 0, sizeof ( tio ) );

    tio.c_iflag = 0; // raw input
    tio.c_oflag = 0; // raw output
    tio.c_lflag = 0; // no input processing

    // CS8 = 8n1
    // CRTSCTS = hardware flow control
    // CREAD = enable read
    // CLOCAL = local connection, no modem control
    tio.c_cflag = CS8 | CRTSCTS | CREAD | CLOCAL;

    // nonblocking read
    tio.c_cc[ VMIN ] = 0;
    tio.c_cc[ VTIME ] = 0;

    // set I/O baud rate to 115200
    // all modern devices support this speed, so it's a sane value
    // maybe parameterize this in the future
    cfsetospeed ( &tio, B115200 );
    cfsetispeed ( &tio, B115200 );

    // apply changes now
    tcsetattr ( _fd, TCSANOW, &tio );

    return Error::Success;
}
