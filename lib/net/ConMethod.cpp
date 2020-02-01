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

#include "ConMethod.hpp"

using namespace Pravala;

ConMethod::ConMethod(): _port ( 0 ), _proto ( ProtoInvalid )
{
}

ConMethod::ConMethod ( const String & str ): _port ( 0 ), _proto ( ProtoInvalid )
{
    setup ( str );
}

void ConMethod::clear()
{
    _args.clear();
    _addr.clear();
    _port = 0;
    _proto = ProtoInvalid;
}

bool ConMethod::setup ( const String & str )
{
    StringList strList = str.split ( "/" );

    if ( strList.size() < 2 )
    {
        clear();
        return false;
    }

    if ( strList[ 0 ].startsWith ( "UDP:", false ) )
    {
        strList[ 0 ] = strList[ 0 ].substr ( 4 );

        _proto = ProtoUdp;
    }
    else if ( strList[ 0 ].startsWith ( "TCP:", false ) )
    {
        strList[ 0 ] = strList[ 0 ].substr ( 4 );

        _proto = ProtoTcp;
    }
    else
    {
        _proto = ProtoBoth;
    }

    _addr = strList[ 0 ];

    bool ok = false;
    _port = strList[ 1 ].toUInt16 ( &ok );

    if ( !ok || _port < 1 || !_addr.isValid() )
    {
        clear();
        return false;
    }

    _args.clear();

    for ( size_t i = 2; i < strList.size(); ++i )
    {
        _args.append ( strList[ i ] );
    }

    return true;
}
