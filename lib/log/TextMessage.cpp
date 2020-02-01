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

#include "basic/IpSubnet.hpp"
#include "basic/WString.hpp"
#include "basic/MsvcSupport.hpp"

#include "proto/ExtProtoError.hpp"

#include "sys/Time.hpp"
#include "TextMessage.hpp"

using namespace Pravala;

TextMessage & TextMessage::operator<< ( const Time & value )
{
    _buf.append ( String ( "%1.%2" )
                  .arg ( String::number ( value.getSeconds() ),
                         String::number ( value.getMilliSeconds(), String::Int_Dec, 3, true ) ) );
    return *this;
}

TextMessage & TextMessage::operator<< ( const IpAddress & value )
{
    _buf.append ( value.toString ( true ) );
    return *this;
}

TextMessage & TextMessage::operator<< ( const List<IpAddress> & value )
{
    _buf.append ( toString ( value ) );
    return *this;
}

TextMessage & TextMessage::operator<< ( const IpSubnet & value )
{
    _buf.append ( value.toString ( true ) );
    return *this;
}

TextMessage & TextMessage::operator<< ( const SockAddr & value )
{
    _buf.append ( value.toString() );
    return *this;
}

TextMessage & TextMessage::operator<< ( const ErrorCode & value )
{
    _buf.append ( value.toString() );
    return *this;
}

TextMessage & TextMessage::operator<< ( const ERRCODE & value )
{
    _buf.append ( value.toString() );
    return *this;
}

TextMessage & TextMessage::operator<< ( const ExtProtoError & value )
{
    _buf.append ( value.toString() );
    return *this;
}

TextMessage & TextMessage::operator<< ( const WString & value )
{
    _buf.append ( value.toString() );
    return *this;
}

#ifdef SYSTEM_WINDOWS

TextMessage & TextMessage::operator<< ( const GUID & guid )
{
    // Format defined here:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa373931%28v=vs.85%29.aspx
    // The braces are used because it appears to be a common string format for the adapter name
    // as returned in IP_ADAPTER_ADDRESSES by GetAdaptersAddresses and IP_ADAPTER_INDEX_MAP by GetInterfaceInfo

    // Format: "{01234567-0123-0123-0123-012345678901}" - 38 characters

    char buf[ 40 ];

    //                                       A    B    C    D   E    F   G   H   I   J   K
    const int len = snprintf ( buf, 40, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                               guid.Data1,    // A
                               guid.Data2,    // B
                               guid.Data3,    // C
                               guid.Data4[ 0 ], // D
                               guid.Data4[ 1 ], // E
                               guid.Data4[ 2 ], // F
                               guid.Data4[ 3 ], // G
                               guid.Data4[ 4 ], // H
                               guid.Data4[ 5 ], // I
                               guid.Data4[ 6 ], // J
                               guid.Data4[ 7 ]  // K
                    );

    _buf.appendData ( buf, len );
    return *this;
}
#endif
